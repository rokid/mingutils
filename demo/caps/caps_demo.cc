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

static int32_t test1(bool c);
static int32_t test2(bool c);
static int32_t test3(bool c);
static int32_t test4(bool c);
static int32_t test5(bool c);
static int32_t test6(bool c);
static int32_t test_random(int32_t depth, bool c);

static const char* err_msgs[] = {
	"parse(not duplicated)",
	"parse(duplicated)"
};

static void print_prompt(const char* progname) {
	static const char* form = "caps随机数据序列化反序列化测试\n\n"
		"USAGE: %s [options]\n"
		"options:\n"
		"\t--help        打印此帮助信息\n"
		"\t--repeat=*    测试重复次数\n"
		"\t--use-c-api   使用c接口";
	printf(form, progname);
}

int main(int argc, char** argv) {
	// parse arguments
	clargs_h h = clargs_parse(argc, argv);
	if (h == 0 || clargs_opt_has(h, "help")) {
		print_prompt(argv[0]);
		clargs_destroy(h);
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
	bool use_c_api = clargs_opt_has(h, "use-c-api");
	clargs_destroy(h);

	// rand init
	steady_clock::time_point tp = steady_clock::now();
	uint32_t s = duration_cast<nanoseconds>(tp.time_since_epoch()).count();
	srand(s);

	// random test loop
	printf("press enter start test, repeat %d times, %s\n", repeat,
			use_c_api ? "c api" : "c++ api");
	getchar();
	int32_t i;
	int32_t r;
	for (i = 0; i < repeat; ++i) {
		r = test1(use_c_api);
		if (r != 0) {
			printf("test add integers failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
		r = test2(use_c_api);
		if (r != 0) {
			printf("test add float failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
		r = test3(use_c_api);
		if (r != 0) {
			printf("test add long failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
		r = test4(use_c_api);
		if (r != 0) {
			printf("test add double failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
		r = test5(use_c_api);
		if (r != 0) {
			printf("test add string failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
		r = test6(use_c_api);
		if (r != 0) {
			printf("test add binary failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
		r = test_random(2, use_c_api);
		if (r != 0) {
			printf("test random caps failed\n");
			printf("in step %s\n", err_msgs[r - 1]);
			return 1;
		}
	}
	printf("test finish, press enter quit\n");
	getchar();
	return 0;
}

static int32_t test1(bool c) {
	RandomCapsFactory fac(c);
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_integer();
	}
	return fac.check();
}

static int32_t test2(bool c) {
	RandomCapsFactory fac(c);
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_float();
	}
	return fac.check();
}

static int32_t test3(bool c) {
	RandomCapsFactory fac(c);
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_long();
	}
	return fac.check();
}

static int32_t test4(bool c) {
	RandomCapsFactory fac(c);
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_double();
	}
	return fac.check();
}

static int32_t test5(bool c) {
	RandomCapsFactory fac(c);
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_string();
	}
	return fac.check();
}

static int32_t test6(bool c) {
	RandomCapsFactory fac(c);
	int32_t i;
	for (i = 0; i < MAX_MEMBERS; ++i) {
		fac.gen_binary();
	}
	return fac.check();
}

static int32_t test_random(int32_t depth, bool c) {
	RandomCapsFactory fac(c);
	fac.gen_object(depth);
	return fac.check();
}
