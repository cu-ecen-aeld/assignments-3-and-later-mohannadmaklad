#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>


void init_syslog(void);

int main(int argc, char** argv){
	init_syslog();

	if(argc != 3){
		syslog(LOG_ERR, "Invalid Arguments!");
		syslog(LOG_DEBUG, "Expected 2 arguments, found %d", argc);
		exit(1);
	}

	const char* path = (const char*) argv[1];
	const void* buffer = (const void*) argv[2];

	if(strlen(path) == 0){
		syslog(LOG_ERR, "Invalid Path!");
		exit(1);
	}

	syslog(LOG_DEBUG, "Writing %s to %s", (char*) buffer, (char*) path);

	int fd = open((const char*) argv[1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if(fd == -1){
		syslog(LOG_ERR, "Couldn't find nor create the file");
		exit(1);
	}

	syslog(LOG_DEBUG, "Writing %s to %s", path, (char *)buffer);

	write(fd, buffer, strlen(buffer));
	close(fd);

	closelog();
	return 0;
}

void init_syslog(void){
	openlog("MyWriter", LOG_PID , LOG_USER);
}
