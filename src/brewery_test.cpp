#define CROW_MAIN
#include "crow.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <wiringPi.h>
#include <ds18b20.h>
#include "crow_integration.h"
#include "brewery_components.h"
#include "board_layout.h"

/*
	build with:
		g++ brewery_test.cpp -I ../crow/include/ --std=c++17 -pthread -lwiringPi -lboost_system -latomic -Wno-psabi -g
	need to have loaded the 1-wire bus via:
		sudo dtoverlay w1-gpio
*/

/* relay output */
constexpr auto HLT_REFLOW_VALVE_PIN = RELAY1_PIN;
constexpr auto PUMP_ASSEMBLY_INPUT_VALVE_PIN = RELAY2_PIN;
constexpr auto PUMP_ASSEMBLY_OUTPUT_VALVE_PIN = RELAY3_PIN;
/* I2C connected */
constexpr auto HLT_TEMP_PIN = 65; /* this doesnt map to anything physical, just needs to be unique */
constexpr auto HLT_TEMP_ID = "0000055823d0";
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

struct HotLiquorTank : public ComponentTuple<FlowSensor<HLT_INPUT_FLOW_PIN>, Heater, Valve, Pump, TempSensor, FlowSensor<HLT_OUTPUT_FLOW_PIN>> {
	HotLiquorTank(std::string name) : ComponentTuple(name, "input_flow", Heater{"heater",50,200,HLT_HEATER_PIN}, Valve{"reflow_valve",HLT_REFLOW_VALVE_PIN}, Pump{"pump",HLT_PUMP_PIN}, TempSensor{"reflow_temp", HLT_TEMP_PIN, HLT_TEMP_ID}, "output_flow") {}
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

struct MashTun : public ComponentTuple<LevelSensor, FlowSensor<MT_OUTPUT_FLOW_PIN>> {
	MashTun(std::string name) : ComponentTuple(name, "liquid_max", "output_flow") {}
};

struct BrewKettle : public ComponentTuple<Button> {
	BrewKettle(std::string name) : ComponentTuple(name, Button{"heater",BREW_KETTLE_HEATER_PIN}) {}
};

struct PumpAssembly : public ComponentTuple<Valve, Pump, TempSensor, Valve> {
	PumpAssembly(std::string name) : ComponentTuple(name, Valve{"input_valve",PUMP_ASSEMBLY_INPUT_VALVE_PIN}, Pump{"pump",PUMP_ASSEMBLY_PUMP_PIN}, TempSensor{"temp", PUMP_ASSEMBLY_TEMP_PIN, PUMP_ASSEMBLY_TEMP_ID}, Valve{"output_valve",PUMP_ASSEMBLY_OUTPUT_VALVE_PIN}) {}
};

struct Brewery : public ComponentTuple<HotLiquorTank, MashTun, BrewKettle, PumpAssembly> {
	Brewery(std::string name) : ComponentTuple(name, "hlt", "mt", "bk", "pump_assembly") {}
	void update()
	{
		auto& HLT = this->get<0>();
		HLT.update();
	}
};

int main(int argc, char* argv[])
{
	wiringPiSetup();
	Brewery brewery("brewery");
	std::thread update_thread([&](){
			while(true)
			{
				brewery.update();
				using namespace std::chrono_literals;
				std::this_thread::sleep_for(100ms);
			}
		});
	crow::SimpleApp app;
    crow::mustache::set_base("/home/pi/Brewing");

	for(int arg = 1; arg < argc; ++arg )
	{
		std::string argstr = argv[arg];
		if( argstr == "--debug" or argstr == "-debug" )
			app.loglevel(crow::LogLevel::Debug);
		if( argstr == "--template_dir" )
		{
			if( arg+1 < argc )
				crow::mustache::set_base(argv[++arg]);
			else
			{
				std::cerr << "need directory after --template_dir option!" << std::endl;
				return -1;
			}
		}
	}

    CROW_ROUTE(app, "/")
    ([&]{
        crow::mustache::context ctx;
		ctx["title"] = "brewery controller test";
		ctx["brewery_layout"] = brewery.generateLayout();
		ctx["update_js"] = brewery.generateUpdateJS({});
        return crow::mustache::load("static_main.html").render(ctx);
    });

	CROW_ROUTE(app, "/reboot")
	([&]{
		system("shutdown -r now");
		return "rebooting system.";
	});
	CROW_ROUTE(app, "/shutdown")
	([&]{
		system("shutdown -P now");
		return "shutting down system.";
	});
	CROW_ROUTE(app, "/quit")
	([&]{
		app.stop();
		return "";
	});
	
	brewery.registerEndpoints(app,"");
	
	app.port(40080).run();
}
