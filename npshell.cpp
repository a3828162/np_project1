#include<iostream>
#include<string>
#include<sstream>
#include<unistd.h>
#include<map>
#include<vector>
using namespace std;

/*
commandType
0 : init
1 : build in command
2 : operation command
*/

/*
tokenType
0 : init
1 : cat
2 : ls
3 : loop
4 : noop
5 : removetag
6 : removetag0
7 : setenv
8 : printenv
9 : exit
*/

/*
operationType
0 : init
1 : write >
2 : pipe |
3 : pipenumber |1
4 : exclamation ?
5 : exclamationnumber ?1
*/

struct command
{
    command() : state(0), numberpipeleft(0), currentTokenType(0),
                commandType(0), operationType(0), currentTokenaFileName("") {}
    int commandType;
    int operationType;
    int currentTokenType;
    string currentTokenaFileName;
    int state;
    int numberpipeleft;
    vector<string> tokens;
};

map<string, string> envs;

bool isBuildinCmd(command currentcmd){
    return currentcmd.tokens[0] == "setenv" || currentcmd.tokens[0] == "printenv" || currentcmd.tokens[0] == "exit";
}

void writetofile(){

}

void numberpipe(){

}

void pipe(){

}

void processexec(int type){
    switch (type)
    {
    case 1:
        
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    default:
        break;
    }
}

void processToken(command &cmd){
    int i = 0, pipeCommend = 0;
    for(int i=0;i<cmd.tokens.size();++i) {
        if(cmd.tokens[i] == "cat") {
            processexec(1);
        } else if(cmd.tokens[i] == "ls") {
            processexec(2);
        } else if(cmd.tokens[i] == "noop") {
            processexec(3);
        } else if(cmd.tokens[i] == "number") {
            processexec(4);
        } else if(cmd.tokens[i] == "removetag") {
            processexec(5);
        } else if(cmd.tokens[i] == "removetag0") {
            processexec(6);
        } else {

        }
    }
}

void processCommand(command &cmd){
    if(isBuildinCmd(cmd)){
        cmd.commandType = 1;
        if(cmd.tokens[0] == "setenv"){
            cmd.currentTokenType = 7;
            envs[cmd.tokens[1]] = cmd.tokens[2];
        } else if(cmd.tokens[0] == "printenv"){
            cmd.currentTokenType = 8;
            if(envs.find(cmd.tokens[1]) != envs.end()) cout << envs[cmd.tokens[1]] << endl;
        } else if(cmd.tokens[0] == "exit"){
            cmd.currentTokenType = 9;
            return;
        }
    } else {
        cmd.currentTokenType = 2;
        processToken(cmd);
    }
}

void executable(){

    string cmdLine;
    stringstream ss;

    while(cout << "% " && getline(cin, cmdLine)){
        command currentcmd;
        
        cin.clear();
        ss << cmdLine;
        string token;

        while(!ss.str().empty()){
            while(ss >> token ) {
                currentcmd.tokens.push_back(token);
                if((token[0] == 'i' || token[0] == 'i') && isdigit(token[1])){
                    processCommand(currentcmd);
                    if(currentcmd.currentTokenType == 9) return;
                    break;
                }
            }
        }
        if(currentcmd.currentTokenType == 9) return;
        processCommand(currentcmd);

        ss.clear();
    }
}

int main(){
    executable();
    
    return 0;
}