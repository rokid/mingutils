#include <string.h>
#include "random_caps_factory.h"
#include "demo_defs.h"

#define MEMBER_TYPE_INTEGER 0
#define MEMBER_TYPE_FLOAT 1
#define MEMBER_TYPE_LONG 2
#define MEMBER_TYPE_DOUBLE 3
#define MEMBER_TYPE_STRING 4
#define MEMBER_TYPE_BINARY 5
#define MEMBER_TYPE_OBJECT 6
#define MEMBER_TYPE_MAX 7

#define VISIBLE_CHAR_NUMBER 95
#define FIRST_VISIBLE_CHAR ' '

using namespace std;

RandomCapsFactory::RandomCapsFactory() {
	this_caps = caps_create();
}

RandomCapsFactory::~RandomCapsFactory() {
	caps_destroy(this_caps);
	size_t i;
	for (i = 0; i < sub_objects.size(); ++i) {
		delete sub_objects[i];
	}
}

void RandomCapsFactory::gen_integer() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int v = rand();
	integers.push_back(v);
	member_types.push_back(MEMBER_TYPE_INTEGER);
	caps_write_integer(this_caps, (int32_t)v);
}

void RandomCapsFactory::gen_float() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	float v = (float)rand() / (float)RAND_MAX;
	floats.push_back(v);
	member_types.push_back(MEMBER_TYPE_FLOAT);
	caps_write_float(this_caps, v);
}

void RandomCapsFactory::gen_long() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int64_t v = rand();
	longs.push_back(v);
	member_types.push_back(MEMBER_TYPE_LONG);
	caps_write_long(this_caps, v);
}

void RandomCapsFactory::gen_double() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	double v = (double)rand() / (double)RAND_MAX;
	doubles.push_back(v);
	member_types.push_back(MEMBER_TYPE_DOUBLE);
	caps_write_double(this_caps, v);
}

static char random_visible_char() {
	return rand() % VISIBLE_CHAR_NUMBER + FIRST_VISIBLE_CHAR;
}

void RandomCapsFactory::gen_string() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int32_t mod = MAX_STRING_LENGTH - MIN_STRING_LENGTH + 1;
	int32_t len = rand() % mod + MIN_STRING_LENGTH;
	int32_t i;
	strings.resize(strings.size() + 1);
	vector<string>::reverse_iterator it = strings.rbegin();
	(*it).resize(len);
	for (i = 0; i < len; ++i) {
		(*it)[i] = random_visible_char();
	}
	member_types.push_back(MEMBER_TYPE_STRING);
	caps_write_string(this_caps, (*it).c_str());
}

static uint8_t random_byte() {
	return rand() % 0x100;
}

void RandomCapsFactory::gen_binary() {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int32_t mod = MAX_BINARY_LENGTH - MIN_BINARY_LENGTH + 1;
	int32_t len = rand() % mod + MIN_BINARY_LENGTH;
	int32_t i;
	binarys.resize(binarys.size() + 1);
	vector<string>::reverse_iterator it = binarys.rbegin();
	(*it).resize(len);
	for (i = 0; i < len; ++i) {
		(*it)[i] = random_byte();
	}
	member_types.push_back(MEMBER_TYPE_BINARY);
	caps_write_binary(this_caps, (*it).data(), len);
}

void RandomCapsFactory::gen_object(uint32_t enable_sub_object) {
	if (member_types.size() >= MAX_MEMBERS)
		return;
	int32_t sub_member_size = rand() % MAX_MEMBERS;
	int32_t i;
	RandomCapsFactory* sub = new RandomCapsFactory();
	for (i = 0; i < sub_member_size; ++i) {
		sub->gen_random_member(enable_sub_object);
	}
	caps_write_object(this_caps, sub->caps());
	member_types.push_back(MEMBER_TYPE_OBJECT);
	sub_objects.push_back(sub);
}

void RandomCapsFactory::gen_random_member(uint32_t enable_sub_object) {
	int32_t mod = enable_sub_object ? MEMBER_TYPE_MAX : MEMBER_TYPE_MAX - 1;
	int32_t type = rand() % mod;

	switch (type) {
		case MEMBER_TYPE_INTEGER:
			gen_integer();
			break;
		case MEMBER_TYPE_FLOAT:
			gen_float();
			break;
		case MEMBER_TYPE_LONG:
			gen_long();
			break;
		case MEMBER_TYPE_DOUBLE:
			gen_double();
			break;
		case MEMBER_TYPE_STRING:
			gen_string();
			break;
		case MEMBER_TYPE_BINARY:
			gen_binary();
			break;
		case MEMBER_TYPE_OBJECT:
			gen_object(enable_sub_object - 1);
			break;
	}
}

bool RandomCapsFactory::check() {
	int32_t size = caps_serialize(this_caps, nullptr, 0);
	int8_t* buf = new int8_t[size];
	caps_t rcaps;
	int32_t i;
	int32_t r = CAPS_SUCCESS;

	caps_serialize(this_caps, buf, size);
	if (caps_parse(buf, size, &rcaps) != CAPS_SUCCESS) {
		delete[] buf;
		return false;
	}

	int32_t ci = 0;
	int32_t cl = 0;
	int32_t cf = 0;
	int32_t cd = 0;
	int32_t cS = 0;
	int32_t cB = 0;
	int32_t cO = 0;
	int32_t iv;
	float fv;
	int64_t lv;
	double dv;
	const char* Sv;
	const void* Bv;
	uint32_t Blen;
	caps_t Ov;
	for (i = 0; i < member_types.size(); ++i) {
		r = CAPS_SUCCESS;
		switch (member_types[i]) {
			case MEMBER_TYPE_INTEGER:
				r = caps_read_integer(rcaps, &iv);
				if (r == CAPS_SUCCESS) {
					if (iv != integers[ci++])
						r = -10000;
				}
				break;
			case MEMBER_TYPE_FLOAT:
				r = caps_read_float(rcaps, &fv);
				if (r == CAPS_SUCCESS) {
					if (fv != floats[cf++])
						r = -10001;
				}
				break;
			case MEMBER_TYPE_LONG:
				r = caps_read_long(rcaps, &lv);
				if (r == CAPS_SUCCESS) {
					if (lv != longs[cl++])
						r = -10002;
				}
				break;
			case MEMBER_TYPE_DOUBLE:
				r = caps_read_double(rcaps, &dv);
				if (r == CAPS_SUCCESS) {
					if (dv != doubles[cd++])
						r = -10003;
				}
				break;
			case MEMBER_TYPE_STRING:
				r = caps_read_string(rcaps, &Sv);
				if (r == CAPS_SUCCESS) {
					if (strcmp(Sv, strings[cS++].c_str()))
						r = -10004;
				}
				break;
			case MEMBER_TYPE_BINARY:
				r = caps_read_binary(rcaps, &Bv, &Blen);
				if (r == CAPS_SUCCESS) {
					if (memcmp(Bv, binarys[cB].data(), binarys[cB].length()))
						r = -10005;
					++cB;
				}
				break;
			case MEMBER_TYPE_OBJECT:
				r = caps_read_object(rcaps, &Ov);
				if (r == CAPS_SUCCESS) {
					RandomCapsFactory* t = sub_objects[cO++];
					if (!t->check())
						r = -10006;
				}
				break;
		}
		if (r != CAPS_SUCCESS) {
			printf("check failed, error = %d\n", r);
			break;
		}
	}
	delete[] buf;
	caps_destroy(rcaps);
	return r == CAPS_SUCCESS;
}
