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
#include<fcntl.h>
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
    string redirectFileName;
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
    redirectFileName = "";
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
    int pipetype; // 0 : init, 1 : | , 2 : ! , 3 : |1 , 4 : !1
    int fd[2];
};

vector<pipestruct> pipes;
vector<pipestruct> numberPipes;

bool isBuildinCmd(command currentcmd){
    return currentcmd.tokens[0] == "setenv" || currentcmd.tokens[0] == "printenv" || currentcmd.tokens[0] == "exit";
}

int matchNumberPipeQueue(int left){
    for(int i=0;i<numberPipes.size();++i){
        if(left == numberPipes[i].numberleft) return i;
    }
    return -1;
}

void decreaseNumberPipeLeft(){
    for(int i = 0;i<numberPipes.size();++i){
        --numberPipes[i].numberleft;
        if(numberPipes[i].numberleft < 0){
            numberPipes.erase(numberPipes.begin()+i);
            --i;
        } 
    }
}

void forkandexec(command &cmd, int left){
    int pid = fork();
    if(pid < 0) {
        cerr << "fork error!" << endl;
    }else if(pid == 0) { // chld process

        /*if(pipes.size()>0){
            cout << "fd[0]" << pipes[pipes.size()-1].fd[0] << "fd[1]" << pipes[pipes.size()-1].fd[1] << endl;
        }*/

        for(int i=0;i<numberPipes.size();++i){
            if(numberPipes[i].numberleft == 0){
                cout << "FD:" << numberPipes[i].fd[0] << endl;
                close(numberPipes[i].fd[1]);
                dup2(numberPipes[i].fd[0], STDIN_FILENO);
                close(numberPipes[i].fd[0]); 
            }
        }

        if(pipes.size()!=0){
            if(cmd.previosOP == 0 && cmd.nextOP != 0) {
                
                /*if(cmd.nextOP == 3 || cmd.nextOP == 4){
                    dup2(pipes[pipes.size()-1].fd[1], STDOUT_FILENO);
                    if(cmd.nextOP == 4){
                        dup2(pipes[pipes.size()-1].fd[1], STDERR_FILENO);
                    } 
                    close(pipes[pipes.size()-1].fd[1]);
                    close(pipes[pipes.size()-1].fd[0]);
                }else {*/
                dup2(pipes[pipes.size()-1].fd[1], STDOUT_FILENO);
                close(pipes[pipes.size()-1].fd[1]);
                close(pipes[pipes.size()-1].fd[0]);
                //}
            } else if(cmd.previosOP != 0 && cmd.nextOP == 0){
                dup2(pipes[pipes.size()-1].fd[0], STDIN_FILENO);
                close(pipes[pipes.size()-1].fd[0]);
                //close(pipes[pipes.size()-1].fd[1]);
            } else if(cmd.previosOP != 0 && cmd.nextOP != 0){
                if(cmd.nextOP != 2){
                    if(cmd.nextOP == 3 || cmd.nextOP == 4){
                        int index = matchNumberPipeQueue(left);
                        if(index != -1){
                            dup2(numberPipes[index].fd[1], STDOUT_FILENO);
                            if(cmd.nextOP == 4) dup2(numberPipes[index].fd[1], STDERR_FILENO);
                            close(numberPipes[index].fd[1]);
                            close(numberPipes[index].fd[0]);
                        }   
                    }else{
                        dup2(pipes[pipes.size()-2].fd[0], STDIN_FILENO);
                        close(pipes[pipes.size()-2].fd[0]);
                        dup2(pipes[pipes.size()-1].fd[1], STDOUT_FILENO);
                        close(pipes[pipes.size()-1].fd[1]);
                        close(pipes[pipes.size()-1].fd[0]);  
                    }

                } else {
                    dup2(pipes[pipes.size()-1].fd[0], STDIN_FILENO);
                    close(pipes[pipes.size()-1].fd[0]);
                    int filefd = open(cmd.redirectFileName.c_str(), O_TRUNC | O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                    dup2(filefd, STDOUT_FILENO);
                    //close(fd);
                }
            }
        } else {
            if(cmd.previosOP == 0 && (cmd.nextOP == 3 || cmd.nextOP == 4) && left != 0){
                int index = matchNumberPipeQueue(left);
                if(index != -1){
                    dup2(numberPipes[index].fd[1], STDOUT_FILENO);
                    if(cmd.nextOP == 4) dup2(numberPipes[index].fd[1], STDERR_FILENO);
                    close(numberPipes[index].fd[1]);
                    close(numberPipes[index].fd[0]);
                }
            }else if(cmd.nextOP == 2){
                int filefd = open(cmd.redirectFileName.c_str(), O_TRUNC | O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
                dup2(filefd, STDOUT_FILENO);
                //close(fd);
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
                if(cmd.nextOP != 2){
                    if(cmd.nextOP == 3 || cmd.nextOP == 4){

                        //close(pipes[pipes.size()-1].fd[1]);
                        //numberPipes.push_back(pipes[pipes.size()-1]);
                    } else {
                        close(pipes[pipes.size()-1].fd[1]);
                    }
                }
            } else if(cmd.previosOP != 0 && cmd.nextOP == 0){
                close(pipes[pipes.size()-1].fd[0]);
            } else if(cmd.previosOP != 0 && cmd.nextOP != 0){
                if(cmd.nextOP != 2){
                    if(cmd.nextOP == 3 || cmd.nextOP == 4){
                        //close(pipes[pipes.size()-2].fd[0]);
                        close(pipes[pipes.size()-1].fd[0]);
                        //numberPipes.push_back(pipes[pipes.size()-1]);
                    } else {
                        close(pipes[pipes.size()-2].fd[0]);
                        close(pipes[pipes.size()-1].fd[1]);
                    }
                } else {
                    close(pipes[pipes.size()-1].fd[0]);
                }
            }
        }

        for(int i=0;i<numberPipes.size();++i){
            if(numberPipes[i].numberleft == 0){
                cout << "FD:" << numberPipes[i].fd[0] << endl;
                close(numberPipes[i].fd[1]);
                close(numberPipes[i].fd[0]); 
            }
        }

        while(waitpid(-1, NULL, WNOHANG) == 0); 

    }
}

void processToken(command &cmd){

    // 記得把 parent process pipe fd要關掉
    int i = 0, left = 0;

    for(;i<cmd.tokens.size();++i) {
        if(cmd.currentToken == "") cmd.currentToken = cmd.tokens[i];
        else if(!cmd.isOPToken(cmd.tokens[i])) cmd.tokenArgument.push_back(cmd.tokens[i]);
        
        if(cmd.isOPToken(cmd.tokens[i]) || i == cmd.tokens.size()-1){
            
            cmd.previosOP = cmd.nextOP;
            if(i==cmd.tokens.size() - 1 ){
                if(cmd.isNumberPipe(cmd.tokens[i])){
                    cmd.setNextOP(cmd.tokens[i]);
                    if(cmd.nextOP == 3 || cmd.nextOP ==4){
                        left = stoi(cmd.tokens[i].substr(1));
                        int inPipeQueue = matchNumberPipeQueue(left);
                        if(inPipeQueue == -1){
                            numberPipes.push_back(pipestruct{});
                            if(pipe(numberPipes[numberPipes.size()-1].fd)<0){
                                cerr << "pipe error!" << endl;
                            }
                            numberPipes[numberPipes.size()-1].pipetype = cmd.nextOP == 3 ? 3 : 4;
                            numberPipes[numberPipes.size()-1].numberleft = left;
                        }
                    } else {
                        pipes.push_back(pipestruct{});
                        if(pipe(pipes[pipes.size()-1].fd) < 0) {
                            cerr << "create pipe fail" << endl;
                        }
                    }
                } else {
                    cmd.nextOP = 0;
                }
            } 
            else {
                cmd.setNextOP(cmd.tokens[i]);
                if(cmd.nextOP != 2){
                    pipes.push_back(pipestruct{});
                    if(pipe(pipes[pipes.size()-1].fd) < 0) {
                        cerr << "create pipe fail" << endl;
                    }
                }
            }

            if(cmd.nextOP == 2){
                if(i+1 < cmd.tokens.size()){
                    cmd.redirectFileName = cmd.tokens[i+1];
                    cmd.tokens.pop_back();
                } 
            }
            
            forkandexec(cmd, left);
            cmd.currentToken = "";
            cmd.tokenArgument.clear();
        } 
    }
    decreaseNumberPipeLeft();
    pipes.clear();
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