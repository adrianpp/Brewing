#define CROW_MAIN
#include "crow.h"
#include <string>
#include <vector>
#include <thread>
#include <wiringPi.h>
#include <ds18b20.h>

/*
	build with:
		g++ brewery_test.cpp -I ../crow/include/ --std=c++17 -pthread -lwiringPi -lboost_system -g
	need to have loaded the 1-wire bus via:
		sudo dtoverlay w1-gpio gpiopin=26
*/

/* relay output */
constexpr auto HLT_REFLOW_VALVE_PIN = 0;
constexpr auto PUMP_ASSEMBLY_INPUT_VALVE_PIN = 4;
constexpr auto PUMP_ASSEMBLY_OUTPUT_VALVE_PIN = 6;
/* I2C connected */
constexpr auto HLT_TEMP_PIN = 65; /* this doesnt map to anything physical, just needs to be unique */
constexpr auto HLT_TEMP_ID = "0000055823d0";
constexpr auto PUMP_ASSEMBLY_TEMP_PIN = 66; /* this doesnt map to anything physical, just needs to be unique */
constexpr auto PUMP_ASSEMBLY_TEMP_ID = "derpderpderp";
/* SSR output */
constexpr auto HLT_PUMP_PIN = 2;
constexpr auto PUMP_ASSEMBLY_PUMP_PIN = 5;
/* Digital in */
// HLT_INPUT_FLOW_PIN
// HLT_OUTPUT_FLOW_PIN
// MT_LIQUID_MAX_PIN
// MT_OUTPUT_FLOW_PIN

/* Digital out */
constexpr auto HLT_HEATER_PIN = 7;
constexpr auto BREW_KETTLE_HEATER_PIN = 3;

struct Named {
	std::string name;
public:
	Named(std::string n) : name(n) {}
	std::string getName() const {return name;}
};

struct ComponentBase : public Named {
	using Named::Named;
	virtual crow::json::wvalue getStatus()=0;
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) {
		app.route_dynamic(endpointPrefix+"/"+this->getName()+"/status")
		([&](){
			return this->getStatus();
		});
	}
	virtual std::string generateLayout()=0;
	virtual std::string generateUpdateJS(std::vector<std::string> parent){return "";}
};

template<class First, class...Rest>
class ComponentTuple : public ComponentTuple<Rest...> {
	First first;
protected:
	virtual std::string generateChildLayout() override {
		std::string ret = first.generateLayout();
		ret += ComponentTuple<Rest...>::generateChildLayout();
		return ret;
	}
public:
	template<class CFirst, class...CRest>
	ComponentTuple(std::string name, CFirst cf, CRest... cr) : first(cf), ComponentTuple<Rest...>(name, cr...) {}
	template<int N>
	auto& get() {
		if constexpr (N == 0)
			return first;
		else
			return ComponentTuple<Rest...>::template get<N-1>();	
	}
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret = ComponentTuple<Rest...>::getStatus();
		ret[first.getName()] = first.getStatus();
		return ret;
	}
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		first.registerEndpoints(app, endpointPrefix+"/"+this->getName());
		ComponentTuple<Rest...>::registerEndpoints(app, endpointPrefix);
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent) override {
		std::string ret = ComponentTuple<Rest...>::generateUpdateJS(parent);
		parent.push_back(this->getName());
		ret += first.generateUpdateJS(parent);
		return ret;
	}
};

template<class First>
class ComponentTuple<First> : public ComponentBase {
	First first;
protected:
	virtual std::string generateChildLayout() {
		return first.generateLayout();
	}
public:
	template<class CFirst>
	ComponentTuple(std::string name, CFirst cf) : first(cf), ComponentBase(name) {}
	template<int N>
	auto& get() {
		static_assert(N == 0);
		return first;
	}
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret;
		ret[first.getName()] = first.getStatus();
		return ret;
	}
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		first.registerEndpoints(app, endpointPrefix+"/"+this->getName());
		ComponentBase::registerEndpoints(app, endpointPrefix);
	}
	virtual std::string generateLayout() override {
		std::string ret = "<fieldset id=\"" + this->getName() + "\">\n";
		ret += "<legend>" + this->getName() + "</legend>\n";
		ret += this->generateChildLayout();
		ret += "</fieldset>\n";
		return ret;
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent) override {
		parent.push_back(this->getName());
		return first.generateUpdateJS(parent);
	}
};

template<class T>
struct ReadableValue : public ComponentBase {
	using ComponentBase::ComponentBase;
	virtual T get()=0;
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret;
		ret = std::to_string(get());
		return ret;
	}
	virtual std::string generateLayout() override {
		return "<div id=\"" + this->getName() + "\"></div>\n";
	}
	virtual std::string generateSelector(std::vector<std::string> parent)
	{
		std::string selector;
		for(auto p : parent)
			selector += "#" + p + " > ";
		selector += "#" + this->getName();
		return selector;
	}
	virtual std::string generateEndpoint(std::vector<std::string> parent)
	{
		std::string endpoint;
		for(auto p : parent)
			endpoint += "/" + p;
		endpoint += "/" + this->getName();
		return endpoint;
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent)
	{
		std::string selector = generateSelector(parent);
		std::string endpoint = generateEndpoint(parent);
		return "registerText('" + endpoint + "', '" + selector + "');\n";
	}
};

class DigitalPin {
	const int pin;
	const int mode;
	const bool active_high;
public:
	constexpr DigitalPin(int p, int mode=OUTPUT, bool active_high=true) : pin(p), mode(mode), active_high(active_high) {
	}
	void setup() const {pinMode(pin, mode);off();}
	void on() const {digitalWrite(pin, active_high?HIGH:LOW);}
	void off()const {digitalWrite(pin, active_high?LOW:HIGH);}
};

class TempSensor : public ReadableValue<double> {
	int pin_num;
	std::thread update_thread;
	float temp = 0.0;
	void update() {
		while(true) {
			temp = (analogRead(pin_num) / 10.0) * 1.8 + 32; // return in F
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(2000ms);
		}
	}
public:
	TempSensor(std::string name, int pin_num, const char* deviceId) : 
		ReadableValue<double>(name),
		pin_num{pin_num},
		update_thread{[&](){this->update();}}
	{
		ds18b20Setup(pin_num, deviceId);
	}
	TempSensor(const TempSensor& rhs) :
		ReadableValue<double>(rhs.getName()),
		pin_num{rhs.pin_num},
		update_thread{[&](){this->update();}}
	{}
	~TempSensor()
	{
		auto id = update_thread.native_handle();
		update_thread.detach();
		pthread_cancel(id);
	}
	virtual double get() {
		return temp;
	}
	double getTempF() {
		return this->get();
	}
};

class FlowSensor : public ReadableValue<double> {
public:
	using ReadableValue<double>::ReadableValue;
	virtual double get() {
		return 5.0;
	}
	double getFlowInLiter() {
		double f = this->get();
		return f;
	}
};

class LevelSensor : public ReadableValue<int> {
public:
	using ReadableValue<int>::ReadableValue;
	virtual int get() {
		return false;
	}
};

template<class T>
class WriteableValue : public ReadableValue<T> {
	T value{0};
public:
	WriteableValue(std::string name, T v=0) : ReadableValue<T>(name), value(v) {}
	virtual void set(T v) {value = v;}
	virtual T get() override {return value;}
};

class Button : public WriteableValue<int> {
	DigitalPin pin;
public:
	Button(std::string name, int pin_num, int v=0) : WriteableValue<int>(name,v), pin(pin_num, OUTPUT, false) {
		pin.setup();
	}
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		ReadableValue<int>::registerEndpoints(app, endpointPrefix);
		app.route_dynamic(endpointPrefix+"/"+this->getName()+"/toggle")
		([&](){
			this->set(!this->get());
			return "";
		});
	}
	virtual void set(int v) override {if(v) pin.on(); else pin.off(); WriteableValue<int>::set(v);}
	virtual std::string generateLayout() override {
		return "<button id=\"" + this->getName() + "\">" + this->getName() + "</button>\n";
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent)
	{
		std::string selector = generateSelector(parent);
		std::string endpoint = generateEndpoint(parent);
		return "registerButton('" + endpoint + "', '" + selector + "');\n";
	}
};

#include <sstream>

template<class T>
class TargetValue : public WriteableValue<T> {
	T MinValue;
	T MaxValue;
public:
	TargetValue(std::string name, T min, T max) : WriteableValue<T>(name, min), MinValue(min), MaxValue(max) {}
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		WriteableValue<T>::registerEndpoints(app, endpointPrefix);
		app.route_dynamic(endpointPrefix+"/"+this->getName()+"/set_target")
		([&](const crow::request& req){
			std::stringstream ss;
			ss << req.url_params.get("value");
			T v;
			ss >> v;
			this->set(v);
			return "";
		});
	}
	virtual std::string generateLayout() override {
		std::string ret;
	   	ret += "<div id=\"" + this->getName() + "_label\"></div>\n";
	   	ret += "<input id=\"" + this->getName() + "\" type='range'/>\n";
		return ret;
	}
	virtual std::string generateUpdateJS(std::vector<std::string> parent) override
	{
		std::string selector = WriteableValue<T>::generateSelector(parent);
		std::string endpoint = WriteableValue<T>::generateEndpoint(parent);
		return "registerTargetValue(\'" + endpoint + "\', \'" + selector + "\'," + std::to_string(MinValue) + ", " + std::to_string(MaxValue) + ");\n";
	}
};

class Valve : public Button {
	using Button::Button;
};

class Pump : public Button {
	using Button::Button;
};

class Heater : public TargetValue<double> {
	DigitalPin pin;
public:
	Heater(std::string name, int MinValue, int MaxValue, int pin_num) : TargetValue<double>(name, MinValue, MaxValue), pin(pin_num, OUTPUT, true) {
		pin.setup();
	}
	void on() {pin.on();}
	void off() {pin.off();}
};

struct HotLiquorTank : public ComponentTuple<FlowSensor, Heater, Valve, Pump, TempSensor, FlowSensor> {
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

struct MashTun : public ComponentTuple<LevelSensor, FlowSensor> {
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
	if( argc > 1 )
		if( argv[1] == std::string("-debug") )
			app.loglevel(crow::LogLevel::Debug);

    crow::mustache::set_base("/home/pi/Brewing");

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
	
	brewery.registerEndpoints(app,"");
	
	app.port(40080).run();
}
