#include <stdlib.h>
#include <stdio.h>
#include "sockethelp.h"
#include <iostream>
#include <string>
#include <pthread.h>
#include <windows.h>
#include <list>
#include <mutex>

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

void receiveCallBack(AsyncResult ar)
{
    void ** state = (void **)((long long int)ar.AsyncState);
    Socket * geter = (Socket*)state[0];
    byte * data = (byte*)state[1];
    int bufsize = geter->EndReceive(ar);
    if(bufsize > 0)
    {
        printf("get %s\n", data);
        byte * senddata = (byte*)((char *)"OK");
        geter->Send(senddata, 2, 0);
        bool code = geter->BeginReceive(data, DEFAULT_BUFLEN, 0, receiveCallBack, state);
        if(!code)
        {
            pthread_mutex_lock(&mutex1);
            geter->Close();
            pthread_mutex_unlock(&mutex1);
            free(data);
            free(state);
            exit(0);
        }
    }
    else
    {
        pthread_mutex_lock(&mutex1);
        geter->Close();
        pthread_mutex_unlock(&mutex1);
        free(data);
        free(state);
        exit(0);
    }
}

Socket client;

void whenexit()
{
    pthread_mutex_lock(&mutex1);
    client.Close();
    pthread_mutex_unlock(&mutex1);

    Sleep(1000);

    WSACleanup();

    pthread_mutex_destroy(&mutex1);
}

int main()
{
    pthread_mutex_init(&mutex1, 0);
    startWSA();
    atexit(whenexit);
    
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;


    client = *(new Socket(hints));
    client.Connect("127.0.0.1", 27575);

    byte * data = (byte *)malloc(sizeof(byte) * DEFAULT_BUFLEN);
    void ** state = (void **)malloc(sizeof(void *) * 2);
    state[0] = &client;
    state[1] = data;

    client.BeginReceive(data, DEFAULT_BUFLEN, 0, receiveCallBack, state);

    string ggg = "";
    cin >> ggg;
}