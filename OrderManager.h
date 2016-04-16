#pragma once

#include <stdlib.h>
#include <deque>
#include <string>
#include "elev.h"
#include "Elevator.h"

enum State{MASTER, SLAVE};

extern State current_state;

class OrderManager{
private:
	int nElevators;

public:
	std::deque<Elevator> elevators;
	unsigned int buttonMatrix[4][3];
	unsigned int buffer[4][3];
	int nextOrder;
	std::string smsg;
	void code();
	void decode(std::string);
	OrderManager(int);
	void listen();
	void manage();
	void addElevators(int);
	inline const char* toString(state);
};