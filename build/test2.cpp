#include <iostream>
#include <list>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include <fstream>
#include <json/json.h>
#pragma comment (lib, "libjsoncpp.a")

using namespace std;

string execute(string cmd)
{
    string file_name = "result.txt" ;
    system((cmd + " > " + file_name).c_str()); // redirect output to file

    // open file for input, return string containing characters in the file
    ifstream file(file_name) ;
    file.seekg(0, ios::end);
    size_t length = file.tellg();
    file.seekg(0, ios::beg);

    char * buffer = new char[length];

    file.read(buffer, length);
    file.close();
    return *(new string(buffer));
}


int main()
{
    ifstream file_input("resultjq.txt");
    file_input.seekg(0, ios::end);
    size_t length = file_input.tellg();
    file_input.seekg(0, ios::beg);
    char * buffer = new char[length];
    file_input.read(buffer, length);
    file_input.close();

    string rawJson(buffer);

    JSONCPP_STRING err;
    Json::CharReaderBuilder builder;
    Json::CharReader* reader(builder.newCharReader());
    Json::Value root;
    reader->parse(rawJson.c_str(), rawJson.c_str() + rawJson.length(), &root, &err);

    //cout << root << endl;

    cout << root["docs"][0]["anilist_id"] << endl;

    root["docs"][0]["anilist_id"] = "gewgr";

    //cout << root << endl;

    cout << root["docs"][0]["anilist_id"] << endl;

    Json::FastWriter wr;
    string xxx = wr.write(root["docs"][0]["anilist_id"]);
    cout << xxx << endl;
    //system("C:\\msys64\\usr\\bin\\bash.exe -l -c \"curl -s http://ifconfig.me/all.json\" > resultjq.txt"); 
    //string xxx = execute("C:\\msys64\\usr\\bin\\bash.exe -l -c \"curl -s http://ifconfig.me/all.json\""); 
    //string yyy = execute("jq \".user_agent\" resultjq.txt");
    //printf("%s\n", yyy.c_str());
    //system("pause");
}