#include <script/vm.h>
#include <gtest/gtest.h>

using namespace imp::script;

TEST(ScriptVM, ConstructsValid)
{
	ScriptVM vm;
	EXPECT_TRUE(vm.isValid());
}

TEST(ScriptVM, ExecutesValidLua)
{
	ScriptVM vm;
	std::string error;

	EXPECT_TRUE(vm.execute("x = 1 + 2", &error)) << error;
}

TEST(ScriptVM, RejectsSyntaxErrorWithoutCrashing)
{
	ScriptVM vm;
	std::string error;

	EXPECT_FALSE(vm.execute("this is not valid lua ((((", &error));
	EXPECT_FALSE(error.empty());
}

TEST(ScriptVM, RejectsRuntimeErrorWithoutCrashing)
{
	ScriptVM vm;
	std::string error;

	EXPECT_FALSE(vm.execute("error('boom')", &error));
	EXPECT_FALSE(error.empty());
}

TEST(ScriptVM, RemainsUsableAfterAFailedExecute)
{
	ScriptVM vm;
	std::string error;

	EXPECT_FALSE(vm.execute("this is not valid lua ((((", &error));
	EXPECT_FALSE(vm.execute("error('boom')", &error));

	error.clear();
	EXPECT_TRUE(vm.execute("y = 10", &error)) << error;
}

TEST(ScriptVM, BoundFunctionRoundTripsThroughLua)
{
	ScriptVM vm;
	int capturedArg = -1;

	vm.bindIntFunction("ping", [&capturedArg](int value) -> int
		{
			capturedArg = value;
			return value * 2;
		});

	std::string error;
	EXPECT_TRUE(vm.execute("result = ping(21)", &error)) << error;
	EXPECT_EQ(capturedArg, 21);
}
