
#include <stdio.h>
#include <thread>
#include "elev.h"
#include "OrderManager.h"
#include "Elevator.h"
#include "sendrec.h"
#include "Timer.h"
#include <iostream>

OrderManager orderManager(NUMBEROFELEVATORS);
struct code_message packet;
int input;
int id;

void listenForOrders(){
	while(true){
		switch(orderManager.current_state){
			case MASTER:
				orderManager.listen();
				orderManager.orderElevator();
				orderManager.listenCommand(id);
				break;
			case SLAVE:
				orderManager.listenCommand(id);
				break;
		}
	}
}

void drive(){
	while(true){
		orderManager.manage(id);
	}
}

void Sender(){
	while(true){
		switch(orderManager.current_state){
			case MASTER:
				usleep(250000);
				orderManager.code(id);
				udp_Broadcaster(orderManager.smsg);
				//printf("%i%i%i%i%i%i%i%i\n",orderManager.orderBuffer[0][0],orderManager.orderBuffer[0][1],orderManager.orderBuffer[1][0], orderManager.orderBuffer[1][1], orderManager.orderBuffer[2][0], orderManager.orderBuffer[2][1], orderManager.orderBuffer[3][0], orderManager.orderBuffer[3][1]);
				//printf("CF: %i, SI: %i, DI %i\n", orderManager.elevators[id].currentFloor, orderManager.elevators[id].getStateIndex(), orderManager.elevators[id].directionIndex);
				break;
			case SLAVE:
				usleep(250000);
				orderManager.code(id);
				udp_Sender(orderManager.smsg, MASTERPORT, packet.rip);
				break;
		}
	}
}

void Reciever(){
	while(true){
		switch(orderManager.current_state){
			case MASTER:
				packet = udp_Reciever();
				//printf("Message: %s\n", packet.data);
				orderManager.decode(packet.data, id);
				printf("CF: %i, SI: %i, DI %i\n", orderManager.elevators[id].currentFloor, orderManager.elevators[id].getStateIndex(), orderManager.elevators[id].directionIndex);
				break;
			case SLAVE:
				packet = udp_recieve_broadcast();
				//printf("Message: %s\n", packet.data);
				orderManager.decode(packet.data, id);
				break;
		}
	}
}

int main() {
	elev_init();
	udp_init(MASTERPORT);
	printf("PROGRAM STARTED\n");
	/*int nBytes = 0;
	Timer timer;
	timer.start();
	current_state = MASTER;
	while(!timer.is_time_out(3)){
		nBytes = Bytes();
		if(nBytes > 0){
			current_state = SLAVE;
			packet = udp_recieve_broadcast();
		}
	}
	printf("complete");*/

	printf("I AM\n");

	std::cin >> input;
	id = input - 1;

	switch(input){
		case 1:
			orderManager.current_state = MASTER;
			break;
		default:
			orderManager.current_state = SLAVE;
			break;
	}

    std::thread t1(listenForOrders);
    std::thread t2(drive);
    std::thread t3(Sender);
    std::thread t4(Reciever);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}