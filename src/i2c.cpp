#include "i2c.h"
#include <filesystem>
	
const std::filesystem::path devices_path{"/sys/bus/w1/devices"};

bool is_i2c_setup()
{
	return std::filesystem::directory_entry{devices_path}.exists();
}

const std::string prefix = "28-";

std::vector<std::string> get_i2c_devices()
{
	std::vector<std::string> ret;
#ifdef MOCK
	ret.push_back("050505");
	ret.push_back("3A3A3A");
#else
	for (auto const& dir_entry : std::filesystem::directory_iterator{devices_path})
	{
		std::string name = dir_entry.path().filename().string();
		if( name != "w1_bus_master1" )
			ret.push_back(name.substr(prefix.size())); //strip off prefix
	}
#endif
	return ret;
}

#include <map>

std::map<int, std::string> pin_to_device_id;

extern "C" int ds18b20Setup (const int pinBase, const char *deviceId);

bool setI2CDeviceForPin(int pin, std::string device_id)
{
	auto ret = ds18b20Setup(pin, device_id.c_str());
	if( ret )
		pin_to_device_id[pin] = device_id;
	return ret;
}

std::string getI2CDeviceForPin(int pin)
{
	if( pin_to_device_id.count(pin) )
		return pin_to_device_id[pin];
	else
		return "[unmapped]";
}

