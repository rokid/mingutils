#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "rlog.h"

#define TAG "sock-timeout"

int main(int argc, char** argv) {
  TCPSocketArg logarg;
  logarg.host = "0.0.0.0";
  logarg.port = 33333;
  rokid_log_ctl(ROKID_LOG_CTL_DEFAULT_ENDPOINT, "tcp-socket", &logarg);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("socket create failed: %s\n", strerror(errno));
    return 1;
  }
  struct sockaddr_in addr;
  struct hostent* hp;
  hp = gethostbyname("localhost");
  if (hp == nullptr) {
    printf("dns failed for host localhost: %s\n", strerror(errno));
    ::close(fd);
    return 1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, hp->h_addr_list[0], sizeof(addr.sin_addr));
  addr.sin_port = htons(logarg.port);
  if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
    printf("socket connect failed: %s\n", strerror(errno));
    ::close(fd);
    return 1;
  }
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  ssize_t r;
  char buf[256];
  int32_t n = 0;
  while (true) {
    r = read(fd, buf, sizeof(buf) - 1);
    if (errno == EAGAIN) {
      printf("socket read timeout, try again\n");
      ++n;
      if (n % 3 == 0) {
        KLOGI(TAG, "hello world");
      }
      continue;
    }
    if (r < 0) {
      printf("socket read error: %d, %s\n", errno, strerror(errno));
      break;
    }
    if (r == 0) {
      printf("socket read eof\n");
      break;
    }
    buf[r] = '\0';
    printf("socket read: %s", buf);
    if (n % 2 == 0) {
      tv.tv_sec = 0;
      tv.tv_usec = 500000;
    } else {
      tv.tv_sec = 1;
      tv.tv_usec = 0;
    }
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  }
  ::close(fd);
  return 0;
}
