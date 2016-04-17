#include "OrderManager.h"
#include "Elevator.h"
#include "elev.h"
#include "channels.h"
#include "io.h"
#include "sendrec.h"
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
#include <unistd.h>
#include <deque>
#include <algorithm>

using namespace std;

Elevator::Elevator(int id){
	identity = id;
	currentState = DOWN;
	direction = DOWN;
	directionIndex = 1;
	currentFloor = 0;
	orders.push_back(0);


	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < N_BUTTONS; j++){
			elevatorOrderMatrix[i][j] = 0;
			costMatrix[i][j] = 0;
			costActiveMatrix[i][j] = 0;
		}
	}
}

void Elevator::run(){
	//Elevator state
	switch(currentState){
		case UP:
			if(driveToFloor()){
				toOpen();
			}
			break;

		case DOWN:
			if(driveToFloor()){
				toOpen();
			}
			break;

		case IDLE:
			if(getNextOrder() > currentFloor){
				toUp();
			}
			else if(getNextOrder() < currentFloor){
				toDown();
			}
			else if(orders.size() > 0){
				toOpen();
			}
			
			ordersOnHoldUp.clear();
			ordersOnHoldDown.clear();
			break;

		case OPEN:
			//Switch dir top and bottom
			if(currentFloor == 0){
				direction = UP;
				directionIndex = 0;
			}
			else if(currentFloor == (N_FLOORS - 1)){
				direction = DOWN;
				directionIndex = 1;
			}

			//Go in the dir of the next order
			if(getNextOrder() > currentFloor){
				toUp();
			}
			else if(getNextOrder() < currentFloor){
				toDown();
			}


			else if((direction == UP) && (isOver(ordersOnHoldUp) || isOver(ordersOnHoldDown))){
				if(!isOver(ordersOnHoldUp)){orders.push_back(ordersOnHoldDown.front());}
				toUp();
			}	
			else if((direction == DOWN) && (isUnder(ordersOnHoldDown) || isUnder(ordersOnHoldUp))){
				if(!isUnder(ordersOnHoldDown)){orders.push_back(ordersOnHoldUp.front());}
				toDown();
			}

			//Turns, removes the floor you are turning on from the on hold lists
			else if((direction == UP) && isUnder(ordersOnHoldDown)){
				if(isCurrentFloor(ordersOnHoldDown)){ordersOnHoldDown.pop_front();}
				toDown();
			}
			else if((direction == DOWN) && isOver(ordersOnHoldUp)){
				if(isCurrentFloor(ordersOnHoldUp)){ordersOnHoldUp.pop_front();}
				toUp();
			}	
			else{
				toIdle();
			}
			break;
	}
}

void Elevator::toUp(){
	switch(currentState){
		case UP:
			extendOrdersUp();
			toUp();
			break;

		case DOWN:
			//Never happens
			break;

		case IDLE:
			break;

		case OPEN:
			extendOrdersUp();
			break;

	}
	direction = UP;
	currentState = UP;
	directionIndex = 0;
}

void Elevator::toDown(){
	switch(currentState){
		case UP:
			//Never happens
			break;

		case DOWN:
			extendOrdersDown();
			toDown();
			break;

		case IDLE:
			break;

		case OPEN:
			extendOrdersDown();
			elev_set_door_open_lamp(0);
			break;

	}
	direction = DOWN;
	currentState = DOWN;
	directionIndex = 1;
}

void Elevator::toIdle(){
	switch(currentState){
		case UP:
			break;

		case DOWN:
			break;

		case IDLE:
			break;

		case OPEN:
			elev_set_door_open_lamp(0);
			break;
	}
	currentState = IDLE;
}

void Elevator::toOpen(){
	switch(currentState){
		case UP:
			stopAtFloor();
			break;

		case DOWN:
			stopAtFloor();
			break;

		case IDLE:
			stopAtFloor();
			break;

		case OPEN:
			//
			break;
	}
	currentState = OPEN;
}

void Elevator::addOrder(int newOrder, elev_button_type_t buttonType){
		switch(buttonType){

			case BUTTON_CALL_UP:
				if(currentState == IDLE && notInQue(newOrder, orders)){
					orders.push_back(newOrder);
				}
				else if(notInQue(newOrder, ordersOnHoldUp) && (notInQue(newOrder, orders) || direction == DOWN)){
					ordersOnHoldUp.push_back(newOrder);
					sort(ordersOnHoldUp.begin(), ordersOnHoldUp.end());
				}
				break;

			case BUTTON_CALL_DOWN:
				if(currentState == IDLE && notInQue(newOrder, orders)){
					orders.push_back(newOrder);
				}
				else if(notInQue(newOrder, ordersOnHoldDown) && (notInQue(newOrder, orders) || direction == UP)){
					ordersOnHoldDown.push_back(newOrder);
					sort(ordersOnHoldDown.rbegin(), ordersOnHoldDown.rend()); //Decending
				}
				break;

			case BUTTON_COMMAND:

				if((currentState == UP) && (newOrder <= currentFloor) && notInQue(newOrder, ordersOnHoldDown)){
					ordersOnHoldDown.push_back(newOrder);
					sort(ordersOnHoldDown.rbegin(), ordersOnHoldDown.rend());
				}
				else if((currentState == DOWN) && (newOrder >= currentFloor) && notInQue(newOrder, ordersOnHoldUp)){
					ordersOnHoldUp.push_back(newOrder);
					sort(ordersOnHoldUp.begin(), ordersOnHoldUp.end());
				}
				else if((currentState == UP) && (newOrder > currentFloor) && notInQue(newOrder, orders) && notInQue(newOrder, ordersOnHoldUp) && notInQue(newOrder, ordersOnHoldDown)){
					orders.push_back(newOrder);
				}
				else if((currentState == DOWN) && (newOrder < currentFloor) && notInQue(newOrder, orders) && notInQue(newOrder, ordersOnHoldUp) && notInQue(newOrder, ordersOnHoldDown)){
					orders.push_back(newOrder);
				}
				else if(((currentState == OPEN) || (currentState == IDLE)) && notInQue(newOrder, orders) && notInQue(newOrder, ordersOnHoldUp) && notInQue(newOrder, ordersOnHoldDown)){
					orders.push_back(newOrder);
				}
				
				break;
		}
}
void Elevator::clearOrder(){
	if(currentState == OPEN){
		elevatorOrderMatrix[currentFloor][directionIndex] = 0;
		elevatorOrderMatrix[currentFloor][2] = 0;

		//Turns
		if((nextOrder < currentFloor) && (directionIndex == 0)){
			elevatorOrderMatrix[currentFloor][1] = 0;
		}
		else if((nextOrder > currentFloor) && (directionIndex == 1)){
			elevatorOrderMatrix[currentFloor][0] = 0;
		}
	}

	if(currentState == IDLE || currentFloor == 0 || currentFloor == N_FLOORS - 1){
		elevatorOrderMatrix[currentFloor][0] = 0;
		elevatorOrderMatrix[currentFloor][1] = 0;
	}
}
bool Elevator::driveToFloor(){
	int destionationFloor = getNextOrder();

	while(elev_get_floor_sensor_signal() != destionationFloor){
		destionationFloor = getNextOrder(); //Update floor
		sortOrders();

		if(currentState == UP){
			extendOrdersUp();
		}
		else if(currentState == DOWN){
			extendOrdersDown();
		}

		int tempFloor = elev_get_floor_sensor_signal();
        if(tempFloor != -1){
            elev_set_floor_indicator(tempFloor);
            currentFloor = tempFloor;
        }

        if(currentFloor < destionationFloor){
            elev_set_motor_direction(DIRN_UP);
        }
        else if(currentFloor > destionationFloor){
            elev_set_motor_direction(DIRN_DOWN);
        }
    }
    elev_set_motor_direction(DIRN_STOP);
    
    return true;
}

void Elevator::stopAtFloor(){
	currentState = OPEN;
	currentFloor = elev_get_floor_sensor_signal();
    elev_set_floor_indicator(elev_get_floor_sensor_signal());
    elev_set_door_open_lamp(1);
    sleep(1);
    elev_set_door_open_lamp(0);
    orders.pop_front();

    printOrders("Orders", orders);
    printOrders("UpOrders", ordersOnHoldUp);
    printOrders("DownOrders", ordersOnHoldDown);
    printf("\n");
}

int Elevator::getNextOrder(){
	if(orders.size() > 0){
		nextOrder = orders.front();
		return nextOrder;
	}
	else{
		return currentFloor;
	}
}

state Elevator::getState(int index){
	switch(index){
		case 0: return UP;
		case 1: return DOWN;
		case 2: return IDLE;
		case 3: return OPEN;
		default: return IDLE;
	}
}

bool Elevator::notInQue(int newOrder, deque<int> que){
	for(int i = 0; i < que.size(); i++){
		if(que[i] == newOrder){
			return false;
		}
	}
	return true;
}

void Elevator::extendOrdersUp(){
	for(int i = 0; i < ordersOnHoldUp.size(); i++){
		if(notInQue(ordersOnHoldUp[i], orders) && (ordersOnHoldUp[i] > currentFloor)){
			orders.push_back(ordersOnHoldUp[i]);
			ordersOnHoldUp.erase(ordersOnHoldUp.begin() + i);
		}
	}
}

void Elevator::extendOrdersDown(){
	for(int i = 0; i < ordersOnHoldDown.size(); i++){
		if(notInQue(ordersOnHoldDown[i], orders) && (ordersOnHoldDown[i] < currentFloor)){
			orders.push_back(ordersOnHoldDown[i]);
			ordersOnHoldDown.erase(ordersOnHoldDown.begin() + i);
		}
	}
}

bool Elevator::isOver(std::deque<int> que){
	for(int i = 0; i < que.size(); i++){
		if(que[i] > currentFloor){
			return true;
		}
	}
	return false;
}

bool Elevator::isUnder(std::deque<int> que){
	for(int i = 0; i < que.size(); i++){
		if(que[i] < currentFloor){
			return true;
		}
	}
	return false;
}

bool Elevator::isCurrentFloor(std::deque<int> que){
	for(int i = 0; i < que.size(); i++){
		if(que[i] == currentFloor){
			return true;
		}
	}
	return false;
}

void Elevator::sortOrders(){
	if(currentState == UP){
		sort(orders.begin(), orders.end()); //Sort acending
	}
	else if(currentState == DOWN){
		sort(orders.rbegin(), orders.rend()); //Sort decending
	}
}

int Elevator::getDirectionIndex(){
	if(direction == UP){
		//Up order
		directionIndex =0;
		return 0;
	}
	else if(direction == DOWN){
		//Down order
		directionIndex =1;
		return 1;
	}
	return 2;
}

int Elevator::getStateIndex(){
	switch(currentState){
		case UP: return 0;
		case DOWN: return 1;
		case IDLE: return 2;
		case OPEN: return 3;
	}
}

void Elevator::printOrders(string word, deque<int> que){
	printf("%s: [", word.c_str());
	for(int i = 0; i < que.size(); i++){
		printf("%i, ", que[i]);
	}
	printf("]\n");
}

inline const char* Elevator::toString(state s){
	switch(s){
		case UP: return "UP";
    	case DOWN: return "DOWN";
		case IDLE: return "IDLE";
		case OPEN: return "OPEN";
		default: return "Unknown";
	}
}
bool Elevator::checkifOrderEmpty(){
	for(int i = 0; i < N_FLOORS; i++){
		for(int j = 0; j < (N_BUTTONS - 1); j++){
			if(elevatorOrderMatrix[i][j] == 1){return false;}
		}
	}
	return true;
}