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

  if (datasize <= sizeof(Header))
    return CAPS_ERR_INVAL;
  if (duplicated && bin_data)
    delete bin_data;
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
  if ((header->magic & VERSION_MASK) != CAPS_VERSION)
    return CAPS_ERR_VERSION_UNSUPP;

  member_declarations = reinterpret_cast<const char*>(b + datasize);
  --member_declarations;
  num_members = (uint8_t)member_declarations[0];
  if (datasize < sizeof(Header) + ALIGN4(num_members + 1))
    return CAPS_ERR_CORRUPTED;
  --member_declarations;
  for (i = 0; i < num_members; ++i) {
    switch (member_declarations[-(int32_t)i]) {
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
        break;
      default:
        return CAPS_ERR_CORRUPTED;
    }
  }
  long_values = reinterpret_cast<const int64_t*>(header + 1);
  number_values = reinterpret_cast<const int32_t*>(long_values + num_long);
  bin_sizes = reinterpret_cast<const uint32_t*>(number_values + num_num);
  binary_section = reinterpret_cast<const int8_t*>(bin_sizes + num_bin);
  if (datasize < reinterpret_cast<const int8_t*>(binary_section) - b)
    return CAPS_ERR_CORRUPTED;
  uint32_t bin_sec_size = 0;
  for (i = 0; i < num_bin; ++i) {
    bin_sec_size += ALIGN4(bin_sizes[i]);
  }
  string_section = reinterpret_cast<const char*>(binary_section + bin_sec_size);
  // TODO: 检查 string section 与 member declarations 不重叠
  return CAPS_SUCCESS;
}

uint32_t CapsReader::binary_size() const {
  return bin_data ? header->length : 0;
}

int32_t CapsReader::read(int32_t& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'i')
    return CAPS_ERR_INCORRECT_TYPE;
  r = number_values[0];
  ++number_values;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(uint32_t& r) {
  int32_t i;
  int32_t rt = read(i);
  if (rt == CAPS_SUCCESS)
    r = (uint32_t)i;
  return rt;
}

int32_t CapsReader::read(float& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'f')
    return CAPS_ERR_INCORRECT_TYPE;
  memcpy(&r, number_values, sizeof(r));
  ++number_values;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(int64_t& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'l')
    return CAPS_ERR_INCORRECT_TYPE;
  r = long_values[0];
  ++long_values;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(uint64_t& r) {
  int64_t i;
  int32_t rt = read(i);
  if (rt == CAPS_SUCCESS)
    r = (uint64_t)i;
  return rt;
}

int32_t CapsReader::read(double& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'd')
    return CAPS_ERR_INCORRECT_TYPE;
  memcpy(&r, long_values, sizeof(r));
  ++long_values;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(const char*& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'S')
    return CAPS_ERR_INCORRECT_TYPE;
  r = string_section;
  string_section += strlen(r) + 1;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(const void*& r, uint32_t& length) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'B')
    return CAPS_ERR_INCORRECT_TYPE;
  r = binary_section;
  length = bin_sizes[0];
  binary_section += ALIGN4(length);
  ++bin_sizes;
  ++current_read_member;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(string& r) {
  const char* s;
  int32_t code = read(s);
  if (code != CAPS_SUCCESS)
    return code;
  r = s;
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(vector<uint8_t>& r) {
  const void* b;
  uint32_t l;
  int32_t code = read(b, l);
  if (code != CAPS_SUCCESS)
    return code;
  r.resize(l);
  memcpy(r.data(), b, l);
  return CAPS_SUCCESS;
}

int32_t CapsReader::read_string(string& r) {
  return read(r);
}

int32_t CapsReader::read_binary(string& r) {
  const void* b;
  uint32_t l;
  int32_t code = read(b, l);
  if (code != CAPS_SUCCESS)
    return code;
  r.assign(reinterpret_cast<const char*>(b), l);
  return CAPS_SUCCESS;
}

int32_t CapsReader::read(shared_ptr<Caps>& r) {
  if (end_of_object())
    return CAPS_ERR_EOO;
  if (current_member_type() != 'O')
    return CAPS_ERR_INCORRECT_TYPE;

  shared_ptr<CapsReader> sub;
  int32_t code = CAPS_SUCCESS;
  
  if (bin_sizes[0] > 0) {
    sub = make_shared<CapsReader>();
    code = sub->parse(binary_section, bin_sizes[0], true);
  }
  binary_section += bin_sizes[0];
  ++bin_sizes;
  ++current_read_member;
  r = static_pointer_cast<Caps>(sub);
  return code;
}

CapsReader::~CapsReader() noexcept {
  if (duplicated)
    delete[] bin_data;
}

int8_t CapsReader::current_member_type() const {
  if (end_of_object())
    return '\0';
  return member_declarations[-(int32_t)current_read_member];
}

bool CapsReader::end_of_object() const {
  return (uint8_t)member_declarations[1] <= current_read_member;
}

void CapsReader::record(CapsReaderRecord& rec) const {
  rec.number_values = number_values;
  rec.long_values = long_values;
  rec.bin_sizes = bin_sizes;
  rec.binary_section = binary_section;
  rec.string_section = string_section;
  rec.current_read_member = current_read_member;
}

void CapsReader::rollback(const CapsReaderRecord& rec) {
  number_values = rec.number_values;
  long_values = rec.long_values;
  bin_sizes = rec.bin_sizes;
  binary_section = rec.binary_section;
  string_section = rec.string_section;
  current_read_member = rec.current_read_member;
}

uint32_t CapsReader::size() const {
  return member_declarations[1];
}

int32_t CapsReader::next_type() const {
  int32_t r = current_member_type();
  if (r == '\0')
    return CAPS_ERR_EOO;
  return r;
}

} // namespace rokid
