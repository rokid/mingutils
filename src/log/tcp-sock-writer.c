#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "common.h"

static int32_t init_socket(BuiltinTCPSocketInst* inst, const char* host, int32_t port) {
  int fd = socket(PF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;
  int v = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
  struct sockaddr_in addr;
  struct hostent* hp;
  hp = gethostbyname(host);
  if (hp == NULL) {
    printf("gethostbyname failed for host %s: %s\n", host, strerror(errno));
    close(fd);
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy(&addr.sin_addr, hp->h_addr_list[0], sizeof(addr.sin_addr));
  addr.sin_port = htons(port);
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    close(fd);
    printf("socket bind failed: %s\n", strerror(errno));
    return -1;
  }
  listen(fd, 10);
  inst->listen_fd = fd;
  return 0;
}

static int32_t new_connection(BuiltinTCPSocketInst* inst, int fd) {
  int32_t i;

  pthread_mutex_lock(&inst->write_mutex);
  for (i = 0; i < MAX_TCP_SOCKET_CONN; ++i) {
    if (inst->cli_sockets[i] < 0) {
      inst->cli_sockets[i] = fd;
      break;
    }
  }
  pthread_mutex_unlock(&inst->write_mutex);
  return i == MAX_TCP_SOCKET_CONN ? -1 : 0;
}

static void clear_connection(BuiltinTCPSocketInst* inst, int32_t idx) {
  int fd = inst->cli_sockets[idx];
  pthread_mutex_lock(&inst->write_mutex);
  inst->cli_sockets[idx] = -1;
  pthread_mutex_unlock(&inst->write_mutex);
  close(fd);
}

static void* listen_routine(void* arg) {
  BuiltinTCPSocketInst* inst = (BuiltinTCPSocketInst*)arg;
  fd_set all_fds;
  fd_set rfds;
  struct sockaddr_in addr;
  socklen_t addr_len;
  int new_fd;
  int max_fd;
  int sock;
  int32_t i;
  int listen_fd = inst->listen_fd;
  ssize_t r;
  char buf[256];

  FD_ZERO(&all_fds);
  FD_SET(listen_fd, &all_fds);
  max_fd = listen_fd + 1;
  while (1) {
    rfds = all_fds;
    if (select(max_fd, &rfds, NULL, NULL, NULL) < 0) {
      if (errno == EAGAIN) {
        sleep(1);
        continue;
      }
      printf("select failed: %s\n", strerror(errno));
      break;
    }
    // accept new connection
    if (FD_ISSET(listen_fd, &rfds)) {
      addr_len = sizeof(addr);
      new_fd = accept(listen_fd, (struct sockaddr*)&addr, &addr_len);
      if (new_fd < 0) {
        printf("accept failed: %s\n", strerror(errno));
        continue;
      }
      if (new_connection(inst, new_fd) < 0) {
        printf("too many connections, reject.\n");
        shutdown(new_fd, SHUT_RDWR);
        close(new_fd);
      } else {
        if (new_fd >= max_fd)
          max_fd = new_fd + 1;
        FD_SET(new_fd, &all_fds);
        printf("accept new connection %d\n", new_fd);
      }
    }
    // read
    for (i = 0; i < MAX_TCP_SOCKET_CONN; ++i) {
      sock = inst->cli_sockets[i];
      if (sock >= 0 && FD_ISSET(sock, &rfds)) {
        r = read(sock, buf, sizeof(buf));
        printf("read from client fd %d, ret = %ld\n", sock, r);
        if (r <= 0) {
          FD_CLR(sock, &all_fds);
          clear_connection(inst, i);
        }
      }
    }
  }
  return NULL;
}

int32_t start_tcp_listen(void* arg1, void* arg2) {
  if (arg1 == NULL || arg2 == NULL)
    return -1;

  BuiltinTCPSocketInst* inst = (BuiltinTCPSocketInst*)arg1;
  TCPSocketArg* tcparg = (TCPSocketArg*)arg2;

  pthread_mutex_lock(&inst->init_mutex);
  if (inst->listen_fd >= 0) {
    pthread_mutex_unlock(&inst->init_mutex);
    return 0;
  }
  if (init_socket(inst, tcparg->host, tcparg->port) < 0) {
    pthread_mutex_unlock(&inst->init_mutex);
    return -1;
  }
  pthread_mutex_unlock(&inst->init_mutex);

  pthread_t listen_thr;
  pthread_create(&listen_thr, NULL, listen_routine, inst);
  pthread_detach(listen_thr);
  return 0;
}

int32_t tcp_socket_log_writer(RokidLogLevel lv, const char* tag,
    const char* fmt, va_list ap, void* arg1, void* arg2) {
  if (arg1 == NULL)
    return -1;
  BuiltinTCPSocketInst* inst = (BuiltinTCPSocketInst*)arg1;
  int32_t i;
  pthread_mutex_lock(&inst->write_mutex);
  for (i = 0; i < MAX_TCP_SOCKET_CONN; ++i) {
    if (inst->cli_sockets[i] >= 0) {
      std_log_writer(lv, tag, fmt, ap, inst->log_buffer,
          (void*)(intptr_t)inst->cli_sockets[i]);
    }
  }
  pthread_mutex_unlock(&inst->write_mutex);
  return 0;
}
