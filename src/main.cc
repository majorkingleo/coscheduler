/**
 * Simple tasks being scheduled
 * @author Copyright (c) 2023 - 2024 Martin Oberzalek
 */
#include <iostream>
#include <OutDebug.h>
#include <format.h>
#include <chrono>
#include <coroutine>
#include "date.h"
#include <thread>
#include "coscheduler/CoScheduler.hpp"
#include "coscheduler/CoGenerator.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace Tools;
using namespace CoScheduler;


static std::string to_hhmmssms( const std::chrono::time_point<std::chrono::system_clock> tp )
{
	using namespace std::chrono;

	auto dp = date::floor<date::days>(tp);  // dp is a sys_days, which is a
	                                      // type alias for a C::time_point
	//auto ymd = date::year_month_day{dp};
	auto time = date::make_time(duration_cast<milliseconds>(tp-dp));

	return format("%02d:%02d:%02d.%04d",
			time.hours().count(),
			time.minutes().count(),
			time.seconds().count(),
			time.subseconds().count() );
}

static std::string to_hhmmss( const std::chrono::time_point<std::chrono::system_clock> tp )
{
	using namespace std::chrono;

	auto dp = date::floor<date::days>(tp);  // dp is a sys_days, which is a
	                                      // type alias for a C::time_point
	//auto ymd = date::year_month_day{dp};
	auto time = date::make_time(duration_cast<milliseconds>(tp-dp));

	return format("%02d:%02d:%02d",
			time.hours().count(),
			time.minutes().count(),
			time.seconds().count() );
}


Scheduler::yield_type task_function_a()
{
	const std::chrono::milliseconds shedule_time = 1000ms;

	while( true )
	{
		CPPDEBUG( format( "%s: %s", __FUNCTION__, to_hhmmssms(system_clock::now()) ) );
		co_yield YIELD( shedule_time, 1ms );
	}

	co_return;
}


Scheduler::yield_type task_function_b()
{
	const std::chrono::milliseconds shedule_time = 5500ms;
	auto last_run = system_clock::now();

	while( true )
	{
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now() - last_run);

		CPPDEBUG( format( "%s: %s diff: %dms", __FUNCTION__, to_hhmmssms(system_clock::now()), diff.count() ) );
		last_run = system_clock::now();

		auto next_delay = shedule_time;

		if( diff > shedule_time ) {
			next_delay -= diff - shedule_time;
		}

		co_yield YIELD( next_delay, 1ms );
	}

	co_return;
}


Scheduler::yield_type sub_function_c()
{
	const std::chrono::milliseconds shedule_time = 800ms;
	auto last_run = system_clock::now();

	for( unsigned i = 0; i < 10; i++ )
	{
		auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now() - last_run);

		CPPDEBUG( format( "%s: %s count: %d diff: %dms", __FUNCTION__, to_hhmmssms(system_clock::now()), i, diff.count() ) );
		last_run = system_clock::now();
		co_yield YIELD( shedule_time, 1ms );
	}

	co_return;
}

Scheduler::yield_type task_function_c()
{
	const std::chrono::milliseconds shedule_time = 3000ms;

	while( true )
	{
		CPPDEBUG( format( "%s: %s", __FUNCTION__, to_hhmmssms(system_clock::now()) ) );

		auto sub = sub_function_c();
		while( sub ) {
			co_yield sub();
		}

		co_yield YIELD( shedule_time, 1ms );
	}

	co_return;
}


int main( int argc, char **argv )
{
	Tools::x_debug = new OutDebug();

	try {

		Scheduler sch;

		auto task_a = task_function_a();
		auto task_b = task_function_b();
		auto task_c = task_function_c();

		sch.add_task_reference( task_a );
		sch.add_task_reference( task_b );
		sch.add_task_reference( task_c );


		sch.infinite_schedule();


	} catch( const std::exception & error ) {
		std::cout << "Error: " << error.what() << std::endl;
		return 1;
	}

	return 0;
}

