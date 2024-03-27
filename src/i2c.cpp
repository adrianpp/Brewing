#include "i2c.h"
#include <filesystem>
	
const std::filesystem::path devices_path{"/sys/bus/w1/devices"};

bool is_i2c_setup()
{
	return std::filesystem::directory_entry{devices_path}.exists();
}

std::vector<std::string> get_i2c_devices()
{
	std::vector<std::string> ret;
	for (auto const& dir_entry : std::filesystem::directory_iterator{devices_path})
	{
		std::string name = dir_entry.path().filename().string();
		if( name != "w1_bus_master1" )
			ret.push_back(name);
	}
	return ret;
}

