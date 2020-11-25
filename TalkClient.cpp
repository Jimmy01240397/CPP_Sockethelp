#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <list>
#include <algorithm>
#include "sockethelp.h"

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

Socket client;

void receiveCallBack(AsyncResult ar)
{
    void ** state = (void **)((long long int)ar.AsyncState);
    Socket * geter = (Socket*)state[0];
    byte * data = (byte*)state[1];
    string fromip = "";
    int fromport = 0;
    int bufsize = geter->EndReceiveFrom(ar, &fromip, &fromport);
    if(bufsize == sizeof(int))
    {
        int * talkSize = (int*)((void*)data);
        byte * talkData = new byte[*talkSize + 1];
        memset(talkData, 0, *talkSize + 1);
        for(int i = 0, catchSize = *talkSize; i < *talkSize; )
        {
            int getsize = geter->ReceiveFrom(talkData + i, catchSize, 0, &fromip, &fromport);
            i += getsize;
            catchSize -= getsize;
        }

        printf("%s\n", (char*)talkData);

        bool code = geter->BeginReceiveFrom(data, sizeof(int), 0, receiveCallBack, state);
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

void whenexit()
{
    pthread_mutex_lock(&mutex1);

    byte * writedata = (byte*)((char*)"exit");
    int len = 4;
    client.SendTo("127.0.0.1", 24444, (byte*)&len, sizeof(int), 0);
    client.SendTo("127.0.0.1", 24444, writedata, len, 0);

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
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    byte * data = (byte*)malloc(sizeof(int));
    client = *(new Socket(hints));
    void ** state = (void **)malloc(sizeof(void *) * 2);
    state[0] = &client;
    state[1] = data;

    client.initUDPClient();
    client.SendTo("127.0.0.1", 24444, (byte*)((char*)"GO"), 2, 0);

    client.BeginReceiveFrom(data, sizeof(int), 0, receiveCallBack, state);
    string com = "";
    for(cin >> com; com != "exit"; cin >> com)
    {
        byte * writedata = (byte*)com.data();
        int len = com.length();
        client.SendTo("127.0.0.1", 24444, (byte*)&len, sizeof(int), 0);
        client.SendTo("127.0.0.1", 24444, writedata, len, 0);
    }
}