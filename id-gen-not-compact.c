//-------------------------------------------------------------------
//
// File:      id-gen.c
//
// @author    Georges Younes <georges.r.younes@gmail.com>
//
// @copyright 2016-2017 Georges Younes
//
// This file is provided to you under the Apache License,
// Version 2.0 (the "License"); you may not use this file
// except in compliance with the License.  You may obtain
// a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License. 
//
//
//-------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

typedef struct _ByteArray {
  size_t len; /**< Number of bytes in the `data` field. */
  uint8_t* data; /**< Pointer to an allocated array of data bytes. */
} ByteArray;

void printByteArray(ByteArray ba){
  printf("Byte array is: ");
  for (int i = 0; i < ba.len; ++i)
    printf("%x ", ba.data[i]);
  printf("\n");
}

int is_full(ByteArray ba, start)
{
  for (int i = start; i < ba.len-1; ++i)
    if (ba.data[i] != 0x7f)
      return 0;
  return 1;
}

ByteArray incrementByteArray(ByteArray ba)
{
  ByteArray newba;
  if (ba.data[ba.len-1] == 0x7f)
  {
    newba.len = ba.len+1;
    newba.data = malloc(newba.len);
    for (int i = 0; i < ba.len; ++i)
      newba.data[i] = ba.data[i];
    newba.data[newba.len-1] = 0x01;
  }
  else
  {
    newba.len = ba.len;
    newba.data = malloc(newba.len);
    for (int i = 0; i < ba.len; ++i)
      newba.data[i] = ba.data[i];
    newba.data[newba.len-1]++;
  }
  return newba;
}

ByteArray ByteArray_GenerateBetween(ByteArray ba1, ByteArray ba2)
{
  assert(lessThan(ba1, ba2));

  ByteArray res;

  for (int i = 0; i < ba1.len; ++i)
  {
    uint8_t diff = ba2.data[i] - ba1.data[i];

    if (diff == 0)
    {
      if (ba1.len > i+1)
        continue;
      else
      {
        if (ba2.data[i+1] == 0x01)
        {
          res.len = i+3;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba2.data[j];
          res.data[i+1] = 0x00;
          res.data[i+2] = 0x40;
        }
        else
        {
          res.len = i+2;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba2.data[j];
          // res.data[i+1] = (uint8_t)ceil((double)ba2.data[i+1]/2);
          res.data[i+1] = (ba2.data[i+1]+1)/2;
        }
        break;
      }
    }
    else if (diff == 1)
    {

      if ((ba2.len-i>1 && ba1.len-i==1) || (ba2.len-i==1 && ba1.len-i >1 && is_full(ba1, i+1)))
      {
        //increment
        res = incrementByteArray(ba1);
      }
      else
      {
        // append
        if (ba2.len-i>1)
        {
          res.len = i+1;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba2.data[j]; 
        }
        else
        {
          res.len = i+2;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba1.data[j];
          res.data[i+1] = 0x40;
        }
      }
      break;
    }
    else
    { //new tab size is always 1
      if (ba1.len - i > 1)
      {
        //divide
        res.len = i+1;
        res.data = malloc(res.len);
        for (int j = 0; j < i; ++j)
          res.data[j] = ba1.data[j];
        // res.data[i] = (uint8_t)ceil((double)(ba2.data[i]+ba1.data[i])/2);
        res.data[i] = (ba2.data[i]+ba1.data[i]+1)/2;
      }
      else
      {
        //increment
        // res.len = i+1;
        // res.data = malloc(res.len);
        // for (int j = 0; j < i; ++j)
        //   res.data[j] = ba1.data[j];
        // res.data[i] = ba1.data[i];
        // res.data[i]++;
        res = incrementByteArray(ba1);
      }
      break;
    }
  }
  //printByteArray(res);
  assert(lessThan(ba1, res));
  assert(lessThan(res, ba2));
  return res;
}

int compare(ByteArray a, ByteArray b)
{
  for (int i = 0; i < MIN(a.len, b.len); ++i)
    if(a.data[i] < b.data[i])
      return -1;
    else if(a.data[i] > b.data[i])
      return 1;
    else
      continue;
  if(a.len<b.len)
    return -1;
  else
    return 0;
}

int lessThan(ByteArray a, ByteArray b){
  return compare(a, b) == -1;
}

int greaterThan(ByteArray a, ByteArray b){
  return compare(a, b) == 1;
}

int equalsTo(ByteArray a, ByteArray b){
  return compare(a, b) == 0;
}

ByteArray IncPrintByteArray(ByteArray ba){
  printByteArray(ba);
  ba.data[ba.len-1]++;
  printByteArray(ba);
  return ba;
}

struct node {
    ByteArray ba;
    struct node *next;
};
typedef struct node node;

/* linked list functions */
void InsertOrdered(node *, ByteArray);
int Delete(node *, ByteArray);
void Traverse(node *);
node * createList();

node * createList()
{
   /* initializing list */
    node *head, *tail;
    head = (node *)malloc(sizeof(node));
    tail = (node *)malloc(sizeof(node));

    head->ba.len = 1;
    head->ba.data = malloc(head->ba.len);
    head->ba.data[0] = 0x00;
    head->next = tail;

    tail->ba.len = 1;
    tail->ba.data = malloc(tail->ba.len);
    tail->ba.data[0] = 0x80;
    tail->next = NULL;
    
    return head;
}

node * InsertAfter(node *pos)
{
    node * next = pos->next;
    assert(next != NULL);
    ByteArray ba = ByteArray_GenerateBetween(pos->ba, next->ba);
    node * newnode;
    newnode = (node *)malloc(sizeof(node));
    newnode->ba = ba;

    pos->next = newnode;
    newnode->next = next;
    return newnode;
}

void pushFront(node *head)
{
    ByteArray ba = ByteArray_GenerateBetween(head->ba, head->next->ba);
    node * newnode;
    newnode = (node *)malloc(sizeof(node));
    newnode->ba = ba;

    newnode->next = head->next;
    head->next = newnode;
}

void pushBack(node *head)
{
    node * previous = head;
    node * last = head->next;
    while (last->next != NULL)
    {
        previous = last;
        last = last->next;
    }
    ByteArray ba = ByteArray_GenerateBetween(previous->ba, last->ba);
    node * newnode;
    newnode = (node *)malloc(sizeof(node));
    newnode->ba = ba;

    newnode->next = last;
    previous->next = newnode;
}

int Delete(node *head, ByteArray ba)
{
    node * previous = head;
    node * current = head->next;
    while (current != NULL && !equalsTo(current->ba, ba))
    {
        previous = current;
        current = current->next;
    }
    if (current != head && current != NULL) /* if list empty or data not found */
    {
        previous->next = current->next;
        free(current);
        return 0;
    }
    else
        return 1;
}
 
void Traverse(node * head)
{
    node * current = head;
    while (current != NULL)
    {
        //printf("%d ", current->ba);
        printByteArray(current->ba);
        current = current->next;
    }
    printf("\n");
}

void printSeqSize(node * head)
{
  int a[10]= {0,0,0,0,0,0,0,0,0,0};
  node * current = head->next;
  while (current != NULL)
  {
    a[current->ba.len-1]++;
    current = current->next;
  }
  for (int i = 0; i < 10; ++i)
    printf("ids of size %d Byte(s): %d\n", i+1, a[i]);
}

int main(int argc, char **argv) {
  node* seq = createList();
  Traverse(seq);
  //node * next = InsertAfter(seq);
  for (int i = 0; i < 5; ++i){
    pushFront(seq);
    // pushBack(seq);
    // next = InsertAfter(next);
  }
  Traverse(seq);
  printSeqSize(seq);

  // ByteArray ba1;
  // ba1.len = 2;
  // ba1.data = malloc(ba1.len);
  // ba1.data[0] = 0x3e;
  // ba1.data[1] = 0x0e;
  // printByteArray(ba1);

  // ByteArray ba2;
  // ba2.len = 1;
  // ba2.data = malloc(ba2.len);
  // ba2.data[0] = 0x3f;
  // // ba2.data[1] = 0x0e;
  // printByteArray(ba2);

  // ByteArray res = ByteArray_GenerateBetween(ba1,ba2);
  // printByteArray(res);

  return 0;
}