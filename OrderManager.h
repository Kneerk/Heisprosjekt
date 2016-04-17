#pragma once

#include <stdlib.h>
#include <deque>
#include <string>
#include "elev.h"
#include "Elevator.h"


#define COSTBORDER 5
#define NUMBEROFELEVATORS 3

enum State{MASTER, SLAVE};

class OrderManager{
private:
	int fl;
	int bu;
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
	OrderManager(int);
	void listen();
	void orderElevator();
	void listenCommand(int);
	void clearOrders();
	int costFunction(int,int,elev_button_type_t);
	void updateOrderBuffer();
	void updateCostMatrix();
	void findLowestCost(int);
	void manage(int);
	void addElevators(int);
	bool checkifOrderEmpty(int);
	inline const char* toString(state);
};