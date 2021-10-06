// The MIT License (MIT)
//
// Copyright (c) 2016, 2017, 2021 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// 7f704d5f-9811-4b91-a918-57c1bb646b70
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
/*

   Name: Angel Delgado
   ID:

 */
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11    // Mav shell only supports five arguments

#define MAX_HISTORY_ITEMS 15    // Max number of commands that will be remembered in the history

#define MAX_PID_ITEMS_KEPT 15   // Max number of pids that will kept in history

//Structure used for the purpose of keeping track of the last 15 commands used
//the commands will be kept in a linked list for ease of shifting up and down
struct Node
{
  char *string;         // Command previously used
  int number;           // Current ranking in the linked list
  struct Node *next;    // Pointer to the next entry in the history
};

//Structure used for the purpose of keeping track of the last 15 pids of created children
//the pids will be kept in a linked list for ease of shifting up and down
struct pidNode
{
  int pid;
  int number;
  struct pidNode *next;
};

//function prototypes
void deleteNode(struct Node *head);
void trimNodes(struct Node *head);
void addNodeLast(struct Node *head, struct Node *current, char *str);
void printHistory(struct Node *head);
char *findStringInHistory(int x, struct Node *head);
void show(char x);
void sanitizeString(char * strPtr);
void printPids(struct pidNode *head);
void addPidLast(struct pidNode *head, struct pidNode *current, int add);
void deletePid(struct pidNode *head);
void trimPids(struct pidNode *head);

int main()
{
  char * cmd_str = (char *)malloc(MAX_COMMAND_SIZE); // pointer used to keep the input from the user
  int control = 1;                                   // variable used as a the control variable for the main loop
  struct Node *head = NULL;                          // pointer to the history linked list
  struct pidNode *pidHead = NULL;                    // pointer to the pids linked list
  while(control)
  {
    // clear the current buffer by setting it all to '\0'
    memset(cmd_str, '\0', MAX_COMMAND_SIZE);
    // Print out the msh prompt
    printf ("msh> ");
    
    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while(!fgets (cmd_str, MAX_COMMAND_SIZE, stdin));
    
    // send the buffer straight from the user to the sanitizeString() method in order to get
    // rid of any leading white space
    sanitizeString(cmd_str);
    
    // checks to see if the first character is a '!' if it is, try to exchange the current command
    // for the command with the corresponding number inputed after the '!' from the history
    if(strncmp(cmd_str, "!", 1) == 0)
    {
      char * tempFind = findStringInHistory(atoi(cmd_str + 1), head);
      if(tempFind != NULL)
        strcpy(cmd_str, tempFind);
      else
      {
        printf("Command not in history.\n");
        continue;
      }
    }
    
    // a cmd_str of length 1 would mean that the string is empty.
    // that input needs nothing to happen so the loop is reinitiated.
    if(strlen(cmd_str) < 2)
      continue;
    
    // initialize a node to keep track of the history if one doesn't already exists.
    // otherwise send the string to the addNodeLast method to add to the end of the linked list
    if(head == NULL)
    {
      head = (struct Node *)malloc(sizeof(struct Node));
      head->string = (char *)malloc(sizeof(char) * (strlen(cmd_str) + 1));
      strcpy(head->string, cmd_str);
      head->next = NULL;
      head->number = 0;
    }
    else
    {
      addNodeLast(head, head, cmd_str);
    }
    
    
    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];
    
    int token_count = 0;
    
    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr;
    
    char *working_str = strdup( cmd_str );
    
    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;
    
    // Tokenize the input strings with whitespace used as the delimiter
    while ((token_count < MAX_NUM_ARGUMENTS) &&
           ((argument_ptr = strsep(&working_str, WHITESPACE )) != NULL))
    {
      token[token_count] = strndup( argument_ptr, MAX_COMMAND_SIZE );
      if(strlen( token[token_count] ) == 0)
      {
        free(token[token_count]);
        token[token_count] = NULL;
      }
      token_count++;
    }
    
    
    //if there are any tokenized inputs proceed
    if(token_count)
    {
      // if the input is quit or stop exit out of the loop after deallocating the dynamic memory
      if((strcmp(token[0], "quit") == 0 || strcmp(token[0], "stop") == 0) && token[1] == NULL)
      {
        control = 0;
      }
      //
      else if(strcmp(token[0], "history") == 0 && token[1] == NULL)
      {
        printHistory(head);
      }
      else if(strcmp(token[0], "cd") == 0)
      {
        if(chdir(token[1]) != 0)
          printf("No such directory.\n");
      }
      else if(strcmp(token[0], "listpids") == 0 && token[1] == NULL)
      {
        if(pidHead == NULL)
        {
          printf("No child processes have been created in this instance.\n");
        }
        else
        {
          printPids(pidHead);
        }
      }
      else
      {
        int pid = fork();
        if(pid == 0)
        {
          int ret = execvp(token[0], &token[0]);
          if(ret == -1)
            printf("%s: Command not found.\n", token[0]);
          control = 0;
        }
        else
        {
          int status;
          wait(&status);
          if(pidHead == NULL)
          {
            pidHead = (struct pidNode *)malloc(sizeof(struct pidNode));
            pidHead->pid = pid;
            pidHead->next = NULL;
            pidHead->number = 0;
          }
          else
          {
            addPidLast(pidHead, pidHead, pid);
          }
          
        }
      }
    }
    
    
    
    //from this point on, all that happens is the deallocation of all dynamic memory
    for(int token_index = 0; token_index < token_count; token_index++)
    {
      free(token[token_index]);
    }
    
    free(working_root);
    free(argument_ptr);
    
  }
  deletePid(pidHead);
  deleteNode(head);
  free(cmd_str);
  return 0;
}

// accepts a char pointer and will get rid of any leading white spoce by shifting the whole string
// in place. no return type as everything is done on the same pointer.
void sanitizeString(char * strPtr)
{
  while(*strPtr == ' ' || *strPtr == '\t')
  {
    for(int i = 0; i < MAX_COMMAND_SIZE - 1; i++)
    {
      *(strPtr + i) = *(strPtr + i + 1);
    }
    *(strPtr + MAX_COMMAND_SIZE - 1) = '\0';
  }
}

// helper function for testing. prints a line of any char passed in
void show(char x)
{
  for(int i = 0; i < 10; i++)
    printf("%c", x);
  printf("\n");
}

// the head of the history linked list and a number get passed in
// if the number passed in matches any node's number then the string it holds gets returned
char *findStringInHistory(int x, struct Node *head)
{
  if(head == NULL)
    return NULL;
  if(head->number == x - 1)
    return head->string;
  return findStringInHistory(x, head->next);
}

// when passed in a Node pointer it will recursively go through all the nodes in the linked list
// and decrease the number internal variable of all of them for the purpose of only considering
// MAX_HISTORY_ITEMS of them for printing and retrieving
void trimNodes(struct Node *head)
{
  if(head == NULL)
    return;
  head->number = head->number - 1;
  trimNodes(head->next);
}

// when a Node pointer gets passed in. it will recursively deallocate and delete all data
// following the passed in Node in the linked list.
// used to fully deallocate the list.
void deleteNode(struct Node *head)
{
  if(head == NULL)
    return;
  deleteNode(head->next);
  free(head->string);
  head->string = NULL;
  free(head);
  head = NULL;
}

// this function originally receives 2 copies of the starting pointer of the linked list
// and a string that will be added to the linked list.
// the funcion dynamically allocates the needed memory for the string, and calculates the
// number in line it needs. if the number of currently tracked history items exceedes
// MAX_HISTORY_ITEMS then the function trimNodes() gets called to decrement all items in the
// linked list to keep track of only MAX_HISTORY_ITEMS of them.
// any new nodes created will be added to the end of the list.
void addNodeLast(struct Node *head, struct Node *current, char *str)
{
  if(current->next == NULL)
  {
    struct Node *res = (struct Node *)malloc(sizeof(struct Node));
    res->string = (char *)malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(res->string, str);
    res->number = current->number + 1;
    res->next = NULL;
    current->next = res;
    if(res->number == MAX_HISTORY_ITEMS)
      trimNodes(head);
  }
  else
  {
    addNodeLast(head, current->next, str);
  }
}

// this function accepts the root of the history linked list.
// the function will recursively print the history items in the linked list
// whose number variable is bigger than -1. this is why decrementing the number of all history
// items functions as a way to not print them/use them
void printHistory(struct Node *head)
{
  if(head == NULL)
    return;
  if(head->number > -1)
    printf("%d: %s", head->number + 1, head->string);
  printHistory(head->next);
}

// this function accepts the root of the pids linked list.
// the function will recursively print the pids in the linked list
// whose number variable is bigger than -1. this is why decrementing the number of all pid
// items functions as a way to not print them/use them
void printPids(struct pidNode *head)
{
  if(head == NULL)
    return;
  if(head->number > -1)
    printf("%d: %d\n", head->number + 1, head->pid);
  printPids(head->next);
}

// this function originally receives 2 copies of the root of the pids linked list
// and a number 'add' that will be added to the linked list.
// the funcion dynamically allocates the needed memory for the node, and calculates the
// number in line it needs. if the number of currently tracked pids exceedes
// MAX_PID_ITEMS_KEPT then the function trimPids() gets called to decrement all items in the
// linked list to keep track of only MAX_PID_ITEMS_KEPT of them.
// any new nodes created will be added to the end of the list.
void addPidLast(struct pidNode *head, struct pidNode *current, int add)
{
  if(current->next == NULL)
  {
    struct pidNode *res = (struct pidNode *)malloc(sizeof(struct pidNode));
    res->pid = add;
    res->number = current->number + 1;
    res->next = NULL;
    current->next = res;
    if(res->number == MAX_PID_ITEMS_KEPT)
      trimPids(head);
  }
  else
  {
    addPidLast(head, current->next, add);
  }
}

// when a Node pointer gets passed in. it will recursively deallocate and delete all data
// following the passed in Node in the linked list.
// used to fully deallocate the list.
void deletePid(struct pidNode *head)
{
  if(head == NULL)
    return;
  deletePid(head->next);
  free(head);
  head = NULL;
}

// when passed in a pidNode pointer it will recursively go through all the nodes in the linked list
// and decrease the number internal variable of all of them for the purpose of only considering
// MAX_PID_ITEMS_KEPT of them for printing
void trimPids(struct pidNode *head)
{
  if(head == NULL)
    return;
  head->number = head->number - 1;
  trimPids(head->next);
}
