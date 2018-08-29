#include <stdio.h>
#include <unistd.h>
#include "rlog.h"

#define TAG "rlog_demo"

int main(int argc, char** argv) {
	KLOGE(TAG, "hello world");
	KLOGW(TAG, "hello world");
	KLOGI(TAG, "hello world");
	KLOGD(TAG, "hello world");
	KLOGV(TAG, "hello world");

	TCPSocketArg logarg;
	logarg.host = "0.0.0.0";
	logarg.port = 7777;
	rokid_log_ctl(ROKID_LOG_CTL_DEFAULT_ENDPOINT, "tcp-socket", &logarg);

	printf("press enter key to send tcp socket log\n");
	getchar();

	int32_t i;
	for (i = 0; i < 100; ++i) {
		KLOGE(TAG, "hello world %d", i);
		KLOGW(TAG, "hello world %d", i);
		KLOGI(TAG, "hello world %d", i);
		KLOGD(TAG, "hello world %d", i);
		KLOGV(TAG, "hello world %d", i);
		sleep(1);
	}

	return 0;
}
