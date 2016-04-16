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

#define DEFAULT_PORT 20019
#define BROADCAST_IP "127.0.0.1"

using namespace std;

State current_state;
OrderManager::OrderManager(int nElevators){
	this->nElevators = nElevators;
	addElevators(nElevators);
	nextOrder = 0;

	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS; j++){
			buttonMatrix[i][j] = 0;
            buffer[i][j] = 0;
		}
	}
}

void OrderManager::addElevators(int nElevators){
	for(int i = 0; i < nElevators; i++){
		Elevator newElevator;
		elevators.push_back(newElevator);
	}
}

void OrderManager::listen(){
    //printf("%s\n", toString(elevators[0].getCurrentState()));
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS; j++){
			switch(current_state){
				case MASTER:
				//Listen for orders
				if(elev_get_button_signal((elev_button_type_t)j, i)){
					buttonMatrix[i][j] = 1;
				}
				//Set lights
				elev_set_button_lamp((elev_button_type_t)j, i, buttonMatrix[i][j]);

				if (elevators[0].currentState == OPEN){
						buttonMatrix[elevators[0].currentFloor][elevators[0].directionIndex] = 0;
						buttonMatrix[elevators[0].currentFloor][2] = 0;
					}
					if(elevators[0].orders.empty()){ //Fiks
						buttonMatrix[elevators[0].currentFloor][0] = 0;
						buttonMatrix[elevators[0].currentFloor][1] = 0;
					}			
				break;
				//Add and clear
				case SLAVE:
				if(elev_get_button_signal((elev_button_type_t)3, i)){
					buttonMatrix[i][3] = 1;
				}
				elev_set_button_lamp((elev_button_type_t)j, i, buttonMatrix[i][j]));


				if(buttonMatrix[i][j]){
					elevators[0].addOrder(i, (elev_button_type_t)j);
					/*int currentFloor = elev_get_floor_sensor_signal();
					if(elevators[0].getCurrentState() == OPEN){
						int index = elevators[0].getDirectionIndex();
						buttonMatrix[currentFloor][index] = 0;
						buttonMatrix[currentFloor][2] = 0;
					}
					if(elevators[0].orders.empty()){ //Fiks
						buttonMatrix[currentFloor][0] = 0;
						buttonMatrix[currentFloor][1] = 0;*/
				}

				break;		
			}
		}
	}
}
 

void OrderManager::manage(){
	elevators[0].run(); //Temporerally
}

void OrderManager::code(){
	switch(current_state){
		case MASTER:
    		smsg = "S";
    		for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j < N_BUTTONS-1; j++){
            		smsg += to_string(buttonMatrix[i][j]);
        		}
    		}
    	//printf("CODE: %s\n", msg.c_str());
    	break;
    	case SLAVE:
    		smsg = "R";
    		smsg += to_string(elevators[0].getStateIndex());
    		smsg += to_string(elevators[0].getDirectionIndex());
    		smsg += to_string(elevators[0].currentFloor);
    	break;

    }
}

void OrderManager::decode(std::string rmsg){
	switch(current_state){
		case MASTER:
			elevators[0].stateIndex = rmsg[1] - 48;
			elevators[0].directionIndex = rmsg[2]- 48;
			elevators[0].currentFloor = rmsg[3]- 48;
			rmsg = "";
			elevators[0].currentState = elevators[0].getState(elevators[0].stateIndex);
		break;

		case SLAVE:
    		int n = 1;
    		for(int i = 0; i < N_FLOORS; i++){
        		for(int j = 0; j< N_BUTTONS-1; j++){
            		buttonMatrix[i][j] = rmsg[n] - 48;
            		n++;
        		}
    		}
    		rmsg = "";
			//printf("DECODED: %i%i%i%i%i%i%i%i%i%i%i%i\n",buttonMatrix[0][0],buttonMatrix[0][1],buttonMatrix[0][2],buttonMatrix[1][0],buttonMatrix[1][1],buttonMatrix[1][2],buttonMatrix[2][0],buttonMatrix[2][1],buttonMatrix[2][2],buttonMatrix[3][0],buttonMatrix[3][1],buttonMatrix[3][2]);
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