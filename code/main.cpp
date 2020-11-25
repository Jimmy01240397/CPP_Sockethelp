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

    // Initialize Winsock
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
    // TODO you will need to write some code in this function
    char temp[DEFAULT_BUFLEN]={};

    itoa(K_MAX,temp,10);
    strcpy(buff,temp);
    return strlen(buff);
}

void receiveCallBack(AsyncResult ar)
{
    // function generator
    long long int * data = (long long int *)ar.AsyncState;
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

        // send result to pygame
        client->Send(sendbuf, sendbuflen, 0);
        if(!client->BeginReceive(recvbuf, DEFAULT_BUFLEN, 0, receiveCallBack, (void *)data))
        {
            pthread_mutex_lock(&mutex1);
            client->Close();
            for(auto i = clients.begin(); i != clients.end(); i++)
            {
                //printf("%lld", (long long int)(*i));
                if((*i) == client)
                {
                    clients.erase(i);
                    break;
                }
            }
            delete client;
            pthread_mutex_unlock(&mutex1);
        }
    }
    else
    {
        pthread_mutex_lock(&mutex1);
        client->Close();
        for(auto i = clients.begin(); i != clients.end(); i++)
        {
            //printf("%lld", (long long int)(*i));
            if((*i) == client)
            {
                clients.erase(i);
                break;
            }
        }
        delete client;
        pthread_mutex_unlock(&mutex1);
    }
    
}

void acceptCallBack(AsyncResult ar) 
{
    long long int * data = (long long int *)ar.AsyncState;
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

    long long int * receivedata = (long long int *)malloc(sizeof(long long int) * 4);
    receivedata[0] = (long long int)client;
    receivedata[1] = (long long int)recvbuf;
    receivedata[2] = (long long int)sendbuf;
    receivedata[3] = (long long int)counter;
    client->BeginReceive(recvbuf, DEFAULT_BUFLEN, 0, receiveCallBack, (void *)receivedata);
}

void whenexit()
{
    pthread_mutex_lock(&mutex1);
    for(auto i = clients.begin(); i != clients.end(); i++)
    {
        ((Socket*)(*i))->Close();
        //delete ((Socket*)(*i));
    }

    clients.clear();
    pthread_mutex_unlock(&mutex1);

    //Sleep(3000);

    //printf("exit");
    ListenServer.Close();

    //delete &server;
    // closesocket(client);

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
    //new((void*)&ListenServer) Socket(hints);

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


    long long int indata[2] = {(long long int)&ListenServer, (long long int)&clients};

    //printf("gogo");
    ListenServer.BeginAccept(acceptCallBack, (void*)&indata);
    //printf("gogo2");
    /*while (true)
    {
        Sleep(10);
    }*/
    

    string end = "";

    cin >> end;
    //whenexit();
}


