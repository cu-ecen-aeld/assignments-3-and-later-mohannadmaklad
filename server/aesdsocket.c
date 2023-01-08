#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>

#ifndef CONNECTION_PORT 		/* allow CONNECTION_PORT to be defined by the user*/
	#define CONNECTION_PORT "9000" 	
#endif

#ifndef SOCK_BACKLOG			/* allow SOCK_BACKLOG to be defined by the user */
	#define SOCK_BACKLOG 1
#endif

#define MAX_RECV_DATA_SIZE 100
#define OUTFILE_PATH "/var/tmp/aesdsocketdata"

char buf[MAX_RECV_DATA_SIZE];

struct sockaddr_storage client_addr;

void signal_handler(int s){
	syslog(LOG_DEBUG, "Caught signal, exiting");
	closelog();
	exit(0);
}

int main(int argc, char** argv) {
	
	/* initialize syslog */
	openlog("AESDSOCKET", LOG_PID , LOG_USER);
	FILE* f = fopen(OUTFILE_PATH,"w+");
	fclose(f);
	bzero(buf,MAX_RECV_DATA_SIZE);
	signal(SIGINT, signal_handler);	
	signal(SIGTERM, signal_handler);	
	/* check if argument -d is passed */
		/* Fork */
	
	if(argc == 2) 
		if(strcmp(argv[1],"-d") == 0){
			if(fork() > 0) {
			syslog(LOG_DEBUG, "sub process created, main process is terminating...");
			exit(0);
		} 
	}	
/* open stream socket bound to port 9000 */
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1) exit(EXIT_FAILURE);
	
	/* getaddrinfo params */
	struct addrinfo hints, *res;
	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;

	if( getaddrinfo(NULL, CONNECTION_PORT, &hints, &res) == -1){
		syslog(LOG_DEBUG, "getaddrinfo failed");
		exit(EXIT_FAILURE);
	}
	
	/* socket options: enable address reuse */
	int reuse_flag = 1;
	int sockopt_ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_flag, sizeof(int));
	if(sockopt_ret == -1){
		syslog(LOG_DEBUG, "Failed to set socket options");
		exit(EXIT_FAILURE);
	}

	int bind_ret = bind(sockfd, res->ai_addr, res->ai_addrlen);
	if(bind_ret == -1) {
		int saved_errno = errno;
		close(sockfd);
		syslog(LOG_DEBUG, "Failed to bind socket to port %s", CONNECTION_PORT);
		syslog(LOG_DEBUG, "errno %d", saved_errno);
		exit(EXIT_FAILURE);
	}
	
	freeaddrinfo(res);
	
	/* listens for and accepts a connection */
	if(listen(sockfd, SOCK_BACKLOG) != 0){
		syslog(LOG_DEBUG, "listen failed");
		exit(EXIT_FAILURE);
	}
	
	while(1){
		socklen_t len = sizeof(client_addr);
		int accept_fd = accept(sockfd, (struct sockaddr*)&client_addr, &len);	
		if(accept_fd == -1) continue;
		
		char peer_name[INET6_ADDRSTRLEN];
		struct sockaddr* client_sockaddr = (struct sockaddr*) &client_addr;
		
		void* caddr;

		if (client_sockaddr->sa_family == AF_INET) {
			caddr = &(((struct sockaddr_in*)client_sockaddr)->sin_addr);
		} else if(client_sockaddr->sa_family == AF_INET6){
			caddr = &(((struct sockaddr_in6*)client_sockaddr)->sin6_addr);
		}

		inet_ntop(AF_INET, caddr, peer_name, sizeof(peer_name));
		syslog(LOG_DEBUG, "Accepted connection from %s", peer_name);
		
		FILE* outfile = fopen(OUTFILE_PATH,"a");		
		while(1){
		
			ssize_t received_bytes = recv(accept_fd, buf, MAX_RECV_DATA_SIZE-1, 0);
			
			if(received_bytes == -1){
				syslog(LOG_DEBUG, "Couldn't read from the socket");
				exit(EXIT_FAILURE);
			}

			fwrite(buf,1,received_bytes,outfile);
			
			//keep receiving data until a new line is detected
			if(buf[received_bytes-1] == '\n'){
				fclose(outfile);
				break;
			}
		}

		FILE* file = fopen("/var/tmp/aesdsocketdata","r");
		char local_buf[MAX_RECV_DATA_SIZE];
		char* c;
		while((c = fgets(local_buf,MAX_RECV_DATA_SIZE, file)) != NULL) {
			send(accept_fd, local_buf, strnlen(local_buf,MAX_RECV_DATA_SIZE), 0);
		}
		
		fclose(file);
		close(accept_fd);
		//close(sockfd);
		syslog(LOG_DEBUG, "Closed connection from %s\n", peer_name);
	}
	
	return 0;
}

