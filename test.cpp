#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <map>

using namespace std;

typedef void (*AsyncCallback)(int ar);

class now
{
private:
    int ttt = 979;

public:
    int _port = 0;
    bool isListen = false;
    string _ip = "";
    bool isbind = false;

    map<long long int, int*> AcceptSocket;
    map<long long int, int> ReceiveSizeSave;
    map<long long int, int> SendSizeSave;

    now()
    {
        ttt = 74;
    }
    now(int dog)
    {
        ttt = dog;
    }
    ~now()
    {
        
    }
    int rrr = 9;
    long long int gggg(long long int yy, int uu)
    {
        AsyncCallback gg = (AsyncCallback)yy;
        rrr = uu;
        gg(54);
        long long int h = (long long int)this;
        printf("%lld\n", h);
        return h;
    }

    now * trt()
    {
        now * yyy = (now *)malloc(sizeof(now));
        new((void*)yyy) now[1];
        yyy->rrr = 999;
        return yyy;
    }

};

void B(void * go)
{
    long long int * doing = (long long int *)go;
    printf("%lld %lld %lld %lld %lld", doing[0], doing[1], doing[2], doing[3], doing[4]);
}

void A()
{
    int dd = 9;
    int dd5 = 8;
    long long int yy = 6618977;
    long long int gg = (long long int)&dd;
    long long int rr = (long long int)&dd5;
    long long int aaa[5] = {8,6618977,dd,dd5,rr};
    printf("%d\n", sizeof(dd));
    printf("%d\n", sizeof(aaa[0]));
    printf("%d\n", sizeof(&dd));
    printf("%lld\n", gg);
    printf("%lld\n", (int*)gg);
    printf("%lld\n", (int*)rr);
    //printf("%lld %lld %lld %lld %lld", doing[0], doing[1], doing[2], doing[3], doing[4]);
    B((void *)aaa);
}

void gsf(int bbs)
{
    printf("%d\n", bbs);
    printf("%d\n", bbs);
}


int main()
{
    int * port = (int*)malloc(sizeof(int));
    *port = 98;

    string g = "aaa";
    string h = "aa";
    printf("%d\n", g.length());
    if(g == h)
    {
        cout << "good" << endl;
    }
    else
    {
        cout << "bad" << endl;
    }
    

    /*string go = "hher";
    char* kk = (char *)"ere";
    string * theip = (string *)malloc(sizeof(string));
    new((void*)theip) string();
    cout << *theip << endl;
    *theip = go;*/
    cout << *port << endl;
    //A();
}