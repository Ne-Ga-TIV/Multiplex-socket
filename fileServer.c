#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "8888"
struct clientData
{
	char name[50];
	char msg[974];
	char time[20];
	char task[20];
};
struct clientData* get_client_data(char *buf){
	struct clientData *data = malloc(sizeof(struct clientData));
	bzero(data->name, sizeof data->name);
	bzero(data->msg, sizeof data->msg);
	bzero(data->time, sizeof data->time);
	bzero(data->task, sizeof data->task);

	int len = strlen(buf);
	int size;
	for(int i = 0, j = 0; i <= len; i++){
		if(buf[i] == '@'){
			j++;
			size = i;
			continue;
		} 
		if(j == 0) data->name[i] = buf[i];
		if(j == 1) data->time[i - size - 1] = buf[i];
		if(j == 2) data->msg[i - size - 1] = buf[i];
		if(j >= 3) data->task[i - size - 1] = buf[i - 1];
	}

	return data;
}



int main(int argc, char **argv){

    FILE *file = NULL;
    int listener, rv, charServer;
    socklen_t addrlen;
    struct sockaddr_storage remoteaddr; 
    char buf[20000]; // buffer for client data
	struct addrinfo hints, *ai, *p;


	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "fileserver: %s\n", gai_strerror(rv));
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
		fprintf(stderr, "fileserver: failed to bind\n");
		exit(2);
	}

	freeaddrinfo(ai); // all done with this

    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    else printf("FS listening\n");
	addrlen = sizeof(remoteaddr);
    if((charServer = accept(listener, (struct sockaddr*)&remoteaddr , &addrlen)) == -1){
        perror("accept");
        exit(4);
    }else printf("chat server connect\n");

    while (1)
    {
		bzero(buf, sizeof buf);

		if(read(charServer, buf, sizeof buf) <= 0){
			perror("read");
			exit(5);
		}
		pid_t pid = fork();
		
		if(pid < 0){
			perror("fork");
			exit(6);
		}else if(pid == 0){
			FILE *file;
			char file_name[50] = {0};
			struct clientData *data = get_client_data(buf);
			strcat(file_name, "Data/");
			strcat(file_name, data->name);
			strcat(file_name, ".txt");
			printf("TASK = %s\nNAME = %s\nTIME = %s\nMSG = %s\n", data->task, data->name, data->time, data->msg);
			
			if(!strcmp(data->task, "@FS LOAD")){
				file = fopen(file_name, "r");
				char msgs[100000];
				strcat(msgs, data->name);
				strcat(msgs, "@");
				int i = strlen(data->name) + 1;
				char ch;
				while((ch = getc(file)) != EOF){
					msgs[i++] = ch;
				}
				
				msgs[i] = '\0';
				printf("FILE\n%s", msgs);
				write(charServer, msgs, strlen(msgs));
				free(data);
				data = NULL;
				fclose(file);

			}else if (!strcmp(data->task, "@FS SAVE")){
				file = fopen(file_name, "a");
				fprintf(file, "%s %s\n", data->time, data->msg);
				fclose(file);

			}
			
		}

    }
    
    close(listener);

}