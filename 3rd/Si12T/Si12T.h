// SPDX-FileCopyrightText: 2026 karaage0703
// SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
// SPDX-License-Identifier: MIT
//
// Linux 版 Si12T (容量タッチセンサ IC、3 ch) 最小ドライバ。
// Linux の i2c-dev (/dev/i2c-X) 経由で M5Stack 公式 StackChan K151 の
// 頭部タッチセンサを読む。
//
// 使い方:
//   Si12T touch("/dev/i2c-1");
//   if (touch.begin()) {
//       // polling task で loop:
//       Si12T::Gesture g = touch.poll();
//       if (g == Si12T::Gesture::Press) { ... }
//   }

#ifndef XANGI_STACKCHAN_SI12T_H_
#define XANGI_STACKCHAN_SI12T_H_

#include <stdint.h>

// 7bit I2C address (LTR-507 SEL pin tied to GND)
#define SI12T_GND_ADDRESS 0x68

class Si12T {
public:
    enum class Gesture : uint8_t {
        None          = 0,
        Press         = 1,
        Release       = 2,
        SwipeForward  = 3,
        SwipeBackward = 4,
    };

    enum class SensitivityType : uint8_t {
        Low  = 0,
        High = 1,
    };

    enum class SensitivityLevel : uint8_t {
        L0 = 0, L1, L2, L3, L4, L5, L6, L7,
    };

    explicit Si12T(const char* device = "/dev/i2c-1",
                   uint8_t i2c_addr = SI12T_GND_ADDRESS);
    ~Si12T();

    Si12T(const Si12T&) = delete;
    Si12T& operator=(const Si12T&) = delete;

    // 初期化失敗 (デバイス無し / I2C エラー / 権限不足) は false を返す。
    bool begin(SensitivityType sens_type   = SensitivityType::Low,
               SensitivityLevel sens_level = SensitivityLevel::L3);

    // 50ms 周期で呼ぶ。内部の TouchState (IDLE / TOUCHED / SWIPING) を更新して
    // 検出したジェスチャを返す。
    Gesture poll();

    // 直近の raw intensity (3 ch、各 0-3) を取得。デバッグ用。
    void getIntensity(uint8_t out[3]) const;

    // -100..100 の重心位置 (intensity 加重平均、None なら 0)。
    int16_t getPosition() const;

private:
    // --- low-level I2C ---
    bool writeReg(uint8_t reg, uint8_t value);
    bool readReg(uint8_t reg, uint8_t* out);

    // --- setup helpers (Si12T.cpp ESP-IDF 版から移植) ---
    bool enableChannel();
    bool setCtrl1();
    bool setCtrl2();
    bool setSensitivity(SensitivityType type, SensitivityLevel level);

    // --- runtime ---
    bool readTouchResult(uint8_t* out);
    static void parseTouchResult(uint8_t raw, uint8_t out[3]);

    const char* device_;
    uint8_t addr_;
    int fd_ = -1;
    bool    ready_     = false;
    uint8_t intensity_[3] = {0, 0, 0};

    // GestureRecognizer state
    enum class TouchState : uint8_t { Idle, Touched, Swiping };
    TouchState state_           = TouchState::Idle;
    int16_t    initial_position_ = 0;
    int16_t    swipe_threshold_  = 40;  // -100..100 範囲での閾値
};

#endif  // XANGI_STACKCHAN_SI12T_H_
