/**
 * @author Martin Oberzalek
 */
#if 0
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

using namespace std::chrono_literals;
using namespace std::chrono;
using namespace Tools;


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

// this is copy & paste from https://en.cppreference.com/w/cpp/language/coroutines
template<typename T>
struct Generator
{
    // The class name 'Generator' is our choice and it is not required for coroutine
    // magic. Compiler recognizes coroutine by the presence of 'co_yield' keyword.
    // You can use name 'MyGenerator' (or any other name) instead as long as you include
    // nested struct promise_type with 'MyGenerator get_return_object()' method.

    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type // required
    {
        T value_;
        std::exception_ptr exception_;

        Generator get_return_object()
        {
            return Generator(handle_type::from_promise(*this));
        }
        std::suspend_always initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { exception_ = std::current_exception(); } // saving
                                                                              // exception

        template<std::convertible_to<T> From> // C++20 concept
        std::suspend_always yield_value(From&& from)
        {
            value_ = std::forward<From>(from); // caching the result in promise
            return {};
        }
        void return_void() {}
    };

    handle_type h_;

    Generator(handle_type h) : h_(h) {}
    ~Generator() { h_.destroy(); }
    explicit operator bool()
    {
        fill(); // The only way to reliably find out whether or not we finished coroutine,
                // whether or not there is going to be a next value generated (co_yield)
                // in coroutine via C++ getter (operator () below) is to execute/resume
                // coroutine until the next co_yield point (or let it fall off end).
                // Then we store/cache result in promise to allow getter (operator() below
                // to grab it without executing coroutine).
        return !h_.done();
    }
    T operator()()
    {
        fill();
        full_ = false; // we are going to move out previously cached
                       // result to make promise empty again
        return std::move(h_.promise().value_);
    }

private:
    bool full_ = false;

    void fill()
    {
        if (!full_)
        {
            h_();
            if (h_.promise().exception_)
                std::rethrow_exception(h_.promise().exception_);
            // propagate coroutine exception in called context

            full_ = true;
        }
    }
};

struct YIELD
{
	using clock = std::chrono::high_resolution_clock;
	using timepoint_t = std::chrono::time_point<clock>;

	timepoint_t last_run;
	timepoint_t next_run;
	std::chrono::nanoseconds expected_duration;

	YIELD() {}

	YIELD( std::chrono::nanoseconds next_run_in, std::chrono::nanoseconds expected_duration_ )
	: last_run( clock::now() ),
	  next_run( last_run + next_run_in ),
	  expected_duration( expected_duration_ )
	{}
};



Generator<YIELD> task_function_a()
{
	const std::chrono::milliseconds shedule_time = 1000ms;

	while( true )
	{
		CPPDEBUG( format( "%s: %s", __FUNCTION__, to_hhmmssms(system_clock::now()) ) );
		co_yield YIELD( shedule_time, 1ms );
	}

	co_return;
}


Generator<YIELD> task_function_b()
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


Generator<YIELD> sub_function_c()
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

Generator<YIELD> task_function_c()
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

		std::vector<Generator<YIELD>*> tasks;

		auto gen_a = task_function_a();
		auto gen_b = task_function_b();
		auto gen_c = task_function_c();

		tasks.push_back( &gen_a );
		tasks.push_back( &gen_b );
		tasks.push_back( &gen_c );

		while( true )
		{
			std::list<Generator<YIELD>*> generators;
			auto tp = YIELD::clock::now();

			for( Generator<YIELD>* gen : tasks ) {
				if( gen->h_.promise().value_.next_run  <= tp ) {
					generators.push_back( gen );
				}
			}

			generators.sort([]( auto a, auto b ){
				return a->h_.promise().value_.next_run < b->h_.promise().value_.next_run;
			});

			for( auto gen : generators ) {
				(*gen)();
			}

			if( generators.empty() ) {
				std::this_thread::sleep_for(10ms);
			}
		}


	} catch( const std::exception & error ) {
		std::cout << "Error: " << error.what() << std::endl;
		return 1;
	}

	return 0;
}

#endif
