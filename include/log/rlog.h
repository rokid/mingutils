#ifndef ROKID_LOG_H
#define ROKID_LOG_H

#include <stdint.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ROKID_LOGLEVEL_VERBOSE = 0,
  ROKID_LOGLEVEL_DEBUG,
  ROKID_LOGLEVEL_INFO,
  ROKID_LOGLEVEL_WARNING,
  ROKID_LOGLEVEL_ERROR,

  ROKID_LOGLEVEL_NUMBER,
} RokidLogLevel;

typedef enum {
  ROKID_LOG_CTL_ADD_ENDPOINT = 0,
  ROKID_LOG_CTL_REMOVE_ENDPOINT,
  ROKID_LOG_CTL_ASSOCIATE,
  ROKID_LOG_CTL_DISASSOCIATE,
  ROKID_LOG_CTL_DEFAULT_ENDPOINT,
} RokidLogControlOperation;

typedef int32_t (*RokidLogWriter)(RokidLogLevel, const char* tag,
    const char* fmt, va_list ap, void* arg_of_endpoint,
    void* arg_of_association);

typedef struct {
  const char* name;
  RokidLogWriter writer;
  void* arg;
} RokidLogEndpoint;

typedef struct {
  const char* tag;
  const char* endpoint;
} RokidLogAssociation;

typedef struct {
  const char* host;
  int32_t port;
} TCPSocketArg;

void rokid_log_print(RokidLogLevel lv, const char* tag, const char* fmt, ...);

// 'rokid_log_ctl' can take an optional second argument.
// second argument format is determined by 'op'.
//
// op == ROKID_LOG_CTL_ADD_ENDPOINT
//   add log write endpoint
//   arg1: [const char*]  name of endpoint
//   arg2: [RokidLogWriter]  log write function
//   arg3: [void*]  additional arg of endpoint
// op == ROKID_LOG_CTL_REMOVE_ENDPOINT
//   remove endpoint
//   arg1: [const char*], name of endpoint
// op == ROKID_LOG_CTL_ASSOCIATE
//   associate tag and endpoint, log with the 'tag' will write to specified endpoint
//   arg1: [const char*],  log tag
//   arg2: [const char*],  name of endpoint
//   arg3: [void*],  addtional arg of association, write function will receive this arg
// op == ROKID_LOG_CTL_DISASSOCIATE
//   tag and endpoint disassociate
//   arg1: [const char*],  log tag
//   arg2: [const char*],  name of endpoint
//   if arg2 is NULL, disassociate all endpoints associated with the 'tag'
// op == ROKID_LOG_CTL_DEFAULT_ENDPOINT
//   set default endpoint
//   arg1: [const char*],  name of endpoint
//   arg2: [void*],  additional arg for write function of default endpoint
//
// return 0 if success
// return -1 if failed
int32_t rokid_log_ctl(RokidLogControlOperation op, ...);

// return number of endpoints (builtin + plugin)
// 'endpoints' can be NULL
// if 'endpoints' not NULL, 'arr_size' specify 'endpoints' array size
// note: builtin log endpoint names is "stdout", "file", "tcp-socket"
int32_t rokid_log_endpoints(RokidLogEndpoint* endpoints, uint32_t arr_size);

// return number of associations
// 'associations' can be NULL
// if 'associations' not NULL, 'max_associations' specify 'associations' array size
int32_t rokid_log_associations(RokidLogAssociation* associations, uint32_t max_associations);

// generate current timestamp string
void rokid_log_timestamp(char* buf, uint32_t bufsize);

#ifdef __cplusplus
}
#endif // extern "C"

#ifndef ROKID_LOG_ENABLED
#define ROKID_LOG_ENABLED 2
#endif

#if ROKID_LOG_ENABLED <= 0
#define KLOGV(tag, fmt, ...) rokid_log_print(ROKID_LOGLEVEL_VERBOSE, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGV(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 1
#define KLOGD(tag, fmt, ...) rokid_log_print(ROKID_LOGLEVEL_DEBUG, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGD(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 2
#define KLOGI(tag, fmt, ...) rokid_log_print(ROKID_LOGLEVEL_INFO, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGI(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 3
#define KLOGW(tag, fmt, ...) rokid_log_print(ROKID_LOGLEVEL_WARNING, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGW(tag, fmt, ...)
#endif

#if ROKID_LOG_ENABLED <= 4
#define KLOGE(tag, fmt, ...) rokid_log_print(ROKID_LOGLEVEL_ERROR, tag, fmt, ##__VA_ARGS__)
#else
#define KLOGE(tag, fmt, ...)
#endif

#endif // ROKID_LOG_H
