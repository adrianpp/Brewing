#include "brewery_components.h"
#include <chrono>
#include <wiringPi.h>
#include "i2c.h"

void DigitalPin::setup() const {
	pinMode(pin, mode);
	off();
}
void DigitalPin::on() const {
	digitalWrite(pin, active_high?HIGH:LOW);
}
void DigitalPin::off() const {
	digitalWrite(pin, active_high?LOW:HIGH);
}


std::size_t time_in_seconds()
{
	auto dur = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::seconds>(dur).count();
}

void TempSensor::update() {
	auto temp = (analogRead(pin_num) / 10.0) * 1.8 + 32; // return in F
	auto cur_time = time_in_seconds() - start_time;
	std::lock_guard<std::mutex> g{mut};
	tempHistory.push_back({cur_time, temp});
}
TempSensor::TempSensor(std::string name, int pin_num, const char* deviceId) :
	Named(name),
	pin_num{pin_num},
	start_time(time_in_seconds()),
	update_thread([&](){this->update();},2000)
{
	setI2CDeviceForPin(pin_num, deviceId);
}
TempSensor::TempSensor(const TempSensor& rhs) :
	Named(rhs.getName()),
	pin_num{rhs.pin_num},
	start_time{rhs.start_time},
	update_thread([&](){this->update();},2000)
{}
double TempSensor::getTempF() {
	if( tempHistory.size() )
	{
		std::lock_guard<std::mutex> g{mut};
		return tempHistory.back().second;
	}
	else
		return 0.0;
}

void CountEdges::update(void* v) {
	CountEdges* me = static_cast<CountEdges*>(v);
	++(me->edges);
}
CountEdges::CountEdges(int PinNum, int EdgeType) {
	wiringPiISR_data(PinNum, EdgeType, &update, this);
}

int FlowSensor::edgeCount() {
	return sensor.getEdges() - initialEdgeCount;
}
void FlowSensor::resetFlowCount() {
	initialEdgeCount = sensor.getEdges();
}
FlowSensor::FlowSensor(std::string n, int PinNum, int EdgesPerLiter) : ReadableValue<double>{n}, sensor{PinNum, INT_EDGE_RISING}, EdgesPerLiter{EdgesPerLiter} {
	resetFlowCount();
	//		CROW_LOG_INFO << "FlowSensor<" << PinNum << ", " << EdgesPerLiter << ">:";
	//		CROW_LOG_INFO << "initialEdgeCount = " << initialEdgeCount;
	//		CROW_LOG_INFO << "edgeCount() = " << edgeCount();
}
double FlowSensor::getFlowInLiters() {
	return edgeCount() / static_cast<double>(EdgesPerLiter);
}
double FlowSensor::getFlowInGallons() {
	return getFlowInLiters() / LitersPerGallon;
}
double FlowSensor::get() {
	return getFlowInGallons();
}
