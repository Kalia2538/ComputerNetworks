#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>

#define SEND_BUFFER_SIZE 2048


/* TODO: client()
 * Open socket and send message from stdin.
 * Return 0 on success, non-zero on failure
*/
int client(char *server_ip, char *server_port) {

  int s;
  char buff[SEND_BUFFER_SIZE];

  int sockfd, numbytes;  
  char buf[SEND_BUFFER_SIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(server_ip, server_port, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
        perror("client: connect");
          continue;
    }

    break;

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
  }

  int x;
  while ((x = fread(buff, 1, SEND_BUFFER_SIZE, stdin)) > 0) {
    int val = send(sockfd, buff, x, 0);
    if(val < 1) {
      perror("simplex-talk: send");
    }  
  }

  close(sockfd);
  return 0;
}

/*
 * main()
 * Parse command-line arguments and call client function
*/
int main(int argc, char **argv) {
  char *server_ip;
  char *server_port;

  if (argc != 3) {
    fprintf(stderr, "Usage: ./client-c [server IP] [server port] < [message]\n");
    exit(EXIT_FAILURE);
  }

  server_ip = argv[1];  
  server_port = argv[2];
  fflush(stdout);

  return client(server_ip, server_port);
}