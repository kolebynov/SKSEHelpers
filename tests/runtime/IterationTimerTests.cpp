#include "IterationTimer.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>

namespace
{
	using Seconds = std::chrono::duration<double>;
}

TEST_CASE("IterationTimer fires once the iteration time is reached and keeps the remainder", "[IterationTimer]")
{
	IterationTimer<double> timer{ Seconds{ 1.0 } };

	CHECK_FALSE(timer.Tick(0.75));
	CHECK(timer.Progress() == 0.75);

	int iterations = -1;
	CHECK(timer.Tick(0.5, iterations));
	CHECK(iterations == 1);
	CHECK(timer.ElapsedTime() == Seconds{ 0.25 });
}

TEST_CASE("IterationTimer reports every iteration covered by a large delta", "[IterationTimer]")
{
	IterationTimer<double> timer{ Seconds{ 0.5 } };

	int iterations = 0;
	CHECK(timer.Tick(Seconds{ 1.75 }, iterations));
	CHECK(iterations == 3);
	CHECK(timer.ElapsedTime() == Seconds{ 0.25 });

	CHECK_FALSE(timer.Tick(0.125, iterations));
	CHECK(iterations == 0);
}

TEST_CASE("IterationTimer starts from the given elapsed time", "[IterationTimer]")
{
	IterationTimer<double> timer{ Seconds{ 1.0 }, Seconds{ 0.75 } };

	CHECK(timer.Tick(0.25));
	CHECK(timer.ElapsedTime() == Seconds::zero());
}

TEST_CASE("Disabled IterationTimer does not advance", "[IterationTimer]")
{
	IterationTimer<double> timer{ Seconds{ 1.0 }, Seconds::zero(), false };

	int iterations = -1;
	CHECK_FALSE(timer.Tick(5.0, iterations));
	CHECK(iterations == 0);
	CHECK(timer.ElapsedTime() == Seconds::zero());

	timer.SetEnabled(true);
	CHECK(timer.Tick(1.0));
}

TEST_CASE("IterationTimer with a non-positive iteration time never fires", "[IterationTimer]")
{
	IterationTimer<double> timer{ Seconds::zero() };

	CHECK_FALSE(timer.Tick(1.0));
	CHECK(timer.ElapsedTime() == Seconds::zero());
	CHECK(timer.Progress() == 0.0);

	timer.SetIterationTime(Seconds{ 0.5 });
	CHECK(timer.Tick(0.5));
}

TEST_CASE("IterationTimer Reset clears elapsed time but keeps the iteration time", "[IterationTimer]")
{
	IterationTimer<double> timer{ Seconds{ 1.0 } };
	CHECK_FALSE(timer.Tick(0.5));

	timer.Reset();
	CHECK(timer.ElapsedTime() == Seconds::zero());
	CHECK(timer.IterationTime() == Seconds{ 1.0 });
	CHECK_FALSE(timer.Tick(0.75));
}
