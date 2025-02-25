/**
 * Example configuration for a scheduler
 * using containers without heap usage
 * @author Copyright (c) 2023 - 2024 Martin Oberzalek
 */

#pragma once

#include <static_vector.h>
#include <chrono>
#include <thread>
#include "coscheduler/CoScheduler.hpp"

struct StaticConf
{
	using yield_type = CoScheduler::CoGenerator<CoScheduler::YIELD>;

	static constexpr unsigned MAX_TASKS = 10;
	static constexpr unsigned MAX_WAITABLE_OBJECTS = 10;
	static constexpr unsigned MAX_WAIT_OBJECTS = 10;

	using CONTAINER_TASKS            = Tools::static_vector<yield_type*,MAX_TASKS>;
	using CONTAINER_WAITABLE_OBJECTS = Tools::static_vector<CoScheduler::WaitForBase*,MAX_WAITABLE_OBJECTS>;
	using CONTAINER_WAIT_OBJECTS     = Tools::static_vector<CoScheduler::WaitForBase*,MAX_WAIT_OBJECTS>;


	/*
	 * idle function, this is for testing on the pc,
	 * on a microcontroller, you may do nothing here
	 */
	inline static void idle()
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for( 10ms );
	}
};


