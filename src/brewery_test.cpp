#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>
#include <wiringPi.h>
#include <ds18b20.h>
#include "crow_integration.h"
#include "brewery_components.h"
#include "board_layout.h"
#include "web_components.h"
#include "i2c.h"

/*
	build with:
		g++ brewery_test.cpp -I ../crow/include/ --std=c++17 -pthread -lwiringPi -lboost_system -latomic -Wno-psabi -g
	need to have loaded the 1-wire bus via:
		sudo dtoverlay w1-gpio
		then devices will be at /sys/bus/w1/devices/
*/

/* relay output */
constexpr auto HLT_REFLOW_VALVE_PIN = RELAY1_PIN;
constexpr auto PUMP_ASSEMBLY_INPUT_VALVE_PIN = RELAY2_PIN;
constexpr auto PUMP_ASSEMBLY_OUTPUT_VALVE_PIN = RELAY3_PIN;
/* I2C connected */
constexpr auto HLT_TEMP_PIN = 65; /* this doesnt map to anything physical, just needs to be unique */
constexpr auto HLT_TEMP_ID = "00000554cba2";
constexpr auto PUMP_ASSEMBLY_TEMP_PIN = 66; /* this doesnt map to anything physical, just needs to be unique */
constexpr auto PUMP_ASSEMBLY_TEMP_ID = "derpderpderp";
/* SSR output */
constexpr auto HLT_PUMP_PIN = SSR1_PIN;
constexpr auto PUMP_ASSEMBLY_PUMP_PIN = SSR2_PIN;
/* Digital in */
constexpr auto HLT_INPUT_FLOW_PIN = DIG1_PIN;
constexpr auto HLT_OUTPUT_FLOW_PIN = DIG2_PIN;
constexpr auto MT_OUTPUT_FLOW_PIN = DIG3_PIN;
//MT_LIQUID_MAX_PIN

/* Digital out */
constexpr auto HLT_HEATER_PIN = DIG4_PIN;
constexpr auto BREW_KETTLE_HEATER_PIN = DIG5_PIN;

struct HotLiquorTank : public ComponentTuple<FlowSensor, Heater, Valve, Pump, TempSensor, FlowSensor> {
	HotLiquorTank(std::string name) :
		ComponentTuple(name,
				std::make_tuple("input_flow", HLT_INPUT_FLOW_PIN),
				std::make_tuple("heater",50,200,HLT_HEATER_PIN),
				std::make_tuple("reflow_valve",HLT_REFLOW_VALVE_PIN),
				std::make_tuple("pump",HLT_PUMP_PIN),
				std::make_tuple("reflow_temp", HLT_TEMP_PIN, HLT_TEMP_ID),
				std::make_tuple("output_flow", HLT_OUTPUT_FLOW_PIN)
			) {}
	void update()
	{
		auto& heater = this->get<1>();
		auto& temp = this->get<4>();
		if( temp.getTempF() < heater.get() )
			heater.on();
		else
			heater.off();
	}
};

struct MashTun : public ComponentTuple<LevelSensor, FlowSensor> {
	MashTun(std::string name) :
		ComponentTuple(name,
				"liquid_max",
				std::make_tuple("output_flow", MT_OUTPUT_FLOW_PIN)
			) {}
};

struct BrewKettle : public ComponentTuple<Button> {
	BrewKettle(std::string name) : ComponentTuple(name, std::make_tuple("heater",BREW_KETTLE_HEATER_PIN)) {}
};

struct PumpAssembly : public ComponentTuple<Valve, Pump, TempSensor, Valve> {
	PumpAssembly(std::string name) :
		ComponentTuple(name,
				std::make_tuple("input_valve",PUMP_ASSEMBLY_INPUT_VALVE_PIN),
				std::make_tuple("pump",PUMP_ASSEMBLY_PUMP_PIN),
				std::make_tuple("temp", PUMP_ASSEMBLY_TEMP_PIN, PUMP_ASSEMBLY_TEMP_ID),
				std::make_tuple("output_valve",PUMP_ASSEMBLY_OUTPUT_VALVE_PIN)
			) {}
};

struct Brewery : public ComponentTuple<HotLiquorTank, MashTun, BrewKettle, PumpAssembly> {
	Brewery(std::string name) : ComponentTuple(name, "hlt", "mt", "bk", "pump_assembly") {}
	void update()
	{
		auto& HLT = this->get<0>();
		HLT.update();
	}
};

std::string generateLayout(Brewery& ct)
{
	std::string ret;
	for_each_component(ct, [&](auto&& comp) {
			ret += generateLayout(std::forward<decltype(comp)>(comp));
		});
	return ret;
}

int main(int argc, char* argv[])
{
	wiringPiSetup();
	Brewery brewery("brewery");
	RepeatThread update_thread([&](){
		brewery.update();
	}, 100);
	SimpleApp app;
	crow_mustache_set_base("/home/admin/Brewing");

	for(int arg = 1; arg < argc; ++arg )
	{
		std::string argstr = argv[arg];
		if( argstr == "--debug" or argstr == "-debug" )
			app.loglevel(SimpleApp::Debug);
		if( argstr == "--template_dir" )
		{
			if( arg+1 < argc )
				crow_mustache_set_base(argv[++arg]);
			else
			{
				std::cerr << "need directory after --template_dir option!" << std::endl;
				return -1;
			}
		}
	}

    app.route_dynamic("/",
    [&]{
		JSONWrapper ctx;
		ctx.set("title", "brewery controller test");
		ctx.set("brewery_layout", generateLayout(brewery));
		ctx.set("update_js", generateUpdateJS(brewery, {}));
		return crow_mustache_load("static_main.html", ctx);
    });
	app.route_dynamic("/reboot",
	[&]{
		system("shutdown -r now");
		return "rebooting system.";
	});
	app.route_dynamic("/shutdown",
	[&]{
		system("shutdown -P now");
		return "shutting down system.";
	});
	app.route_dynamic("/quit",
	[&]{
		app.stop();
		return "";
	});
	app.route_dynamic("/i2c/status",
	[&]() -> std::string {
		if( is_i2c_setup() )
			return "true";
		return "false";
	});
	app.route_dynamic("/i2c/list",
	[&]{
		std::string ret("[");
		bool first = true;
		for(auto&& e : get_i2c_devices())
		{
			if( first )
				first = false;
			else
				ret += ",";
			ret += "{\"value\":\"";
			ret += e;
			ret += "\"}";
		}
		ret += "]";
		return ret;
	});

	registerEndpoints(brewery, app,"");

	app.run_on_port(40080);
}
