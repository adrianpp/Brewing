CXX      := g++
CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror --std=c++17 -Wno-psabi
LDFLAGS  := -L/usr/lib -lstdc++ -lm -pthread -lboost_system -latomic
BUILD    := build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := run_brewery
INCLUDE  := -Iinclude/ -Icrow/include/
MOCK_SRC := $(wildcard src/*MOCK.cpp)
SRC      := $(wildcard src/*.cpp)
OBJECTS  = $(SRC:src/%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES = $(OBJECTS:.o=.d)

ifeq (,$(filter mock,$(MAKECMDGOALS)))
	LDFLAGS += -lwiringPi
	SRC := $(filter-out $(MOCK_SRC), $(SRC))
endif

ifneq (,$(filter debug,$(MAKECMDGOALS)))
	CXXFLAGS += -DDEBUG -g
else ifneq (,$(filter release,$(MAKECMDGOALS)))
	CXXFLAGS += -O2
endif

all: crow WiringPi systemDepends build $(APP_DIR)/$(TARGET)

crow:
	git clone https://github.com/crowcpp/crow.git
	cd crow && git checkout b18fbb18f02264f56abf7ebdf664ce3f5f97ece5
	sed -i 's/constexpr //g' crow/include/crow/version.h

ifeq (,$(filter mock,$(MAKECMDGOALS)))
WiringPi:
	git clone https://github.com/WiringPi/WiringPi
	cd WiringPi && ./build
else
WiringPi:
endif

systemDepends:
	./get_system_dependencies.sh

$(OBJ_DIR)/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all clean debug release mock build info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	-@rm -rvf $(BUILD)

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
