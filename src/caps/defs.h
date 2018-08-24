#pragma once

#include <stdint.h>

#define ALIGN4(v) ((v) + 3 & ~3)
#define ALIGN8(v) ((v) + 7 & ~7)
#define MAGIC_NUM 0x7d1a8200
#define VERSION_MASK 0xff

namespace rokid {

typedef struct {
	uint32_t magic;
	uint32_t length;
} Header;

typedef struct {
	const int32_t* number_values;
	const int64_t* long_values;
	const uint32_t* bin_sizes;
	const int8_t* binary_section;
	const char* string_section;
	uint32_t current_read_member;
} CapsReaderRecord;

} // namespace rokid
