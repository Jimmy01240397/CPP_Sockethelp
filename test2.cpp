#include <iostream>
#include <list>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include <fstream>

using namespace std;

string execute(string cmd)
{
    string file_name = "result.txt" ;
    system((cmd + " > " + file_name).c_str()); // redirect output to file

    // open file for input, return string containing characters in the file
    ifstream file(file_name) ;
    file.seekg(0, std::ios::end);
    size_t length = file.tellg();
    file.seekg(0, std::ios::beg);

    char * buffer = new char[length];

    file.read(buffer, length);
    file.close();
    return *(new string(buffer));
}


int main()
{
    system("C:\\msys64\\usr\\bin\\bash.exe -l -c \"curl -s http://ifconfig.me/all.json\" > resultjq.txt");
    //string xxx = execute("C:\\msys64\\usr\\bin\\bash.exe -l -c \"curl -s http://ifconfig.me/all.json\"");
    string yyy = execute("jq \".user_agent\" resultjq.txt");
    printf("%s\n", yyy.c_str());
    system("pause");
}