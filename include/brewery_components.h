#ifndef BREWERY_COMPONENTS_H__
#define BREWERY_COMPONENTS_H__

#include "crow.h"
#include <thread>
#include <atomic>
#include <string>
#include <wiringPi.h>
#include <ds18b20.h>
#include "crow_integration.h"

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
	std::atomic<double> temp = 0.0;
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
		return getTempF();
	}
	double getTempF() {
		return temp;
	}
};

template<int PinNum, int EdgeType>
class CountEdges {
	static std::atomic<int> edges;
	static void update() {
		++edges;
	}
public:
	CountEdges() {
		wiringPiISR(PinNum, EdgeType, &update);
	}
	int getEdges() {
		return edges;
	}
};

template<int PinNum, int EdgeType>
std::atomic<int> CountEdges<PinNum,EdgeType>::edges = 0;

static constexpr auto LitersPerGallon = 3.785412;

template<int PinNum, int EdgesPerLiter=600>
class FlowSensor : public ReadableValue<double> {
	CountEdges<PinNum, INT_EDGE_RISING> sensor;
	int initialEdgeCount;
	int edgeCount() {
		return sensor.getEdges() - initialEdgeCount;
	}
public:
	using ReadableValue<double>::ReadableValue;
	void resetFlowCount() {
		initialEdgeCount = sensor.getEdges();
	}
	FlowSensor() { 
		resetFlowCount();
		CROW_LOG_INFO << "FlowSensor<" << PinNum << ", " << EdgesPerLiter << ">:";
		CROW_LOG_INFO << "initialEdgeCount = " << initialEdgeCount;
		CROW_LOG_INFO << "edgeCount() = " << edgeCount();
	}
	double getFlowInLiters() {
		return edgeCount() / static_cast<double>(EdgesPerLiter);
	}
	double getFlowInGallons() {
		return getFlowInLiters() / LitersPerGallon;
	}
	virtual double get() {
		return getFlowInGallons();
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



#endif

