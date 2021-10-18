# CPP_Sockethelp
A C++ Socket for class. 

## Usage
use it with.
``` C++
#inclube "sockethelp.h"
```

for windows pls enable WSA first
``` C++
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

int main()
{
    startWSA();
    .
    .
    .
}
```

### server

set up your server socket for tcp
``` C++
int main()
{
    .
    .
    .
    
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    ListenServer = *(new Socket(hints));

    //bind server ip
    if(!ListenServer.Bind("0.0.0.0", 27575))
    {
        printf("Bind error\n");
        WSACleanup();
        return 0;
    }

    //start listen
    if(!ListenServer.Listen())
    {
        printf("Listen error\n");
        WSACleanup();
        return 0;
    }
    
    .
    .
    .
}
```

set your async client accept callback function and start a async client accept function
``` C++
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

    .
    .
    .
}

int main()
{
    .
    .
    .
    void* indata[2] = {&ListenServer, &clients};

    ListenServer.BeginAccept(acceptCallBack, (void*)&indata);
    
    string end = "";

    cin >> end;
}
```

set your async client receive callback function and start a async client receive function
``` C++
void receiveCallBack(AsyncResult ar)
{
    void ** data = (void **)ar.AsyncState;
    Socket * client = (Socket*)data[0];
    int iResult = client->EndReceive(ar);
    if(iResult > 0)
    {
        byte *recvbuf = (byte*)data[1];

        //something you want to do like.
        //printf("%s\n", (char*)recvbuf); => "Hi"
        //client->Send("hello", 6, 0);
    }
    else
    {
        //close
    }
}
void acceptCallBack(AsyncResult ar) 
{
    .
    .
    .
    byte *recvbuf = (byte*)malloc(sizeof(byte) * DEFAULT_BUFLEN);

    void ** receivedata = (void **)malloc(sizeof(void *) * 2);
    receivedata[0] = client;
    receivedata[1] = recvbuf;
    client->BeginReceive(recvbuf, DEFAULT_BUFLEN, 0, receiveCallBack, (void *)receivedata);
}
```

### client
set up your client socket for tcp and connect to server
``` C++
int main()
{
    .
    .
    .
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;


    client = *(new Socket(hints));
    client.Connect("127.0.0.1", 27575);
    .
    .
    .
}
```

set your async receive callback function and start a async receive function
``` C++
void receiveCallBack(AsyncResult ar)
{
    void ** state = (void **)((long long int)ar.AsyncState);
    Socket * geter = (Socket*)state[0];
    byte * data = (byte*)state[1];
    int bufsize = geter->EndReceive(ar);
    if(bufsize > 0)
    {
        //something you want to do like.
        printf("%s\n", data); //hello
        byte * senddata = (byte*)((char *)"hi");
        geter->Send(senddata, 2, 0);
        bool code = geter->BeginReceive(data, DEFAULT_BUFLEN, 0, receiveCallBack, state);
        if(!code)
        {
            //LOCK(&mutex1, geter->Close(););
            //free(data);
            //free(state);
            //exit(0); => close
        }
    }
    else
    {
        //LOCK(&mutex1, geter->Close(););
        //free(data);
        //free(state);
        //exit(0); => close
    }
}
int main()
{
    .
    .
    .
    byte * data = (byte *)malloc(sizeof(byte) * DEFAULT_BUFLEN);
    void ** state = (void **)malloc(sizeof(void *) * 2);
    state[0] = &client;
    state[1] = data;

    client.BeginReceive(data, DEFAULT_BUFLEN, 0, receiveCallBack, state);

    string ggg = "";
    cin >> ggg;
}
```

## Template
### TCP
server: [main.cpp](https://github.com/Jimmy01240397/CPP_Sockethelp/blob/master/main.cpp)

client: [client.cpp](https://github.com/Jimmy01240397/CPP_Sockethelp/blob/master/client.cpp)

### UDP
A chat server

server: [TalkServer.cpp](https://github.com/Jimmy01240397/CPP_Sockethelp/blob/master/TalkServer.cpp)

client: [TalkClient.cpp](https://github.com/Jimmy01240397/CPP_Sockethelp/blob/master/TalkClient.cpp)
