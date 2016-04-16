#include "Timer.h"

bool Timer::is_time_out(int timeLength){
	return !((startTime + timeLength) >  time(NULL));
}

void Timer::start(){
	startTime = time(NULL);
}