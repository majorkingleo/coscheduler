/*
 * CoScheduler.cpp
 *
 *  Created on: 14.12.2023
 *      Author: martin.oberzalek
 */
#include "CoScheduler.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <CpputilsDebug.h>
#include <format.h>

using namespace std::chrono_literals;
using namespace CoScheduler;
using namespace Tools;

Scheduler::~Scheduler()
{

}

bool Scheduler::schedule()
{
	Conf::CONTAINER_TASKS generators;
	const auto tp = YIELD::clock::now();

	for( yield_type *t : tasks ) {

		const auto & value = t->get_handle().promise().value_;

		if( value.next_run  <= tp ) {
			generators.push_back( t );
		}
	}

	if( generators.empty() ) {
		return false;
	}

	generators.sort([]( auto a, auto b ) {

		auto & a_value = a->get_handle().promise().value_;
		auto & b_value = b->get_handle().promise().value_;

		if( a_value.wait_for_object != nullptr && b_value.wait_for_object == nullptr ) {
			return true;
		}
		else if( a_value.wait_for_object == nullptr && b_value.wait_for_object != nullptr ) {
			return false;
		}

		return a->get_handle().promise().value_.next_run < b->get_handle().promise().value_.next_run;
	});

#if 0
	if( generators.size() > 1 ) {
		CPPDEBUG( format( "generators: %d", generators.size() ) );

		for( auto gen : generators ) {
			auto & value = gen->get_handle().promise().value_;
			CPPDEBUG( format( "task: %p %s", gen, value.wait_for_object ? "waiting" : "" ) );
		}

		CPPDEBUG( "" );
	}
#endif

	for( auto gen : generators ) {

		auto & value = gen->get_handle().promise().value_;

		// ignore members, that are waiting for an object
		if( value.wait_for_object && !value.wait_for_object->condition_reached() ) {
			//tasks_to_schedule_next.push_back( gen );
			continue;
		}


		(*gen)();

		if( gen->get_handle().done() ) {
			remove_task( gen );
		}
	}

	return true;
}

void Scheduler::idle()
{
	std::this_thread::sleep_for( 10ms );
}


void Scheduler::infinite_schedule()
{
	while( true ) {
		if( !schedule() ) {
			idle();
		}
	}
}

void Scheduler::remove_task( yield_type* task )
{
	task->get_handle().destroy();

	for( auto it = tasks.begin(); it != tasks.end(); it++ ) {
		if( *it == task ) {
			tasks.erase(it);
			return;
		}
	}
}
