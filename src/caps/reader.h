#pragma once

#include <stdint.h>
#include "defs.h"

namespace rokid {

class CapsReader : public Caps {
public:
	~CapsReader() noexcept;

	int32_t parse(const void* data, uint32_t datasize);

	inline const void* binary_data() const { return bin_data; }

	// override from 'Caps'
	int32_t type() const { return CAPS_TYPE_READER; }

	uint32_t binary_size() const;

	int32_t read(int32_t& r);
	int32_t read(float& r);
	int32_t read(int64_t& r);
	int32_t read(double& r);
	int32_t read(const void*& r, uint32_t& length);
	int32_t read(const char*& r);
	int32_t read(CapsReader*& r);

private:
	const Header* header = nullptr;
	const int8_t* member_declarations = nullptr;
	const uint32_t* number_values = nullptr;
	const StringInfo* string_infos = nullptr;
	const BinaryInfo* binary_infos = nullptr;
	const int64_t* long_values = nullptr;
	const int8_t* binary_section = nullptr;
	const int8_t* string_section = nullptr;
	uint32_t current_read_member = 0;
	CapsReader* sub_objects = nullptr;
	CapsReader* current_sub_object = nullptr;

	const int8_t* bin_data = nullptr;
};

} // namespace rokid
