#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
using namespace std;

string trim (string input){
    int i = 0;
    while (i < input.size() && input[i] == ' '){i++;}
    if (i < input.size()){input = input.substr(i);}
    else {return "";}

    i = input.size() - 1;
    while (i >= 0 && input[i] == ' '){--i;}
    if (i >= 0){input = input.substr(0, i+1);}
    else {return "";}

    return input;
}

char** vec_to_char_array(vector<string>& parts){
    char** result = new char* [parts.size() + 1];
    for (int i = 0; i < parts.size(); ++i){
        result[i] = (char*) parts[i].c_str();
    }
    result [parts.size()] = NULL;
    return result;
}

vector<string> split(string line, char separator = ' '){
    vector<string> split_str;
    bool quotes_exist = false;
    int num_quotes = 0;

    string str = "";
    for (int i = 0; i < line.size(); ++i){
        if (line[i] == '\'' || line[i] == '\"'){ // when we detect the quotes we say that 1 quote is found 
            quotes_exist = true;
            ++num_quotes;
        }
        if (num_quotes == 2){ // the second quote is found and we are stopping the process 
            quotes_exist = false;
            num_quotes = 0;
            if (separator != ' '){str += line[i];}
            if (i == line.size()-1){split_str.push_back(str);str = "";}
        }
        if (!quotes_exist && line[i] != '\'' && line[i] != '\"'){
            if (line[i] == separator){
                split_str.push_back(str);
                str = "";
            }
            else if (i == line.size()-1){
                str += line[i];
                split_str.push_back(str);
                break;
            }
            else {str += line[i];}
        }
        if (num_quotes == 1) {
            if (separator != ' '){str += line[i];} // when the first quote is found it will put the chars into the str and stop when the second quote is detected
            else if (line[i] != '\'' && line[i] != '\"'){str += line[i];}
        }
    }
    return split_str;
}

int main (){
    vector <int> bgs;
    int stdin = dup(0);
    int stdout = dup(1);

    char buf[10000];
    bool change_is_made = false;
    string prev_dir;
    string new_dir;

    while (true){

        dup2(stdin, 0); // making the stdin and stdout clean 
        dup2(stdout, 1);

        for (int i = 0; i < bgs.size(); ++i){
            if (waitpid(bgs[i], 0, WNOHANG) == bgs[i]){
                cout << "Process: " << bgs[i] << " ended" << endl;
                bgs.erase(bgs.begin()+i);
                i--;
            }
        }
        cout << "My Shell$ "; //print a prompt
        string inputline;
        getline (cin, inputline); //get a line from standard input
        if (inputline == string("exit")){
            cout << "Bye!! End of shell" << endl;
            break;
        }
        inputline = trim(inputline);
        bool bg = false;
        if (inputline[inputline.size()-1] == '&'){
            bg = true;
            inputline = inputline.substr(0, inputline.size()-1);
        }

        // first we need to check if there is change in the directory
        // need to check if there is cd in the string, and it is supposed to be at the beggining 
        // and it is not supposed to be in quotes, so index of cd == 0
        int pos = inputline.find("cd");
        if (pos == 0){
            int pos_2 = inputline.find('-');
            if (pos_2 != string::npos){
                if (!change_is_made){
                    printf("bash: cd: OLDPWD Not Set\n");
                }
                else{
                    string temp = new_dir;
                    new_dir = prev_dir;
                    prev_dir = temp;
                    printf("The old directory: %s\n", prev_dir.c_str());
                    chdir(new_dir.c_str());
                    printf("The new directory: %s\n", new_dir.c_str());
                }
            }
            else{
                prev_dir = getcwd(buf, sizeof(buf));
                printf("The old directory: %s\n", prev_dir.c_str());
                change_is_made = true;
                string dir_change = trim(inputline.substr(3));
                chdir(dir_change.c_str());
                new_dir = getcwd(buf, sizeof(buf));
                printf("The new directory: %s\n", new_dir.c_str());
            }
        }
        else{

            vector<string> parts = split(inputline, '|');

            for (int i = 0; i < parts.size(); ++i){
                            
                int fds[2];
                pipe(fds);
                int pid = fork ();
                if (pid == 0){ //child process

                    string temp_line = trim(parts[i]);

                    vector<string> parts_4 = split(temp_line, '<'); // check if the line contains the read symbol
                    string command = temp_line;
                    bool output_to_exists = false;
                    if (parts_4.size() > 1){
                        string read_file = trim(parts_4[1]);
                        command = trim (parts_4[0]);

                        vector<string> parts_5 = split(read_file, '>'); // here I check if the splitted line contain the write symbol
                        if (parts_5.size() > 1){
                            output_to_exists = true;
                            string output_file = trim(parts_5[1]);
                            read_file = trim(parts_5[0]);
                            int fd = open(read_file.c_str(), O_RDONLY);
                            if (fd == -1){
                                cout << "bash: " << read_file << ": No such file or directory" << endl;
                                return 0;
                            }
                            dup2(fd, 0);
                            fd = open(output_file.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRUSR);
                            dup2(fd, 1);
                            close(fd);
                        }
                        else {
                            int fd = open(read_file.c_str(), O_RDONLY, S_IRWXU | S_IRUSR);
                            if (fd == -1){
                                cout << "bash: " << read_file << ": No such file or directory" << endl;
                                return 0;
                            }
                            dup2(fd, 0);
                            close (fd);
                        }
                    }
                    if (!output_to_exists){
                        vector<string> parts_3 = split(temp_line, '>'); // check if the line contains the write symbol, cannot have the > a <, write to a and a read from something
                        //string output_file;
                        if (parts_3.size() > 1){
                            string output_file = trim(parts_3[1]);
                            command = trim(parts_3[0]);
                            int fd = open(output_file.c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU | S_IRUSR);
                            dup2(fd, 1);
                            close (fd);
                        }
                    }

                    if (i < parts.size()-1){
                        dup2(fds[1], 1);
                    }

                    vector<string> parts_2 = split(command);
                    char** args = vec_to_char_array(parts_2);
                    execvp (args [0], args);
                    return 0; // if the user types an incorrect command then exec is going to return and the process will not be finished 
                }
                else{
                    if (!bg && i == parts.size()-1){
                        waitpid(pid, 0, 0);
                    } //parent waits for child process
                    else {bgs.push_back(pid);}
                    dup2(fds[0], 0);
                    close(fds[1]);
                }
            }
        }
    }
    return 0;
}