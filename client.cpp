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


char localhost[] = "127.0.0.1";
int myPort;
#define bufferSize 1024
char ETX = (char) 3;
char username[bufferSize];
#define PASS 1
#define FAIL_NO_USER 2
#define FAIL_PASS_NO_MATCH 3

int createSocket() {
    // below are codes from the given guide
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET; // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    // get ready to connect
    status = getaddrinfo(localhost, "25785", &hints, &servinfo);
    int fd;
    fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    connect(fd, servinfo->ai_addr, servinfo->ai_addrlen);
    /*Retrieve the locally-bound name of the specified socket and store it in the
    sockaddr structure*/
    struct sockaddr_storage my_addr;
    socklen_t addrlen;
	addrlen = sizeof my_addr;
    int getsock_check=getsockname(fd, (struct sockaddr*)&my_addr,
    (socklen_t *)&addrlen);
    //Error checking
    if (getsock_check== -1) {
        perror("getsockname");
        exit(1);
    }
    struct sockaddr_in *sin = (struct sockaddr_in *)&my_addr;
    myPort = ntohs(sin->sin_port);
    return fd;
}
// return 1 if success, 0 if fail
int authenticate(int fd) {
    // get username and password
    cout << "Please enter the username: ";
    string s;
    getline(cin, s);
    strcpy(username, s.c_str());
    cout << "Please enter the password: ";
    char password[bufferSize];
    getline(cin, s);
    strcpy(password, s.c_str());
    char message[2 * bufferSize];;
    strcpy(message, username);
    strcat(message, &ETX);
    strcat(message, password);
    int len;
    len = strlen(message);

    send(fd, message, len, 0);
    cout << username << " sent an authentication request to the main server." << endl;
    char result[bufferSize];
    recv(fd, result, bufferSize, 0);
    int ret = int(result[0]);
    return ret;
}

int sendQuery(char* message, int fd) {
    int len;
    len = strlen(message);
    send(fd, message, len, 0);
    cout << username << " sent a request to the main server." << endl;
    char result[bufferSize];
    int nbytes = recv(fd, result, bufferSize, 0);
    result[nbytes] = '\0';
    cout << "The client received the response from the Main server using TCP over port ";
    cout << myPort << "." << endl;
    cout << result << endl << endl << endl;
    cout << "-----Start a new request-----" << endl;
}


int query(int fd) {
    char message[bufferSize];
    string s;
    cout << "Please enter the course code to query: ";
    getline(cin, s);
    strcpy(message, s.c_str());
    if (s.find(' ') == string::npos) {
        cout << "Please enter the category (Credit / Professor / Days / CourseName): ";
        getline(cin, s);
        strcat(message, &ETX);
        strcat(message, s.c_str());
    } else {
        cout << username << " sent a request with multiple CourseCode to the main server." << endl;
    }
    sendQuery(message, fd);
}

int main() {
    cout << "The client is up and running." << endl;
    int fd = createSocket();
    int c = 3;
    while (c > 0) {
        int ret = authenticate(fd);
        if (ret == PASS) {
        cout << username << " received the result of authentication using TCP over port ";
        cout << myPort << ". Authentication is successful" << endl;
        break;
        } 
        cout << username << " received the result of authentication using TCP over port ";
        cout << myPort << ". Authentication failed: ";
        if (ret == FAIL_NO_USER) {
            cout << "Username Does not exist" << endl << endl;
        } else {
            cout << "Password does not match" << endl << endl;
        }
        c = c - 1;
        cout << "Attempts remaining:" << c << endl;
    }
    if (c == 0) {
        cout << "Authentication Failed for 3 attempts. Client will shut down." << endl;
        return 0; // end the program
    }
    while (1) {
      query(fd);
    }
    return 0;
}