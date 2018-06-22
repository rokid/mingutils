#pragma once

#include <string>
#include <vector>
#include "caps.h"

class RandomCapsFactory {
public:
	RandomCapsFactory();

	~RandomCapsFactory();

	void gen_integer();
	void gen_float();
	void gen_long();
	void gen_double();
	void gen_string();
	void gen_binary();
	void gen_object(uint32_t enable_sub_object);

	inline caps_t caps() const { return this_caps; }

	// 进行序列化反序列化测试并检查结果
	bool check();

private:
	void gen_random_member(uint32_t enable_sub_object);

private:
	std::vector<uint8_t> member_types;
	std::vector<int32_t> integers;
	std::vector<float> floats;
	std::vector<int64_t> longs;
	std::vector<double> doubles;
	std::vector<std::string> strings;
	std::vector<std::string> binarys;
	std::vector<RandomCapsFactory*> sub_objects;
	caps_t this_caps;
};
