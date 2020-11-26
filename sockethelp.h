#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string>
#include <map>

#define LOCK(lockobject, doing) \
    pthread_mutex_lock(lockobject); \
    doing \
    pthread_mutex_unlock(lockobject); 

#define BeginBase(ison, map, _doing, threadfunc)  \
    LOCK(&mutex1, bool _ison = ison;); \
    if(_ison) \
    { \
        auto x = map.find(state); \
        if(x != map.end()) \
        { \
            return false; \
        } \
        _doing \
        pthread_t t1; \
        pthread_create(&t1, NULL, threadfunc, (void *)data); \
        return true; \
    } \
    else \
    { \
        return false; \
    } 

#define AsyncThread(state, map, indata, callback, dofreeafter, dofreeend)  \
        AsyncResult ardata; \
        ardata.AsyncState = (void *)state; \
        map[ardata.AsyncState] = indata; \
        AsyncCallback _doing = (AsyncCallback)(callback); \
        dofreeafter; \
        free(data); \
        _doing(ardata); \
        dofreeend; \
        return NULL;

#define EndBase(map, output, type) \
        auto iter = map.find(ar.AsyncState); \
        if(iter != map.end()) \
        { \
            output = type iter->second; \
        } \
        map.erase(ar.AsyncState); 

using namespace std;

struct AsyncResult
{
    void * AsyncState;
};

typedef void (*AsyncCallback)(AsyncResult ar);

typedef unsigned char byte;

class Socket
{
public:
    bool isListen = false;

    Socket()
    {
        pthread_mutex_init(&mutex1, 0);
    }
    Socket(struct addrinfo hints)
    {
        pthread_mutex_init(&mutex1, 0);
        this->hints = hints;
        
        // Create a SOCKET for connecting to server
        me = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
        //printf("%d\n", me);
    }

    ~Socket()
    {
        Close();
        pthread_mutex_destroy(&mutex1);
    }

    bool BeginAccept(AsyncCallback callback, void * state);

    bool BeginConnect(string ip, int port, AsyncCallback callback, void * state);

    bool BeginReceive(byte* buffer, int size, int socketFlags, AsyncCallback callback, void * state);

    bool BeginSend(byte* buffer, int offset, int size, int socketFlags, AsyncCallback callback, void * state);

    bool BeginReceiveFrom(byte* buffer, int size, int socketFlags, AsyncCallback callback, void * state);
    
    bool BeginSendTo(byte* buffer, int offset, int size, int socketFlags, string sendtoip, int sendtoport, AsyncCallback callback, void * state);

    void startPeer(SOCKET _socket, string ip, int port, bool connected)
    {
        me = _socket;
        _ip = ip;
        _port = port;
        isbind = connected;
    }

    bool Bind(string ip, int port)
    {
        if(!isbind)
        {
            this->_ip = ip;
            this->_port = port;

            int iResult;

            isbind = false;

            struct addrinfo *result = NULL;

            int iSendResult;

            char portstring[10];
            sprintf(portstring, "%d", port);
            iResult = getaddrinfo(ip.c_str(), portstring, &hints, &result);
            if (iResult != 0)
            {
                return false;
            }
            iResult = bind(me, result->ai_addr, (int)result->ai_addrlen);
            if (iResult == SOCKET_ERROR)
            {
                freeaddrinfo(result);
                closesocket(me);
                return false;
            }

            freeaddrinfo(result);
            isbind = true;
            return true;
        }
        else
        {
            return false;
        }
    }

    Socket * Accept()
    {
        LOCK(&mutex1, bool _isListen = isListen;); 
        if(_isListen)
        {
            struct sockaddr_in client_addr;
            int client_addrlen = sizeof(client_addr);
            SOCKET client = accept(me, (struct sockaddr*)&client_addr, &client_addrlen);
            if(client == INVALID_SOCKET)
            {
                return NULL;
            }
            string client_ip(inet_ntoa(client_addr.sin_addr));
            int client_port=ntohs(client_addr.sin_port);
            bool client_isbind = true;
            Socket * clientPtr = (Socket *)malloc(sizeof(Socket));
            new((void*)clientPtr) Socket[1];
            clientPtr->startPeer(client, client_ip, client_port, client_isbind);

            return clientPtr;
        }
        else
        {
            return NULL;
        }
    }

    Socket* EndAccept(AsyncResult ar)
    {
        Socket* client;
        EndBase(AcceptSocket, client, );
        return client; 
    }

    bool Connect(string ip, int port)
    {
        LOCK(&mutex1, bool _isbind = isbind;); 
        if(_isbind)
        {
            return false;
        }

        me = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);

        struct sockaddr_in info;
        ZeroMemory(&info, sizeof(info));
        info.sin_family = hints.ai_family;

        info.sin_addr.s_addr = inet_addr(ip.c_str());
        info.sin_port = htons(port);

        int backcode = connect(me,(struct sockaddr *)&info,sizeof(info));
        if(backcode == 0)
        {
            isbind = true;
        }
        return backcode == 0; 
    }

    void initUDPClient()
    {
        isbind = true;
    }

    int EndConnect(AsyncResult ar)
    {
        bool code = false;
        EndBase(ConnectStateSave, code, );
        return code; 
    }

    int Receive(byte* buffer, int size, int socketFlags)
    {
        return recv(me, (char*)buffer, size, socketFlags);
    }

    int EndReceive(AsyncResult ar)
    {
        int size = -1;
        EndBase(ReceiveSizeSave, size, );
        return size; 
    }

    int Send(byte* buffer, int size, int socketFlags)
    {
        LOCK(&mutex1, int ans = send(me, (char*)buffer, size, socketFlags);); 
        return ans;
    }

    int EndSend(AsyncResult ar)
    {
        int size = -1;
        EndBase(SendSizeSave, size, );
        return size; 
    }

    int ReceiveFrom(byte* buffer, int size, int socketFlags, string* remoteIP, int* remotePort)
    {
        struct sockaddr_in peeraddr;
        socklen_t peerlen = sizeof(peeraddr);

        int data = recvfrom(me, (char*)buffer, size, socketFlags, (struct sockaddr *)&peeraddr, &peerlen);
        *remoteIP = *(new string(inet_ntoa(peeraddr.sin_addr)));
        *remotePort = ntohs(peeraddr.sin_port);
        return data;
    }

    int EndReceiveFrom(AsyncResult ar, string* remoteIP, int* remotePort)
    {
        void** ResultData;
        EndBase(ReceiveFromDataSave, ResultData, (void**));
        if(ResultData != NULL)
        {
            *remoteIP = *((string*)ResultData[1]);
            *remotePort = (int)((long long int)ResultData[2]);
            return (int)((long long int)ResultData[0]);
        }
        else
        {
            return -1;
        }
        
    }

    int SendTo(string sendIPto, int sendPortto, byte* buffer, int size, int socketFlags)
    {
        struct sockaddr_in servaddr;
        ZeroMemory(&servaddr, sizeof(servaddr));
        servaddr.sin_family = hints.ai_family;
        servaddr.sin_port = htons(sendPortto);
        servaddr.sin_addr.s_addr = inet_addr(sendIPto.c_str());

        LOCK(&mutex1, int ans = sendto(me, (char*)buffer, size, socketFlags, (struct sockaddr *)&servaddr, sizeof(servaddr));); 
        return ans;
    }

    int EndSendTo(AsyncResult ar)
    {
        int size = -1;
        EndBase(SendSizeSave, size, );
        return size;
    }

    bool Listen()
    {
        if(!isListen)
        {
            int iResult = listen(me, SOMAXCONN);
            if (iResult == SOCKET_ERROR) {
                closesocket(me);
                isListen = false;
                return false;
            }
            isListen = true;
            return true;
        }
        else
        {
            return false;
        }
    }

    void Close()
    {
        LOCK(&mutex1, bool _isbind = isbind;); 
        if(_isbind)
        {
            int iResult = shutdown(me, SD_SEND);
            if (iResult == SOCKET_ERROR) {
                closesocket(me);
            }
        }
        AcceptSocket.clear();
        ReceiveSizeSave.clear();
        ReceiveFromDataSave.clear();
        SendSizeSave.clear();
        ConnectStateSave.clear();
        isbind = false;
    }

    bool isBind(){ return isbind; }
    string ip(){ return _ip; }
    int port() { return _port; }
    bool connected(){ return isbind; }

private:
    int _port = 0;
    string _ip = "";
    struct addrinfo hints;
    bool isbind = false;
    SOCKET me = INVALID_SOCKET;

    pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

    map<void *, Socket*> AcceptSocket;
    map<void *, int> ReceiveSizeSave;
    map<void *, long long int> ReceiveFromDataSave;
    map<void *, int> SendSizeSave;
    map<void *, bool> ConnectStateSave;

    static void * ThreadAccept(void *arg)
    {
        void ** data = (void**)arg;
        Socket* server = (Socket*)data[0];
        Socket* now = server->Accept();
        if(now == NULL)
        {
            return NULL;
        }
        AsyncThread(data[2], server->AcceptSocket, now, data[1], , );
    }

    static void * ThreadConnect(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* connecter = (Socket*)data[0];
        bool iConnect = connecter->Connect(*((string *)data[1]), (int)((long long int)data[2]));
        AsyncThread(data[4], connecter->ConnectStateSave, iConnect, data[3], free(((string *)data[1])), );
    }

    static void * ThreadReceive(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* taker = (Socket*)data[0];
        int iResult = taker->Receive((byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]));
        AsyncThread(data[5], taker->ReceiveSizeSave, iResult, data[4], , );
    }

    static void * ThreadSend(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* sender = (Socket*)data[0];
        int iResult = sender->Send((byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]));
        AsyncThread(data[5], sender->SendSizeSave, iResult, data[4], , );
    }

    static void * ThreadReceiveFrom(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* taker = (Socket*)data[0];
        string remoteip = "";
        int port = 0;
        int iResult = taker->ReceiveFrom((byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]), &remoteip, &port);
        void** Resultdata = (void**)malloc(sizeof(void*) * 3);
        new((void*)Resultdata) void*[3]{(void*)((long long int)iResult), &remoteip, (void*)((long long int)port)};
        AsyncThread(data[5], taker->ReceiveFromDataSave, (long long int)Resultdata, data[4], , free(Resultdata));
    }

    static void * ThreadSendTo(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* sender = (Socket*)data[0];
        string toip = *((string*)data[4]);
        int toport = (int)((long long int)data[5]);
        int iResult = sender->SendTo(toip, toport, (byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]));
        AsyncThread(data[7], sender->SendSizeSave, iResult, data[6], , );
    }
};

bool Socket::BeginAccept(AsyncCallback callback, void * state)
{
    BeginBase(isListen, AcceptSocket, 
        void ** data = (void **)malloc(sizeof(void *) * 3);
        (new((void*)data) void*[3]{this, (void *)((long long int)callback), state});
    , Socket::ThreadAccept);
}

bool Socket::BeginConnect(string ip, int port, AsyncCallback callback, void * state)
{
    BeginBase(isbind, ConnectStateSave, 
        string * theip = (string *)malloc(sizeof(string));
        new((void*)theip) string();
        *theip = ip;
        void ** data = (void **)malloc(sizeof(void *) * 5);
        (new((void*)data) void*[5]{this, &ip, (void *)((long long int)port), (void *)((long long int)callback), state});
    , Socket::ThreadConnect);
}

bool Socket::BeginReceive(byte* buffer, int size, int socketFlags, AsyncCallback callback, void * state)
{
    BeginBase(isbind, ReceiveSizeSave, 
        void ** data = (void **)malloc(sizeof(void *) * 6);
        (new((void*)data) void*[6]{this, buffer, (void *)((long long int)size), (void *)((long long int)socketFlags), (void *)((long long int)callback), state});
    , Socket::ThreadReceive);
}

bool Socket::BeginSend(byte* buffer, int offset, int size, int socketFlags, AsyncCallback callback, void * state)
{
    BeginBase(isbind, SendSizeSave, 
        void ** data = (void **)malloc(sizeof(void *) * 6);
        (new((void*)data) void*[6]{this, (buffer + offset), (void *)((long long int)size), (void *)((long long int)socketFlags), (void *)((long long int)callback), state});
    , Socket::ThreadSend);
}

bool Socket::BeginReceiveFrom(byte* buffer, int size, int socketFlags, AsyncCallback callback, void * state)
{
    BeginBase(isbind, ReceiveFromDataSave, 
        void ** data = (void **)malloc(sizeof(void *) * 6);
        (new((void*)data) void*[6]{this, buffer, (void *)((long long int)size), (void *)((long long int)socketFlags), (void *)((long long int)callback), state});
    , Socket::ThreadReceiveFrom);
}

bool Socket::BeginSendTo(byte* buffer, int offset, int size, int socketFlags, string sendtoip, int sendtoport, AsyncCallback callback, void * state)
{
    BeginBase(isbind, SendSizeSave, 
        string * toip = (string*)malloc(sizeof(string));
        new((void*)toip) string();
        *toip = sendtoip;
        void ** data = (void **)malloc(sizeof(void *) * 8);
        (new((void*)data) void*[8]{this, (buffer + offset), (void *)((long long int)size), (void *)((long long int)socketFlags), toip, (void *)((long long int)sendtoport), (void *)((long long int)callback), state});
    , Socket::ThreadSendTo);
}