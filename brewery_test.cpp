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
	virtual void modify(std::vector<std::string>)=0;
};

template<class First, class...Rest>
class ComponentTuple : public ComponentTuple<Rest...> {
	First first;
public:
	template<class CFirst, class...CRest>
	ComponentTuple(std::string name, CFirst cf, CRest... cr) : first(cf), ComponentTuple<Rest...>(name, cr...) {}
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret = ComponentTuple<Rest...>::getStatus();
		ret[first.getName()] = first.getStatus();
		return ret;
	}
	template<std::size_t N>
	auto get() {
		if constexpr (N == 0)
			return first;
		else
			return ComponentTuple<Rest...>::template get<N-1>();
	}
	virtual void modify(std::vector<std::string> s) override {
		if( this->getName() != s.at(0) )
		{
			CROW_LOG_DEBUG << this->getName() << " is handling; did not match";
			return;
		}
		if( first.getName() == s.at(1) )
		{
			s.erase(s.begin()); //pop_front
			CROW_LOG_DEBUG << this->getName() << " is handling; pass along to child";
			return first.modify(s);
		}
		CROW_LOG_DEBUG << this->getName() << " is handling; checking next";
		ComponentTuple<Rest...>::modify(s);
	}
};

template<class First>
class ComponentTuple<First> : public ComponentBase {
	First first;
public:
	template<class CFirst>
	ComponentTuple(std::string name, CFirst cf) : first(cf), ComponentBase(name) {}
	virtual crow::json::wvalue getStatus() override {
		crow::json::wvalue ret;
		ret[first.getName()] = first.getStatus();
		return ret;
	}
	template<std::size_t N>
	auto get() {
		static_assert( N == 0 and "N too large for component!" );
		return first;
	}
	virtual void modify(std::vector<std::string> s) override {
		if( this->getName() != s.at(0) )
		{
			CROW_LOG_DEBUG << this->getName() << " is handling; did not match";
			return;
		}
		CROW_LOG_DEBUG << this->getName() << " is handling; pass along to child";
		s.erase(s.begin()); //pop_front
		first.modify(s); // pass it along
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
	virtual void modify(std::vector<std::string> s) override {
		CROW_LOG_DEBUG << this->getName() << " is handling; but is readable, not writable";
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
	using ReadableValue<T>::ReadableValue;
	void set(T v) {value = v;}
	virtual T get() override {return value;}
	virtual void modify(std::vector<std::string> s) override {
		if( this->getName() != s.at(0) )
		{
			CROW_LOG_DEBUG << this->getName() << " is handling; did not match";
			return;
		}
		CROW_LOG_DEBUG << this->getName() << " is handling; toggling value";
		s.erase(s.begin());
		value = !value;
	}
};

class Valve : public WriteableValue<double> {
	using WriteableValue<double>::WriteableValue;
};

class Pump : public WriteableValue<double> {
	using WriteableValue<double>::WriteableValue;
};

class Heater : public WriteableValue<double> {
	using WriteableValue<double>::WriteableValue;
};

struct HotLiquorTank : public ComponentTuple<FlowSensor, Heater, Valve, Pump, FlowSensor> {
	HotLiquorTank(std::string name) : ComponentTuple(name, "input_flow", "heater", "reflow_valve", "pump", "output_flow") {}
};

struct MashTun : public ComponentTuple<LevelSensor, TempSensor> {
	MashTun(std::string name) : ComponentTuple(name, "liquid_max", "reflow_temp") {}
};

struct BrewKettle : public ComponentTuple<Heater> {
	BrewKettle(std::string name) : ComponentTuple(name, "heater") {}
};

struct PumpAssembly : public ComponentTuple<Valve, Pump, TempSensor, Valve> {
	PumpAssembly(std::string name) : ComponentTuple(name, "input_valve", "pump", "temp", "output_valve") {}
};

struct Brewery : public ComponentTuple<HotLiquorTank, MashTun, BrewKettle, PumpAssembly> {
	Brewery(std::string name) : ComponentTuple(name, "hlt", "mt", "bk", "pump_assembly") {}
};

std::vector<std::string> split(const std::string& str, const std::string& delim) {
	std::vector<std::string> strings;
	size_t start = 0;
	size_t end = 0;
	while (start != std::string::npos and end != std::string::npos and start < str.length() and end < str.length()) {
		end = str.find(delim, start);
		if( end != start )
			strings.push_back(str.substr(start, end - start));
		start = end + delim.length();
	}
	return strings;
}

int main(int argc, char* argv[])
{
	Brewery brewery("brewery");
	crow::SimpleApp app;
	if( argc > 1 )
		if( argv[1] == std::string("-debug") )
			app.loglevel(crow::LogLevel::Debug);

    crow::mustache::set_base(".");

    CROW_ROUTE(app, "/")
    ([]{
        crow::mustache::context ctx;
		ctx["title"] = "brewery controller test";
        return crow::mustache::load("example_chat.html").render(ctx);
    });

	CROW_ROUTE(app, "/status")
	([&]{
		return brewery.getStatus();
	});

	CROW_ROUTE(app, "/modify/<string>")
	([&](std::string s){
		CROW_LOG_DEBUG << s;
		auto strs = split(s, "%20");
		for(auto s : strs)
			CROW_LOG_DEBUG << s;
		brewery.modify(strs);
		return "";
	});

	app.port(40080)
        //.multithreaded()
        .run();
}
