#include "caps.h"
#include "reader.h"
#include "writer.h"

using namespace rokid;

caps_t caps_create() {
	return reinterpret_cast<caps_t>(new CapsWriter());
}

int32_t caps_parse(const void* data, uint32_t length, caps_t* result) {
	if (data == nullptr || length == 0 || result == nullptr)
		return CAPS_ERR_INVAL;
	CapsReader* reader = new CapsReader();
	int32_t r = reader->parse(data, length);
	if (r != CAPS_SUCCESS)
		return r;
	*result = reinterpret_cast<caps_t>(reader);
	return CAPS_SUCCESS;
}

int32_t caps_serialize(caps_t caps, void* buf, uint32_t bufsize) {
	if (caps == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	if (buf == nullptr || bufsize == 0)
		return writer->binary_size();
	return static_cast<CapsWriter*>(writer)->serialize(buf, bufsize);
}

int32_t caps_write_integer(caps_t caps, int32_t v) {
	if (caps == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(v);
	return CAPS_SUCCESS;
}

int32_t caps_write_float(caps_t caps, float v) {
	if (caps == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(v);
	return CAPS_SUCCESS;
}

int32_t caps_write_long(caps_t caps, int64_t v) {
	if (caps == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(v);
	return CAPS_SUCCESS;
}

int32_t caps_write_double(caps_t caps, double v) {
	if (caps == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(v);
	return CAPS_SUCCESS;
}

int32_t caps_write_string(caps_t caps, const char* v) {
	if (caps == 0 || v == nullptr)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(v);
	return CAPS_SUCCESS;
}

int32_t caps_write_binary(caps_t caps, const void* v, uint32_t length) {
	if (caps == 0 || v == nullptr || length == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(v, length);
	return CAPS_SUCCESS;
}

int32_t caps_write_object(caps_t caps, caps_t v) {
	if (caps == 0 || v == 0)
		return CAPS_ERR_INVAL;
	Caps* writer = reinterpret_cast<Caps*>(caps);
	if (writer->type() != CAPS_TYPE_WRITER)
		return CAPS_ERR_RDONLY;
	static_cast<CapsWriter*>(writer)->write(reinterpret_cast<Caps*>(v));
	return CAPS_SUCCESS;
}

int32_t caps_read_integer(caps_t caps, int32_t* r) {
	if (caps == 0 || r == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*r);
}

int32_t caps_read_long(caps_t caps, int64_t* r) {
	if (caps == 0 || r == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*r);
}

int32_t caps_read_float(caps_t caps, float* r) {
	if (caps == 0 || r == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*r);
}

int32_t caps_read_double(caps_t caps, double* r) {
	if (caps == 0 || r == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*r);
}

int32_t caps_read_string(caps_t caps, const char** r) {
	if (caps == 0 || r == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*r);
}

int32_t caps_read_binary(caps_t caps, const void** r, uint32_t* length) {
	if (caps == 0 || r == nullptr || length == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*r, *length);
}

int32_t caps_read_object(caps_t caps, caps_t* r) {
	if (caps == 0 || r == nullptr)
		return CAPS_ERR_INVAL;
	Caps* reader = reinterpret_cast<Caps*>(caps);
	if (reader->type() != CAPS_TYPE_READER)
		return CAPS_ERR_WRONLY;
	return static_cast<CapsReader*>(reader)->read(*reinterpret_cast<CapsReader**>(r));
}

void caps_destroy(caps_t caps) {
	if (caps)
		delete reinterpret_cast<Caps*>(caps);
}

int32_t caps_binary_info(const void* data, uint32_t* version, uint32_t* length) {
	const uint32_t* h = reinterpret_cast<const uint32_t*>(data);
	if ((h[0] & (~VERSION_MASK)) != MAGIC_NUM)
		return CAPS_ERR_CORRUPTED;
	if (version)
		*version = h[0] & VERSION_MASK;
	if (length)
		*length = h[1];
	return CAPS_SUCCESS;
}
