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

Socket server;

list<string> clients;

string * split(string str, string delimiter)
{
    char * charstr = (char*)str.c_str();
	char * chardelimiter = (char*)delimiter.c_str();
	char *context;
    int cont = 0;
    list<string> strlist;
	for (char *p = strtok_s(charstr, chardelimiter, &context); p!= NULL; p = strtok_s(NULL, delimiter.data(),&context))
	{
        string x(p);
        strlist.push_back(x);
        cont++;
    }
    string * outdata = (string*)malloc(sizeof(string) * cont);
    auto xxx = strlist.begin();
    for(int i = 0; i < cont; i++)
    {
        new((void*)&(outdata[i])) string();
        outdata[i] = *xxx;
        xxx++;
    }
    return outdata;
}

void receiveCallBack(AsyncResult ar)
{
    void ** state = (void **)((long long int)ar.AsyncState);
    Socket * geter = (Socket*)state[0];
    byte * data = (byte*)state[1];
    string fromip = "";
    int fromport = 0;
    int bufsize = server.EndReceiveFrom(ar, &fromip, &fromport);
    if(bufsize == sizeof(int))
    {
        string ipport = fromip + ":" + to_string(fromport);
        int * talkSize = (int*)((void*)data);
        byte * talkData = new byte[*talkSize + 1];
        memset(talkData, 0, *talkSize + 1);
        for(int i = 0, catchSize = *talkSize; i < *talkSize; )
        {
            string datafromip = "";
            int datafromport = 0;
            int getsize = geter->ReceiveFrom(talkData + i, catchSize, 0, &datafromip, &datafromport);
            if(getsize > 0 && datafromip == fromip && datafromport == fromport)
            {
                i += getsize;
                catchSize -= getsize;
            }
        }
        if(*(new string((char*)talkData)) == "exit")
        {
            for(auto i = clients.begin(); i != clients.end(); i++)
            {
                if(*i == ipport)
                {
                    printf("one exit\n");
                    clients.erase(i);
                }
            }
        }
        else
        {
            for(auto i = clients.begin(); i != clients.end(); i++)
            {
                string * clipport = split(*i, ":");
                if(*i != ipport)
                {
                    geter->SendTo(clipport[0], stoi(clipport[1]), data, sizeof(int), 0);
                    geter->SendTo(clipport[0], stoi(clipport[1]), talkData, *talkSize, 0);
                }
                free(clipport);
            }
        }
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
    else if(bufsize == 0)
    {
        pthread_mutex_lock(&mutex1);
        geter->Close();
        pthread_mutex_unlock(&mutex1);
        free(data);
        free(state);
        exit(0);
    }
    else
    {
        char ex[3] = {((char*)data)[0], ((char*)data)[1], '\0'};
        if(*(new string(ex)) == "GO")
        {
            printf("one in\n");
            string ipport = fromip + ":" + to_string(fromport);
            auto fin = find(clients.begin(), clients.end(), ipport);
            if(fin == clients.end())
            {
                clients.push_back(ipport);
            }
        }
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
}

void whenexit()
{
    pthread_mutex_lock(&mutex1);
    server.Close();
    pthread_mutex_unlock(&mutex1);

    Sleep(2000);

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
    hints.ai_flags = AI_PASSIVE;

    byte * data = (byte*)malloc(sizeof(int));
    server = *(new Socket(hints));
    void ** state = (void **)malloc(sizeof(void *) * 2);
    state[0] = &server;
    state[1] = data;

    if(!server.Bind("0.0.0.0", 24444))
    {
        printf("Bind error\n");
        WSACleanup();
        return 0;
    }
    server.BeginReceiveFrom(data, sizeof(int), 0, receiveCallBack, state);

    string ggg = "";
    cin >> ggg;
    return 0;
}