#ifndef BREW_BOARD_LAYOUT_H__
#define BREW_BOARD_LAYOUT_H__

template<int physical_pin>
constexpr auto physical_to_wiring()
{
	constexpr auto WIRING_PINS[40] = {
		-1, // PIN1 = 3v3
		-1, // PIN2 = 5v
		 8, // PIN3 = Wiring 8
		-1, // PIN4 = 5v
		 9, // PIN5 = Wiring 9
		-1, // PIN6 = ground
		 7, // PIN7 = Wiring 7
		15, // PIN8 = Wiring 15
		-1, // PIN9 = ground
		16, // PIN10 = Wiring 16
		 0, // PIN11 = Wiring 0
		 1, // PIN12 = Wiring 1
		 2, // PIN13 = Wiring 2
		-1, // PIN14 = ground
		 3, // PIN15 = Wiring 3
		 4, // PIN16 = Wiring 4
		-1, // PIN17 = 3v3
		 5, // PIN18 = Wiring 5
		12, // PIN19 = Wiring 12
		-1, // PIN20 = ground
		13, // PIN21 = Wiring 13
		 6, // PIN22 = Wiring 6
		14, // PIN23 = Wiring 14
		10, // PIN24 = Wiring 10
		-1, // PIN25 = ground
		11, // PIN26 = Wiring 11
		30, // PIN27 = Wiring 30
		31, // PIN28 = Wiring 31
		21, // PIN29 = Wiring 21
		-1, // PIN30 = ground
		22, // PIN31 = Wiring 22
		26, // PIN32 = Wiring 26
		23, // PIN33 = Wiring 23
		-1, // PIN34 = ground
		24, // PIN35 = Wiring 24
		27, // PIN36 = Wiring 27
		25, // PIN37 = Wiring 25
		28, // PIN38 = Wiring 28
		-1, // PIN39 = ground
		29  // PIN40 = Wiring 29
	};
	constexpr auto ret = WIRING_PINS[physical_pin-1]; // 1 indexed
	static_assert(ret != -1 and "Cannot map pin!");
	return ret;
}

constexpr auto SSR1_PIN = physical_to_wiring<33>();
constexpr auto SSR2_PIN = physical_to_wiring<35>();
constexpr auto SSR3_PIN = physical_to_wiring<37>();

constexpr auto I2C_PIN = physical_to_wiring<7>();

constexpr auto RELAY1_PIN = physical_to_wiring<3>();
constexpr auto RELAY2_PIN = physical_to_wiring<15>();
constexpr auto RELAY3_PIN = physical_to_wiring<40>();
constexpr auto RELAY4_PIN = physical_to_wiring<32>();
constexpr auto RELAY5_PIN = physical_to_wiring<22>();
constexpr auto RELAY6_PIN = physical_to_wiring<18>();
constexpr auto RELAY7_PIN = physical_to_wiring<16>();
constexpr auto RELAY8_PIN = physical_to_wiring<12>();

constexpr auto DIG1_PIN = physical_to_wiring<5>();
constexpr auto DIG2_PIN = physical_to_wiring<11>();
constexpr auto DIG3_PIN = physical_to_wiring<13>();
constexpr auto DIG4_PIN = physical_to_wiring<29>();
constexpr auto DIG5_PIN = physical_to_wiring<31>();


#endif

