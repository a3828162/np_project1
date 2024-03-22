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
operationType
0 : init
1 : |
2 : >
3 : |1
4 : ?1
*/
/*void signal_child(int signal){
    int status;
    wait(&status);
    return;
}*/

class command
{
public:
    int commandType;
    int previosOP;
    int nextOP;
    int state;
    int numberpipeleft;
    string currentToken;
    vector<string> tokenArgument;
    vector<string> tokens;

    bool isNumberPipe(string token);
    bool isOPToken(string token);
    void setNextOP(string token);
    char **buildArgv();
    command(/* args */);
    ~command();
};

command::command(/* args */)
{
    state = 0;
    numberpipeleft = 0;
    currentToken = "";
    commandType = 0;
    nextOP = 0;
    previosOP = 0;
}

command::~command()
{
    tokens.clear();
    tokenArgument.clear();
}

bool command::isNumberPipe(string token){
    if(token.size()<2) return false;
    return (token[0]=='|'||token[0]=='!')&&isdigit(token[1]);
}

bool command::isOPToken(string token){
    if(token == "|" || token == ">"){
        return true;
    }else if(token.size() >= 2 &&
     (token[0] == '|' || token[0] == '!') &&
     isdigit(token[1])){
        return true;
    }

    return false;
}

void command::setNextOP(string token){
    if(token == "|"){
        nextOP = 1;
    } else if(token == ">"){
        nextOP = 2;
    } else if(token.size()>=2){
        if(token[0] == '|' && isdigit(token[1])) nextOP = 3;
        else if(token[0] = '!' && isdigit(token[1])) nextOP = 4;
    }
}

char **command::buildArgv(){
    int size = tokenArgument.size() + 2;
    char **argv = new char*[size];
    argv[0] = new char[currentToken.size()];
    strcpy(argv[0], currentToken.c_str());
    for(int i=1;i<size-1;++i){
        argv[i] = new char[tokenArgument[i].size()];
        strcpy(argv[i], tokenArgument[i-1].c_str());
    }
    argv[size - 1] = NULL;
    return argv;
}

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

void forkandexec(command &cmd){

    int pid = fork();
    if(pid == 0) { // chld process

        /*if(pipes.size()>0){
            cout << "fd[0]" << pipes[pipes.size()-1].fd[0] << "fd[1]" << pipes[pipes.size()-1].fd[1] << endl;
        }*/
        if(pipes.size()!=0){
            if(cmd.previosOP == 0 && cmd.nextOP != 0) {
                dup2(pipes[pipes.size()-1].fd[1], STDOUT_FILENO);
                close(pipes[pipes.size()-1].fd[1]);
                close(pipes[pipes.size()-1].fd[0]);
            } else if(cmd.previosOP != 0 && cmd.nextOP == 0){
                dup2(pipes[pipes.size()-1].fd[0], STDIN_FILENO);
                close(pipes[pipes.size()-1].fd[0]);
                //close(pipes[pipes.size()-1].fd[1]);
            } else if(cmd.previosOP != 0 && cmd.nextOP != 0){
                dup2(pipes[pipes.size()-2].fd[0], STDIN_FILENO);
                close(pipes[pipes.size()-2].fd[0]);
                dup2(pipes[pipes.size()-1].fd[1], STDOUT_FILENO);
                close(pipes[pipes.size()-1].fd[1]);
                close(pipes[pipes.size()-1].fd[0]);                
            }
        }

        int a = 0;
        char **argv = cmd.buildArgv();
        a = execvp(cmd.currentToken.c_str(), argv);
        if(a==-1) cerr << "Undefined command" << endl;
        exit(0);
    } else { // parent process
        if(pipes.size()!=0) {
            if(cmd.previosOP == 0 && cmd.nextOP != 0) {
                close(pipes[pipes.size()-1].fd[1]);
            } else if(cmd.previosOP != 0 && cmd.nextOP == 0){
                close(pipes[pipes.size()-1].fd[0]);
            } else if(cmd.previosOP != 0 && cmd.nextOP != 0){
                close(pipes[pipes.size()-2].fd[0]);
                close(pipes[pipes.size()-1].fd[1]);
            }
        }

        while(waitpid(-1, NULL, WNOHANG) == 0); 
    }
}

void processToken(command &cmd){

    // 記得把 parent process pipe fd要關掉
    int i = 0, leftPipeIndex = -1, rightPipeIndex = -1;

    for(;i<cmd.tokens.size();++i) {
        if(cmd.currentToken == "") cmd.currentToken = cmd.tokens[i];
        else if(!cmd.isOPToken(cmd.tokens[i])) cmd.tokenArgument.push_back(cmd.tokens[i]);
        //else if(cmd.tokens[i][0] != '|') cmd.tokenArgument.push_back(cmd.tokens[i]);
        
        //if(cmd.tokens[i][0] == '|' || i == cmd.tokens.size()-1){
        if(cmd.isOPToken(cmd.tokens[i]) || i == cmd.tokens.size()-1){
            
            cmd.previosOP = cmd.nextOP;
            if(i==cmd.tokens.size() - 1) cmd.nextOP = 0;
            else {
                pipes.push_back(pipestruct{});
                if(pipe(pipes[pipes.size()-1].fd) < 0) {
                    cerr << "create pipe fail" << endl;
                }
                 
                cmd.nextOP = 1;
            }
            
            forkandexec(cmd);
            cmd.currentToken = "";
            cmd.tokenArgument.clear();
        } 
    }
    /*for(int i=0;i<pipes.size();++i){
        close(pipes[i].fd[0]);
        close(pipes[i].fd[1]);
    }
    pipes.clear();*/
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
        
        ss << cmdLine;
        string token;
        vector<string> tmp;
        
        while(ss>>token){
            tmp.push_back(token);
        }

        for(int i=0;i<tmp.size();++i){
            currentcmd.tokens.push_back(tmp[i]);
            if(currentcmd.isNumberPipe(tmp[i]) || i == tmp.size() - 1){
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

// pipe example
    /*char *cm1[2] = {"ls", NULL};
    char *cm2[2] = {"cat", NULL};

    int f[2];
    if(pipe(f)<0) cerr << "error" << endl;
    if(fork()==0){
        //close(f[0]);
        dup2(f[1], STDOUT_FILENO);
        //close(f[1]);
        execvp(cm1[0],cm1);
    }else{
        if(fork()==0){
            dup2(f[0], STDIN_FILENO);
            close(f[0]);
            close(f[1]);
            execvp(cm2[0],cm2);
        }
        else {
            //close(f[0]);
            close(f[1]);
            wait(NULL);
            wait(NULL);
        }

    }*/