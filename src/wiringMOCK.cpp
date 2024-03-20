extern "C" {
void wiringPiSetup() {}
void pinMode(int,int) {}
void digitalWrite(int,int) {}
int analogRead(int) {return 0;}
void wiringPiISR(int,int,void(*)()) {}
void wiringPiISR_data(int,int,void(*)(void*),void*) {}
void ds18b20Setup(int, const char*) {}
}
