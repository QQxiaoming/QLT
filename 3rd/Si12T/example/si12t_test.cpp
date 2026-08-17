#include "Si12T.h"

#include <cstdio>

int main(int argc, char *argv[])
{
	const char *i2c_device = argc > 1 ? argv[1] : "/dev/i2c-1";
	si12t_config_t config = {.i2c_device = i2c_device, .dev_addr = SI12T_GND_ADDRESS};
	si12t_handle_t handle = nullptr;

	int ret = si12t_init(&config, &handle);
	if (ret != 0) {
		std::fprintf(stderr, "Failed to open %s: %d\n", i2c_device, ret);
		return 1;
	}

	ret = si12t_setup(handle, SI12T_TYPE_LOW, SI12T_SENSITIVITY_LEVEL_3);
	uint8_t touch_result = 0;
	if (ret == 0) {
		ret = si12t_read_touch_result(handle, &touch_result);
	}
	if (ret == 0) {
		si12t_parse_touch_result_to(touch_result, si12t_point_type);
		std::printf("touch: %u %u %u\n", si12t_point_type[0], si12t_point_type[1], si12t_point_type[2]);
	} else {
		std::fprintf(stderr, "Si12T I2C operation failed: %d\n", ret);
	}

	si12t_delete(handle);
	return ret == 0 ? 0 : 1;
}
