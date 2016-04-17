#pragma once

#include <stdlib.h>
#include <deque>
#include <string>
#include "elev.h"
#include "Elevator.h"


#define COSTBORDER 5

enum State{MASTER, SLAVE};

class OrderManager{
private:
	int fl; //floor
	int bu; //button
	int fl2;
	int bu2;
public:
	int nElevators;
	State current_state;
	std::deque<Elevator> elevators;
	unsigned int buttonMatrix[N_FLOORS][N_BUTTONS];
	unsigned int orderBuffer[N_FLOORS][N_BUTTONS];
	unsigned int bufferMatrix[N_FLOORS][N_BUTTONS];
	int nextOrder;
	std::string smsg;
	void code(int);
	void decode(std::string, int);
	OrderManager();
	void listen();
	void orderElevator();
	void listenCommand(int);
	void clearOrders();
	int costFunction(int,int,elev_button_type_t);
	void updateOrderBuffer();
	void updateCostMatrix();
	void findLowestCost(int);
	void findLowestActiveCost(int);
	void manage(int);
	void addElevators(int);
	bool checkifOrderEmpty(int);
	inline const char* toString(state);
};