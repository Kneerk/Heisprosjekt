#include "OrderManager.h"
#include "Elevator.h"
#include "Timer.h"
#include "elev.h"
#include "channels.h"
#include "io.h"
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
#include <unistd.h>
#include <deque>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define DEFAULT_PORT 20019
#define BROADCAST_IP "127.0.0.1"

using namespace std;

OrderManager::OrderManager(){
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS; j++){
			buttonMatrix[i][j] = 0;
			orderBuffer[i][j] = 0;
			bufferMatrix[i][j] = 0;
		}
	}
}

void OrderManager::addElevators(int number){
	nElevators = number;
	for(int i = 0; i < nElevators; i++){
		Elevator newElevator(i);
		elevators.push_back(newElevator);
	}
}

void OrderManager::listen(){
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS; j++){

			//Listen for outside buttons
			if(elev_get_button_signal((elev_button_type_t)j, i)){
				buttonMatrix[i][j] = 1;
			}
			//Listen for orders from remote
			if(bufferMatrix[i][j] == 1){
				buttonMatrix[i][j] = 1;
				bufferMatrix[i][j] = 0;
			}
		}
	}
		
}

void OrderManager::orderElevator(){
	int cost = 0;
	int identity;
	int temp = 0;
	for(int id = 0; id < nElevators; id++){
		updateOrderBuffer();
		updateCostMatrix();
		findLowestCost(id);
		findLowestActiveCost(id);
		if ((fl != -1) && (bu != -1)){
			identity = id;
			cost = elevators[identity].costMatrix[fl][bu];
			temp = cost;
			for(int id = 0; id < nElevators; id ++){
				if (elevators[id].costMatrix[fl][bu] < cost){
					temp = -1;
				}

			}
			if((cost < COSTBORDER) && (temp == cost)){
				elevators[identity].elevatorOrderMatrix[fl][bu] = orderBuffer[fl][bu];
			}
			else if((cost < elevators[identity].costActiveMatrix[fl2][bu2]) && (temp == cost)){
				elevators[identity].elevatorOrderMatrix[fl][bu] = orderBuffer[fl][bu];
			}
			else if ((cost > 0) && elevators[identity].checkifOrderEmpty() && (temp == cost)){
				elevators[identity].elevatorOrderMatrix[fl][bu] = orderBuffer[fl][bu];
			}
		}
	}
}

void OrderManager::listenCommand(int id){
	//Listen for command
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS; j++){
			//Set
			if(elev_get_button_signal((elev_button_type_t)2, i)){
				elevators[id].elevatorOrderMatrix[i][2] = 1;
				buttonMatrix[i][2] = 1;
			}
			if(elev_get_button_signal((elev_button_type_t)j, i) && current_state == SLAVE){
				bufferMatrix[i][j] = 1;
			}

			elev_set_button_lamp((elev_button_type_t)j, i, buttonMatrix[i][j]);
			//Add
			if(elevators[id].elevatorOrderMatrix[i][j]){
				elevators[id].addOrder(i, (elev_button_type_t)j);
			}
			//Clear
			if(elev_get_floor_sensor_signal() != -1){
				elevators[id].elevatorOrderMatrix[elev_get_floor_sensor_signal()][2] = 0;
				buttonMatrix[elev_get_floor_sensor_signal()][2] = 0;
			}
		}
	}

	if(current_state == MASTER){
		//Clear
		clearOrders();
		for(int id = 0; id < nElevators; id++){
			elevators[id].clearOrder();
		}
	}
}
void OrderManager::manage(int id){
	elevators[id].identity = id;
	elevators[id].run();
}
int OrderManager::costFunction(int id, int floorOrder, elev_button_type_t type){
	int cost = 1;

	int directionIndex = elevators[id].directionIndex;
	int currentFloor = elevators[id].currentFloor;
	state currentState = elevators[id].currentState;
	
	cost += abs(floorOrder - currentFloor);

	if((directionIndex != (int)type) && (currentState != IDLE)){
		if(directionIndex == 0){
		cost = 10 + (currentFloor - floorOrder);
		}
		else if(directionIndex == 1){
		cost = 10 + (floorOrder - currentFloor);
		}
	}

	if((directionIndex == (int)type) && (currentState != IDLE)){
		if((directionIndex == 0) && (currentFloor > floorOrder)){
			cost += 20;
		}
		else if((directionIndex == 1) && (currentFloor < floorOrder)){
			cost += 20;
		}
		else if((currentFloor == floorOrder) && (currentState != OPEN)){
			cost += 20;
		}
	}

	if(currentState == IDLE){
		if((currentFloor > floorOrder) && ((int)type == 0)){
			cost += 4;
		}
		else if((currentFloor < floorOrder) && ((int)type == 1)){
			cost += 4;
		}
	}

	return cost;
}
void OrderManager::updateOrderBuffer(){
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < (N_BUTTONS - 1); j++){
			orderBuffer[i][j] = buttonMatrix[i][j];
			for(int id = 0; id < nElevators; id++){
				orderBuffer[i][j] -= elevators[id].elevatorOrderMatrix[i][j];
				if(orderBuffer[i][j] == -1){
					orderBuffer[i][j] = 0;
				}
			}
		}
	}
}
void OrderManager::updateCostMatrix(){
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS-1; j++){
			for(int id = 0; id < nElevators; id++){
				elevators[id].costMatrix[i][j] = costFunction(id, i, (elev_button_type_t)j)*orderBuffer[i][j];
				elevators[id].costActiveMatrix[i][j] = costFunction(id, i, (elev_button_type_t)j)*elevators[id].elevatorOrderMatrix[i][j];
			}
		}
	}
}
void OrderManager::findLowestActiveCost(int id){
	fl2 = -1;
	bu2 = -1;
	int cost = 0;
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS-1; j++){
			if (cost == 0){
				cost = elevators[id].costActiveMatrix[i][j];
				if (cost != 0){
					fl2 = i;
					bu2 = j;
				}
			}
			else if ((elevators[id].costActiveMatrix[i][j] < cost) && (elevators[id].costActiveMatrix[i][j] != 0)){
				cost = elevators[id].costActiveMatrix[i][j];
				fl2 = i;
				bu2 = j;
			}
		}
	}
}
void OrderManager::findLowestCost(int id){
	fl = -1;
	bu = -1;
	int cost = 0;
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS-1; j++){
			if (cost == 0){
				cost = elevators[id].costMatrix[i][j];
				if (cost != 0){
					fl = i;
					bu = j;
				}
			}
			else if ((elevators[id].costMatrix[i][j] < cost) && (elevators[id].costMatrix[i][j] != 0)){
				cost = elevators[id].costMatrix[i][j];
				fl = i;
				bu = j;
			}
		}
	}
}
void OrderManager::clearOrders(){
	for(int id = 0; id < nElevators; id++){
		//clear
		if(elevators[id].currentState == OPEN){
			buttonMatrix[elevators[id].currentFloor][elevators[id].directionIndex] = 0;
			buttonMatrix[elevators[id].currentFloor][2] = 0;

			//Turns
			if((elevators[id].nextOrder < elevators[id].currentFloor) && (elevators[id].directionIndex == 0)){
				buttonMatrix[elevators[id].currentFloor][1] = 0;
			}
			else if((elevators[id].nextOrder > elevators[id].currentFloor) && (elevators[id].directionIndex == 1)){
				buttonMatrix[elevators[id].currentFloor][0] = 0;
			}

			//Highest lowest
			if((elevators[id].currentFloor == (N_FLOORS - 1)) && (elevators[id].directionIndex == 0)){
				buttonMatrix[elevators[id].currentFloor][1] = 0;
			}
			else if((elevators[id].currentFloor == 0) && (elevators[id].directionIndex == 1)){
				buttonMatrix[elevators[id].currentFloor][0] = 0;
			}
		}

		if(elevators[id].currentState == IDLE){
			buttonMatrix[elevators[id].currentFloor][0] = 0;
			buttonMatrix[elevators[id].currentFloor][1] = 0;
		
		}
	}
}
void OrderManager::code(int id){
	switch(current_state){
		case MASTER:
    		smsg = "S";
    		for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j < (N_BUTTONS - 1); j++){
            		smsg += to_string(buttonMatrix[i][j]);
        		}
    		}

    		for(int id = 0; id < nElevators; id++){
    			for(int i = 0; i < N_FLOORS; i++){
        			for(int j = 0; j < (N_BUTTONS - 1); j++){ 
        				smsg += to_string(elevators[id].elevatorOrderMatrix[i][j]);
        			}
    			}
    		}
    		
    	break;
    	case SLAVE:
    		smsg = "R";
    		smsg += to_string(elevators[id].identity);
    		smsg += to_string(elevators[id].getStateIndex());
    		smsg += to_string(elevators[id].directionIndex);
    		smsg += to_string(elevators[id].currentFloor);
    		smsg += to_string(elevators[id].nextOrder);
    		for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j < (N_BUTTONS - 1); j++){
        				smsg += to_string(bufferMatrix[i][j]);
        				bufferMatrix[i][j] = 0;
        		}
        	}
        	printf("%s\n", smsg.c_str());

    	break;

    }
}
void OrderManager::decode(std::string rmsg, int identity){
	int id = rmsg[1] -48;
	int r;
	switch(current_state){
		case MASTER:
			elevators[id].identity = rmsg[1] -48;
			elevators[id].stateIndex = rmsg[2] - 48;
			elevators[id].directionIndex = rmsg[3]- 48;
			elevators[id].currentFloor = rmsg[4]- 48;
			elevators[id].nextOrder = rmsg[5] - 48;
			elevators[id].currentState = elevators[id].getState(elevators[id].stateIndex);
			r = 6;
			for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j < (N_BUTTONS - 1); j++){
            		bufferMatrix[i][j] = rmsg[r] - 48;
            		r++;
        		}
    		}
    		rmsg = "";
			break;

		case SLAVE:
    		int n = 1;
    		for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j < (N_BUTTONS - 1); j++){
            		buttonMatrix[i][j] = rmsg[n] - 48;
            		n++;
        		}
    		}
    		n = 1 + N_FLOORS*2 + identity*N_FLOORS*2;
    		for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j < (N_BUTTONS - 1); j++){
            		elevators[identity].elevatorOrderMatrix[i][j] = rmsg[n] - 48;
            		n++;
        		}
    		}
    		rmsg = "";
			break;
    }
}
inline const char* OrderManager::toString(state s){
    switch(s){
        case UP: return "UP";
        case DOWN: return "DOWN";
        case IDLE: return "IDLE";
        case OPEN: return "OPEN";
        default: return "Unknown";
    }
}