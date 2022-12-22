#include <iostream>
#include <cstring>

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

#define bufferSize 1024
char ETX = (char) 3;
#define localhost "127.0.0.1"
#define MY_PORT_TCP "25785"
#define MY_PORT_UDP "24785"
#define PORT_C "21785"
#define PORT_CS "22785"
#define PORT_EE "23785"
#define BACKLOG 10
#define PASS 1
#define FAIL_NO_USER 2
#define FAIL_PASS_NO_MATCH 3
#define CLIENT_QUIT 20
char username[bufferSize];

void encription(char *s) {
    for (int i = 0; i < strlen(s); i++) {
        if ((48 <= (int)s[i] && (int)s[i] <= 57)) {
            if (s[i] < 54) {
                s[i] += 4;
            } else {
                s[i] -= 6;
            }
        } else if(isalpha(s[i])) {
            if (s[i] >= 'a' && s[i] <= 'v') {
                s[i] += 4;
            } else if(s[i] >= 'w' && s[i] <= 'z') {
                s[i] -= 22;
            } else if (s[i] >= 'A' && s[i] <= 'V') {
                s[i] += 4;
            } else if(s[i] >= 'W' && s[i] <= 'Z') {
                s[i] -= 22;
            }
        }
    }
}

int createUdpSocket() {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(NULL, MY_PORT_UDP, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
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
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    return sockfd;
}

// create tcp socket and listen and return the child socket
int createTcpSocket() {
    struct addrinfo hints, *servinfo, *p;
    int sockfd;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use IPv4 or IPv6, whichever
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me
    int rv;
    if ((rv = getaddrinfo(NULL, MY_PORT_TCP, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // make a socket:
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    // struct sockaddr_storage their_addr;
    // socklen_t addr_size;
    // addr_size = sizeof their_addr;
    
    return sockfd;
}

int cred(char *message, int fd_udp, char *buf) {
    struct addrinfo hints, *servinfo;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(localhost, PORT_C, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    int numbytes;
    if ((numbytes = sendto(fd_udp, message, strlen(message), 0,
    servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
    perror("talker: sendto");
    exit(1);
    }
    cout << "The main server sent an authentication request to serverC." << endl;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
	addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(fd_udp, buf, bufferSize , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
    cout << "The main server received the result of the authentication request ";
    cout << "from ServerC using UDP over port " << MY_PORT_UDP << "." << endl;

    return int(buf[0]);
}

void printhelper(char *message) {
    strcpy(username, message);
    char *delimiter = (char*)memchr(username, ETX, strlen(message));
    *delimiter = '\0';
    cout << "The main server received the authentication for " << username;
    cout << " using TCP over port " << MY_PORT_TCP << "." << endl;
}

int authentication(int fd_tcp, int fd_udp) {
    char message[bufferSize];
    int ret = recv(fd_tcp, message, bufferSize, 0);
    if (ret == 0) {
        cout << "client has closed connection" << endl;
        return CLIENT_QUIT;
    }
    message[ret] = '\0';
    printhelper(message);
    encription(message);
    char result[bufferSize];
    cred(message, fd_udp, result);
    send(fd_tcp, result, strlen(result), 0);
    cout << "The main server sent the authentication result to the client." << endl;
    return int(result[0]);
}

int sendquery(char *message, int fd_udp, char *buf, const char *port) {
    struct addrinfo hints, *servinfo;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    getaddrinfo(localhost, port, &hints, &servinfo);
    int numbytes;
    if ((numbytes = sendto(fd_udp, message, strlen(message), 0,
    servinfo->ai_addr, servinfo->ai_addrlen)) == -1) {
    perror("talker: sendto");
    exit(1);
    }
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
	addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(fd_udp, buf, bufferSize , 0,
            (struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}
    buf[numbytes] = '\0';
    return 0;
}

void printhelper2(char *message) {
    char t[bufferSize];
    strcpy(t, message);
    char *courseCode = t;
	char *delimiter = (char*)memchr(t, ETX, strlen(t));
	char *category = delimiter + 1;
	*delimiter = '\0';
    cout << "The main server received from " << username << " to query course ";
    cout << courseCode << " about " << category << " using TCP over port ";
    cout << MY_PORT_TCP << "." << endl;

}

int extraCredit(string s, int fd_udp, char *result) {

    int flag = 1;
    int f1 = 0;
    int f2 = 0;
    strcpy(result, "CourseCode: Credits, Professor, Days, Course Name\n");
    cout << s << endl;
    while (flag) {
        int index = s.find(' ');
        string token;
        if (index != string::npos) {
            token = s.substr(0, index); 
            s.erase(0, index + 1);
        } else {
            token = s;
            flag = 0;
        }
        char message[bufferSize];
        strcpy(message, token.c_str());
        char department[bufferSize];
        strncpy(department, message, 2);
        department[2] = '\0';
        int flag = 0;
        char buf[bufferSize];
        if (!strcmp(department, "CS")) {
            cout << "The main server sent a request to serverCS." << endl;
            sendquery(message, fd_udp, buf, PORT_CS);
            cout << "The main server received the response from serverCS using UDP";
            cout << " over port " << MY_PORT_UDP << "." << endl;
        } else {
            cout << "The main server sent a request to serverEE." << endl;
            sendquery(message, fd_udp, buf, PORT_EE);
            cout << "The main server received the response from serverEE using UDP";
            cout << " over port " << MY_PORT_UDP << "." << endl;
        }
        strcat(result, buf);
        strcat(result, "\n");
    }
    return 0;
}

int query(int fd_tcp, int fd_udp) {
    char message[bufferSize];
    int nbytes = recv(fd_tcp, message, bufferSize, 0);
    if (nbytes == 0) {
        cout << "client has closed connection" << endl;
        return CLIENT_QUIT;
    }
    message[nbytes] = '\0';
    char result[2 * bufferSize];
    string msg = message;
    if (msg.find(' ') != string::npos) {
        extraCredit(msg, fd_udp, result);
        send(fd_tcp, result, strlen(result), 0);
        return 0;  // go to the extra credit part
    }
    printhelper2(message);
    char department[bufferSize];
    strncpy(department, message, 2);
    department[2] = '\0';
    int flag = 0;
    if (!strcmp(department, "CS")) {
        cout << "The main server sent a request to serverCS." << endl;
        sendquery(message, fd_udp, result, PORT_CS);
        cout << "The main server received the response from serverCS using UDP";
        cout << " over port " << MY_PORT_UDP << "." << endl;
    } else {
        cout << "The main server sent a request to serverEE." << endl;
        sendquery(message, fd_udp, result, PORT_EE);
        cout << "The main server received the response from serverEE using UDP";
        cout << " over port " << MY_PORT_UDP << "." << endl;
    }
    send(fd_tcp, result, strlen(result), 0);
    cout << "The main server sent the query information to the client." << endl;
}

int main() {
    cout << "The main server is up and running." << endl;

    int fd_tcp_parent = createTcpSocket();
    int fd_udp = createUdpSocket();
    int fd_tcp;
    while(1) {
        int flag = 0;
        fd_tcp = accept(fd_tcp_parent, NULL, NULL);
        while(1) {
            int ret = authentication(fd_tcp, fd_udp);
            if (ret == PASS) {
                flag = 1;
                break;
            } else if (ret == CLIENT_QUIT) {
                close(fd_tcp);
                break;
            }
        }
        while (flag) {
            int ret = query(fd_tcp, fd_udp);
            if (ret == CLIENT_QUIT) {
                close(fd_tcp);
                break;
            }
        }
    }
    return 0;
}