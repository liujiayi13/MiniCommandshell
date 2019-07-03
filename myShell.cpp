#include "myShell.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
std::map <string, string> MyShell::LocalVars;               //set the map to store local variables
  extern char ** environ;
  
int main(int argc, char * argv[]) {
  do {
    MyShell myshell(environ);
    try{
    myshell.getCurPath();                 //get current working path
    myshell.getPath();                    //get all the path we have to search for command
    switch (myshell.ReadInput()) {          //read the input
      case 0: {
        return 0;    //exit
      }
      case 1: {
        continue;     //do other command not included in execve()
      }
      case 2: {
        myshell.DoCommand();    //do the command by execve()
      }
    }
    }
    catch(std::exception & err){     //catch exception
      cout<< err.what()<<endl;
      continue;}
  } while (1);
  return 0;
}
