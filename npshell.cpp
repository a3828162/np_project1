#include<iostream>
#include<string>
#include<string.h>
#include<sstream>
#include<unistd.h>
#include<map>
#include<signal.h>
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
1 : >
2 : |
3 ; !
4 : |1
5 : ?1
*/
/*void signal_child(int signal){
    int status;
    wait(&status);
    return;
}*/

struct command
{
    command() : state(0), numberpipeleft(0), currentToken(""),
                commandType(0), previosOP(0), nextOP(0) {}
    int commandType;
    int previosOP;
    int nextOP;
    string currentToken;
    vector<string> tokenArgument;
    int state;
    int numberpipeleft;
    vector<string> tokens;
};

struct pipestruct
{
    pipestruct() : numberleft(0), pipetype(0) {}
    int numberleft; 
    int pipetype; // 1 : | , 2 : ! , 3 : |1 , 4 : !1
    int fd[2];
};

vector<pipestruct> pipes;
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

char **buildArgv(vector<string> v, string cmdname){
    int size = v.size() + 2;
    char **argv = new char*[size];
    argv[0] = new char[cmdname.size()];
    strcpy(argv[0], cmdname.c_str());
    for(int i=1;i<size-1;++i){
        argv[i] = new char[v[i].size()];
        strcpy(argv[i], v[i-1].c_str());
    }
    argv[size - 1] = NULL;
    return argv;
}

void forkandexec(command &cmd){

    int pid = fork();
    if(pid == 0) { // chld process
        
        //cout << "PreviosOp:" << cmd.previosOP << endl;
        //cout << "NextOp:" << cmd.nextOP << endl;
        switch (cmd.nextOP)
        {
        case 1:

            if(cmd.previosOP != 0) dup2(pipes[pipes.size()-1].fd[0], STDIN_FILENO);
            if(cmd.nextOP != 0) dup2(pipes[pipes.size()-1].fd[1], STDOUT_FILENO);
            break;
        default:
            if(cmd.previosOP != 0) dup2(pipes[pipes.size()-1].fd[0], STDIN_FILENO);
            break;
        }
        if(pipes.size()>0){
            close(pipes[pipes.size()-1].fd[0]);
            close(pipes[pipes.size()-1].fd[1]);
        }
        int a = 0;
        char **argv = buildArgv(cmd.tokenArgument, cmd.currentToken);
        a = execvp(cmd.currentToken.c_str(), argv);
        if(a==-1) cerr << "Undefined command" << endl;
        exit(0);
    } else { // parent process
        if(cmd.nextOP == 0 && pipes.size()!=0){
            close(pipes[pipes.size()-1].fd[0]);
            close(pipes[pipes.size()-1].fd[1]);
        }
        waitpid(-1, NULL, 0);
    }
}

void processToken(command &cmd){
    int i = 0;

    for(;i<cmd.tokens.size();++i) {
        if(cmd.currentToken == "") cmd.currentToken = cmd.tokens[i];
        else if(cmd.tokens[i][0] != '|') cmd.tokenArgument.push_back(cmd.tokens[i]);
        
        if(cmd.tokens[i][0] == '|' || i == cmd.tokens.size()-1){
            cmd.previosOP = cmd.nextOP;
            if(i==cmd.tokens.size() - 1) cmd.nextOP = 0;
            else {
                cmd.nextOP = 1;
                pipes.push_back(pipestruct{});
                if(pipe(pipes[pipes.size()-1].fd) < 0) {
                    cerr << "create pipe fail" << endl;
                } 
            }
            
            forkandexec(cmd);
            cmd.currentToken = "";
            cmd.tokenArgument.clear();
        } 
        
    }
}

void processCommand(command &cmd){
    if(isBuildinCmd(cmd)){
        cmd.commandType = 1;
        if(cmd.tokens[0] == "setenv"){

            if(cmd.tokens.size() < 3){
                cerr << "loss parameters" << endl;
                return;
            } 
            setenv(cmd.tokens[1].c_str(), cmd.tokens[2].c_str(), 1);
        } else if(cmd.tokens[0] == "printenv"){
            if(cmd.tokens.size()<2){
                cerr << "loss parameters" << endl;
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
    //void signal_child(int);
    //signal(SIGCHLD, signal_child);
    setenv("PATH" , "bin:.", 1);
    executable();
    
    return 0;
}