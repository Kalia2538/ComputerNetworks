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

#define QUEUE_LENGTH 10
#define RECV_BUFFER_SIZE 2048
struct socket_info
{
  // Server IP Address
  int ip_address;
  // TCP Port Number
  int tcp_portnum;
};


/* TODO: server()
 * Open socket and wait for client to connect
 * Print received message to stdout
 * Return 0 on success, non-zero on failure
*/
int server(char *server_port) {
    char buff[RECV_BUFFER_SIZE];
    int s, new_s, buff_len;
    socklen_t addr_len;
    struct sockaddr_in sin;
    sin.sin_family = PF_INET;
    sin.sin_port = server_port;
    sin.sin_addr.s_addr = INADDR_ANY; // FIGURE OUT WHAT THIS MEANS
    int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC); // DC PF_UNSPEC

    int success = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin) );

    listen(sockfd, QUEUE_LENGTH);
  while (1)
  {
    new_s = accept(s, (struct sockaddr *) &sin, &addr_len);
    while (buff_len = recv(new_s, buff, sizeof(buff), 0))
    {
      fputs(buff, stdout);
    }
    close(new_s);
  }
  

    return 0;
}

/*
 * main():
 * Parse command-line arguments and call server function
*/
int main(int argc, char **argv) {
  char *server_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server-c [server port]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  return server(server_port);
}
