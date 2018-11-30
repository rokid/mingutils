#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <cctype>
#include <vector>
#include "clargs.h"

static const uint32_t BUFFER_LENGTH = 1024 * 1024;

using namespace std;

class clargs_inst : public CLArgs {
public:
  clargs_inst() {
    clpairs.reserve(16);
    buffer = (char*)mmap(nullptr, BUFFER_LENGTH, PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    empty_pair.key = nullptr;
    empty_pair.value = nullptr;
  }

  ~clargs_inst() {
    munmap(buffer, BUFFER_LENGTH);
  }

  uint32_t size() const {
    return clpairs.size();
  }

  const CLPair& operator[](uint32_t idx) const {
    return at(idx);
  }

  const CLPair& at(uint32_t idx) const {
    if (idx >= clpairs.size())
      return empty_pair;
    return clpairs[idx];
  }

  bool has(const char* key) const {
    size_t sz = clpairs.size();
    size_t i;
    for (i = 0; i < sz; ++i) {
      if (clpairs[i].match(key))
        return true;
    }
    return false;
  }

  bool parse(int32_t argc, char** argv) {
    int32_t i;
    size_t len;
    const char* s;
    uint32_t psr;
    char prev_single_opt = '\0';

    for (i = 0; i < argc; ++i) {
      s = argv[i];
      len = strlen(s);
      if (len > 2) {
        // --foo=bar
        // --foo
        if (s[0] == '-' && s[1] == '-') {
          if (parse_pair(s + 2, len - 2)) {
            prev_single_opt = '\0';
            continue;
          }
          goto parse_arg;
        }
      }
      if (len > 1) {
        if (s[0] == '-') {
          // 检测到单个'-'加上单个字母或数字
          // 记录在prev_single_opt变量中
          // 否则清空prev_single_opt变量
          // -abc
          // -a xxoo
          // -b -c
          psr = parse_single_minus(s + 1, len - 1);
          if (psr > 0) {
            prev_single_opt = psr == 1 ? s[1] : '\0';
            continue;
          }
        }
      }

parse_arg:
      if (prev_single_opt) {
        char* val = copy_str_to_buf(s, len);
        set_last_pair_value(val);
        prev_single_opt = '\0';
      } else {
        char* val = copy_str_to_buf(s, len);
        add_pair(nullptr, val);
      }
    }
    return true;
  }

private:
  bool parse_pair(const char* s, size_t l) {
    if (!isalpha(s[0]))
      return false;
    size_t i;
    char* key = nullptr;
    char* val = nullptr;
    for (i = 1; i < l; ++i) {
      if (s[i] == '=') {
        key = copy_str_to_buf(s, i);
        val = copy_str_to_buf(s + i + 1, l - i - 1);
        break;
      }
    }
    if (i == l)
      key = copy_str_to_buf(s, l);
    if (l > i + 1) {
      // '=' 后面第一个字符不能又是'='
      if (val[0] == '=')
        return false;
    }
    add_pair(key, val);
    return true;
  }

  uint32_t parse_single_minus(const char* s, size_t l) {
    size_t i;
    for (i = 0; i < l; ++i) {
      if (!isalnum(s[i]))
        return 0;
    }
    char* key;
    for (i = 0; i < l; ++i) {
      key = copy_str_to_buf(s + i, 1);
      add_pair(key, nullptr);
    }
    return l;
  }

  void add_pair(const char* s1, const char* s2) {
    CLPair t;
    t.key = s1;
    t.value = s2;
    clpairs.push_back(t);
  }

  void set_last_pair_value(const char* v) {
    clpairs.back().value = v;
  }

  char* copy_str_to_buf(const char* p, uint32_t size) {
    char* r = buffer + buf_offset;
    if (size)
      memcpy(r, p, size);
    r[size] = '\0';
    buf_offset += size + 1;
    return r;
  }

private:
  vector<CLPair> clpairs;
  char* buffer;
  uint32_t buf_offset = 0;
  CLPair empty_pair;
};

shared_ptr<CLArgs> CLArgs::parse(int32_t argc, char** argv) {
  shared_ptr<clargs_inst> r = make_shared<clargs_inst>();
  if (!r->parse(argc, argv))
    return nullptr;
  return static_pointer_cast<CLArgs>(r);
}

bool CLPair::match(const char* key) const {
  if (key == nullptr && this->key == nullptr)
    return true;
  if (this->key == nullptr || key == nullptr)
    return false;
  return strcmp(key, this->key) == 0;
}

bool CLPair::to_integer(int32_t& res) const {
  if (value == nullptr)
    return false;
  char* ep;
  res = strtol(value, &ep, 10);
  return ep != value && ep[0] == '\0';
}

clargs_h clargs_parse(int32_t argc, char** argv) {
  clargs_inst* r = new clargs_inst();
  if (!r->parse(argc, argv)) {
    delete r;
    return 0;
  }
  return reinterpret_cast<intptr_t>(r);
}

void clargs_destroy(clargs_h handle) {
  if (handle == 0)
    return;
  delete reinterpret_cast<clargs_inst*>(handle);
}

uint32_t clargs_size(clargs_h handle) {
  return reinterpret_cast<clargs_inst*>(handle)->size();
}

int32_t clargs_get(clargs_h handle, uint32_t idx, const char** key, const char** value) {
  clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
  if (idx >= inst->size())
    return -1;
  *key = inst->at(idx).key;
  *value = inst->at(idx).value;
  return 0;
}

int32_t clargs_get_integer(clargs_h handle, uint32_t idx, const char** key, int32_t* res) {
  clargs_inst* inst = reinterpret_cast<clargs_inst*>(handle);
  if (idx >= inst->size())
    return -1;
  *key = inst->at(idx).key;
  if (!inst->at(idx).to_integer(*res))
    return -2;
  return 0;
}
