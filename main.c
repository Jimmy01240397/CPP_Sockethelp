#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#define PORT (char*)"27575"
int iResult, iSendResult;
#define DEFAULT_BUFLEN 256
SOCKET get_tcp_server(char *port)
{
    WSADATA wsaData;
    int iResult;
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET client = INVALID_SOCKET;
    struct addrinfo *result = NULL;
    struct addrinfo hints;
    int iSendResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0)
    {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET)
    {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR)
    {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    printf("Socket is created successfully. \n");
    return ListenSocket;
}

int data_collector(char *buff, const int K_MAX)
{
    // TODO you will need to write some code in this function
    char temp[DEFAULT_BUFLEN]={};
    itoa(K_MAX,temp,10);
    strcpy(buff,temp);
    return strlen(buff);
}
int main()
{
    char recvbuf[DEFAULT_BUFLEN]={};
    int recvbuflen = DEFAULT_BUFLEN;
    char sendbuf[DEFAULT_BUFLEN]={};
    int sendbuflen = DEFAULT_BUFLEN;
    SOCKET server = get_tcp_server(PORT);
    // wait and accept a client socket connection
    SOCKET client = accept(server, NULL, NULL);
    // do receive and send
    long counter = 0;
    do
    {
        // function generator

        sendbuflen = data_collector(sendbuf, counter);

        // send result to pygame
        printf("send %s \n",sendbuf);
        iSendResult = send(client, sendbuf, sendbuflen, 0);

        // receive pygame ack
        iResult = recv(client, recvbuf, recvbuflen, 0);

        // update counter and k
        counter++;
        // collect timer data
    } while (iResult > 0);

    printf("SOCKET :%lld\n", server);
    // shutdown the connection since we're done
    iResult = shutdown(client, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(client);
        WSACleanup();
        return 1;
    }
    closesocket(server);
    WSACleanup();
    return 0;
}
