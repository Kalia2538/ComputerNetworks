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
  struct sockaddr_in sin;
  int s;
  char buff[SEND_BUFFER_SIZE];
  sin.sin_family = PF_INET;
  sin.sin_port = server_port;
  sin.sin_addr.s_addr = INADDR_ANY; // FIGURE OUT WHAT THIS MEANS
  // open socket
  int sockfd = socket(PF_INET, SOCK_STREAM, PF_UNSPEC); // DC PF_UNSPEC
  // active open
  int con_estab = connect(sockfd, (struct sockaddr *) &sin, sizeof(sin) );

  while (fgets(buff, sizeof(buff), stdin)) {
    buff[SEND_BUFFER_SIZE-1] = '\0';
    int len = strlen(buff) + 1;
    send(s, buff, len, 0);
  }

  // loop - while we haven't reached an eof
    // take a chunk and send it to server
    // ??? handle partial sends
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
  return client(server_ip, server_port);
}
