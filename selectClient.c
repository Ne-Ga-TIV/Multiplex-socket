#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define MAXSIZE 1024

int main()
{
    int sock_fd;

    char buff[MAXSIZE], name[25], ip[INET6_ADDRSTRLEN], port[10];
    struct addrinfo *result, hints, *tmp;

    printf("Please enter ip address and port: ");
    scanf("%s %s", ip, port);
    fflush(stdin);

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    if((getaddrinfo(ip, port, &hints,  &result)) != 0){
        printf("ERROR: getaddrinfo\n");
        exit(0);
    }

    for(tmp = result; tmp != NULL; tmp = tmp->ai_next){
        if((sock_fd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol)) < 0)
            continue;
        if(connect(sock_fd, tmp->ai_addr, tmp->ai_addrlen) < 0){
            close(sock_fd);
            continue;
        }
        break;
    }

    if(sock_fd < 0){
        printf("ERROR: socket\n");
        exit(0);
    }
    freeaddrinfo(result);

    fcntl(sock_fd, F_SETFL, O_NONBLOCK);
    
    fd_set read_fds, master;
    FD_ZERO(&read_fds);
    FD_ZERO(&master);
    FD_SET(STDIN_FILENO, &master);
    FD_SET(sock_fd, &master);

    int fdmax = STDIN_FILENO > sock_fd ? STDIN_FILENO : sock_fd;

    for(;;){
        read_fds = master;

        bzero(buff, MAXSIZE);
        
        if((select(fdmax+1, &read_fds, NULL, NULL, NULL)) < 0){
            printf("ERROR: select\n");
            exit(0);
        }

       
        if(FD_ISSET(sock_fd, &read_fds)){
            read(sock_fd, buff, sizeof buff);
            printf("%s\n", buff);
        }
    
        if(FD_ISSET(STDIN_FILENO, &read_fds)){
            
            int i = 0, ch;
            while ((ch = getchar()) != '\n')
                buff[i++] = ch;

            write(sock_fd, buff, strlen(buff));
        }

        
           
    }
    close(sock_fd);
}