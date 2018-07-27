#include <string.h>
#include "writer.h"
#include "reader.h"

using namespace std;

namespace rokid {

static void copy_from_reader(CapsReader* dst, const CapsReader* src) {
	const void* data = src->binary_data();
	uint32_t size = src->binary_size();
	if (data != nullptr && size > 0) {
		dst->parse(data, size, true);
	}
}

static void copy_from_writer(CapsReader* dst, const CapsWriter* src) {
	uint32_t size = src->binary_size();
	if (size > 0) {
		int8_t* data = new int8_t[size];
		if (src->serialize(data, size) > 0) {
			dst->parse(data, size, true);
		}
		delete[] data;
	}
}

CapsReader& CapsReader::operator = (const Caps& o) {
	if (o.type() == CAPS_TYPE_WRITER)
		copy_from_writer(this, static_cast<const CapsWriter*>(&o));
	else
		copy_from_reader(this, static_cast<const CapsReader*>(&o));
	return *this;
}

int32_t CapsReader::parse(const void* data, uint32_t datasize, bool dup) {
	uint8_t num_str = 0;
	uint8_t num_bin = 0;
	uint8_t num_obj = 0;
	uint8_t num_num = 0;
	uint8_t num_long = 0;
	uint8_t i;
	uint8_t num_members;
	const int8_t* b;

	if (datasize < sizeof(Header))
		return CAPS_ERR_INVAL;
	if (dup) {
		bin_data = new int8_t[datasize];
		memcpy(const_cast<int8_t*>(bin_data), data, datasize);
		duplicated = 1;
		b = reinterpret_cast<const int8_t*>(bin_data);
	} else {
		duplicated = 0;
		b = reinterpret_cast<const int8_t*>(data);
		bin_data = b;
	}

	header = reinterpret_cast<const Header*>(b);
	if (header->length != datasize)
		return CAPS_ERR_CORRUPTED;
	if ((header->magic & (~VERSION_MASK)) != (MAGIC_NUM & (~VERSION_MASK)))
		return CAPS_ERR_CORRUPTED;
	if ((header->magic & VERSION_MASK) > CAPS_VERSION)
		return CAPS_ERR_VERSION_UNSUPP;

	member_declarations = reinterpret_cast<const int8_t*>(header + 1);
	// first element is number of members
	++member_declarations;
	number_values = reinterpret_cast<const uint32_t*>(b + header->number_section);
	string_infos = reinterpret_cast<const StringInfo*>(b + header->string_info_section);
	binary_infos = reinterpret_cast<const BinaryInfo*>(b + header->binary_info_section);
	long_values = reinterpret_cast<const int64_t*>(b + header->long_section);
	binary_section = reinterpret_cast<const int8_t*>(b + header->binary_section);
	string_section = reinterpret_cast<const int8_t*>(b + header->string_section);

	// caclulate number of strings, number of binary, number of long values
	num_members = member_declarations[-1];
	for (i = 0; i < num_members; ++i) {
		switch (member_declarations[i]) {
			case 'i':
			case 'f':
				++num_num;
				break;
			case 'l':
			case 'd':
				++num_long;
				break;
			case 'S':
				++num_str;
				break;
			case 'B':
				++num_bin;
				break;
			case 'O':
				++num_bin;
				++num_obj;
		}
	}
	uint32_t ssize = 0;
	for (i = 0; i < num_str; ++i) {
		ssize += string_infos[i].len + 1;
	}
	const int8_t* e = string_section + ssize;
	if (e - b != datasize)
		return CAPS_ERR_CORRUPTED;
	// TODO: check member offsets valid
	//       check StringInfo offsets valid
	//       check BinaryInfo offsets valid
	return CAPS_SUCCESS;
}

uint32_t CapsReader::binary_size() const {
	return bin_data ? header->length : 0;
}

int32_t CapsReader::read(int32_t& r) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'i')
		return CAPS_ERR_INCORRECT_TYPE;
	r = number_values[0];
	++number_values;
	++current_read_member;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read(float& r) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'f')
		return CAPS_ERR_INCORRECT_TYPE;
	memcpy(&r, number_values, sizeof(r));
	++number_values;
	++current_read_member;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read(int64_t& r) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'l')
		return CAPS_ERR_INCORRECT_TYPE;
	r = long_values[0];
	++long_values;
	++current_read_member;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read(double& r) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'd')
		return CAPS_ERR_INCORRECT_TYPE;
	memcpy(&r, long_values, sizeof(r));
	++long_values;
	++current_read_member;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read(const char*& r) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'S')
		return CAPS_ERR_INCORRECT_TYPE;
	const StringInfo* info = string_infos;
	r = reinterpret_cast<const char*>(bin_data + info->offset);
	++string_infos;
	++current_read_member;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read(const void*& r, uint32_t& length) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'B')
		return CAPS_ERR_INCORRECT_TYPE;
	const BinaryInfo* info = binary_infos;
	r = bin_data + info->offset;
	length = info->len;
	++binary_infos;
	++current_read_member;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read_string(string& r) {
	const char* s;
	int32_t code = read(s);
	if (code != CAPS_SUCCESS)
		return code;
	r = s;
	return CAPS_SUCCESS;
}

int32_t CapsReader::read_binary(string& r) {
	const void* b;
	uint32_t l;
	int32_t code = read(b, l);
	if (code != CAPS_SUCCESS)
		return code;
	r.assign((const char*)b, l);
	return CAPS_SUCCESS;
}

int32_t CapsReader::read(shared_ptr<Caps>& r) {
	if (current_read_member >= member_declarations[-1])
		return CAPS_ERR_EOO;
	if (member_declarations[current_read_member] != 'O')
		return CAPS_ERR_INCORRECT_TYPE;
	const BinaryInfo* info = binary_infos;
	shared_ptr<CapsReader> sub;
	int32_t code = CAPS_SUCCESS;
	
	if (info->len) {
		sub = make_shared<CapsReader>();
		code = sub->parse(bin_data + info->offset, info->len, true);
	}
	++binary_infos;
	++current_read_member;
	r = static_pointer_cast<Caps>(sub);
	return code;
}

CapsReader::~CapsReader() noexcept {
	if (duplicated)
		delete[] bin_data;
}

} // namespace rokid
