#ifndef BREWERY_COMPONENTS_H__
#define BREWERY_COMPONENTS_H__

#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <tuple>
#include <wiringPi.h>
#include <ds18b20.h>

struct Named {
	std::string name;
public:
	Named(std::string n) : name(n) {}
	std::string getName() const {return name;}
};

template<class...Comps>
struct ComponentTuple : Named, std::tuple<Comps...> {
	template<class...Ts>
	ComponentTuple(std::string s, Ts&&...ts) : Named(s), std::tuple<Comps...>(std::forward<Ts>(ts)...) {}
};

namespace std {
template<class...Comps>
struct tuple_size<ComponentTuple<Comps...>> {
	static constexpr auto value = sizeof...(Comps);
};
}

template<class...Ts, class Func>
void for_each_component(ComponentTuple<Ts...>& tup, Func&& f)
{
	std::apply(
			[&](auto&&...children){
				([&](auto&& child) {
					f(child);
				 }(children),...);
			},
			tup);
}

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

struct RepeatThread {
	bool finished = false;
	unsigned ms_count = 0;
	std::thread t;
	template<class F>
	RepeatThread(F f, unsigned ms_sleep_time) : t([=](){
		while(!finished)
		{
			if( ms_count == 0 )
				f();
			++ms_count;
			if( ms_count == ms_sleep_time )
				ms_count = 0;
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1ms);
		}
	})
	{}
	~RepeatThread()
	{
		finished = true;
		t.join();
	}
};

class TempSensor : public Named {
	int pin_num;
	std::vector<double> tempHistory;
	std::mutex mut;
	RepeatThread update_thread;
	void update() {
		auto temp = (analogRead(pin_num) / 10.0) * 1.8 + 32; // return in F
		mut.lock();
		tempHistory.push_back(temp);
		mut.unlock();
	}
public:
	TempSensor(std::string name, int pin_num, const char* deviceId) :
	    Named(name),
		pin_num{pin_num},
		update_thread([&](){this->update();},2000)
	{
		ds18b20Setup(pin_num, deviceId);
	}
	TempSensor(const TempSensor& rhs) :
		Named(rhs.getName()),
		pin_num{rhs.pin_num},
		update_thread([&](){this->update();},2000)
	{}
	double getTempF() {
		if( tempHistory.size() )
		{
			std::lock_guard<std::mutex> g{mut};
			return tempHistory.back();
		}
		else
			return 0.0;
	}
	friend class HistoryAccess;
	class HistoryAccess {
		TempSensor& t;
	public:
		HistoryAccess(TempSensor& t) : t{t} {t.mut.lock();}
		double operator [](unsigned i) {return t.tempHistory[i];}
		std::size_t size() {return t.tempHistory.size();}
		~HistoryAccess() {t.mut.unlock();}
	};
	HistoryAccess getHistory() {return {*this};}
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

template<class T>
struct ReadableValue : public Named {
	using Named::Named;
	virtual T get()=0;
};

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
//		CROW_LOG_INFO << "FlowSensor<" << PinNum << ", " << EdgesPerLiter << ">:";
//		CROW_LOG_INFO << "initialEdgeCount = " << initialEdgeCount;
//		CROW_LOG_INFO << "edgeCount() = " << edgeCount();
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
	Button(std::string name, int pin_num, int v=0) : WriteableValue<int>(name,v), pin(pin_num, OUTPUT) {
		pin.setup();
	}
	virtual void set(int v) override {if(v) pin.on(); else pin.off(); WriteableValue<int>::set(v);}
};

template<class T>
class TargetValue : public WriteableValue<T> {
	T MinValue;
	T MaxValue;
public:
	TargetValue(std::string name, T min, T max) : WriteableValue<T>(name, min), MinValue(min), MaxValue(max) {}
	T getMin(){return MinValue;}
	T getMax(){return MaxValue;}
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
	Heater(std::string name, int MinValue, int MaxValue, int pin_num) : TargetValue<double>(name, MinValue, MaxValue), pin(pin_num, OUTPUT) {
		pin.setup();
	}
	void on() {pin.on();}
	void off() {pin.off();}
};

#endif

