/**
 * Mutext test
 * @author Copyright (c) 2023 - 2024 Martin Oberzalek
 */
#include "ColoredOutput.h"
#include <iostream>
#include <OutDebug.h>
#include <format.h>
#include <chrono>
#include <coroutine>
#include "coscheduler/CoScheduler.hpp"
#include "coscheduler/CoGenerator.hpp"
#include "CoSchedulerStaticConf.h"

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace Tools;

using Scheduler = CoScheduler::Scheduler<StaticConf>;
using YIELD = CoScheduler::YIELD;
using mutex = CoScheduler::mutex;

Scheduler sch;
mutex my_mutex;
std::string my_var;

Scheduler::yield_type task_function_lock_unlock( const char* instance, std::chrono::nanoseconds schedule_time_ )
{
	bool toggle = false;
	std::chrono::nanoseconds schedule_time = schedule_time_;
	CPPDEBUG( Tools::format( "%s: at start %s", instance, my_var ) );

	while(true)
	{
		if( toggle ) {
			while( !my_mutex.try_lock() ) {
				//CPPDEBUG( Tools::format( "%s: waiting to lock", instance ) );
				co_yield YIELD(my_mutex);
			}
			my_var = instance;
		} else {
			CPPDEBUG( Tools::format( "%s: %s", instance, my_var ) );
			my_mutex.unlock();
		}

		toggle = !toggle;

		co_yield YIELD(schedule_time);
	}

	co_return;
}

int main( int argc, char **argv )
{
	ColoredOutput co;

	Tools::x_debug = new OutDebug();

	try {


		auto task_foo = task_function_lock_unlock( "foo", 800ms );
		auto task_bar = task_function_lock_unlock( "bar", 1000ms );

		sch.add_task_reference(task_foo);
		sch.add_task_reference(task_bar);

		sch.infinite_schedule();


	} catch( const std::exception & error ) {
		std::cout << "Error: " << error.what() << std::endl;
		return 1;
	}

	return 0;
}

