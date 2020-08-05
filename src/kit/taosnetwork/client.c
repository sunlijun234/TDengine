/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <argp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 200

typedef struct {
  int   port;
  char *host;
} info_s;

typedef struct Arguments {
  char *   host;
  uint16_t port;
  uint16_t max_port;
} SArguments;

static struct argp_option options[] = {
    {0, 'h', "host", 0, "The host to connect to TDEngine. Default is localhost.", 0},
    {0, 'p', "port", 0, "The TCP or UDP port number to use for the connection. Default is 6041.", 1},
    {0, 'm', "max port", 0, "The max TCP or UDP port number to use for the connection. Default is 6050.", 2}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {

  SArguments *arguments = state->input;
  switch (key) {
    case 'h':
      arguments->host = arg;
      break;
    case 'p':
      arguments->port = atoi(arg);
      break;
    case 'm':
      arguments->max_port = atoi(arg);
      break;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, 0, 0};

int checkTcpPort(info_s *info) {
  int   port = info->port;
  char *host = info->host;
  int   clientSocket;

  struct sockaddr_in serverAddr;
  char               sendbuf[BUFFER_SIZE];
  char               recvbuf[BUFFER_SIZE];
  int                iDataNum;
  if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    return -1;
  }
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);

  serverAddr.sin_addr.s_addr = inet_addr(host);

  //printf("=================================\n");
  if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
    perror("connect");
    return -1;
  }
  //printf("Connect to: %s:%d...success\n", host, port);

  sprintf(sendbuf, "send port_%d", port);
  send(clientSocket, sendbuf, strlen(sendbuf), 0);
  //printf("Send msg_%d: %s\n", port, sendbuf);

  recvbuf[0] = '\0';
  iDataNum = recv(clientSocket, recvbuf, BUFFER_SIZE, 0);
  recvbuf[iDataNum] = '\0';
  //printf("Read ack msg_%d: %s\n", port, recvbuf);

  //printf("=================================\n");
  close(clientSocket);
  return 0;
}

void *checkUdpPort(info_s *info) {
  int   port = info->port;
  char *host = info->host;
  int   clientSocket;

  struct sockaddr_in serverAddr;
  char               sendbuf[BUFFER_SIZE];
  char               recvbuf[BUFFER_SIZE];
  int                iDataNum;
  if ((clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("socket");
    return -1;
  }
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(host);

  sprintf(sendbuf, "send msg port_%d by udp", port);

  socklen_t sin_size = sizeof(*(struct sockaddr *)&serverAddr);

  int code = sendto(clientSocket, sendbuf, strlen(sendbuf), 0, (struct sockaddr *)&serverAddr, (int)sin_size);
  if (code < 0) {
    perror("sendto");
    return -1;
  }

  //printf("Send msg_%d by udp: %s\n", port, sendbuf);

  recvbuf[0] = '\0';
  iDataNum = recvfrom(clientSocket, recvbuf, BUFFER_SIZE, 0, (struct sockaddr *)&serverAddr, &sin_size);
  recvbuf[iDataNum] = '\0';
  //printf("Read ack msg_%d from udp: %s\n", port, recvbuf);

  close(clientSocket);
  return 0;
}

int main(int argc, char *argv[]) {
  SArguments arguments = {"127.0.0.1", 6030, 6060};
  info_s  info;
  int ret;
  
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  printf("host: %s\tport: %d\tmax_port: %d\n\n", arguments.host, arguments.port, arguments.max_port);

  int   port = arguments.port;

  info.host = arguments.host;

  for (; port < arguments.max_port; port++) {
    printf("test: %s:%d\n", info.host, port);

    info.port = port;
    ret = checkTcpPort(&info);
    if (ret != 0) {
      printf("tcp port:%d test fail.", port);
    } else {
      printf("tcp port:%d test ok.", port);
    }
    
    checkUdpPort(&info);
    if (ret != 0) {
      printf("udp port:%d test fail.", port);
    } else {
      printf("udp port:%d test ok.", port);
    }
  }
}
