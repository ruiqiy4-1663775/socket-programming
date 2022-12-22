#include <iostream>
#include <cstring>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

using namespace std;

#define MAXBUFLEN 1024
#define MYPORT "21785"
char ETX = (char) 3;
#define PASS 1
#define FAIL_NO_USER 2
#define FAIL_PASS_NO_MATCH 3


int validation(char *message) {
	char *username = message;
	char *delimiter = (char*)memchr(message, ETX, strlen(message));
	char *password = delimiter + 1;
	*delimiter = '\0';
	ifstream infile;
	infile.open("cred.txt");
	char buffer[2 * MAXBUFLEN];
	while (infile >> buffer) {
		delimiter = (char*)memchr(buffer, ',', strlen(buffer));
		char * p = delimiter + 1;
		*delimiter = '\0';
		if (strcmp(username, buffer) == 0) {
			if (strcmp(password, p) == 0) {
				return PASS;
			} else {
				return FAIL_PASS_NO_MATCH;
			}
		}
	}
	return FAIL_NO_USER;
}

int createSocket() {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	cout << "The ServerC is up and running using UDP on port " << MYPORT << "." << endl;
	return sockfd;
    //printf("listener: waiting to recvfrom...\n");
}

// fill the buffer
int receive(int sockfd, char *message) {
	//printf("listener: waiting to recvfrom...\n");
	char s[INET6_ADDRSTRLEN];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
	addr_len = sizeof their_addr;
    int numbytes;
	if ((numbytes = recvfrom(sockfd, message, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
   // printf("listener: got packet from %s\n",
		//i//net_ntop(their_addr.ss_family,
			//get_in_addr((struct sockaddr *)&their_addr),
			//s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	message[numbytes] = '\0';
	cout << "The ServerC received an authentication request from the Main Server." << endl;
	int result = validation(message);
	char re[1];
	re[0] = char(result);
	if ((numbytes = sendto(sockfd, re, strlen(re), 0,
		(struct sockaddr *)&their_addr, addr_len)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
	cout << "The ServerC finished sending the response to the Main Server." << endl;
}


int main() {
	int sockfd = createSocket();
	char message[2 * MAXBUFLEN];
	while (1) {
		receive(sockfd, message);
	}

    return 0;
}


