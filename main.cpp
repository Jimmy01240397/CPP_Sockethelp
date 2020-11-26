#include <stdlib.h>
#include <stdio.h>
#include "sockethelp.h"
#include <iostream>
#include <string>
#include <pthread.h>
#include <windows.h>
#include <list>

#define DEFAULT_BUFLEN 256

using namespace std;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

int startWSA()
{
    WSADATA wsaData;

    int code = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (code != 0)
    {
        printf("WSAStartup failed with error: %d\n", code);
        return 1;
    }
    return 0;
}

list<Socket*> clients;
Socket ListenServer;

int data_collector(char *buff, const int K_MAX)
{
    char temp[DEFAULT_BUFLEN]={};

    itoa(K_MAX,temp,10);
    strcpy(buff,temp);
    return strlen(buff);
}

void receiveCallBack(AsyncResult ar)
{
    void ** data = (void **)ar.AsyncState;
    Socket * client = (Socket*)data[0];
    int iResult = client->EndReceive(ar);
    if(iResult > 0)
    {
        int * counter = (int *)data[3];
        byte *recvbuf = (byte*)data[1];
        byte *sendbuf = (byte*)data[2];

        (*counter)++;

        int sendbuflen = data_collector((char*)sendbuf, *counter);

        printf("send %s \n",(char*)sendbuf);

        client->Send(sendbuf, sendbuflen, 0);
        if(!client->BeginReceive(recvbuf, DEFAULT_BUFLEN, 0, receiveCallBack, (void *)data))
        {
            LOCK(&mutex1, 
                client->Close();
                for(auto i = clients.begin(); i != clients.end(); i++)
                {
                    if((*i) == client)
                    {
                        clients.erase(i);
                        break;
                    }
                }
                delete client;
            ); 
        }
    }
    else
    {
        LOCK(&mutex1, 
            client->Close();
            for(auto i = clients.begin(); i != clients.end(); i++)
            {
                if((*i) == client)
                {
                    clients.erase(i);
                    break;
                }
            }
            delete client;
        ); 
    }
}
void acceptCallBack(AsyncResult ar) 
{
    void ** data = (void **)((long long int)ar.AsyncState);
    Socket * ListenServer = (Socket *)data[0];
    list<Socket*> * clients = (list<Socket*> *)data[1];
    Socket * client = ListenServer->EndAccept(ar);
    if(client == NULL)
    {
        return;
    }
    clients->push_back(client);
    ListenServer->BeginAccept(acceptCallBack, (void*)data);

    byte *recvbuf = (byte*)malloc(sizeof(byte) * DEFAULT_BUFLEN);
    byte *sendbuf = (byte*)malloc(sizeof(byte) * DEFAULT_BUFLEN);
    int * counter = (int *)malloc(sizeof(int));

    *counter = 0;

    int sendbuflen = data_collector((char*)sendbuf, *counter);

    printf("send %s \n",(char*)sendbuf);
    client->Send(sendbuf, sendbuflen, 0);

    void ** receivedata = (void **)malloc(sizeof(void *) * 4);
    receivedata[0] = client;
    receivedata[1] = recvbuf;
    receivedata[2] = sendbuf;
    receivedata[3] = (void*)((long long int)counter);
    client->BeginReceive(recvbuf, DEFAULT_BUFLEN, 0, receiveCallBack, (void *)receivedata);
}

void whenexit()
{
    LOCK(&mutex1, 
        for(auto i = clients.begin(); i != clients.end(); i++)
        {
            ((Socket*)(*i))->Close();
        }
        clients.clear();
    ); 
    ListenServer.Close();
    Sleep(3000);
    WSACleanup();
    pthread_mutex_destroy(&mutex1);
}

int main()
{
    pthread_mutex_init(&mutex1, 0);
    startWSA();
    struct addrinfo hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    ListenServer = *(new Socket(hints));

    atexit(whenexit);

    if(!ListenServer.Bind("0.0.0.0", 27575))
    {
        printf("Bind error\n");
        WSACleanup();
        return 0;
    }

    if(!ListenServer.Listen())
    {
        printf("Listen error\n");
        WSACleanup();
        return 0;
    }

    void* indata[2] = {&ListenServer, &clients};

    ListenServer.BeginAccept(acceptCallBack, (void*)&indata);

    string end = "";

    cin >> end;
}