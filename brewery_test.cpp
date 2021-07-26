#define CROW_MAIN
#include "crow.h"
#include <string>
#include <vector>

struct Named {
	std::string name;
public:
	Named(std::string n) : name(n) {}
	std::string getName() {return name;}
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

class TempSensor : public ReadableValue<double> {
public:
	using ReadableValue<double>::ReadableValue;
	virtual double get() {
		return 117.0;
	}
	double getTempF() {
		double f = this->get();
		return f;
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
	void set(T v) {value = v;}
	virtual T get() override {return value;}
};

class Button : public WriteableValue<int> {
public:
	using WriteableValue<int>::WriteableValue;
	virtual void registerEndpoints(crow::SimpleApp& app, std::string endpointPrefix) override {
		ReadableValue<int>::registerEndpoints(app, endpointPrefix);
		app.route_dynamic(endpointPrefix+"/"+this->getName()+"/toggle")
		([&](){
			this->set(!this->get());
			return "";
		});
	}
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
public:
	using TargetValue<double>::TargetValue;
};

struct HotLiquorTank : public ComponentTuple<FlowSensor, Heater, Valve, Pump, TempSensor, FlowSensor> {
	HotLiquorTank(std::string name) : ComponentTuple(name, "input_flow", Heater{"heater",100,200}, "reflow_valve", "pump", "reflow_temp", "output_flow") {}
};

struct MashTun : public ComponentTuple<LevelSensor, FlowSensor> {
	MashTun(std::string name) : ComponentTuple(name, "liquid_max", "output_flow") {}
};

struct BrewKettle : public ComponentTuple<Button> {
	BrewKettle(std::string name) : ComponentTuple(name, "heater") {}
};

struct PumpAssembly : public ComponentTuple<Valve, Pump, TempSensor, Valve> {
	PumpAssembly(std::string name) : ComponentTuple(name, "input_valve", "pump", "temp", "output_valve") {}
};

struct Brewery : public ComponentTuple<HotLiquorTank, MashTun, BrewKettle, PumpAssembly> {
	Brewery(std::string name) : ComponentTuple(name, "hlt", "mt", "bk", "pump_assembly") {}
};

int main(int argc, char* argv[])
{
	Brewery brewery("brewery");
	crow::SimpleApp app;
	if( argc > 1 )
		if( argv[1] == std::string("-debug") )
			app.loglevel(crow::LogLevel::Debug);

    crow::mustache::set_base(".");

    CROW_ROUTE(app, "/")
    ([&]{
        crow::mustache::context ctx;
		ctx["title"] = "brewery controller test";
		ctx["brewery_layout"] = brewery.generateLayout();
		ctx["update_js"] = brewery.generateUpdateJS({});
        return crow::mustache::load("static_main.html").render(ctx);
    });

	brewery.registerEndpoints(app,"");
	
	app.port(40080).run();
}
