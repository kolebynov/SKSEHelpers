#include "CallPipeline.h"
#include "TraitUtils.h"
#include "VariantUtils.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

static_assert(!FunctionTraits<int>::IsFunction);
static_assert(!FunctionTraits<void (*)(int)>::IsFunction);
static_assert(FunctionTraits<float(int, char)>::IsFunction);
static_assert(std::is_same_v<FunctionTraits<float(int, char)>::ReturnType, float>);
static_assert(std::is_same_v<FunctionTraits<float(int, char)>::Parameters, std::tuple<int, char>>);
static_assert(std::is_same_v<FunctionTraits<void()>::Parameters, std::tuple<>>);

static_assert(std::is_same_v<TupleAppendT<std::tuple<>, int>, std::tuple<int>>);
static_assert(std::is_same_v<TupleAppendT<std::tuple<int, char>, float&>, std::tuple<int, char, float&>>);

TEST_CASE("CallPipeline runs the outermost wrapper first", "[CallPipeline]")
{
	std::vector<std::string> trace;
	auto pipeline = StartCallPipeline([&](int value) {
		trace.emplace_back("original");
		return value + 1;
	})
	                    .Wrap([&](const auto& next, int value) {
							trace.emplace_back("inner");
							return next(value) * 10;
						})
	                    .Wrap([&](const auto& next, int value) {
							trace.emplace_back("outer");
							return next(value + 100);
						});

	CHECK(pipeline(1) == 1020);
	CHECK(trace == std::vector<std::string>{ "outer", "inner", "original" });
}

TEST_CASE("CallPipeline wrapper can skip the original", "[CallPipeline]")
{
	int originalCalls = 0;
	auto pipeline = StartCallPipeline([&](int value) {
		++originalCalls;
		return value;
	}).Wrap([](const auto& next, int value) { return value < 0 ? 0 : next(value); });

	CHECK(pipeline(-5) == 0);
	CHECK(originalCalls == 0);
	CHECK(pipeline(5) == 5);
	CHECK(originalCalls == 1);
}

TEST_CASE("CallPipeline forwards references", "[CallPipeline]")
{
	auto pipeline = StartCallPipeline([](int& value) { value += 1; }).Wrap([](const auto& next, int& value) {
		value *= 2;
		next(value);
	});

	int value = 3;
	pipeline(value);
	CHECK(value == 7);
}

TEST_CASE("CallPipeline copies lvalue wrappers", "[CallPipeline]")
{
	const auto addOne = [](const auto& next, int value) { return next(value) + 1; };
	auto pipeline = StartCallPipeline([](int value) { return value; }).Wrap(addOne).Wrap(addOne);

	CHECK(pipeline(1) == 3);
}

TEST_CASE("Match dispatches on the active alternative", "[VariantUtils]")
{
	using Value = std::variant<int, std::string>;
	auto describe = [](const Value& value) {
		return Match(
			value, [](int number) { return "int " + std::to_string(number); },
			[](const std::string& text) { return "string " + text; });
	};

	CHECK(describe(Value(42)) == "int 42");
	CHECK(describe(Value("x")) == "string x");
}

TEST_CASE("Match can modify the variant in place", "[VariantUtils]")
{
	std::variant<int, std::string> value = 1;
	Match(value, [](int& number) { number += 1; }, [](std::string& text) { text += "!"; });
	CHECK(std::get<int>(value) == 2);

	value = std::string("hi");
	Match(value, [](int& number) { number += 1; }, [](std::string& text) { text += "!"; });
	CHECK(std::get<std::string>(value) == "hi!");
}
