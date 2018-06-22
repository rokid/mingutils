#pragma once

#include <stdint.h>

#define CAPS_TYPE_WRITER 0
#define CAPS_TYPE_READER 1

#define ALIGN4(v) ((v) + 3 & ~3)
#define ALIGN8(v) ((v) + 7 & ~7)
#define MAGIC_NUM 0x7d1a8201
#define VERSION_MASK 0xff

namespace rokid {

class Caps {
public:
	virtual ~Caps() = default;

	virtual int32_t type() const = 0;

	virtual uint32_t binary_size() const = 0;
};

typedef struct {
	uint32_t magic;
	uint32_t length;
	uint16_t number_section;
	uint16_t long_section;
	uint16_t string_info_section;
	uint16_t binary_info_section;
	uint32_t string_section;
	uint32_t binary_section;
} Header;

typedef struct StringInfo {
	uint32_t len;
	uint32_t offset;
} StringInfo;
typedef struct StringInfo BinaryInfo;

} // namespace rokid
