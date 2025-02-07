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
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  // hints.ai_flags = AI_PASSIVE;

  int rv;
  if ((rv = getaddrinfo("127.0.0.1", server_port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }
  // socklen_t sin_size;
  // // printf("in the server function\n");
  char buff[RECV_BUFFER_SIZE];
  // int s, new_s, buff_len;
  // socklen_t addr_len;
  // // struct addrinfo hints;
  // struct sockaddr_in sin;
  // // struct getaddr
  // sin.sin_family = AF_INET;
  // int portnum = atoi(server_port);
  // // printf("after atoi - port \n");
  // sin.sin_port = portnum;
  // // printf("after sin_port \n");
  // sin.sin_addr.s_addr = atoi("127.0.0.1"); // FIGURE OUT WHAT THIS MEANS
  int sockfd;
  // printf("after ip -- s_addr \n");
  int yes = 1;
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
        perror("server: socket");
        continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd);
        perror("server: bind");
        continue;
    }

    break;
  }

  if (listen(sockfd, QUEUE_LENGTH) == -1) {
    perror("listen");
    exit(1);
  }
  socklen_t sin_size;
  int new_fd;
  while(1) {  // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        continue;
    }
    int abc;
    // printf("\n new first buff_len ");
    // printf("%d\n", buff_len);
    // printf("this is buff: ");
    // printf("%s", buff);
    while ((abc = recv(new_fd, buff, RECV_BUFFER_SIZE, 0)) > 0)
    {
      // fflush(stdout);
      // fputs(buff, stdout);
      fwrite(buff, 1, abc, stdout);
      // printf("%s", buff);
      fflush(stdout);

      // fputs(buff, stdout);
    }

    close(new_fd);  // parent doesn't need this
  }
  // printf("opened the socket \n");
  // printf("error = -1\n");
  // char str1[20]; // Ensure it has enough space
  // printf("value = ");
  // printf("%d", sockfd);
  // printf(" end\n");

  //int success = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin) );
  // printf("bound the socket \n");
  // printf("error = -1\n");
  // char str1[20]; // Ensure it has enough space
  // printf("value = ");
  // printf("%d", success);
  // printf(" end\n");
  //success = listen(sockfd, QUEUE_LENGTH);
  // printf("successful listen \n");
  // printf("error = -1\n");
  //char str1[20]; // Ensure it has enough space
  // printf("value = ");
  // printf("%d", success);
  // printf(" end\n");
  
  

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