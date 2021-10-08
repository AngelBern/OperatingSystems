// The MIT License (MIT)
//
// Copyright (c) 2020 Trevor Bakker
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
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
/*

   Name:
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
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 4

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

void sanitizeString(char * strPtr);
void show(char x);
void printInfo();
void populateInfo();
void populateDirArr();
void ls();
int LBAToOffset(int32_t sector);
int16_t NextLB(uint32_t sector);
void stat(char *str);
int findString(char * str);
int cd(char *str);
void cdMulti(char *str);
void fatRead(char *name, char *pos, char *byt);
void fatGet(char * str);

FILE *filePtr = NULL;
int16_t BPB_BytsPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATs;
int32_t BPB_FATSz32;
void cdMulti(char *str);


int main()
{
  char * cmd_str = (char *)malloc(MAX_COMMAND_SIZE);  // pointer used to keep the input from the user
  int control = 1;
  while(control)
  {
    // clear the current buffer by setting it all to '\0'
    memset(cmd_str, '\0', MAX_COMMAND_SIZE);
    // Print out the mfs prompt
    printf ("mfs> ");
    
    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while(!fgets (cmd_str, MAX_COMMAND_SIZE, stdin));
    
    // send the buffer straight from the user to the sanitizeString() method in order to get
    // rid of any leading white space
    sanitizeString(cmd_str);
    
    // a cmd_str of length 1 would mean that the string is empty.
    // that input needs nothing to happen so the loop is reinitiated.
    if(strlen(cmd_str) < 2)
      continue;
    
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
      if((strcmp(token[0],
                 "exit") == 0 ||
          strcmp(token[0], "quit") == 0 || strcmp(token[0], "stop") == 0) && token[1] == NULL)
      {
        control = 0;
      }
      
      if(strcmp(token[0], "open") == 0)
      {
        if(filePtr != NULL)
          printf("Error: File system image already open.\n");
        else
        {
          filePtr = fopen(token[1], "r");
          if(filePtr == NULL)
            printf("Error: File system image not found.\n");
          else
          {
            populateInfo();
            populateDirArr();
          }
        }
      }
      if(strcmp(token[0], "info") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else printInfo();
      }
      if(strcmp(token[0], "ls") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else ls();
      }
      if(strcmp(token[0], "close") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else
        {
          fclose(filePtr);
          filePtr = NULL;
        }
      }
      if(strcmp(token[0], "stat") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else stat(token[1]);
      }
      if(strcmp(token[0], "cd") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else cd(token[1]);
      }
      if(strcmp(token[0], "read") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else fatRead(token[1], token[2], token[3]);
      }
      if(strcmp(token[0], "get") == 0)
      {
        if (filePtr == NULL)
        {
          printf("Error: File system not open.\n");
        }
        else fatGet(token[1]);
      }
    }
    
    
    
    //from this point on, all that happens is the deallocation of all dynamic memory
    for(int token_index = 0; token_index < token_count; token_index++)
    {
      free(token[token_index]);
    }
    free( working_root );
    
  }
  free(cmd_str);
  if(filePtr != NULL)
  {
    fseek(filePtr, 0, SEEK_SET);
    fclose(filePtr);
  }
  return 0;
}

void stat(char *str)
{
  int i = findString(str);
  if(i == -1)
  {
    printf("Error: File not found.\n");
    return;
  }
  printf("Attribute: \t 0x%x\n", dir[i].DIR_Attr);
  printf("Cluster number:\t %d\n", dir[i].DIR_FirstClusterLow);
  if(dir[i].DIR_Attr == 0x10)
    printf("Size: \t\t 0\n");
  else
    printf("Size: \t\t %d\n", dir[i].DIR_FileSize);
}

void ls()
{
  for(int i = 0; i < 16; i++)
  {
    char filename[12];
    strncpy(filename, &dir[i].DIR_Name[0], 11);
    filename[11] = '\0';
    if(dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
      printf("%s\n", filename);
  }
}

int findString(char * str)
{
  if(strlen(str) > 11 || strlen(str) < 1)
    return -1;
  for(int i = 0; i < 16; i++)
  {
    if(dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
    {
      char filename[12];
      strncpy(filename, &dir[i].DIR_Name[0], 11);
      filename[11] = '\0';
      char normalized[12];
      normalized[11] = '\0';
      memset(&normalized, ' ', 11);
      if(strcmp(str, "..") == 0)
      {
        normalized[0] = '.';
        normalized[1] = '.';
      }
      else
      {
        if(dir[i].DIR_Attr == 0x20)
        {
          for(int i = strlen(str) - 1; i >= 0; i--)
          {
            if(*(str + i) == '.')
              break;
            normalized[11 - (strlen(str) - i)] = toupper(*(str + i));
          }
        }
        for(int i = 0; i < strlen(str); i++)
        {
          if(*(str + i) == '.')
            break;
          normalized[i] = toupper(*(str + i));
        }
      }
      if(strcmp(filename, normalized) == 0)
      {
        return i;
      }
    }
  }
  return -1;
}

void populateDirArr()
{
  fseek(filePtr,
        ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec)),
        SEEK_SET);
  fread(&dir, sizeof(struct DirectoryEntry), 16, filePtr);
  fseek(filePtr, 0, SEEK_SET);
}


void fatGet(char * str)
{
  int index = findString(str);
  if(index == -1)
  {
    printf("Error: File not found.\n");
    return;
  }
  FILE * outputFile = fopen(str, "w");
  unsigned char buffer[512];
  
  int cluster = dir[index].DIR_FirstClusterLow;
  int offset = LBAToOffset(cluster);
  int c = dir[index].DIR_FileSize;
  while(cluster != -1)
  {
    offset = LBAToOffset(cluster);
    fseek(filePtr, offset, SEEK_SET);
    if(c < 512)
    {
      fread(&buffer[0], c, 1, filePtr);
      fwrite(&buffer[0], c, 1, outputFile);
      fclose(outputFile);
      return;
    }
    else
    {
      fread(&buffer[0], 512, 1, filePtr);
      fwrite(&buffer[0], 512, 1, outputFile);
      cluster = NextLB(cluster);
      c -= 512;
    }
  }
  fclose(outputFile);
}

void fatRead(char *name, char *pos, char *byt)
{
  int index = findString(name);
  int position = atoi(pos);
  int bytes = atoi(byt);
  int cluster = dir[index].DIR_FirstClusterLow;
  int offset = LBAToOffset(cluster);
  fseek(filePtr, offset + position, SEEK_SET);
  int8_t temp;
  for(int i = 0; i < bytes; i++)
  {
    fread(&temp, sizeof(char), 1, filePtr);
    printf("%x ", temp);
  }
  printf("\n");
}

int cd(char *str)
{
  if(strstr(str, "/") == NULL)
  {
    int index = findString(str);
    if(index == -1)
    {
      printf("Error: Subdirectory not found.\n");
      return -1;
    }
    if(dir[index].DIR_Attr != 0x10)
    {
      printf("Error: Subdirectory not found.\n");
      return -1;
    }
    int cluster = dir[index].DIR_FirstClusterLow;
    int offset = LBAToOffset(cluster);
    fseek(filePtr, offset, SEEK_SET);
    fread(&dir, sizeof(struct DirectoryEntry), 16, filePtr);
  }
  else
  {
    cdMulti(str);
  }
  return 0;
}

void cdMulti(char *str)
{
  int size = 0;
  int cds = 0;
  for(int i = 0; i <= strlen(str); i++)
  {
    if(*(str + i) == '/' || i == strlen(str))
    {
      char *temp = (char *)malloc((sizeof(char) * size) + 1);
      for(int j = 0; j < size; j++)
      {
        *(temp + j) = *(str - size + j + i);
      }
      *(temp + size) = '\0';
      size = 0;
      if(cd(temp) == -1)
      {
        for(int j = 0; j < cds; j++)
        {
          char *temp2 = (char *)malloc(sizeof(char) * 3);
          *(temp2) = '.';
          *(temp2 + 1) = '.';
          *(temp2 + 2) = '\0';
          cd(temp2);
          free(temp2);
        }
        free(temp);
        return;
      }
      free(temp);
      cds++;
    }
    else size++;
  }
}

/*
 * parameters  : The current sector number that points to a block of data
 * returns     : The value of the address for that block of data
 * description : Finds the starting address of a block of data given the sector number
 *              corresponding to that data block.
 */
int LBAToOffset(int32_t sector)
{
  if(sector == 0)
    sector = 2;
  return ((sector - 2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) +
         (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);
}

/*
 * purpose : given a logical block address, look up into the first FAT and
 * return the logical block address of the block in the file.
 * if there is no further blocks then return -1
 */
int16_t NextLB(uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(filePtr, FATAddress, SEEK_SET);
  fread(&val, 2, 1, filePtr);
  return val;
}

void populateInfo()
{
  fseek(filePtr, 11, SEEK_SET);
  fread(&BPB_BytsPerSec, 2, 1, filePtr);
  fseek(filePtr, 13, SEEK_SET);
  fread(&BPB_SecPerClus, 1, 1, filePtr);
  fseek(filePtr, 14, SEEK_SET);
  fread(&BPB_RsvdSecCnt, 2, 1, filePtr);
  fseek(filePtr, 16, SEEK_SET);
  fread(&BPB_NumFATs, 1, 1, filePtr);
  fseek(filePtr, 36, SEEK_SET);
  fread(&BPB_FATSz32, 4, 1, filePtr);
  fseek(filePtr, 0, SEEK_SET);
}

void printInfo()
{
  printf("BPB_BytsPerSec:\t %d\t 0x%x\n", BPB_BytsPerSec, BPB_BytsPerSec);
  printf("BPB_SecPerClus:\t %d\t 0x%x\n", BPB_SecPerClus, BPB_SecPerClus);
  printf("BPB_RsvdSecCnt:\t %d\t 0x%x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
  printf("BPB_NumFATs:\t %d\t 0x%x\n", BPB_NumFATs, BPB_NumFATs);
  printf("BPB_FATSz32:\t %d\t 0x%x\n", BPB_FATSz32, BPB_FATSz32);
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
