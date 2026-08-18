#include "HardwareHal.h"

#include <fstream>
#include <string>
#include <unistd.h>

#include <smooth_ui_toolkit.hpp>
#include <SCServo.h>
#include <SCS0009.h>
#include "motion.h"
#include "settings.h"

namespace {

constexpr int kBusPowerGpio = 131;
constexpr uint8_t kServoPowerPin = 0;
constexpr uint8_t kRgbDataPin = 13;
constexpr uint8_t kRgbLedCount = 12;

bool writeSysfsValue(const char* path, const char* value)
{
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << value;
    return file.good();
}

}  // namespace


using namespace smooth_ui_toolkit;
using namespace motion;

struct ServoConfig_t {
    int id             = -1;
    int defaultZeroPos = 0;
    Vector2i angleLimit;
    Vector2i rawPosLimit;
    std::string settingNs;
    std::string settingZeroPositionKey;
    bool enablePwmMode = false;
    bool enableStallProtection = false;
};

static uint32_t linux_millis()
{
    struct timespec timestamp = {};
    clock_gettime(CLOCK_MONOTONIC, &timestamp);
    return static_cast<uint32_t>(timestamp.tv_sec * 1000ULL + timestamp.tv_nsec / 1000000ULL);
}

class ScsServo : public Servo {
public:
    static inline const std::string _tag = "ScsServo";

    ScsServo(const ServoConfig_t& config, SCS0009* scs_bus) : _config(config), _runtime_raw_pos_limit(config.rawPosLimit), _scs_bus(scs_bus)
    {
    }

    void init() override
    {
        reset_runtime_limits();
        set_angle_limit(_config.angleLimit);
        get_zero_pos_from_nvs();
        Servo::init();
    }

    void get_zero_pos_from_nvs()
    {
        _zero_pos     = _config.defaultZeroPos;
        bool is_valid = false;

        {
            Settings settings(_config.settingNs, false);
            int nvs_zero_pos = settings.GetInt(_config.settingZeroPositionKey, -1);

            // Limit check
            if (nvs_zero_pos >= _config.rawPosLimit.x && nvs_zero_pos <= _config.rawPosLimit.y) {
                _zero_pos = nvs_zero_pos;
                is_valid  = true;
                printf("id: %d get zero pos: %d from settings\n", _config.id, _zero_pos);
            } else {
                is_valid = false;
                printf("id: %d get invalid zero pos: %d from settings\n", _config.id, nvs_zero_pos);
            }
        }

        if (!is_valid) {
            _zero_pos = _config.defaultZeroPos;
            printf("id: %d override zero pos to default: %d\n", _config.id, _zero_pos);

            Settings settings(_config.settingNs, true);
            settings.SetInt(_config.settingZeroPositionKey, _zero_pos);
        }
    }

    void set_angle_impl(int angle) override
    {
        int mapped_angle = _zero_pos + angle * 16 / 5 / 10;  // 一步对应 0.3125度, 0.3125 = 5/16
        mapped_angle     = uitk::clamp(mapped_angle, _runtime_raw_pos_limit.x, _runtime_raw_pos_limit.y);

        if (update_stall_protection(mapped_angle)) {
            return;
        }

        check_mode(Mode::Position);
        _scs_bus->WritePos(_config.id, mapped_angle, 20, 0);
    }

    int getCurrentAngle() override
    {
        int current_pos = _scs_bus->ReadPos(_config.id);
        if (!is_raw_pos_valid(current_pos)) {
            int fallback_angle = uitk::clamp(Servo::getCurrentAngle(), getAngleLimit().x, getAngleLimit().y);
            printf("id: %d ignore invalid current pos: %d, fallback angle: %d\n", _config.id, current_pos,
                   fallback_angle);
            return fallback_angle;
        }

        int angle = raw_pos_to_angle(current_pos);
        angle     = uitk::clamp(angle, getAngleLimit().x, getAngleLimit().y);
        // mclog::tagInfo(_tag, "id: {} current pos: {} angle: {}", _id, current_pos, angle);
        return angle;
    }

    bool is_moving_impl() override
    {
        int moving = _scs_bus->ReadMove(_config.id);
        // mclog::tagInfo(_tag, "id: {} moving: {}", _id, moving);
        return moving != 0;
    }

    void setTorqueEnabled(bool enabled) override
    {
        Servo::setTorqueEnabled(enabled);
        _scs_bus->EnableTorque(_config.id, enabled ? 1 : 0);
        // mclog::tagInfo(_tag, "id: {} set torque: {}", _id, enabled);
    }

    bool getTorqueEnabled() override
    {
        int torque_enable = _scs_bus->ReadToqueEnable(_config.id);
        // mclog::tagInfo(_tag, "id: {} torque enable: {}", _id, torque_enable);
        return torque_enable > 0;
    }

    void setCurrentAngleAsZero() override
    {
        int current_pos = _scs_bus->ReadPos(_config.id);
        if (!is_raw_pos_valid(current_pos)) {
            printf("id: %d ignore invalid zero calibration pos: %d, keep zero pos: %d\n", _config.id,
                   current_pos, _zero_pos);
            return;
        }

        _zero_pos = current_pos;
        reset_runtime_limits();

        Settings settings(_config.settingNs, true);
        settings.SetInt(_config.settingZeroPositionKey, _zero_pos);

        printf("id: %d set zero pos: %d to settings\n", _config.id, _zero_pos);
    }

    void resetZeroCalibration() override
    {
        _zero_pos = _config.defaultZeroPos;
        reset_runtime_limits();

        Settings settings(_config.settingNs, true);
        settings.SetInt(_config.settingZeroPositionKey, _zero_pos);

        printf("id: %d set zero pos: %d to settings\n", _config.id, _zero_pos);
    }

    void rotate(int velocity) override
    {
        velocity = uitk::clamp(velocity, -1000, 1000);

        if (!_config.enablePwmMode) {
            return;
        }

        int mapped_velocity = map_range(velocity, 0, 1000, 0, 1023);

        check_mode(Mode::PWM);
        _scs_bus->WritePWM(_config.id, mapped_velocity);
    }

private:
    enum class Mode { Position = 0, PWM = 1 };

    ServoConfig_t _config;
    Vector2i _runtime_raw_pos_limit;
    int _zero_pos      = 0;
    Mode _current_mode = Mode::Position;
    SCS0009* _scs_bus;

    static constexpr uint32_t kStallFeedbackIntervalMs = 50;
    static constexpr int kStallMinTargetDeltaRaw       = 8;
    static constexpr int kStallMaxPositionDeltaRaw     = 1;
    static constexpr int kStallCurrentRiseThreshold    = 80;
    static constexpr int kStallLoadRiseThreshold       = 150;
    static constexpr int kStallCurrentAbsThreshold     = 350;
    static constexpr int kStallLoadAbsThreshold        = 650;
    static constexpr int kStallConfirmSamples          = 2;

    uint32_t _last_stall_check_tick = 0;
    int _last_stall_raw_pos         = 0;
    int _last_stall_current_abs     = 0;
    int _last_stall_load_abs        = 0;
    int _last_stall_direction       = 0;
    int _stall_confirm_count        = 0;
    bool _last_stall_feedback_valid = false;

    static int abs_int(int value)
    {
        return value < 0 ? -value : value;
    }

    bool is_raw_pos_valid(int raw_pos) const
    {
        return raw_pos >= _config.rawPosLimit.x && raw_pos <= _config.rawPosLimit.y;
    }

    int raw_pos_to_angle(int raw_pos) const
    {
        return (raw_pos - _zero_pos) * 5 * 10 / 16;
    }

    void reset_runtime_limits()
    {
        _runtime_raw_pos_limit = _config.rawPosLimit;
        set_angle_limit(_config.angleLimit);
        reset_stall_detection();
    }

    void reset_stall_detection()
    {
        _last_stall_feedback_valid = false;
        _last_stall_direction      = 0;
        _stall_confirm_count       = 0;
    }

    bool update_stall_protection(int target_raw_pos)
    {
        if (!_config.enableStallProtection) {
            return false;
        }

        const uint32_t now = linux_millis();
        if (now - _last_stall_check_tick < kStallFeedbackIntervalMs) {
            return false;
        }
        _last_stall_check_tick = now;

        if (_scs_bus->FeedBack(_config.id) < 0) {
            reset_stall_detection();
            return false;
        }

        const int current_raw_pos = _scs_bus->ReadPos(-1);
        const int current_abs     = abs_int(_scs_bus->ReadCurrent(-1));
        const int load_abs        = abs_int(_scs_bus->ReadLoad(-1));

        if (!is_raw_pos_valid(current_raw_pos)) {
            reset_stall_detection();
            return false;
        }

        const int target_delta = target_raw_pos - current_raw_pos;
        if (abs_int(target_delta) < kStallMinTargetDeltaRaw) {
            reset_stall_detection();
            return false;
        }

        const int direction = target_delta > 0 ? 1 : -1;
        if (_last_stall_feedback_valid && direction == _last_stall_direction) {
            const int pos_delta = abs_int(current_raw_pos - _last_stall_raw_pos);
            const bool position_stuck = pos_delta <= kStallMaxPositionDeltaRaw;
            const bool current_spike  = current_abs >= kStallCurrentAbsThreshold ||
                                       current_abs - _last_stall_current_abs >= kStallCurrentRiseThreshold;
            const bool load_spike = load_abs >= kStallLoadAbsThreshold ||
                                    load_abs - _last_stall_load_abs >= kStallLoadRiseThreshold;

            if (position_stuck && (current_spike || load_spike)) {
                _stall_confirm_count++;
            } else if (pos_delta > kStallMaxPositionDeltaRaw) {
                _stall_confirm_count = 0; 
            }
        } else {
            _stall_confirm_count = 0;
        }

        _last_stall_raw_pos         = current_raw_pos;
        _last_stall_current_abs     = current_abs;
        _last_stall_load_abs        = load_abs;
        _last_stall_direction       = direction;
        _last_stall_feedback_valid = true;

        if (_stall_confirm_count < kStallConfirmSamples) {
            return false;
        }

        handle_stall(current_raw_pos, direction, current_abs, load_abs);
        return true;
    }

    void handle_stall(int raw_pos, int direction, int current_abs, int load_abs)
    {
        int angle = raw_pos_to_angle(raw_pos);
        angle     = uitk::clamp(angle, _config.angleLimit.x, _config.angleLimit.y);

        auto angle_limit = getAngleLimit();
        if (direction > 0) {
            if (raw_pos < _runtime_raw_pos_limit.y) {
                _runtime_raw_pos_limit.y = raw_pos;
            }
            if (angle < angle_limit.y) {
                angle_limit.y = angle;
            }
        } else {
            if (raw_pos > _runtime_raw_pos_limit.x) {
                _runtime_raw_pos_limit.x = raw_pos;
            }
            if (angle > angle_limit.x) {
                angle_limit.x = angle;
            }
        }
        set_angle_limit(angle_limit);
        stop_motion_at_angle(angle);
        reset_stall_detection();

        check_mode(Mode::Position);
        _scs_bus->WritePos(_config.id, raw_pos, 20, 0);

        printf("id: %d stall detected, raw: %d, angle: %d, dir: %d, current: %d, load: %d, limit: [%d, %d]\n",
               _config.id, raw_pos, angle, direction, current_abs, load_abs, angle_limit.x, angle_limit.y);
    }

    void check_mode(Mode targetMode)
    {
        if (targetMode == _current_mode) {
            return;
        }

        _scs_bus->Mode(_config.id, static_cast<uint8_t>(targetMode));
        _current_mode = targetMode;
    }
};

HardwareHal::HardwareHal(const char* i2c_device)
    : io_expander_(i2c_device), si12t_(nullptr)
{
}

HardwareHal::~HardwareHal()
{
    if (si12t_ != nullptr) {
        si12t_delete(si12t_);
    }
}

bool HardwareHal::initialize()
{
    return enableBusPower() && initializeExpander();
}

bool HardwareHal::enableBusPower()
{
    // Exporting an already exported GPIO fails with EBUSY, so the result is ignored.
    const std::string gpio_number = std::to_string(kBusPowerGpio);
    writeSysfsValue("/sys/class/gpio/export", gpio_number.c_str());

    return writeSysfsValue("/sys/class/gpio/PI3/direction", "out")
        && writeSysfsValue("/sys/class/gpio/PI3/value", "1");
}

bool HardwareHal::initializeExpander()
{
    if (!io_expander_.begin()) {
        return false;
    }

    io_expander_.setDirection(kServoPowerPin, true);
    io_expander_.setPullMode(kServoPowerPin, true);
    setServoPowerEnabled(false);
    usleep(200 * 1000);

    io_expander_.setDirection(kRgbDataPin, true);
    io_expander_.setPullMode(kRgbDataPin, true);
    io_expander_.setDriveMode(kRgbDataPin, false);
    io_expander_.setLedCount(kRgbLedCount);
    usleep(200 * 1000);

    writeRgbColor(0, 0, 0);
    usleep(50 * 1000);
    writeRgbColor(0, 0, 0);
    usleep(100 * 1000);

    si12t_config_t si12t_cfg = {"/dev/i2c-2", SI12T_GND_ADDRESS};
    if (si12t_init(&si12t_cfg, &si12t_) != 0) {
        si12t_ = nullptr;
        return false;
    }
    if (si12t_setup(si12t_, SI12T_TYPE_LOW, SI12T_SENSITIVITY_LEVEL_3) != 0) {
        si12t_delete(si12t_);
        si12t_ = nullptr;
        return false;
    }

    //servo_init();
    
    return true;
}

void HardwareHal::setServoPowerEnabled(bool enabled)
{
    io_expander_.digitalWrite(kServoPowerPin, enabled);
}

void HardwareHal::writeRgbColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint8_t index = 0; index < kRgbLedCount; ++index) {
        io_expander_.setLedColor(index, r, g, b);
    }
    io_expander_.refreshLeds();
}

void HardwareHal::setRgbLeftLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if(index >= 6) {
        return;
    }
    io_expander_.setLedColor(index, r, g, b);
    io_expander_.refreshLeds();
}

void HardwareHal::setRgbRightLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if(index >= 6) {
        return;
    }
    io_expander_.setLedColor(11 - index, r, g, b);
    io_expander_.refreshLeds();
}

void HardwareHal::setRgbBothLed(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
    setRgbLeftLed(index, r, g, b);
    setRgbRightLed(index, r, g, b);
}

bool HardwareHal::readTouchIntensities(uint8_t intensities[3])
{
    if (si12t_ == nullptr || intensities == nullptr) {
        return false;
    }

    uint8_t touch_result = 0;
    if (si12t_read_touch_result(si12t_, &touch_result) != 0) {
        return false;
    }

    si12t_parse_touch_result_to(touch_result, intensities);
    return true;
}

void HardwareHal::servo_init(void)
{
    _scs_bus.begin(1000000, "/dev/ttySTM2");

    ServoConfig_t yaw_servo_config;
    yaw_servo_config.id                     = 1;
    yaw_servo_config.defaultZeroPos         = 460;
    yaw_servo_config.angleLimit             = Vector2i(-1280, 1280);
    yaw_servo_config.rawPosLimit            = Vector2i(0, 1000);
    yaw_servo_config.settingNs              = "servo";
    yaw_servo_config.settingZeroPositionKey = "zero_pos_1";
    yaw_servo_config.enablePwmMode          = true;

    ServoConfig_t pitch_servo_config;
    pitch_servo_config.id                     = 2;
    pitch_servo_config.defaultZeroPos         = 620;
    pitch_servo_config.angleLimit             = Vector2i(30, 870);
    pitch_servo_config.rawPosLimit            = Vector2i(0, 1000);
    pitch_servo_config.settingNs              = "servo";
    pitch_servo_config.settingZeroPositionKey = "zero_pos_2";
    pitch_servo_config.enableStallProtection  = true;

    auto yaw_servo   = std::make_unique<ScsServo>(yaw_servo_config,&_scs_bus);
    auto pitch_servo = std::make_unique<ScsServo>(pitch_servo_config,&_scs_bus);
    _motion      = std::make_unique<Motion>(std::move(yaw_servo), std::move(pitch_servo));
    _motion->init();
    _motion->goHome(666);
}
