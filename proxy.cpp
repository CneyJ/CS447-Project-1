// Courtney Ramatowski
// 800381650
// CS447-003
// Project 1: Proxy
// March 4, 2024

//-----------DESCRIPTION
// Proxy, sits between basic webserver and client. Checks for "hazardous material" (but not really because it is undefined).

//-----------OVERRIDE
// A lot of the calls used are depreciated, this lets me run/compile without visual studio getting in the way.
#define _CRT_SECURE_NO_WARNINGS  // avoid C4996 warnings (needed for Visual Studio 2022)
#pragma warning (disable : 4996) // Supress any compiler errors about depreciated calls

//-----------INCLUDE
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h> // For threads
#include <stdio.h> // For printf()
#include <vector>

//#pragma comment(lib, "Ws2_32.lib") // Tells linker that Ws2_32.lib file is needed. https://learn.microsoft.com/en-us/windows/win32/winsock/creating-a-basic-winsock-application
using namespace std;

//-----------GLOBALS
const int PROXY_PORT_NUM = 9081; // Port to reach proxy.
const int SERVER_PORT_NUM = 9080; // Port to reach server.
const int MAX_CONNECTIONS = 10;
const int MAX_BUFFER = 4096; // Arbitrary. Pulled 4096 from server code.
const char HAZARDOUS_CONTENTS_CS_01[256] = "CLIENT TO SERVER HAZARD 1";
const char HAZARDOUS_CONTENTS_CS_02[256] = "CLIENT TO SERVER HAZARD 2";
const char HAZARDOUS_CONTENTS_SC_01[256] = "SERVER TO CLIENT HAZARD 1";
const char HAZARDOUS_CONTENTS_SC_02[256] = "SERVER TO CLIENT HAZARD 2";
#define SERVER_ADDRESS "192.168.1.36" // Address to reach server.
vector <sockaddr_in*> clients; 
vector<int> serverSockets, clientSockets; 

//-----------Functions
void clientToServer(void* data);
void serverToClient(void* data);

//-----------MAIN
int main()
{

    printf("It's proxy time!\n");

    // Initialize winsock (Reference: https://learn.microsoft.com/en-us/windows/win32/winsock/initializing-winsock)
    WSADATA wsaData;
    int initSuccess;
    initSuccess = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (initSuccess != 0) { // 0 == success
        printf("WSAStartup failed: %d\n", initSuccess);
        return 1;
    }
    else {
        printf("WSAStartup succeeded: %d\n", initSuccess);
    }

    // Make a socket
    int newSocket = socket(AF_INET, SOCK_STREAM, 6); //af == IPv4, type == SOCK_STREAM (probably need this one), protocol = 6 (TCP) https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
    sockaddr_in ipAddress;
    ipAddress.sin_family = AF_INET;
 //   ipAddress.sin_addr.s_addr = htonl(INADDR_ANY); // htonl documentation https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-htonl
    ipAddress.sin_addr.s_addr = INADDR_ANY;
    ipAddress.sin_port = htons(PROXY_PORT_NUM); // htons documentation https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-htons

    // Bind
    int bindStatus = bind(newSocket, (sockaddr*)&ipAddress, sizeof(ipAddress)); // bind documentation https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-bind
    if (bindStatus < 0) {
        printf("Bind failed: %d\n", bindStatus);
        return 1;
    }
    else {
        printf("Bind succeeded: %d\n", bindStatus);
    }

    // (hey) Listen
    int listenStatus = listen(newSocket, MAX_CONNECTIONS); // https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
    if (listenStatus < 0) {
        printf("Listen failed: %d\n", listenStatus);
        return 1;
    }
    else {
        printf("Listen succeeded: %d\n", listenStatus);
    }

    int index = -1;
    while (true) {
        //Count indeces
        index++;

        clients.push_back(new sockaddr_in);
        int clientInfoSize = sizeof(*clients[index]);
        clientSockets.push_back(accept(newSocket, (sockaddr*)clients[index], &clientInfoSize));

        if (clientSockets[index] < 1) {
            printf("Accept failed: %d\n", clientSockets[index]);
            return 1;
        }
        else {
            printf("connection from  %s", inet_ntoa(clients[index]->sin_addr));
            printf(" arrived at Port # = %d\n", clients[index]->sin_port);
        }

        // Set up server info
        serverSockets.push_back(socket(AF_INET, SOCK_STREAM, 6));
        clients[index]->sin_family = AF_INET; // Address family
        clients[index]->sin_addr.s_addr = inet_addr(SERVER_ADDRESS); // Server
        clients[index]->sin_port = htons(SERVER_PORT_NUM); // Port

        // Connect to server
        int connectionStatus = connect(serverSockets[index], (LPSOCKADDR)clients[index], sizeof(*clients[index]));
        if (connectionStatus < 0) {
            printf("Server connection failed: %d\n", connectionStatus);
            return 1;
        }
        else {
            printf("Server connection succeeded: %d\n", connectionStatus);
        }

        // Spawn threads
        _beginthread(clientToServer, 1, (void*)index);
        _beginthread(serverToClient, 1, (void*)index);
        Sleep(500); // Arbitrary sleep time
    } // End While


    // Clean up!
    WSACleanup();
    closesocket(newSocket);
    clients.clear();
    serverSockets.clear();
    clientSockets.clear();
    return 0;
}


void clientToServer(void* data) {
    int index = *((int*)&data);
    int recvData = 1, sendData = 1;

    // Get data from client, send to server.
    while (recvData > 0 && sendData > 0) {
        char buffer[MAX_BUFFER];
        recvData = recv(clientSockets[index], buffer, MAX_BUFFER, 0);
        if (recvData > 0 && sendData > 0) {
            printf("\tCtoS buffer: %s\n", buffer);

            // Check for hazardous message.
            if (buffer == HAZARDOUS_CONTENTS_CS_01 || buffer == HAZARDOUS_CONTENTS_CS_02) {
                // I decided that nothing has to happen, I just don't pass on the message.
            }
            else {
                sendData = send(serverSockets[index], buffer, recvData, 0);
            }
        }
        else {
            break;
        }
    } // End While

    _endthread();

    return;
}

void serverToClient(void* data) {
    int index = *((int*)&data);
    int recvData = 1, sendData = 1;

    // Get data from server, send to client.
    while (recvData > 0 && sendData > 0) {
        char buffer[MAX_BUFFER];
        recvData = recv(serverSockets[index], buffer, MAX_BUFFER, 0);
        if (recvData > 0 && sendData > 0) {
            printf("\tStoC buffer: %s\n", buffer);
            if (buffer == HAZARDOUS_CONTENTS_SC_01 || buffer == HAZARDOUS_CONTENTS_SC_02) {
                // I decided that nothing has to happen, I just don't pass on the message.
            }
            else {
                sendData = send(clientSockets[index], buffer, recvData, 0);
            }
        }
        else {
            break;
        }
    } // End While

    // Job done, close and clean.
    //closesocket(clientSockets[index]); // For some reason, closing the sockets causes the images to discontinue displaying.
    //closesocket(serverSockets[index]);
    shutdown(clientSockets[index], 1); // closesocket() caused the images to disappear for some reason. shutdown() does not.
    shutdown(serverSockets[index], 1); // shutdown documentation https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-shutdown

    return;
}
