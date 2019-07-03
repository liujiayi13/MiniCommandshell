#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class MyShell
{
  vector<string> Command;                     //input string vector after processing
  vector<string> ECE551PATH;                  //all the path we need for searching for command
  char ** environment;                        //environment
  string curPath;                             //current working path
  static std::map<string, string> LocalVars;  //store local variables

 public:
  MyShell(char ** env) : environment(env) {}
  ~MyShell() {}
  int ReadInput() {
    cout << "myShell:" << curPath << " $ ";
    string Input;
    getline(cin, Input);
    SplitString(Input, " ");  //read input and split it by space
    if (Command.size() == 0) {
      throw invalid_argument("Error : please input command");
    }
    ParseVar();                               //parse "$var" to var's corresponding value
    if (cin.eof() || Command[0] == "exit") {  //exit the program
      return 0;
    }
    else if (Command[0] == "cd") {  //command cd
      ChangeDir();
      return 1;
    }
    else if (Command[0] == "set") {  //command set
      SetVar();
      return 1;
    }

    else if (Command[0] == "inc") {  //increase variable
      IncVar();
      return 1;
    }

    else if (Command[0] == "export") {  //export variable
      ExportVar();
      return 1;
    }
    else {
      return 2;
    }
  }

  void DoCommand() {
    pid_t pid = fork();  //generate child process
    if (pid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    else if (pid == 0) {  //child process
      try {
        SetIO();  //redirection
        ChildProcess();
      }
      catch (exception & err) {
        cout << err.what() << endl;
        exit(EXIT_FAILURE);
      }
    }
    else {
      FatherProcess(pid);  //father process
    }
  }

  void ChildProcess() {
    Command[0] = parseCommand(Command[0]);  //parse command(for example, translate ls to /bin/ls)
    vector<char *> Command_c;
    for (std::vector<std::string>::iterator it = Command.begin(); it != Command.end(); ++it) {
      Command_c.push_back(&(*it)[0]);
    }
    Command_c.push_back(NULL);  //execve() command need NULL as the last string
    execve(Command_c[0], &Command_c[0], environment);
    perror("execve");  // execve() returns only on error
    exit(EXIT_FAILURE);
  }

  void FatherProcess(pid_t pid) {
    pid_t w;
    int wstatus;
    if (pid > 0) {
      do {
        w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);  //wait for the child process to end
        if (w == -1) {
          perror("waitpid");
          exit(EXIT_FAILURE);
        }
        if (WIFEXITED(wstatus)) {
          printf("Program exited with status %d\n", WEXITSTATUS(wstatus));
        }
        else if (WIFSIGNALED(wstatus)) {
          printf("Program was killed by signal %d\n", WTERMSIG(wstatus));
        }
      } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    }
  }

  void getFileVector(
      char * basePath,
      std::vector<std::string> & fileVector) {  //from hw98, use this function to get the full path;
    struct dirent * dp;
    DIR * dir = opendir(basePath);
    if (dir == NULL) {
        if (errno == ENOENT) {
            perror("Not a valid path");
            return;
        } else {
            perror("Opendir error");
            return;
        }
    }
    while ((dp = readdir(dir)) != NULL) {
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
        continue;
      }
      if (dp->d_type == DT_DIR) {
        size_t sz = strlen(dp->d_name);
        char * path = new char[strlen(basePath) + sz + 2];
        strcpy(path, basePath);
        strcat(path, "/");
        strcat(path, dp->d_name);
        getFileVector(path, fileVector);
        delete[] path;
      }
      else if (dp->d_type == DT_REG) {
        char * filename = new char[strlen(basePath) + strlen(dp->d_name) + 2];
        strcpy(filename, basePath);
        strcat(filename, "/");
        strcat(filename, dp->d_name);
        std::string newFile(filename);
        fileVector.push_back(newFile);
        delete[] filename;
      }
      else {
      }
    }
    closedir(dir);
  }

  void getPath() {  //get all the path we have to search for
    string path = getenv("PATH");
    vector<string> allpath;
    size_t pos1 = 0;  //separated by ":" to get the environment path
    size_t pos2 = path.find(":");
    while (pos2 != string::npos) {
      allpath.push_back(path.substr(pos1, pos2 - pos1));
      pos1 = pos2 + 1;
      pos2 = path.find(":", pos1);
    }
    if (pos1 != path.size()) {
      allpath.push_back(path.substr(pos1));
    }
    vector<char *> allpath_c;
    for (std::vector<std::string>::iterator it = allpath.begin(); it != allpath.end(); ++it) {
      allpath_c.push_back(&(*it)[0]);
    }
    for (unsigned int i = 0; i < allpath.size(); i++) {  //use funtion getFileVector
      getFileVector(allpath_c[i], ECE551PATH);
    }
  }
  void printPath() {  //just for test, not use in the shell
    for (unsigned int i = 0; i < ECE551PATH.size(); i++) {
      cout << ECE551PATH[i] << endl;
    }
  }

  string parseCommand(string pre) {  //parse command(for example, translate ls to /bin/ls)
    if (pre[0] == '.' || pre[0] == '/') {
      return pre;
    }
    else {
      for (vector<string>::iterator it = ECE551PATH.begin(); it != ECE551PATH.end(); ++it) {
        size_t pos = (*it).rfind(pre);
        if (pos != string::npos) {
          if ((*it)[pos - 1] == '/' && (pos == (*it).size() - pre.size())) {
            return *it;
          }
        }
      }
      cout << "Command " << pre << " not found" << endl;  //if not found, print error
      exit(EXIT_FAILURE);
    }
  }

  void getCurPath() {  //get current working path
    char curPath_c[80];
    if (getcwd(curPath_c, sizeof(curPath_c)) == NULL) {
      throw invalid_argument("Error : Can't get working path");
    }
    curPath = curPath_c;
  }

  void SetVar() {  //set new variable to its required value
    if (Command.size() != 3) {
      throw invalid_argument("Error : need one variable and one value");
    }
    string var = Command[1];
    string value = Command[2];
    for (string::iterator iter = var.begin(); iter < var.end(); ++iter) {
      if (((*iter < 65) || (*iter > 90 && *iter < 97) || (*iter > 122)) && (*iter != '_')) {
        throw invalid_argument(
            "Error: variable name must be combination of letters, underscore and numbers");
      }
    }

    LocalVars[var] = value;
  }

  size_t FindVar(string Input, size_t pos, int num) {  //find local variable in string temp
    map<string, string>::iterator iter;
    string Input_sub;
    for (size_t a = 1; a <= Input.size(); a++) {
      Input_sub = Input.substr(0, a);
      iter = LocalVars.find(Input_sub);
      if (iter != LocalVars.end()) {
        Command[num].replace(pos, Input_sub.size() + 1, iter->second);
        pos = pos + iter->second.size();
        return pos;
      }
    }
    throw invalid_argument("Error : Can't find this variable");
  }

  bool isnum(string str) {  //judge if a string's content is number
    if ((str[0] < '0' || str[0] > '9') && (str[0] != '-')) {
      return false;
    }
    for (string::iterator iter = str.begin() + 1; iter < str.end(); ++iter) {
      if (*iter < '0' || *iter > '9') {
        return false;
      }
    }
    return true;
  }

  string int2str(int num) {  //convert int to string
    stringstream stream;
    stream << num;
    string str = stream.str();
    return str;
  }

  void SplitString(const string & str,
                   const string & sign) {  //split string s by string c and treat \_ as _
    string::size_type pos1, pos2;
    pos2 = str.find(sign);
    pos1 = 0;
    while (string::npos != pos2) {
      if (str[pos2 - 1] != '\\') {
        Command.push_back(str.substr(pos1, pos2 - pos1));

        pos1 = pos2 + sign.size();
        pos2 = str.find(sign, pos1);
      }
      else {
        pos2 = str.find(sign, pos2 + 1);
      }
    }
    if (pos1 != str.length()) {
      Command.push_back(str.substr(pos1));
    }

    vector<string>::iterator iter = Command.begin();
    while (iter < Command.end()) {
      if (*iter == "\0") {
        Command.erase(iter, iter + 1);
      }
      else {
        iter++;
      }
    }
    for (iter = Command.begin(); iter < Command.end(); ++iter) {
      string temp = *iter;
      int a = 0;
      size_t i = 0;
      while (temp.find("\\", a) != string::npos) {
        i = temp.find("\\", a);
        if (temp[i + 1] == ' ') {
          temp = temp.erase(i, 1);
        }
      }
      *iter = temp;
    }
  }

  void ChangeDir()  //change working path
  {
    if (Command.size() == 1) {
      chdir(getenv("HOME"));
    }
    else if (Command.size() == 2) {
      if (chdir(Command[1].c_str()) == -1) {
        throw invalid_argument("Error : Can't find the path");
      }
    }
    else {
      throw invalid_argument("Error: Command cd need less than 2 parameters");
    }
  }

  void ParseVar()  //parse var(for example, translate $a to a's corresponding value)
  {
    for (unsigned int num = 0; num < Command.size(); num++) {
      size_t pos = 0;
      string Input_Sub;
      while (Command[num].find("$", pos) != string::npos) {
        pos = Command[num].find("$", pos);
        string::size_type i = Command[num].find("$", pos + 1);
        if (i != string::npos) {
          Input_Sub = Command[num].substr(pos + 1, i - pos - 1);
        }
        else {
          Input_Sub = Command[num].substr(pos + 1);
        }
        pos = FindVar(Input_Sub, pos, num);
      }
    }
  }

  void IncVar()  //increase a variable by 1
  {
    if (Command.size() == 1)
    {
      throw invalid_argument("Error : need varibale for increase");
    }
    if (Command.size() > 2) {
      throw invalid_argument("Error : You can only increase 1 variable");
    }
    map<string, string>::iterator iter = LocalVars.find(Command[1]);
    if (iter == LocalVars.end()) {
      throw invalid_argument("Error : Can't find the variable");
    }
    string value = iter->second;
    if (isnum(value)) {
      int num = atoi(value.c_str());
      num++;
      LocalVars[Command[1]] = int2str(num);
    }
    else {
      LocalVars[Command[1]] = "1";
    }
  }

  void ExportVar()  // export the variable to environment
  {
    if (Command.size() < 2) {
      throw invalid_argument("Error : please input the variable you want to export ");
    }
    else if (Command.size() > 2) {
      throw invalid_argument("Error : you can only export 1 variable every time");
    }
    else {
      map<string, string>::iterator iter = LocalVars.find(Command[1]);
      if (iter == LocalVars.end()) {
        throw invalid_argument("Error : can't find this variable ");
      }
      if ((setenv(Command[1].c_str(), (iter->second).c_str(), 1)) != 0) {
        throw invalid_argument("Export error !");
      }
    }
  }

  void SetIO()  //redirection
  {
    vector<string>::iterator pos;
    pos = find(Command.begin(), Command.end(), ">");
    if (pos != Command.end()) {
      int RedirectFile = GetRedirectFile(pos, ">");
      dup2(RedirectFile, STDOUT_FILENO);
      close(RedirectFile);
    }

    pos = find(Command.begin(), Command.end(), "<");
    if (pos != Command.end()) {
      int RedirectFile = GetRedirectFile(pos, "<");
      dup2(RedirectFile, STDIN_FILENO);
      close(RedirectFile);
    }

    pos = find(Command.begin(), Command.end(), "2>");
    if (pos != Command.end()) {
      int RedirectFile = GetRedirectFile(pos, "2>");
      dup2(RedirectFile, STDERR_FILENO);
      close(RedirectFile);
    }
  }

  int GetRedirectFile(vector<string>::iterator pos, string Redirection) {
    if ((pos == Command.end() - 1)) {
      throw invalid_argument("Error : need file for redirection");
    }
    string File = *(pos + 1);
    Command.erase(pos, pos + 2);
    if (find(Command.begin(), Command.end(), Redirection) != Command.end()) {
      string error = "Error : only need one " + Redirection + " redirection";
      throw invalid_argument(error);
    }
    int RedirectFile = open(File.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
    if (RedirectFile == -1) {
      perror("Can't open the file");
      //     throw SetIOException();
    }
    return RedirectFile;
  }
};
