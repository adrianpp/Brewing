CXX      := g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror --std=c++17 -Wno-psabi
LDFLAGS  := -L/usr/lib -lstdc++ -lm -pthread -lboost_system -latomic -lwiringPi
BUILD    := build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := run_brewery
INCLUDE  := -Iinclude/ -Icrow/include/
SRC      :=                      \
	$(wildcard src/*.cpp)

OBJECTS  := $(SRC:src/%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
	:= $(OBJECTS:.o=.d)

all: crow WiringPi systemDepends build $(APP_DIR)/$(TARGET)

crow:
	git clone https://github.com/crowcpp/crow.git
	cd crow && git checkout b18fbb18f02264f56abf7ebdf664ce3f5f97ece5
	sed -i 's/constexpr //g' crow/include/crow/version.h

WiringPi:
	git clone https://github.com/WiringPi/WiringPi
	cd WiringPi && ./build

systemDepends:
	./get_system_dependencies.sh

$(OBJ_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all clean debug release info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(BUILD)

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
