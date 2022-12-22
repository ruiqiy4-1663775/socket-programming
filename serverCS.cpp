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
#define MYPORT "22785"
#define DATA "cs.txt"
#define NAME "CS"

char ETX = (char) 3;


int getIndex(char *category) {
    if (strcmp(category, "Credit") == 0) {
        return 1;
    } else if (strcmp(category, "Professor") == 0) {
        return 2;
    } else if (strcmp(category, "Days") == 0) {
        return 3;
    } else if (strcmp(category, "CourseName") == 0) {
        return 4;
    }
}

int extraCredit(char *result, char *courseCode) {
	ifstream infile;
	infile.open(DATA);
    string s;
	while (getline(infile, s)) {
        char buffer[MAXBUFLEN];
        strcpy(buffer, s.c_str());
		char *delimiter = (char*)memchr(buffer, ',', strlen(buffer));
		char * p = delimiter + 1;
		*delimiter = '\0';
        if(strcmp(buffer, courseCode) == 0) {
            strcpy(result, s.c_str());
			cout << "The course information has been found: " << result << endl;
            return 0;
        }

	}
    strcpy(result, "Didn’t find the course: ");
    strcat(result, courseCode);
	cout << result << endl;
	return 0;
}

int getInfo(char *message, char *result) {
	char *courseCode = message;
	char *delimiter = (char*)memchr(message, ETX, strlen(message));
	if (delimiter == NULL) { // goes to extra credit part
		extraCredit(result, message);
		return 0;
	}
	char *category = delimiter + 1;
	*delimiter = '\0';
    cout << "The Server" << NAME << " received a request from the Main Server about the ";
    cout << category << " of " << courseCode << "." << endl;
    int index = getIndex(category);
	ifstream infile;
	infile.open(DATA);
    string s;
	while (getline(infile, s)) {
        char buffer[MAXBUFLEN];
        strcpy(buffer, s.c_str());
		delimiter = (char*)memchr(buffer, ',', strlen(buffer));
		char * p = delimiter + 1;
		*delimiter = '\0';
        if(strcmp(buffer, courseCode) == 0) {
            for(int i = 0; i < index - 1; i++) {
                delimiter = (char*)memchr(p, ',', strlen(p));
                p = delimiter + 1;
            }
            delimiter = (char*)memchr(p, ',', strlen(p));
            if (delimiter != NULL) {
                *delimiter = '\0';
            }
            strcpy(result, "The ");
            strcat(result, category);
            strcat(result, " of ");
            strcat(result, courseCode);
            strcat(result, " is ");
            strcat(result, p);
			cout << "The course information has been found: " << result << endl;
            return 0;
        }

	}
    strcpy(result, "Didn’t find the course: ");
    strcat(result, courseCode);
	cout << result << endl;
    return 0;
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
    cout << "The Server" << NAME << " is up and running using UDP on port " << MYPORT << "." << endl;
	return sockfd;
}


int receive(int sockfd) {
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
	addr_len = sizeof their_addr;
    int numbytes;
    char message[MAXBUFLEN];
	if ((numbytes = recvfrom(sockfd, message, MAXBUFLEN-1 , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
    message[numbytes] = '\0';
    char result[MAXBUFLEN];
	memset(result, 0, MAXBUFLEN);
	getInfo(message, result);
	if ((numbytes = sendto(sockfd, result, strlen(result), 0,
		(struct sockaddr *)&their_addr, addr_len)) == -1) {
		perror("talker: sendto");
		exit(1);
	}
	cout << "The Server" << NAME << " finished sending the response to the Main Server." << endl;
}


int main() {
	int sockfd = createSocket();
	while (1) {
		receive(sockfd);
	}

    return 0;
}