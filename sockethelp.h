#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string>
#include <map>

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
        pthread_mutex_lock(&mutex1);
        bool _isListen = isListen;
        pthread_mutex_unlock(&mutex1);
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
        auto iter = AcceptSocket.find(ar.AsyncState);
        Socket* client;
        if(iter != AcceptSocket.end())
            client = iter->second;
        AcceptSocket.erase(ar.AsyncState);
        return client;
    }

    bool Connect(string ip, int port)
    {
        pthread_mutex_lock(&mutex1);
        bool _isbind = isbind;
        pthread_mutex_unlock(&mutex1);
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
        auto iter = ConnectStateSave.find(ar.AsyncState);
        bool code = false;
        if(iter != ConnectStateSave.end())
            code = iter->second;
        ConnectStateSave.erase(ar.AsyncState);
        return code;
    }

    int Receive(byte* buffer, int size, int socketFlags)
    {
        return recv(me, (char*)buffer, size, socketFlags);
    }

    int EndReceive(AsyncResult ar)
    {
        auto iter = ReceiveSizeSave.find(ar.AsyncState);
        int size = -1;
        if(iter != ReceiveSizeSave.end())
            size = iter->second;
        ReceiveSizeSave.erase(ar.AsyncState);
        return size;
    }

    int Send(byte* buffer, int size, int socketFlags)
    {
        pthread_mutex_lock(&mutex1);
        int ans = send(me, (char*)buffer, size, socketFlags);
        pthread_mutex_unlock(&mutex1);
        return ans;
    }

    int EndSend(AsyncResult ar)
    {
        auto iter = SendSizeSave.find(ar.AsyncState);
        int size = -1;
        if(iter != SendSizeSave.end())
            size = iter->second;
        SendSizeSave.erase(ar.AsyncState);
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
        auto iter = ReceiveFromDataSave.find(ar.AsyncState);
        void** ResultData;
        if(iter != ReceiveFromDataSave.end())
            ResultData = (void**)iter->second;

        ReceiveFromDataSave.erase(ar.AsyncState);
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

        pthread_mutex_lock(&mutex1);
        int ans = sendto(me, (char*)buffer, size, socketFlags, (struct sockaddr *)&servaddr, sizeof(servaddr));
        pthread_mutex_unlock(&mutex1);
        return ans;
    }

    int EndSendTo(AsyncResult ar)
    {
        auto iter = SendSizeSave.find(ar.AsyncState);
        int size = -1;
        if(iter != SendSizeSave.end())
            size = iter->second;
        SendSizeSave.erase(ar.AsyncState);
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
        pthread_mutex_lock(&mutex1);
        bool _isbind = isbind;
        pthread_mutex_unlock(&mutex1);
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
        AsyncResult ardata;
        ardata.AsyncState = (void *)data[2];
        server->AcceptSocket[ardata.AsyncState] = now;
        AsyncCallback _doing = (AsyncCallback)(data[1]);
        free(data);
        _doing(ardata);
        return NULL;
    }

    static void * ThreadConnect(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* connecter = (Socket*)data[0];
        bool iConnect = connecter->Connect(*((string *)data[1]), (int)((long long int)data[2]));
        AsyncResult ardata;
        ardata.AsyncState = (void *)data[4];
        connecter->ConnectStateSave[ardata.AsyncState] = iConnect;
        AsyncCallback _doing = (AsyncCallback)(data[3]);
        free(((string *)data[1]));
        free(data);
        _doing(ardata);
        return NULL;
    }

    static void * ThreadReceive(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* taker = (Socket*)data[0];
        int iResult = taker->Receive((byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]));
        AsyncResult ardata;
        ardata.AsyncState = (void *)data[5];
        taker->ReceiveSizeSave[ardata.AsyncState] = iResult;
        AsyncCallback _doing = (AsyncCallback)(data[4]);
        free(data);
        _doing(ardata);
        return NULL;
    }

    static void * ThreadSend(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* sender = (Socket*)data[0];
        int iResult = sender->Send((byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]));
        AsyncResult ardata;
        ardata.AsyncState = (void *)data[5];
        sender->SendSizeSave[ardata.AsyncState] = iResult;
        AsyncCallback _doing = (AsyncCallback)(data[4]);
        free(data);
        _doing(ardata);
        return NULL;
    }

    static void * ThreadReceiveFrom(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* taker = (Socket*)data[0];
        string remoteip = "";
        int port = 0;
        int iResult = taker->ReceiveFrom((byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]), &remoteip, &port);
        AsyncResult ardata;
        ardata.AsyncState = (void *)data[5];
        void** Resultdata = (void**)malloc(sizeof(void*) * 3);
        Resultdata[0] = (void*)((long long int)iResult);
        Resultdata[1] = &remoteip;
        Resultdata[2] = (void*)((long long int)port);
        taker->ReceiveFromDataSave[ardata.AsyncState] = (long long int)Resultdata;
        AsyncCallback _doing = (AsyncCallback)(data[4]);
        free(data);
        _doing(ardata);
        free(Resultdata);
        return NULL;
    }

    static void * ThreadSendTo(void *arg)
    {
        void ** data = (void **)((long long int)arg);
        Socket* sender = (Socket*)data[0];

        string toip = *((string*)data[4]);
        int toport = (int)((long long int)data[5]);

        int iResult = sender->SendTo(toip, toport, (byte*)data[1], (int)((long long int)data[2]), (int)((long long int)data[3]));
        AsyncResult ardata;
        ardata.AsyncState = (void *)data[7];
        sender->SendSizeSave[ardata.AsyncState] = iResult;
        AsyncCallback _doing = (AsyncCallback)(data[6]);
        free(data);
        _doing(ardata);
        return NULL;
    }
};

bool Socket::BeginAccept(AsyncCallback callback, void * state)
{
    pthread_mutex_lock(&mutex1);
    bool _isListen = isListen;
    pthread_mutex_unlock(&mutex1);
    if(_isListen)
    {
        auto x = AcceptSocket.find(state);
        if(x != AcceptSocket.end())
            return false;

        void ** data = (void **)malloc(sizeof(void *) * 3);
        data[0] = this;
        data[1] = (void *)((long long int)callback);
        data[2] = state;
        pthread_t t1;
        pthread_create(&t1, NULL, Socket::ThreadAccept, (void *)data);
        return true;
    }
    else
    {
        return false;
    }
}

bool Socket::BeginConnect(string ip, int port, AsyncCallback callback, void * state)
{
    pthread_mutex_lock(&mutex1);
    bool _isbind = isbind;
    pthread_mutex_unlock(&mutex1);
    if(!_isbind)
    {
        auto x = ConnectStateSave.find(state);
        if(x != ConnectStateSave.end())
            return false;

        void ** data = (void **)malloc(sizeof(void *) * 5);
        string * theip = (string *)malloc(sizeof(string));
        new((void*)theip) string();
        *theip = ip;
        data[0] = this;
        data[1] = &ip;
        data[2] = (void *)((long long int)port);
        data[3] = (void *)((long long int)callback);
        data[4] = state;

        pthread_t t1;
        pthread_create(&t1, NULL, Socket::ThreadConnect, (void *)data);
        return true;
    }
    else
    {
        return false;
    }
}

bool Socket::BeginReceive(byte* buffer, int size, int socketFlags, AsyncCallback callback, void * state)
{
    pthread_mutex_lock(&mutex1);
    bool _isbind = isbind;
    pthread_mutex_unlock(&mutex1);
    if(_isbind)
    {
        auto x = ReceiveSizeSave.find(state);
        if(x != ReceiveSizeSave.end())
            return false;

        void ** data = (void **)malloc(sizeof(void *) * 6);
        data[0] = this;
        data[1] = buffer;
        data[2] = (void *)((long long int)size);
        data[3] = (void *)((long long int)socketFlags);
        data[4] = (void *)((long long int)callback);
        data[5] = state;
        pthread_t t1;
        pthread_create(&t1, NULL, Socket::ThreadReceive, (void *)data);
        return true;
    }
    else
    {
        return false;
    }
    
}

bool Socket::BeginSend(byte* buffer, int offset, int size, int socketFlags, AsyncCallback callback, void * state)
{
    pthread_mutex_lock(&mutex1);
    bool _isbind = isbind;
    pthread_mutex_unlock(&mutex1);
    if(_isbind)
    {
        auto x = SendSizeSave.find(state);
        if(x != SendSizeSave.end())
            return false;

        void ** data = (void **)malloc(sizeof(void *) * 6);
        data[0] = this;
        data[1] = (buffer + offset);
        data[2] = (void *)((long long int)size);
        data[3] = (void *)((long long int)socketFlags);
        data[4] = (void *)((long long int)callback);
        data[5] = state;

        pthread_t t1;
        pthread_create(&t1, NULL, Socket::ThreadSend, (void *)data);
        return true;
    }
    else
    {
        return false;
    }
}

bool Socket::BeginReceiveFrom(byte* buffer, int size, int socketFlags, AsyncCallback callback, void * state)
{
    pthread_mutex_lock(&mutex1);
    bool _isbind = isbind;
    pthread_mutex_unlock(&mutex1);
    if(_isbind)
    {
        auto x = ReceiveFromDataSave.find(state);
        if(x != ReceiveFromDataSave.end())
            return false;

        void ** data = (void **)malloc(sizeof(void *) * 6);
        data[0] = this;
        data[1] = buffer;
        data[2] = (void *)((long long int)size);
        data[3] = (void *)((long long int)socketFlags);
        data[4] = (void *)((long long int)callback);
        data[5] = state;
        pthread_t t1;
        pthread_create(&t1, NULL, Socket::ThreadReceiveFrom, (void *)data);
        return true;
    }
    else
    {
        return false;
    }
    
}

bool Socket::BeginSendTo(byte* buffer, int offset, int size, int socketFlags, string sendtoip, int sendtoport, AsyncCallback callback, void * state)
{
    pthread_mutex_lock(&mutex1);
    bool _isbind = isbind;
    pthread_mutex_unlock(&mutex1);
    if(_isbind)
    {
        auto x = SendSizeSave.find(state);
        if(x != SendSizeSave.end())
            return false;

        void ** data = (void **)malloc(sizeof(void *) * 8);
        string * toip = (string*)malloc(sizeof(string));
        new((void*)toip) string();
        *toip = sendtoip;
        data[0] = this;
        data[1] = (buffer + offset);
        data[2] = (void *)((long long int)size);
        data[3] = (void *)((long long int)socketFlags);
        data[4] = toip;
        data[5] = (void *)((long long int)sendtoport);
        data[6] = (void *)((long long int)callback);
        data[7] = state;

        pthread_t t1;
        pthread_create(&t1, NULL, Socket::ThreadSendTo, (void *)data);
        return true;
    }
    else
    {
        return false;
    }
}