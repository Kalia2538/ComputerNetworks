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
  printf("made it to client function \n");
  struct sockaddr_in sin;
  int s;
  char buff[SEND_BUFFER_SIZE];
  sin.sin_family = PF_INET;
  int portnum = atoi(server_port);
  sin.sin_port = portnum;
  sin.sin_addr.s_addr = atoi(server_ip); // FIGURE OUT WHAT THIS MEANS
  // open socket
  int sockfd = socket(PF_INET, SOCK_STREAM, 0); // DC PF_UNSPEC
  printf("opened the socket \n");
  printf("error = -1\n");
  // char str1[20]; // Ensure it has enough space
  printf("value = ");
  printf("%d", sockfd);
  printf(" end\n");
  // printf(sockfd);
  // active open
  int con_estab = connect(sockfd, (struct sockaddr *) &sin, sizeof(sin) );
  printf("success = 0, fail = -1\n");
  // char str[20]; // Ensure it has enough space
  printf("%d", con_estab);
  // printf(con_estab);
  printf("\n");
  int x = 0;
  // printf(fgets(buff, sizeof(buff), stdin));
  while (fgets(buff, sizeof(buff), stdin)) {
    x++;
    buff[SEND_BUFFER_SIZE-1] = '\0';
    int len = strlen(buff) + 1;
    printf(buff);
    int val = send(s, buff, len, 0);
    printf("num of sent bytes: ");
    printf("%d", val);
    printf("\n");
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
  printf("server ip: ");
  printf(argv[1]);
  printf("\n");
  server_port = argv[2];
  printf("server port: ");
  printf(argv[2]);
  printf("\n");
  return client(server_ip, server_port);
}
