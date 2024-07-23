/*
 * CoScheduler.hpp
 *
 *  Created on: 14.12.2023
 *      Author: martin.oberzalek
 */
#pragma once
#ifndef SRC_COSCHEDULER_COSCHEDULER_HPP_
#define SRC_COSCHEDULER_COSCHEDULER_HPP_

#include <coroutine>
#include <CyclicArray.h>
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

	virtual ~WaitFor() {}

	virtual bool condition_reached() const {
		return value;
	}
};

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

class Scheduler
{
public:
	using yield_type = Conf::yield_type;

protected:
	Conf::CONTAINER_TASKS            tasks;
	Conf::CONTAINER_WAITABLE_OBJECTS waitable_objects;
	Conf::CONTAINER_WAIT_OBJECTS     wait_for_objects;

public:
	virtual ~Scheduler();


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

} // namespace CoScheduler

#endif /* SRC_COSCHEDULER_COSCHEDULER_HPP_ */
