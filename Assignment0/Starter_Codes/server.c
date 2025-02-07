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
  // printf("in the server function\n");
  char buff[RECV_BUFFER_SIZE] = "";
  int s, new_s, buff_len;
  socklen_t addr_len;
  // struct addrinfo hints;
  struct sockaddr_in sin;
  // struct getaddr
  sin.sin_family = PF_INET;
  int portnum = atoi(server_port);
  // printf("after atoi - port \n");
  sin.sin_port = portnum;
  // printf("after sin_port \n");
  sin.sin_addr.s_addr = atoi("127.9.0.1"); // FIGURE OUT WHAT THIS MEANS
  // printf("after ip -- s_addr \n");
  int sockfd = socket(AF_INET, SOCK_STREAM, 0); // DC PF_UNSPEC
  // printf("opened the socket \n");
  // printf("error = -1\n");
  // char str1[20]; // Ensure it has enough space
  // printf("value = ");
  // printf("%d", sockfd);
  // printf(" end\n");

  int success = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin) );
  // printf("bound the socket \n");
  // printf("error = -1\n");
  // char str1[20]; // Ensure it has enough space
  // printf("value = ");
  // printf("%d", success);
  // printf(" end\n");
  success = listen(sockfd, QUEUE_LENGTH);
  // printf("successful listen \n");
  // printf("error = -1\n");
  char str1[20]; // Ensure it has enough space
  // printf("value = ");
  // printf("%d", success);
  // printf(" end\n");
  while (1)
  {
    new_s = accept(sockfd, (struct sockaddr *) &sin, &addr_len);
    // printf("accepted a client\n");
    // printf("%d", new_s);
    // while (buff_len = recv(new_s, buff, sizeof(buff), 0))
    // {
    //   printf("%s", buff);
    //   // fputs(buff, stdout);
    // }
    buff_len = recv(new_s, buff, sizeof(buff), 0);
    // printf("\n new first buff_len ");
    // printf("%d\n", buff_len);
    // printf("this is buff: ");
    // printf("%s", buff);
    int abc = 1;
    while (abc)
    {
      // fflush(stdout);
      // fputs(buff, stdout);
      printf("%s", buff);
      fflush(stdout);

      // fputs(buff, stdout);
      abc = recv(new_s, buff, sizeof(buff), 0);
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
  // printf("start of main\n");
  // printf("%d", argc);
  char *server_port;

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server-c [server port]\n");
    exit(EXIT_FAILURE);
  }

  server_port = argv[1];
  // printf("this is the server port num \n");
  // printf(server_port);
  // printf("\n");
  return server(server_port);
}