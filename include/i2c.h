#ifndef I2C_H__
#define I2C_H__

#include <string>
#include <vector>

bool is_i2c_setup();
std::vector<std::string> get_i2c_devices();

bool setI2CDeviceForPin(int, std::string);
std::string getI2CDeviceForPin(int);

#endif

