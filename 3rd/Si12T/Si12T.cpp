#include "Si12T.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

/**
 * @brief Si12T设备结构体
 */
struct si12t_dev_t {
    int i2c_fd;
    uint8_t dev_addr;
};

uint8_t si12t_point_type[3] = {SI12T_OUTPUT_NONE, SI12T_OUTPUT_NONE, SI12T_OUTPUT_NONE};

/**
 * @brief 写寄存器
 */
static int si12t_i2c_write_reg(si12t_handle_t handle, uint8_t reg_addr, uint8_t value)
{
    uint8_t write_buf[2] = {reg_addr, value};
    struct i2c_msg message = {
        .addr = handle->dev_addr,
        .flags = 0,
        .len = sizeof(write_buf),
        .buf = write_buf,
    };
    struct i2c_rdwr_ioctl_data transfer = {.msgs = &message, .nmsgs = 1};
    return ioctl(handle->i2c_fd, I2C_RDWR, &transfer) < 0 ? -errno : 0;
}

/**
 * @brief 读寄存器
 */
static int si12t_i2c_read_reg(si12t_handle_t handle, uint8_t reg_addr, uint8_t *value)
{
    struct i2c_msg messages[2] = {
        {.addr = handle->dev_addr, .flags = 0, .len = 1, .buf = &reg_addr},
        {.addr = handle->dev_addr, .flags = I2C_M_RD, .len = 1, .buf = value},
    };
    struct i2c_rdwr_ioctl_data transfer = {.msgs = messages, .nmsgs = 2};
    return ioctl(handle->i2c_fd, I2C_RDWR, &transfer) < 0 ? -errno : 0;
}

/**
 * @brief 设置所有灵敏度寄存器
 */
static int si12t_set_sens(si12t_handle_t handle, uint8_t value)
{
    int ret = 0;

    ret |= si12t_i2c_write_reg(handle, SI12T_SENSITIVITY1_ADDR, value);
    ret |= si12t_i2c_write_reg(handle, SI12T_SENSITIVITY2_ADDR, value);
    ret |= si12t_i2c_write_reg(handle, SI12T_SENSITIVITY3_ADDR, value);
    ret |= si12t_i2c_write_reg(handle, SI12T_SENSITIVITY4_ADDR, value);
    ret |= si12t_i2c_write_reg(handle, SI12T_SENSITIVITY5_ADDR, value);

    return ret;
}

int si12t_init(const si12t_config_t *config, si12t_handle_t *handle)
{
    if (config == NULL || handle == NULL || config->i2c_device == NULL) {
        return -EINVAL;
    }

    si12t_handle_t dev = (si12t_handle_t)calloc(1, sizeof(struct si12t_dev_t));
    if (dev == NULL) {
        return -ENOMEM;
    }

    dev->dev_addr = config->dev_addr ? config->dev_addr : SI12T_GND_ADDRESS;
    dev->i2c_fd = open(config->i2c_device, O_RDWR | O_CLOEXEC);
    if (dev->i2c_fd < 0) {
        int ret = -errno;
        free(dev);
        return ret;
    }

    *handle = dev;
    return 0;
}

int si12t_setup(si12t_handle_t handle, si12t_type_t sens_type, si12t_sensitivity_level_t sens_level)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    int ret = 0;

    ret |= si12t_enable_channel(handle);
    ret |= si12t_set_ctrl2(handle);
    ret |= si12t_set_ctrl1(handle);
    ret |= si12t_set_sensitivity(handle, sens_type, sens_level);
    ret |= si12t_get_sensitivity(handle);

    return ret;
}

int si12t_delete(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    int ret = close(handle->i2c_fd) < 0 ? -errno : 0;
    free(handle);
    return ret;
}

int si12t_get_sensitivity(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    uint8_t data  = 0;
    int ret = 0;

    ret |= si12t_i2c_read_reg(handle, SI12T_SENSITIVITY1_ADDR, &data);
    ret |= si12t_i2c_read_reg(handle, SI12T_SENSITIVITY2_ADDR, &data);
    ret |= si12t_i2c_read_reg(handle, SI12T_SENSITIVITY3_ADDR, &data);
    ret |= si12t_i2c_read_reg(handle, SI12T_SENSITIVITY4_ADDR, &data);
    ret |= si12t_i2c_read_reg(handle, SI12T_SENSITIVITY5_ADDR, &data);

    return ret;
}

int si12t_set_sensitivity(si12t_handle_t handle, si12t_type_t sens_type, si12t_sensitivity_level_t sens_level)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    if (sens_type != SI12T_TYPE_LOW && sens_type != SI12T_TYPE_HIGH) {
        return -EINVAL;
    }

    uint8_t value = 0x00;

    if (sens_type == SI12T_TYPE_HIGH) {
        switch (sens_level) {
            case SI12T_SENSITIVITY_LEVEL_0:
                value = 0x88;
                break;
            case SI12T_SENSITIVITY_LEVEL_1:
                value = 0x99;
                break;
            case SI12T_SENSITIVITY_LEVEL_2:
                value = 0xAA;
                break;
            case SI12T_SENSITIVITY_LEVEL_3:
                value = 0xBB;
                break;
            case SI12T_SENSITIVITY_LEVEL_4:
                value = 0xCC;
                break;
            case SI12T_SENSITIVITY_LEVEL_5:
                value = 0xDD;
                break;
            case SI12T_SENSITIVITY_LEVEL_6:
                value = 0xEE;
                break;
            case SI12T_SENSITIVITY_LEVEL_7:
                value = 0xFF;
                break;
            default:
                return -EINVAL;
        }
    } else {
        switch (sens_level) {
            case SI12T_SENSITIVITY_LEVEL_0:
                value = 0x00;
                break;
            case SI12T_SENSITIVITY_LEVEL_1:
                value = 0x11;
                break;
            case SI12T_SENSITIVITY_LEVEL_2:
                value = 0x22;
                break;
            case SI12T_SENSITIVITY_LEVEL_3:
                value = 0x33;
                break;
            case SI12T_SENSITIVITY_LEVEL_4:
                value = 0x44;
                break;
            case SI12T_SENSITIVITY_LEVEL_5:
                value = 0x55;
                break;
            case SI12T_SENSITIVITY_LEVEL_6:
                value = 0x66;
                break;
            case SI12T_SENSITIVITY_LEVEL_7:
                value = 0x77;
                break;
            default:
                return -EINVAL;
        }
    }

    return si12t_set_sens(handle, value);
}

int si12t_set_ctrl1(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    uint8_t test;
    // sends register data, Auto Mode, FTC=01, Interrupt(Middle,High), Response 4 (2+2)
    int ret = si12t_i2c_write_reg(handle, SI12T_CTRL1_ADDR, 0x22);
    si12t_i2c_read_reg(handle, SI12T_CTRL1_ADDR, &test);
    return ret;
}

int si12t_set_ctrl2(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    uint8_t test;
    int ret = 0;
    // S/W Reset Enable, Sleep Mode Enable
    ret |= si12t_i2c_write_reg(handle, SI12T_CTRL2_ADDR, 0x0F);
    ret |= si12t_i2c_write_reg(handle, SI12T_CTRL2_ADDR, 0x07);
    si12t_i2c_read_reg(handle, SI12T_CTRL2_ADDR, &test);
    return ret;
}

int si12t_sleep_enable(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }
    return si12t_i2c_write_reg(handle, SI12T_CTRL2_ADDR, 0x07);  // S/W Reset Enable, Sleep Mode Enable
}

int si12t_sleep_disable(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }
    return si12t_i2c_write_reg(handle, SI12T_CTRL2_ADDR, 0x03);  // S/W Reset Disable, Sleep Mode Disable
}

int si12t_enable_channel(si12t_handle_t handle)
{
    if (handle == NULL) {
        return -EINVAL;
    }

    int ret = 0;
    uint8_t data  = 1;

    ret |= si12t_i2c_write_reg(handle, SI12T_REF_RST1_ADDR, 0x00);  // channel 1-8 enable reference calibration
    ret |= si12t_i2c_write_reg(handle, SI12T_REF_RST2_ADDR, 0x00);  // channel 9 enable reference calibration

    ret |= si12t_i2c_write_reg(handle, SI12T_CH_HOLD1_ADDR, 0x00);  // channel 1-8 enable
    ret |= si12t_i2c_write_reg(handle, SI12T_CH_HOLD2_ADDR, 0x00);  // channel 9 enable

    ret |= si12t_i2c_write_reg(handle, SI12T_CAL_HOLD1_ADDR, 0x00);  // channel 1-8 enable reference calibration
    ret |= si12t_i2c_write_reg(handle, SI12T_CAL_HOLD2_ADDR, 0x00);  // channel 9 enable reference calibration

    // Read back to verify
    si12t_i2c_read_reg(handle, SI12T_REF_RST1_ADDR, &data);
    si12t_i2c_read_reg(handle, SI12T_REF_RST2_ADDR, &data);
    si12t_i2c_read_reg(handle, SI12T_CH_HOLD1_ADDR, &data);
    si12t_i2c_read_reg(handle, SI12T_CH_HOLD2_ADDR, &data);
    si12t_i2c_read_reg(handle, SI12T_CAL_HOLD1_ADDR, &data);
    si12t_i2c_read_reg(handle, SI12T_CAL_HOLD2_ADDR, &data);

    return ret;
}

int si12t_read_touch_result(si12t_handle_t handle, uint8_t *touch_result)
{
    if (handle == NULL || touch_result == NULL) {
        return -EINVAL;
    }

    return si12t_i2c_read_reg(handle, SI12T_OUTPUT1_ADDR, touch_result);
}

void si12t_parse_touch_result(uint8_t touch_result)
{
    int index = 0;
    for (int j = 0; j < 6; j += 2) {
        si12t_point_type[index] = (touch_result >> j) & 0x03;
        index++;
    }
}

void si12t_parse_touch_result_to(uint8_t touch_result, uint8_t *parsed_result)
{
    int index = 0;
    for (int j = 0; j < 6; j += 2) {
        parsed_result[index] = (touch_result >> j) & 0x03;
        index++;
    }
}
