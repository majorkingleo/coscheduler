/**
 * @author Martin Oberzalek
 */
#include "ColoredOutput.h"
#include <arg.h>
#include <iostream>
#include <OutDebug.h>
#include <memory>
#include <format.h>
#include <chrono>
#include <coroutine>
#include "date.h"
#include <thread>
#include "coscheduler/CoScheduler.hpp"
#include "coscheduler/CoGenerator.hpp"

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace CoScheduler;
using namespace Tools;


static std::string to_hhmmssms( const std::chrono::time_point<std::chrono::system_clock> tp )
{
	using namespace std::chrono;

	auto dp = date::floor<date::days>(tp);  // dp is a sys_days, which is a
	                                      // type alias for a C::time_point
	//auto ymd = date::year_month_day{dp};
	auto time = date::make_time(duration_cast<milliseconds>(tp-dp));

	return Tools::format("%02d:%02d:%02d.%04d",
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

	return Tools::format("%02d:%02d:%02d",
			time.hours().count(),
			time.minutes().count(),
			time.seconds().count() );
}

Scheduler::yield_type task_function_a()
{
	const std::chrono::milliseconds shedule_time = 1000ms;

	while( true )
	{
		CPPDEBUG( Tools::format( "%s: %s", __FUNCTION__, to_hhmmssms(system_clock::now()) ) );
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

		CPPDEBUG( Tools::format( "%s: %s diff: %dms", __FUNCTION__, to_hhmmssms(system_clock::now()), diff.count() ) );
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

		CPPDEBUG( Tools::format( "%s: %s count: %d diff: %dms", __FUNCTION__, to_hhmmssms(system_clock::now()), i, diff.count() ) );
		last_run = system_clock::now();
		co_yield YIELD( shedule_time, 1ms );
	}

	co_return;
}

Scheduler::yield_type task_function_c()
{
	const std::chrono::milliseconds shedule_time = 3000ms;

	for( unsigned i = 0; i < 2; i++ )
	{
		CPPDEBUG( Tools::format( "%s: %s iteration %d:", __FUNCTION__, to_hhmmssms(system_clock::now()), i ) );

		auto sub = sub_function_c();
		while( sub ) {
			co_yield sub();
		}

		co_yield YIELD( shedule_time, 1ms );
	}

	co_return;
}


class mutex : public WaitFor<bool>
{
public:
	bool try_lock() {

		bool & locked = value;

		if( !locked ) {
			locked = true;
			return true;
		}

		return false;
	}

	void unlock() {
		bool & locked = value;
		locked = false;
	}

	virtual bool condition_reached() const {
		// not locked
		return !value;
	}
};


Scheduler sch;
mutex my_mutex;
std::string my_var;

Scheduler::yield_type task_function_lock_unlock( const char* instance, std::chrono::nanoseconds schedule_time_ )
{
	bool toggle = false;
	std::chrono::nanoseconds schedule_time = schedule_time_;
	CPPDEBUG( Tools::format( "%s: %s", instance, my_var ) );

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

	Arg::Arg arg( argc, argv );
	arg.addPrefix( "-" );
	arg.addPrefix( "--" );

	Arg::OptionChain oc_info;
	arg.addChainR(&oc_info);
	oc_info.setMinMatch(1);
	oc_info.setContinueOnMatch( false );
	oc_info.setContinueOnFail( true );

	Arg::FlagOption o_help( "help" );
	o_help.setDescription( "Show this page" );
	oc_info.addOptionR( &o_help );

	Arg::FlagOption o_debug("d");
	o_debug.addName( "debug" );
	o_debug.setDescription("print debugging messages");
	o_debug.setRequired(false);
	arg.addOptionR( &o_debug );

	if( !arg.parse() )
	{
		std::cout << arg.getHelp(5,20,30, 80 ) << std::endl;
		return 1;
	}

	if( o_debug.getState() )
	{
		Tools::x_debug = new OutDebug();
	}

	if( o_help.getState() ) {
		std::cout << arg.getHelp(5,20,30, 80 ) << std::endl;
		return 1;
	}

	try {
		/*
		auto task_a = task_function_a();
		auto task_b = task_function_b();
		auto task_c = task_function_c();

		sch.add_task_reference(task_a);
		sch.add_task_reference(task_b);
		sch.add_task_reference(task_c);
		*/

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

