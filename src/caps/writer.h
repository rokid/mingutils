#pragma once

#include <stdint.h>
#include <vector>
#include "defs.h"

namespace rokid {

class Member;

class CapsWriter : public Caps {
public:
	CapsWriter();

	~CapsWriter();

	void write(int32_t v);
	void write(int64_t v);
	void write(float v);
	void write(double v);
	void write(const char* v);
	void write(const void* v, uint32_t l);
	void write(Caps* v);

	int32_t serialize(void* buf, uint32_t bufsize);

	// override from 'Caps'
	int32_t type() const { return CAPS_TYPE_WRITER; }

	uint32_t binary_size() const;

private:
	std::vector<Member*> members;
	uint32_t number_member_number = 0;
	uint32_t long_member_number = 0;
	uint32_t string_member_number = 0;
	uint32_t binary_object_member_number = 0;
	uint32_t binary_section_size = 0;
	uint32_t string_section_size = 0;
};

} // namespace rokid
