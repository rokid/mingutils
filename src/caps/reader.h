#pragma once

#include <stdint.h>
#include "defs.h"
#include "caps.h"

namespace rokid {

class CapsReader : public Caps {
public:
	~CapsReader() noexcept;

	CapsReader& operator = (const Caps& o);

	int32_t parse(const void* data, uint32_t datasize, bool duplicate);

	inline const void* binary_data() const { return bin_data; }

	// override from 'Caps'
	int32_t write(int32_t v) { return CAPS_ERR_RDONLY; }
	int32_t write(float v) { return CAPS_ERR_RDONLY; }
	int32_t write(int64_t v) { return CAPS_ERR_RDONLY; }
	int32_t write(double v) { return CAPS_ERR_RDONLY; }
	int32_t write(const char* v) { return CAPS_ERR_RDONLY; }
	int32_t write(const void* v, uint32_t len) { return CAPS_ERR_RDONLY; }
	int32_t write(std::shared_ptr<Caps>& v) { return CAPS_ERR_RDONLY; }
	int32_t serialize(void* buf, uint32_t size) const { return CAPS_ERR_RDONLY; }

	int32_t read(int32_t& r);
	int32_t read(float& r);
	int32_t read(int64_t& r);
	int32_t read(double& r);
	int32_t read_string(std::string& r);
	int32_t read_binary(std::string& r);
	int32_t read(std::shared_ptr<Caps>& r);

	// read string & binary without memcpy
	int32_t read(const char*& r);
	int32_t read(const void*& r, uint32_t& len);

	int32_t type() const { return CAPS_TYPE_READER; }
	uint32_t binary_size() const;


public:
	const Header* header = nullptr;
	const int8_t* member_declarations = nullptr;
	const uint32_t* number_values = nullptr;
	const StringInfo* string_infos = nullptr;
	const BinaryInfo* binary_infos = nullptr;
	const int64_t* long_values = nullptr;

private:
	const int8_t* binary_section = nullptr;
	const int8_t* string_section = nullptr;
	uint32_t current_read_member = 0;
	int32_t duplicated = 0;

	const int8_t* bin_data = nullptr;
};

} // namespace rokid
