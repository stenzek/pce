#pragma once
#include "YBaseLib/Common.h"

typedef bool (*TestSuiteRunnerFunction)();

#define DECLARE_TEST_SUITE(name) bool __testsuite_##name()
#define DEFINE_TEST_SUITE(name) bool __testsuite_##name()
#define INVOKE_TEST_SUITE(name) __testsuite_##name
