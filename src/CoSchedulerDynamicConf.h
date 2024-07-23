/**
 * Example configuration for a scheduler
 * using containers with heap usage
 * @author Copyright (c) 2024 Martin Oberzalek
 */

#pragma once

#include <vector>
#include "coscheduler/CoScheduler.hpp"

/**
 * use std::vector here because it contains only pointers,
 * which are very easy to move and copy.
 */
struct DynamicConf
{
	using yield_type = CoScheduler::CoGenerator<CoScheduler::YIELD>;

	using CONTAINER_TASKS            = std::vector<yield_type*>;
	using CONTAINER_WAITABLE_OBJECTS = std::vector<CoScheduler::WaitForBase*>;
	using CONTAINER_WAIT_OBJECTS     = std::vector<CoScheduler::WaitForBase*>;
};


