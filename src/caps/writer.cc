#include <string.h>
#include <string>
#include "writer.h"
#include "reader.h"
#include "caps.h"

using std::vector;
using std::string;

namespace rokid {

typedef struct {
	char* mdecls;
	uint32_t* ivalues;
	StringInfo* sinfos;
	BinaryInfo* binfos;
	int64_t* lvalues;
	int8_t* bin_section;
	int8_t* str_section;

	uint32_t long_index = 0;
	uint32_t string_index = 0;
	uint32_t binary_index = 0;
	uint32_t cur_strp = 0;
	uint32_t cur_binp = 0;
} WritePointer;

class Member {
public:
	virtual ~Member() = default;

	virtual void do_serialize(Header* header, WritePointer* wp) = 0;
};

class IntegerMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	int32_t value;
};

class LongMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	int64_t value;
};

class FloatMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	float value;
};

class DoubleMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	double value;
};

class StringMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	string value;
};

class BinaryMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	string value;
};

class ObjectMember : public Member {
public:
	void do_serialize(Header* header, WritePointer* wp);

	Caps* value;
};

void IntegerMember::do_serialize(Header* header, WritePointer* wp) {
	wp->mdecls[0] = 'i';
	++wp->mdecls;
	wp->ivalues[0] = value;
	++wp->ivalues;
}

void FloatMember::do_serialize(Header* header, WritePointer* wp) {
	wp->mdecls[0] = 'f';
	++wp->mdecls;
	memcpy(wp->ivalues, &value, sizeof(value));
	++wp->ivalues;
}

void LongMember::do_serialize(Header* header, WritePointer* wp) {
	wp->mdecls[0] = 'l';
	++wp->mdecls;
	wp->lvalues[wp->long_index] = value;
	++wp->long_index;
}

void DoubleMember::do_serialize(Header* header, WritePointer* wp) {
	wp->mdecls[0] = 'd';
	++wp->mdecls;
	memcpy(wp->lvalues + wp->long_index, &value, sizeof(value));
	++wp->long_index;
}

void StringMember::do_serialize(Header* header, WritePointer* wp) {
	wp->mdecls[0] = 'S';
	++wp->mdecls;
	wp->sinfos[wp->string_index].len = value.length();
	wp->sinfos[wp->string_index].offset = header->string_section + wp->cur_strp;
	++wp->string_index;
	memcpy(wp->str_section + wp->cur_strp, value.c_str(), value.length() + 1);
	wp->cur_strp += value.length() + 1;
}

void BinaryMember::do_serialize(Header* header, WritePointer* wp) {
	wp->mdecls[0] = 'B';
	++wp->mdecls;
	wp->binfos[wp->binary_index].len = value.length();
	wp->binfos[wp->binary_index].offset = header->binary_section + wp->cur_binp;
	++wp->binary_index;
	memcpy(wp->bin_section + wp->cur_binp, value.data(), value.length());
	wp->cur_binp += ALIGN4(value.length());
}

void ObjectMember::do_serialize(Header* header, WritePointer* wp) {
	int32_t obj_size;

	wp->mdecls[0] = 'O';
	++wp->mdecls;
	obj_size = value->binary_size();
	wp->binfos[wp->binary_index].len = obj_size;
	wp->binfos[wp->binary_index].offset = header->binary_section + wp->cur_binp;
	++wp->binary_index;
	if (value->type() == CAPS_TYPE_WRITER) {
		static_cast<CapsWriter*>(value)->serialize(wp->bin_section + wp->cur_binp, obj_size);
	} else {
		memcpy(wp->bin_section + wp->cur_binp, static_cast<CapsReader*>(value)->binary_data(), obj_size);
	}
	wp->cur_binp += ALIGN4(obj_size);
}

CapsWriter::CapsWriter() {
	members.reserve(8);
}

CapsWriter::~CapsWriter() noexcept {
	uint32_t i;
	for (i = 0; i < members.size(); ++i) {
		delete members[i];
	}
}

void CapsWriter::write(int32_t v) {
	IntegerMember* m = new IntegerMember();
	m->value = v;
	members.push_back(m);
	++number_member_number;
}

void CapsWriter::write(int64_t v) {
	LongMember* m = new LongMember();
	m->value = v;
	members.push_back(m);
	++long_member_number;
}

void CapsWriter::write(float v) {
	FloatMember* m = new FloatMember();
	m->value = v;
	members.push_back(m);
	++number_member_number;
}

void CapsWriter::write(double v) {
	DoubleMember* m = new DoubleMember();
	m->value = v;
	members.push_back(m);
	++long_member_number;
}

void CapsWriter::write(const char* v) {
	StringMember* m = new StringMember();
	m->value = v;
	members.push_back(m);
	++string_member_number;
	string_section_size += m->value.length() + 1;
}

void CapsWriter::write(const void* v, uint32_t l) {
	BinaryMember* m = new BinaryMember();
	m->value.assign((const char*)v, l);
	members.push_back(m);
	++binary_object_member_number;
	binary_section_size += ALIGN4(l);
}

void CapsWriter::write(Caps* v) {
	ObjectMember* m = new ObjectMember();
	m->value = v;
	members.push_back(m);
	++binary_object_member_number;
	binary_section_size += ALIGN4(v->binary_size());
}

uint32_t CapsWriter::binary_size() const {
	uint32_t r;
	
	r = sizeof(Header);
	r += ALIGN4(members.size() + 1); // member declaration
	r += number_member_number * sizeof(uint32_t); // number section
	r += string_member_number * sizeof(StringInfo); // string info section
	r += binary_object_member_number * sizeof(BinaryInfo); // binary info section
	r = ALIGN8(r);
	// 以上全部加起来ALIGN8
	r += long_member_number * sizeof(int64_t); // long section
	r += binary_section_size;
	r += string_section_size;
	return r;
}

int32_t CapsWriter::serialize(void* buf, uint32_t bufsize) {
	Header* header;
	WritePointer wp;

	if (buf == nullptr)
		return CAPS_ERR_INVAL;
	header = reinterpret_cast<Header*>(buf);
	wp.mdecls = reinterpret_cast<char*>(header + 1);
	wp.ivalues = reinterpret_cast<uint32_t*>(wp.mdecls + ALIGN4(members.size() + 1));
	wp.sinfos = reinterpret_cast<StringInfo*>(wp.ivalues + number_member_number);
	wp.binfos = reinterpret_cast<BinaryInfo*>(wp.sinfos + string_member_number);
	int32_t loff = ALIGN8(reinterpret_cast<int8_t*>(wp.binfos + binary_object_member_number) - reinterpret_cast<int8_t*>(buf));
	wp.lvalues = reinterpret_cast<int64_t*>(reinterpret_cast<int8_t*>(buf) + loff);
	wp.bin_section = reinterpret_cast<int8_t*>(wp.lvalues + long_member_number);
	wp.str_section = wp.bin_section + binary_section_size;
	uint32_t total_size = loff + long_member_number * sizeof(int64_t) + binary_section_size + string_section_size;
	if (total_size > bufsize)
		return total_size;

	header->magic = MAGIC_NUM | VERSION_NUM;
	header->length = total_size;
	header->number_section = reinterpret_cast<int8_t*>(wp.ivalues) - reinterpret_cast<int8_t*>(buf);
	header->long_section = reinterpret_cast<int8_t*>(wp.lvalues) - reinterpret_cast<int8_t*>(buf);
	header->string_info_section = reinterpret_cast<int8_t*>(wp.sinfos) - reinterpret_cast<int8_t*>(buf);
	header->binary_info_section = reinterpret_cast<int8_t*>(wp.binfos) - reinterpret_cast<int8_t*>(buf);
	header->string_section = reinterpret_cast<int8_t*>(wp.str_section) - reinterpret_cast<int8_t*>(buf);
	header->binary_section = reinterpret_cast<int8_t*>(wp.bin_section) - reinterpret_cast<int8_t*>(buf);
	wp.mdecls[0] = members.size();
	++wp.mdecls;

	size_t i;
	for (i = 0; i < members.size(); ++i) {
		members[i]->do_serialize(header, &wp);
	}
	return total_size;
}

} // namespace rokid
