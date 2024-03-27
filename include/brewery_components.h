#ifndef BREWERY_COMPONENTS_H__
#define BREWERY_COMPONENTS_H__

#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <wiringPi.h>

struct Named {
	std::string name;
public:
	Named(std::string n) : name(n) {}
	std::string getName() const {return name;}
};

namespace Details {
template<class T>
class construct {
	template<std::size_t...I, class...Ts>
		static T from_helper_impl(std::index_sequence<I...>, std::tuple<Ts...>&& tup) {
			return T{std::get<I>(tup)...};
		}
	public:
	template<class F>
		static T from(F&& f)
		{
			return {f};
		}
	template<class...Ts>
		static T from(std::tuple<Ts...>&& tup)
		{
			return from_helper_impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(tup));
		}
};
} /* namespace Details */

template<class First, class...Rest>
struct ComponentTuple : Named {
	First first;
	ComponentTuple<Rest...> rest;
	template<class PFirst, class...PRest>
		ComponentTuple(std::string n, PFirst&& pf, PRest&&...pr) :
			Named(n),
			first{Details::construct<First>::from(std::forward<PFirst>(pf))},
		rest{n, std::forward<PRest>(pr)...}
	{}
	template<std::size_t I>
	auto& get()
	{
		static_assert( I < 1+sizeof...(Rest) );
		if constexpr (I == 0)
			return first;
		else
			return rest.template get<I-1>();
	}
};

template<class First>
struct ComponentTuple<First> : Named {
	First first;
	template<class PFirst>
		ComponentTuple(std::string n, PFirst&& pf) :
			Named(n),
			first{Details::construct<First>::from(std::forward<PFirst>(pf))}
	{}
	template<std::size_t I>
	auto& get()
	{
		static_assert( I == 0 );
		return first;
	}
};

template<class...Ts, class Func>
void for_each_component(ComponentTuple<Ts...>& tup, Func&& f)
{
	f(tup.first);
	if constexpr (sizeof...(Ts) > 1)
		for_each_component(tup.rest, f);
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

std::size_t time_in_seconds();

class TempSensor : public Named {
	int pin_num;
	std::size_t start_time;
	// time in seconds since start of program -> temp in F
	std::vector<std::pair<std::size_t,double>> tempHistory;
	std::mutex mut;
	RepeatThread update_thread;
	void update();
public:
	TempSensor(std::string name, int pin_num, const char* deviceId);
	TempSensor(const TempSensor& rhs);
	double getTempF();
	friend class HistoryAccess;
	class HistoryAccess {
		TempSensor& t;
	public:
		HistoryAccess(TempSensor& t) : t{t} {t.mut.lock();}
		auto operator [](unsigned i) {return t.tempHistory[i];}
		std::size_t size() {return t.tempHistory.size();}
		~HistoryAccess() {t.mut.unlock();}
	};
	HistoryAccess getHistory() {return {*this};}
};

class CountEdges {
	int edges;
	static void update(void* v);
public:
	CountEdges(int PinNum, int EdgeType);
	CountEdges(const CountEdges&)=delete; // the ISR uses our address, so we cant move or copy
	int getEdges() {
		return edges;
	}
};

template<class T>
struct ReadableValue : public Named {
	using Named::Named;
	virtual T get()=0;
	virtual ~ReadableValue()=default;
};

static constexpr auto LitersPerGallon = 3.785412;

class FlowSensor : public ReadableValue<double> {
	CountEdges sensor;
	int initialEdgeCount = 0;
	int edgeCount() {
		return sensor.getEdges() - initialEdgeCount;
	}
	int EdgesPerLiter;
public:
	void resetFlowCount() {
		initialEdgeCount = sensor.getEdges();
	}
	FlowSensor(std::string n, int PinNum, int EdgesPerLiter=600) : ReadableValue<double>{n}, sensor{PinNum, INT_EDGE_RISING}, EdgesPerLiter{EdgesPerLiter} {
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

