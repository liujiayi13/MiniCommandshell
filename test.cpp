#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

void getFileVector(char * basePath, std::vector<std::string> & fileVector) {
  //std::vector<std::string> newVector;
  struct dirent * dp;
  DIR * dir = opendir(basePath);
  //cannot open directory
  //if (dir == NULL) {
  //  std::cerr << "Cannot open directory\n";
  //  exit(EXIT_FAILURE);
  //}
  while ((dp = readdir(dir)) != NULL) {
    //if it is a directory
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
      continue;
    }
    if (dp->d_type == DT_DIR) {
      //construct new path
      size_t sz = strlen(dp->d_name);
      char * path = new char[strlen(basePath) + sz + 2];
      strcpy(path, basePath);
      strcat(path, "/");
      strcat(path, dp->d_name);
      getFileVector(path, fileVector);
      delete[] path;
    }
    else if (dp->d_type == DT_REG) {
      //put the file name into the vector
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

string parseCommand(string pre, vector<string> fileVector) {
  if (pre[0] == '.' || pre[0] == '/') {
    return pre;
  }
  else {
    for (vector<string>::iterator it = fileVector.begin(); it != fileVector.end(); ++it) {
      size_t pos = (*it).rfind(pre);
      if (pos != string::npos) {
        if ((*it)[pos - 1] == '/' && (pos = (*it).size() - 1)) {
          return *it;
        }
      }
    }
    cout << "Command " << pre << " not found" << endl;
    exit(EXIT_FAILURE);
  }
}

bool isnum(string str) {  //judge if a string's content is number
    if((str[0] < '0' || str[0] > '9') && (str[0] != '-'))
    {
        return false;
    }
    for (string::iterator iter = str.begin()+1; iter < str.end(); ++iter) {
      if (*iter < '0' || *iter > '9') {
        return false;
      }
    }
    return true;
  }

void SplitString(const string & s, vector<string> & res, const string & c) {
  string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while (string::npos != pos2) {
    if (s[pos2 - 1] != '\\') {
      res.push_back(s.substr(pos1, pos2 - pos1));

      pos1 = pos2 + c.size();
      pos2 = s.find(c, pos1);
    }
    else {
      pos2 = s.find(c, pos2 + 1);
    }
  }
  if (pos1 != s.length()) {
    res.push_back(s.substr(pos1));
  }

  vector<string>::iterator iter = res.begin();
  while (iter < res.end()) {
    if (*iter == "\0") {
      res.erase(iter, iter + 1);
    }
    else {
      iter++;
    }
  }
  for (iter = res.begin(); iter < res.end(); ++iter) {
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

void SetIO(vector<string> Command)  //redirection
  {
    vector<string>::iterator pos;
    pos = find(Command.begin(), Command.end(), " > ");
    if (pos != Command.end()) {
      if ((pos == Command.end() - 1)) {
        throw invalid_argument("Error : need file for redirection");
      }
      string File = *(pos + 1);
      Command.erase(pos, pos + 2);
      int Fileout = open(File.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU);
      if (Fileout == -1) {
        perror("Can't open the file");
        //     throw SetIOException();
      }
      dup2(Fileout, STDOUT_FILENO);
      close(Fileout);
    }
  }

int main(int argc, char * argv[]) {
  //  std::vector<std::string> fileVector;
  //  getFileVector(argv[1], fileVector);
  //  for (unsigned int i = 0; i < fileVector.size(); i++)
  //  std::cout << fileVector[i] << std::endl;
  //   extern char ** environ;
  //   int i = 0;
  //   while (environ[i] != NULL) {
  //     printf("%s\n", environ[i]);
  //     i++;
  //   }
  //  string res = parseCommand("ls", fileVector);
  //  cout << res << endl;
  //  char buf[80];
  //  getcwd(buf, sizeof(buf));
  //  printf("current working directory: %s\n", buf);
  //  string pwd = buf;
  //  cout << pwd << endl;
  //  cout << getenv("HOME") << endl;
  //  map<string, string> test;
  //  test["1"] = "liu";
  //  test["2"] = "jia";
  //  test["3"] = "yi";
  //  map<string, string>::iterator iter,iter2;
  //  iter = test.begin();
  //  iter2 = test.end();
  //  cout << iter2->first  << endl;
  //string str = "12s";
  //int i = atoi(str.c_str());
  //cout << i <<endl;
  // cout << "\\" << endl;
  //vector<string> res;
  //string result;
  //string Input;
  //getline(cin, Input);
  //SplitString(Input, res, " ");
  //  cout << (res[1] == "\0") << endl;
  //  stringstream input(Input);
  //  while (input >> result) {
  //    res.push_back(result);
  //  }
  //  vector<string>::iterator iter1 = res.begin();
  //  while (iter1 < res.end()) {
  //    size_t i = (*iter1).find("\\", 0);
  //    if (i == ((*iter1).size() - 1)) {
  //      string temp = *iter1;
  //      string temp1 = temp.substr(0, temp.size()-1);
  //      res.erase(iter1, iter1 + 1);
  //      *iter1 = temp1 + " " + *iter1;
  //    }
  //    else {
  //      iter1++;
  //    }
  //  }

  //for (vector<string>::iterator iter = res.begin(); iter < res.end(); ++iter) {
  // cout << *iter << endl;
  //}
  //int fd;
 // int refd;
  //string a = "Advanced Programming!\n";
  //const char * buf = a.c_str();
 // fd = open("test.txt", O_RDWR | O_CREAT, 0644);
 // if (fd == -1) {
  //  printf("open file error:\n");
   // exit(-1);
//  }
 // refd = dup2(fd, fileno(stdout));
 // if (refd == -1) {
 //   printf("redirect standard out error:\n");
 //   exit(-1);
  //}
 // cout << "liu jia yi" << endl;
  //write(fileno(stdout), buf, strlen(buf));
  //close(fd);
  //vector<string> Command;
  //Command.push_back("ls");
  //Command.push_back(">");
  string a = "-124";
  cout << isnum(a) << endl;
  return 0;
}
