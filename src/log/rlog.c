#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include "rlog.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif

typedef struct ListItem {
	struct ListItem* next;
	struct ListItem* prev;
} ListItem;

typedef struct Endpoint {
	struct Endpoint* next;
	struct Endpoint* prev;
	char* name;
	RokidLogWriter writer;
	void* arg;
} Endpoint;

typedef struct EndpointPtr {
	struct EndpointPtr* next;
	struct EndpointPtr* prev;
	Endpoint* endpoint;
	void* arg;
} EndpointPtr;

typedef struct Association {
	struct Association* next;
	struct Association* prev;
	char* tag;
	EndpointPtr* endpoints;
} Association;

typedef struct {
	Endpoint* endpoints;
	Association* associations;
	EndpointPtr default_association;
	pthread_mutex_t mutex;
} RokidLogInstance;

static RokidLogInstance log_instance_ = { NULL, NULL };

static void list_add(ListItem** items, ListItem* it) {
	it->prev = NULL;
	it->next = items[0];
	if (items[0])
		items[0]->prev = it;
	items[0] = it;
}

static void list_erase(ListItem** items, ListItem* it) {
	if (it->prev)
		it->prev->next = it->next;
	if (it->next)
		it->next->prev = it->prev;
	if (items[0] == it)
		items[0] = it->next;
	free(it);
}

static void add_endpoint(Endpoint** endpoints, const char* name,
		RokidLogWriter writer, void* arg) {
	size_t len = strlen(name);
	Endpoint* ep = (Endpoint*)malloc(sizeof(Endpoint) + len + 1);
	ep->name = (char*)(ep + 1);
	strcpy(ep->name, name);
	ep->writer = writer;
	ep->arg = arg;
	list_add((ListItem**)endpoints, (ListItem*)ep);
}

static Endpoint* find_endpoint_by_name(Endpoint** endpoints, const char* name) {
	Endpoint* ep = endpoints[0];
	while (ep) {
		if (strcmp(ep->name, name) == 0)
			return ep;
		ep = ep->next;
	}
	return NULL;
}

static void erase_endpoint_by_name(Endpoint** endpoints, const char* name) {
	Endpoint* ep = find_endpoint_by_name(endpoints, name);
	if (ep)
		list_erase((ListItem**)endpoints, (ListItem*)ep);
}

static void add_endpointptr(EndpointPtr** ptrs, Endpoint* ep, void* arg) {
	EndpointPtr* ptr = (EndpointPtr*)malloc(sizeof(EndpointPtr));
	ptr->endpoint = ep;
	ptr->arg = arg;
	list_add((ListItem**)ptrs, (ListItem*)ptr);
}

static EndpointPtr* find_endpointptr_by_name(EndpointPtr** ptrs, const char* name) {
	EndpointPtr* ptr = ptrs[0];
	while (ptr) {
		if (strcmp(ptr->endpoint->name, name) == 0)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

static void erase_endpointptr_by_name(EndpointPtr** ptrs, const char* name) {
	EndpointPtr* ptr = find_endpointptr_by_name(ptrs, name);
	if (ptr)
		list_erase((ListItem**)ptrs, (ListItem*)ptr);
}

static Association* add_association(Association** associations, const char* tag) {
	size_t len = strlen(tag);
	Association* as = (Association*)malloc(sizeof(Association) + len + 1);
	as->tag = (char*)(as + 1);
	strcpy(as->tag, tag);
	as->endpoints = NULL;
	list_add((ListItem**)associations, (ListItem*)as);
	return as;
}

static Association* find_association_by_tag(Association** associations, const char* tag) {
	Association* as = associations[0];
	while (as) {
		if (strcmp(as->tag, tag) == 0)
			return as;
		as = as->next;
	}
	return NULL;
}

static void erase_association_by_tag(Association** associations, const char* tag) {
	Association* as = find_association_by_tag(associations, tag);
	if (as)
		list_erase((ListItem**)associations, (ListItem*)as);
}

// ========= builtin log writer begin ============
typedef struct {
	char* buffer;
	uint32_t size;
} BuiltinLogBuffer;
static BuiltinLogBuffer log_buffer_ = { NULL, 0 };

static char loglevel2char(RokidLogLevel lv) {
	static char level_chars[] = {
		'V', 'D', 'I', 'W', 'E'
	};
	if (lv < 0 || lv >= ROKID_LOGLEVEL_NUMBER)
		return 'U';
	return level_chars[lv];
}

static int32_t std_log_writer(RokidLogLevel lv, const char* tag,
		const char* fmt, va_list ap, void* arg1, void* arg2) {
	char ts[32];
	rokid_log_timestamp(ts, sizeof(ts));
	int32_t r = -2;

	BuiltinLogBuffer* inst = (BuiltinLogBuffer*)arg1;
	int sz = snprintf(inst->buffer, inst->size, "%s/%c  [%s]  ",
			tag, loglevel2char(lv), ts);
	sz += vsnprintf(inst->buffer + sz, inst->size - sz, fmt, ap);
	if (sz < inst->size) {
		inst->buffer[sz] = '\n';
		++sz;
		int fd = (intptr_t)arg2;
		if (fd > 0)
			r = write(fd, inst->buffer, sz);
	}
	return r;
}

#ifdef __ANDROID__
static int to_android_loglevel(RokidLogLevel lv) {
	static int android_loglevel[] = {
		ANDROID_LOG_VERBOSE,
		ANDROID_LOG_DEBUG,
		ANDROID_LOG_INFO,
		ANDROID_LOG_WARN,
		ANDROID_LOG_ERROR
	};
	if (lv < 0 || lv >= ROKID_LOGLEVEL_NUMBER)
		return ANDROID_LOG_DEFAULT;
	return android_loglevel[lv];
}

static int32_t android_log_writer(RokidLogLevel lv, const char* tag,
		const char* fmt, va_list ap, void* arg1, void* arg2) {
	int prio = to_android_loglevel(lv);
	return __android_log_vprint(prio, tag, fmt, ap);
}
#endif

static void init_builtin_log_buffer() {
	uint32_t size = 1024 * 1024;
	void* p = mmap(NULL, size, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (p) {
		log_buffer_.buffer = (char*)p;
		log_buffer_.size = size;
	}
}


/**
static void file_log_writer(RokidLogLevel lv, const char* tag,
		const char* fmt, va_list ap, void* arg1, void* arg2) {
	FILE* fp = get_log_fp((char*)arg2);
	if (fp == NULL)
		return;
	char ts[32];
	rokid_log_timestamp(ts, sizeof(ts));
	fprintf(fp, "%s/%c  [%s]  ", tag, loglevel2char(lv), ts);
	vfprintf(fp, fmt, ap);
	fprintf("\n");
	fflush(fp);
}
*/
// ========= builtin log writer end ============

static void initialize() {
	if (log_instance_.endpoints)
		return;

	pthread_mutex_lock(&log_instance_.mutex);
	if (log_instance_.endpoints) {
		pthread_mutex_unlock(&log_instance_.mutex);
		return;
	}

	init_builtin_log_buffer();

	Endpoint* ep;
	add_endpoint(&log_instance_.endpoints, "stdout", std_log_writer, &log_buffer_);
	ep = log_instance_.endpoints;
	add_endpoint(&log_instance_.endpoints, "file", std_log_writer, &log_buffer_);
#ifdef __ANDROID__
	add_endpoint(&log_instance_.endpoints, "logcat", android_log_writer, NULL);
	// set default endpoint to logcat
	ep = log_instance_.endpoints;
#endif
	// set default endpoint to 'stdout'
	log_instance_.default_association.next = NULL;
	log_instance_.default_association.prev = NULL;
	log_instance_.default_association.endpoint = ep;
	log_instance_.default_association.arg = (void*)STDOUT_FILENO;
	pthread_mutex_unlock(&log_instance_.mutex);
}

void rokid_log_print(RokidLogLevel lv, const char* tag, const char* fmt, ...) {
	va_list ap;

	initialize();

	if (tag == NULL || fmt == NULL)
		return;

	pthread_mutex_lock(&log_instance_.mutex);
	Association* as = find_association_by_tag(&log_instance_.associations, tag);
	EndpointPtr* ptr;
	if (as == NULL)
		ptr = &log_instance_.default_association;
	else
		ptr = as->endpoints;
	while (ptr && ptr->endpoint) {
		va_start(ap, fmt);
		ptr->endpoint->writer(lv, tag, fmt, ap, ptr->endpoint->arg, ptr->arg);
		va_end(ap);
		ptr = ptr->next;
	}
	pthread_mutex_unlock(&log_instance_.mutex);
}

static int32_t ctl_add_endpoint(va_list ap) {
	const char* name = va_arg(ap, const char*);
	RokidLogWriter writer = va_arg(ap, RokidLogWriter);
	void* arg = va_arg(ap, void*);
	if (find_endpoint_by_name(&log_instance_.endpoints, name))
		return -1;
	add_endpoint(&log_instance_.endpoints, name, writer, arg);
	return 0;
}

static int32_t ctl_remove_endpoint(va_list ap) {
	const char* name = va_arg(ap, const char*);
	if (strcmp(log_instance_.default_association.endpoint->name,
				name) == 0) {
		log_instance_.default_association.endpoint = NULL;
		log_instance_.default_association.arg = NULL;
	}
	erase_endpoint_by_name(&log_instance_.endpoints, name);
	return 0;
}

static int32_t ctl_associate(va_list ap) {
	const char* tag = va_arg(ap, const char*);
	const char* epn = va_arg(ap, const char*);
	void* arg = va_arg(ap, void*);

	Endpoint* ep = find_endpoint_by_name(&log_instance_.endpoints, epn);
	if (ep == NULL)
		return -1;
	Association* ass = find_association_by_tag(&log_instance_.associations, tag);
	if (ass == NULL)
		ass = add_association(&log_instance_.associations, tag);
	if (!find_endpointptr_by_name(&ass->endpoints, epn))
		add_endpointptr(&ass->endpoints, ep, arg);
	return 0;
}

static int32_t ctl_disassociate(va_list ap) {
	const char* tag = va_arg(ap, const char*);
	const char* epn = va_arg(ap, const char*);

	Association* ass = find_association_by_tag(&log_instance_.associations, tag);
	if (ass) {
		if (epn) {
			erase_endpointptr_by_name(&ass->endpoints, epn);
			if (ass->endpoints == NULL)
				list_erase((ListItem**)(&log_instance_.associations), (ListItem*)ass);
		} else {
			// erase all associations for specified 'tag'
			EndpointPtr* epp = ass->endpoints;
			EndpointPtr* tmp;
			while (epp) {
				tmp = epp;
				epp = epp->next;
				free(tmp);
			}
			list_erase((ListItem**)(&log_instance_.associations), (ListItem*)ass);
		}
	}
	return 0;
}

static int32_t ctl_default_endpoint(va_list ap) {
	const char* epn = va_arg(ap, const char*);
	void* arg = va_arg(ap, void*);

	Endpoint* ep;
	ep = find_endpoint_by_name(&log_instance_.endpoints, epn);
	if (ep == NULL)
		return -1;
	log_instance_.default_association.endpoint = ep;
	if (strcmp(epn, "stdout") == 0) {
		log_instance_.default_association.arg = (void*)STDOUT_FILENO;
	} else {
		log_instance_.default_association.arg = arg;
	}
	return 0;
}

int32_t rokid_log_ctl(RokidLogControlOperation op, ...) {
	int32_t r = -1;
	va_list ap;

	initialize();

	va_start(ap, op);
	pthread_mutex_lock(&log_instance_.mutex);
	switch (op) {
		case ROKID_LOG_CTL_ADD_ENDPOINT:
			r = ctl_add_endpoint(ap);
			break;
		case ROKID_LOG_CTL_REMOVE_ENDPOINT:
			r = ctl_remove_endpoint(ap);
			break;
		case ROKID_LOG_CTL_ASSOCIATE:
			r = ctl_associate(ap);
			break;
		case ROKID_LOG_CTL_DISASSOCIATE:
			r = ctl_disassociate(ap);
			break;
		case ROKID_LOG_CTL_DEFAULT_ENDPOINT:
			r = ctl_default_endpoint(ap);
			break;
	}
	pthread_mutex_unlock(&log_instance_.mutex);
	va_end(ap);
	return r;
}

int32_t rokid_log_endpoints(RokidLogEndpoint* endpoints, uint32_t arr_size) {
	int32_t c = 0;
	Endpoint* ep;

	initialize();

	pthread_mutex_lock(&log_instance_.mutex);
	ep = log_instance_.endpoints;
	while (ep) {
		if (endpoints && arr_size > c) {
			endpoints[c].name = ep->name;
			endpoints[c].writer = ep->writer;
			endpoints[c].arg = ep->arg;
		}
		++c;
		ep = ep->next;
	}
	pthread_mutex_unlock(&log_instance_.mutex);

	return c;
}

int32_t rokid_log_associations(RokidLogAssociation* associations, uint32_t max_associations) {
	int32_t c = 0;
	Association* as;
	EndpointPtr* ptr;

	initialize();

	pthread_mutex_lock(&log_instance_.mutex);
	as = log_instance_.associations;
	while (as) {
		ptr = as->endpoints;
		while (ptr) {
			if (associations && max_associations > c) {
				associations[c].tag = as->tag;
				associations[c].endpoint = ptr->endpoint->name;
			}
			++c;
			ptr = ptr->next;
		}
		as = as->next;
	}
	pthread_mutex_unlock(&log_instance_.mutex);

	return c;
}

void rokid_log_timestamp(char* buf, uint32_t bufsize) {
	struct timeval tv;
	struct tm ltm;

	if (buf == NULL)
		return;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &ltm);
	snprintf(buf, bufsize, "%04d-%02d-%02d %02d:%02d:%02d.%ld",
			ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday,
			ltm.tm_hour, ltm.tm_min, ltm.tm_sec, tv.tv_usec);
}
