#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include "caps.h"
#include "demo_defs.h"
#include "random_caps_factory.h"
#include "clargs.h"

using std::chrono::steady_clock;
using std::chrono::duration_cast;
using std::chrono::nanoseconds;

static bool test1();
static bool test2();
static bool test3();
static bool test4();
static bool test5();
static bool test6();
static bool test_random(int32_t depth);

static void print_prompt(const char* progname) {
	static const char* form = "caps随机数据序列化反序列化测试\n\n"
		"USAGE: %s [options]\n"
		"options:\n"
		"\t--help        打印此帮助信息\n"
		"\t--repeat=*    测试重复次数\n";
	printf(form, progname);
}

int main(int argc, char** argv) {
	// parse arguments
	clargs_h h = clargs_parse(argc, argv);
	if (h == 0 || clargs_opt_has(h, "help")) {
		print_prompt(argv[0]);
		return 1;
	}
	const char* v = clargs_opt_get(h, "repeat");
	int32_t repeat = 1;
	if (v) {
		char* ep;
		repeat = strtol(v, &ep, 10);
		if (repeat <= 0)
			repeat = 1;
	}

	// rand init
	steady_clock::time_point tp = steady_clock::now();
	uint32_t s = duration_cast<nanoseconds>(tp.time_since_epoch()).count();
	srand(s);

	// random test loop
	printf("press enter start test, repeat %d times\n", repeat);
	getchar();
	int32_t i;
	for (i = 0; i < repeat; ++i) {
		if (!test1()) {
			printf("test add integers failed\n");
			return 1;
		}

		if (!test2()) {
			printf("test add float failed\n");
			return 1;
		}

		if (!test3()) {
			printf("test add long failed\n");
			return 1;
		}

		if (!test4()) {
			printf("test add double failed\n");
			return 1;
		}

		if (!test5()) {
			printf("test add string failed\n");
			return 1;
		}

		if (!test6()) {
			printf("test add binary failed\n");
			return 1;
		}

		if (!test_random(2)) {
			printf("test random caps failed\n");
			return 1;
		}
	}
	printf("test finish, press enter quit\n");
	getchar();
	return 0;
}

static bool test1() {
	RandomCapsFactory fac;
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_integer();
	}
	return fac.check();
}

static bool test2() {
	RandomCapsFactory fac;
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_float();
	}
	return fac.check();
}

static bool test3() {
	RandomCapsFactory fac;
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_long();
	}
	return fac.check();
}

static bool test4() {
	RandomCapsFactory fac;
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_double();
	}
	return fac.check();
}

static bool test5() {
	RandomCapsFactory fac;
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_string();
	}
	return fac.check();
}

static bool test6() {
	RandomCapsFactory fac;
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_binary();
	}
	return fac.check();
}

static bool test_random(int32_t depth) {
	RandomCapsFactory fac;
	fac.gen_object(depth);
	return fac.check();
}
