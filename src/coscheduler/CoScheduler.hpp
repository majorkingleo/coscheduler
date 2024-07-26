/**
 * Coroutine based scheduler
 * @author Copyright (c) 2023 - 2024 Martin Oberzalek
 */
#pragma once
#ifndef SRC_COSCHEDULER_COSCHEDULER_HPP_
#define SRC_COSCHEDULER_COSCHEDULER_HPP_

#include <coroutine>
#include <algorithm>
#include <atomic>
#include "CoGenerator.hpp"

namespace CoScheduler {

class WaitForBase;

struct YIELD
{
	using clock = std::chrono::high_resolution_clock;
	using timepoint_t = std::chrono::time_point<clock>;

	timepoint_t last_run;
	timepoint_t next_run;
	std::chrono::nanoseconds expected_duration;
	WaitForBase *wait_for_object = nullptr;

	YIELD()
	: last_run( clock::now() ),
	  next_run(),
	  expected_duration(0)
	{}

	YIELD( WaitForBase & wait_for_object_ )
	: YIELD()
	{
		wait_for_object = &wait_for_object_;
	}

	YIELD( std::chrono::nanoseconds next_run_in, std::chrono::nanoseconds expected_duration_ = {} )
	: last_run( clock::now() ),
	  next_run( last_run + next_run_in ),
	  expected_duration( expected_duration_ )
	{}
};

class WaitForBase
{
public:
	virtual ~WaitForBase() {}

	virtual bool condition_reached() const = 0;
};

template<class T> class WaitFor : public WaitForBase
{
public:
	using value_type=T;
	mutable value_type value{};

	virtual bool condition_reached() const {
		return value;
	}
};

class mutex : public WaitFor<std::atomic<bool>>
{
public:
	bool try_lock() {

		value_type & locked = value;

		if( !locked ) {
			locked = true;
			return true;
		}

		return false;
	}

	void unlock() {
		value_type & locked = value;
		locked = false;
	}

	bool condition_reached() const override {
		// not locked
		return !value;
	}
};

/*
struct Conf
{
	using yield_type = CoGenerator<YIELD>;

	static constexpr unsigned MAX_TASKS = 10;
	static constexpr unsigned MAX_WAITABLE_OBJECTS = 10;
	static constexpr unsigned MAX_WAIT_OBJECTS = 10;

	using CONTAINER_TASKS            = Tools::CyclicArray<yield_type*,MAX_TASKS>;
	using CONTAINER_WAITABLE_OBJECTS = Tools::CyclicArray<WaitForBase*,MAX_WAITABLE_OBJECTS>;
	using CONTAINER_WAIT_OBJECTS     = Tools::CyclicArray<WaitForBase*,MAX_WAIT_OBJECTS>;
};
*/

template<class Conf> class Scheduler
{
public:
	using yield_type = Conf::yield_type;

protected:
	Conf::CONTAINER_TASKS            tasks;
	Conf::CONTAINER_WAITABLE_OBJECTS waitable_objects;
	Conf::CONTAINER_WAIT_OBJECTS     wait_for_objects;

public:
	virtual ~Scheduler() {};


	void add_task_reference( yield_type & h ) {
		tasks.push_back( &h );
	}

	virtual bool schedule();
	virtual void idle();
	virtual void infinite_schedule();

	void add_waitable_object( WaitForBase & waitable_object ) {
		waitable_objects.push_back( &waitable_object );
	}

	void add_wait_for_object( WaitForBase & waitable_object ) {
		wait_for_objects.push_back( &waitable_object );
	}

protected:
	void remove_task( yield_type* task );

};


template<class Conf>
void Scheduler<Conf>::idle()
{
	Conf::idle();
}


template<class Conf>
void Scheduler<Conf>::infinite_schedule()
{
	while( true ) {
		if( !schedule() ) {
			idle();
		}
	}
}

template<class Conf>
void Scheduler<Conf>::remove_task( yield_type* task )
{
	task->get_handle().destroy();

	for( auto it = tasks.begin(); it != tasks.end(); it++ ) {
		if( *it == task ) {
			tasks.erase(it);
			return;
		}
	}
}

template<class Conf>
bool Scheduler<Conf>::schedule()
{
	typename Conf::CONTAINER_TASKS generators;
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

	std::sort(generators.begin(), generators.end(),[]( auto a, auto b ) {

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

} // namespace CoScheduler

#endif /* SRC_COSCHEDULER_COSCHEDULER_HPP_ */
