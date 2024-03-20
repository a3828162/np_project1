#include<iostream>
#include<string>
#include<sstream>
#include<unistd.h>
#include<map>
#include<vector>
#include<queue>
#include<sys/wait.h>
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
4 : exclamationnumber ?1
*/

struct command
{
    command() : state(0), numberpipeleft(0), currentToken(""),
                commandType(0), operationType(0) {}
    int commandType;
    int operationType;
    string currentToken;
    vector<string> tokenArgument;
    int state;
    int numberpipeleft;
    vector<string> tokens;
};

queue<command> cmdqueue;
map<string, string> envs;

bool isBuildinCmd(command currentcmd){
    return currentcmd.tokens[0] == "setenv" || currentcmd.tokens[0] == "printenv" || currentcmd.tokens[0] == "exit";
}

bool isNumberPipe(string token){
    if(token.size()<2) return false;
    return (token[0]=='|'||token[0]=='!')&&isdigit(token[1]);
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

void forkandexec(command &cmd){

    int pid = fork();
    if(pid == 0) { // chld process
        int a = 0;
        if(cmd.tokenArgument.data() == 0){
            a = execlp(cmd.currentToken.c_str(), cmd.currentToken.c_str(), 
                NULL, NULL);
        } else {
            a = execlp(cmd.currentToken.c_str(), cmd.currentToken.c_str(), 
               cmd.tokenArgument.data()->c_str(), NULL);  
        }
        if(a==-1) cout << "error exec" << endl;
        cout << " from child process" << pid  << endl;
    } else { // parent process
        wait(NULL);
        cout << " from parent process" << pid  << endl;
    }
    //execlp(cmd.currentToken.c_str(), cmd.tokenArgument.data()->c_str());
}

void processToken(command &cmd){
    int i = 0;
    for(;i<cmd.tokens.size();++i) {
        if(cmd.currentToken == "") cmd.currentToken = cmd.tokens[i];
        else cmd.tokenArgument.push_back(cmd.tokens[i]);
        
        if(cmd.tokens[i][0] == '|' || i == cmd.tokens.size()-1) forkandexec(cmd);
    }
}

void processCommand(command &cmd){
    if(isBuildinCmd(cmd)){
        cmd.commandType = 1;
        if(cmd.tokens[0] == "setenv"){

            if(cmd.tokens.size() < 3){
                cout << "loss parameters" << endl;
                return;
            } 
            setenv(cmd.tokens[1].c_str(), cmd.tokens[2].c_str(), 1);
        } else if(cmd.tokens[0] == "printenv"){
            if(cmd.tokens.size()<2){
                cout << "loss parameters" << endl;
                return;
            } 

            cout << getenv(cmd.tokens[1].c_str()) << endl;
        } else if(cmd.tokens[0] == "exit"){
            exit(0);
        }
    } else {
        cmd.commandType = 2;
        processToken(cmd);
    }
}

void executable(){

    string cmdLine;
    stringstream ss;
    setenv("PATH" , "bin:.", 1);
    while(cout << "% " && getline(cin, cmdLine)){
        command currentcmd;
        
        cin.clear();
        ss << cmdLine;
        string token;
        vector<string> tmp;
        
        while(ss>>token){
            tmp.push_back(token);
        }

        for(int i=0;i<tmp.size();++i){
            currentcmd.tokens.push_back(tmp[i]);
            if(isNumberPipe(tmp[i])){
                processCommand(currentcmd);
                currentcmd = command{};
            } else if(i==tmp.size()-1){
                processCommand(currentcmd);
                currentcmd = command{};
            }
        }

        ss.clear();
    }
}

int main(){
    executable();
    
    return 0;
}