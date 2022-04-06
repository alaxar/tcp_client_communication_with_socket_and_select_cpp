#include <iostream>
#include <winsock2.h>
#include <sstream>
using namespace std;


typedef struct client_info {
	int socketfd;
	char *name;
	struct sockaddr_in client;	
} CLIENT;

void set_nonblockingMode(int socketfd) {
	u_long mode = 1;
	ioctlsocket(socketfd, FIONBIO, &mode);
}

int main(int argc, char *argv[]) {
	CLIENT clients_info[1024];
	struct sockaddr_in server, client;
	int socketfd, newSocketfd;
	WORD dllVersion = MAKEWORD(2, 2);
	WSADATA wsData;
	if(WSAStartup(dllVersion, &wsData) != 0) {
		perror("WSAStartup()");
		return -1;
	}
	
	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(2222);
	server.sin_family = AF_INET;
	memset(server.sin_zero, 0, sizeof(server.sin_zero));
	
	
	// create socket
	socketfd = socket(server.sin_family, SOCK_STREAM, 0);
	set_nonblockingMode(socketfd);
	if(socketfd < 0) {
		perror("Socket");
		return -1;
	}
	
	if(bind(socketfd, (struct sockaddr*)&server, sizeof(server)) < 0) {
		perror("bind");
		return -1;
	}
		
	if(listen(socketfd, 5) < 0) {
		perror("listen");
		return -1;
	}
		

	// setting select
	fd_set rfds, copy_rfds, wfds, copy_wfds;
	FD_ZERO(&rfds);
	FD_SET(socketfd, &rfds);

	// timeout
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int ret, s, clientCount = 0;
	s = sizeof(client);
	
	int fd_max = socketfd;
	char buffer[1024];
	while(1) {
		copy_rfds = rfds;
		copy_wfds = wfds;
		ret = select(fd_max, &copy_rfds, &copy_wfds, NULL, NULL);
		
		for(int i = 0; i <= fd_max; i++) {
			if(FD_ISSET(i, &rfds)) {
				if(i == socketfd) {
					// its the server
					newSocketfd = accept(i, (struct sockaddr*)&client, &s);
					set_nonblockingMode(newSocketfd);
					
					// tell the server that a client is connected
					if(newSocketfd > 0) {
						// add client info to the structure.
						clients_info[clientCount].client = client;
						clients_info[clientCount].socketfd = newSocketfd;
						clients_info[clientCount].name = "client";
						cout << "New Client Connected - IP Address: " << inet_ntoa(clients_info[clientCount].client.sin_addr) << " On Port: " << clients_info[clientCount].client.sin_port << " sock: " << clients_info[clientCount].socketfd << endl;
						clientCount++;
					}
					
					
					if(newSocketfd > fd_max)
						fd_max = newSocketfd;
					
					FD_SET(newSocketfd, &rfds);
					
				} else {
					// new client is comming
					int x = recv(i, buffer, 1024, 0);
					string owner;
					if(x > 2) {
						for(int clientInfo = 0; clientInfo <= fd_max; clientInfo++) {				
							if(clientInfo < clientCount) {
								if(clients_info[clientInfo].socketfd == i) {
									ostringstream ss;
									ss << clients_info[clientInfo].name << "@" << inet_ntoa(clients_info[clientInfo].client.sin_addr) << ":" << clients_info[clientInfo].client.sin_port << ":~ " << buffer << endl << "\r";									
									owner = ss.str();
									cout << owner;
								}
							}
							
							// determine to whom the message must be delivered.
							if(strcmp(buffer, "client1") == 0)
							{
								int b;
								char bv[100];
								while(b = recv(i, bv, 1024, 0) > 0);
								cout << "value: " << b << endl;
								cout << "new " << bv << endl;
							}
//							if(i != clientInfo && FD_ISSET(clientInfo, &rfds)) {
//								send(clientInfo, owner.c_str(), owner.size(), 0);
//							}
						}
					} else if(x == 0) {
						// means the client is disconnected.
						ostringstream disconnected;
						for(int left = 0; left <= fd_max; left++) {
							if(FD_ISSET(left, &rfds)) {
								disconnected << clients_info[left].name << "@" << inet_ntoa(clients_info[left].client.sin_addr) << ":" << clients_info[left].client.sin_port << " is disconnected from the server." << endl << "\r";
								string dis = disconnected.str();
								send(left, dis.c_str(), dis.size(), 0);
							}
							FD_CLR(i, &rfds);
						}
					}
					memset(buffer, 0, 1024);
				}
			}
		}
	}
	return 0;
}
