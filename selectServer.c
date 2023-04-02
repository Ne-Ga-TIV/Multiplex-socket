/*
** selectserver.c -- a cheezy multiperson chat server
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

#define PORTCHAT "3222"
#define PORTFILE "8888"     // port we're listening on
#define MAXCLIENTS 20

struct client{
    char name[256], last_msg[1024];
    int sock_fd;
    
}clients[MAXCLIENTS];

// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
char* create_msg(char *name, struct tm time, char* msg, char *task){
    
    char* buf = malloc(2000*sizeof(char));
    bzero(buf, sizeof(buf));
    char time_str[10];
    sprintf(time_str, "%02d:%02d", time.tm_hour, time.tm_min);
    strcat(buf, name);
    strcat(buf, "@");
    strcat(buf, time_str);
    strcat(buf, "@");
    strcat(buf, msg);
    strcat(buf, "@");
    strcat(buf, task);
    return buf;
}
void delete_client(int index)
{
    for(int i = index; i < MAXCLIENTS - 1; i++)
        clients[i] = clients[i+1];
    
}
int name_cheack(char *name){
    
    for(int i = 0; i < MAXCLIENTS; i++)
        if(!strcmp(name, clients[i].name)){ 
            return 1;
        }

    for(int i = 0; name[i] != '\0'; i++)
        if(!((name[i] >= 65 && name[i] <= 90) 
        || (name[i] >= 97 && name[i] <= 122) 
        || (name[i] >= 48 && name[i] <= 57))){
            return 1;
        }

    return 0;

}
int main(void)
{
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax, clients_count = 0, listener, newfd, nbytes, i, rv, fileServer;
    struct sockaddr_storage remoteaddr; 
    socklen_t addrlen;
    char buf[1024]; // buffer for client data
	char remoteIP[INET6_ADDRSTRLEN];
	struct addrinfo hints, *ai, *p;

    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORTCHAT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    	listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		break;
	}

	// if we got here, it means we didn't get bound
	if (p == NULL) {
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this
    
    if ((rv = getaddrinfo(NULL, PORTFILE, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    
    fileServer = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fileServer < 0) { 
			continue;
		}
		
		if (connect(fileServer, p->ai_addr, p->ai_addrlen) < 0) {
			close(fileServer);
			continue;
		}
        else printf("Connect to files server\n");
		break;
	}

    freeaddrinfo(ai);

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }

    FD_SET(listener, &master);
    FD_SET(fileServer, &master);
    // keep track of the biggest file descriptor
    fdmax = listener > fileServer ? listener : fileServer; // so far, it's this one

    // main loop
    for(;;) {
        
        
        read_fds = master; // copy it
        
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read

        for(i = 0; i <= fdmax; i++) {
            
            if (FD_ISSET(i, &read_fds)) { // we got one!!

                if (i == listener) {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
					if((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1){
                        perror("accept");
                        exit(0);
                    } else {
                        clients[clients_count].sock_fd = newfd;
                        bzero(clients[clients_count++].name, 256);
                        
                        
                        if(write(newfd, "ATSIUSKVARDA\n", 13) < 0){
                            perror("write:");
                        }
            
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                       
                       
                        printf("selectserver: new connection from %s on "
                            "socket %d\n",
							inet_ntop(remoteaddr.ss_family,
								get_in_addr((struct sockaddr*)&remoteaddr),
								remoteIP, INET6_ADDRSTRLEN),
							newfd);
                    }
                } else if (i == fileServer){

                    if ((nbytes = read(i, buf, sizeof(buf))) <= 0)
                        perror("recv");

                    char tmp_name[40];
                    int i = 0;
                    for(; i < nbytes && buf[i] != '@'; i++)
                        tmp_name[i] = buf[i];
                   
                    tmp_name[i] = '\0';

                    printf("CLIENT NAME = %s\n", tmp_name);
                    for(int q = 0; q < clients_count; q++)
                        if(!strcmp(tmp_name, clients[q].name)){
                            write(clients[q].sock_fd, buf+i+1, sizeof(buf) - i - 1);
                        }                    
                      
                }
                else {
                    
                    int j = 0;
                    
                    for(; clients[j].sock_fd != i ; j++)
                        ;
                        
                    bzero(buf, 1024);

                    if ((nbytes = read(i, buf, sizeof(buf))) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                            clients_count--;
                            delete_client(j);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } else {


                        for(int i = 0; i < strlen(buf); i++){
                            if(buf[i] < 32)
                                buf[i] = '\0';
                        }

                        if(clients[j].name[0] == 0){
                            if(name_cheack(buf)){
                                write(clients[j].sock_fd, "Try again\n", 11);;
                            
                            }
                            else{
                                strcpy(clients[j].name, buf);
                                write(clients[j].sock_fd, "VARDASOK\n", 9);
                            }
                            continue;
                        }
                        if(!strcmp("@FS SAVE", buf) || !strcmp("@FS LOAD", buf)){

                            time_t t = time(NULL);
                            struct tm tm = *localtime(&t);
                            char *msg = create_msg(clients[j].name, tm, clients[j].last_msg, buf); 
                            printf("MSG = %s\n", msg);
                            write(fileServer, msg,  strlen(msg));
                            
                        }
                        else
                        {

                            strcpy(clients[j].last_msg, buf);       
                            strcpy(buf, "PRANESIMAS");
                            strcat(buf, clients[j].name);
                            strcat(buf, ": ");
                            strcat(buf, clients[j].last_msg);

                            buf[strlen(buf)] = '\n';

                            for(int k = 0; k < clients_count; k++) {
                                // send to everyone!
                                if (FD_ISSET(clients[k].sock_fd, &master)) {
                                    // except the listener and ourselves
                                    if (clients[k].sock_fd != listener && clients[k].name[0] != 0){
                                        if (send(clients[k].sock_fd , buf, strlen(buf), 0) == -1) {
                                            perror("send");
                                        }
                                        
                                    }
                                }
                            }
                        }  

                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}
