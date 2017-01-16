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
#define N127 0x7f
#define N128 0x80

typedef struct _ByteArray{
  size_t len; /**< Number of bytes in the `data` field. */
  uint8_t* data; /**< Pointer to an allocated array of data bytes. */
} ByteArray;

void printByteArray(ByteArray ba){
  printf("Byte array is: ");
  for (int i = 0; i < ba.len; ++i)
    printf("%x ", ba.data[i]);
  printf("\n");
}

ByteArray decompress(ByteArray compba){
  ByteArray ba;
  ba.len = compba.len;
  ba.data = malloc(ba.len);
  int ctr, sum, k=0;
  for (int i = 0; i < compba.len; ++i){
    ctr = 0;
    sum = 0;
    if (compba.data[i] >= N128){
      while(compba.data[i] >= N128){
        ctr++;
        if (ctr == 1)
          sum  = compba.data[i] - N128;
        else
        sum = sum * N128 + compba.data[i] - N128;
        i++;
      };
      ba.len = ba.len - ctr + sum;
      ba.data = realloc(ba.data, ba.len);
      for (int j = k; j < k+sum; ++j)
        ba.data[j] = N127;
      k = k + sum;
      --i;
    }
    else{
      ba.data[k]=compba.data[i];
      k++;
    }  
  }
  return ba;
}

int isTopVal(ByteArray ba){
  return (ba.len == 1 && ba.data[0]==N128);
}

int getNumberOfSevenBits(int num){
  int count = 0;
  while (num > 0){
    count++;
    num = num >> 7;
  }
  return count;
}

ByteArray compress(ByteArray ba){
  ByteArray compba;
  compba.len = ba.len;
  compba.data = malloc(compba.len);
  int ctr, sum, k=0;
  for (int i = 0; i < ba.len; ++i){
    ctr = 0;
    sum = 0;
    if (ba.data[i] == N127){
      while(ba.data[i] == N127){
        ctr++;
        i++;
      };
      sum = getNumberOfSevenBits(ctr);
      compba.len = compba.len - ctr + sum;
      compba.data = realloc(compba.data, compba.len);
      for (int j = k; j < k+sum-1; ++j)
        compba.data[j] = ctr/N128 + N128;
      compba.data[k+sum-1] = ctr % N128 + N128;
      k = k + sum;
      --i;
    }
    else{
      compba.data[k]=ba.data[i];
      k++;
    }  
  }
  return compba;
}

int is_full(ByteArray ba, int start){
  for (int i = start; i < ba.len-1; ++i)
    if (ba.data[i] != N127)
      return 0;
  return 1;
}

ByteArray incrementByteArray(ByteArray ba){
  ByteArray newba;
  if (ba.data[ba.len-1] == N127){
    newba.len = ba.len+1;
    newba.data = malloc(newba.len);
    for (int i = 0; i < ba.len; ++i)
      newba.data[i] = ba.data[i];
    newba.data[newba.len-1] = 0x01;
  }
  else{
    newba.len = ba.len; 
    newba.data = malloc(newba.len);
    for (int i = 0; i < ba.len; ++i)
      newba.data[i] = ba.data[i];
    newba.data[newba.len-1]++;
  }
  return newba;
}

ByteArray ByteArray_GenerateBetweenX(ByteArray ba1, ByteArray ba2, int withCompression){
  if(withCompression){
    if (!isTopVal(ba1))
      ba1 = decompress(ba1);
    if (!isTopVal(ba2))
      ba2 = decompress(ba2);
  }
  assert(lessThan(ba1, ba2));

  ByteArray res;

  for (int i = 0; i < ba1.len; ++i){
    uint8_t diff = ba2.data[i] - ba1.data[i];

    if (diff == 0){
      if (ba1.len - i> i+1)
        continue;
      else{
        if (ba2.data[i+1] == 0x01){
          res.len = i+3;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba2.data[j];
          res.data[i+1] = 0x00;
          res.data[i+2] = 0x40;
        }
        else{
          res.len = i+2;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba2.data[j];
          res.data[i+1] = (ba2.data[i+1]+1)/2;
        }
        break;
      }
    }
    else if (diff == 1){
      if ((ba2.len-i>1 && ba1.len-i==1) || (ba2.len-i==1 && ba1.len-i >1 && is_full(ba1, i+1))){
        //increment
        res = incrementByteArray(ba1);
      }
      else{
        // append
        if (ba2.len-i>1){
          res.len = i+1;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba2.data[j]; 
        }
        else{
          res.len = i+2;
          res.data = malloc(res.len);
          for (int j = 0; j <= i; ++j)
            res.data[j] = ba1.data[j];
          res.data[i+1] = 0x40;
        }
      }
      break;
    }
    else{ 
      //new tab size is always 1
      if (ba1.len - i > 1){
        //divide
        res.len = i+1;
        res.data = malloc(res.len);
        for (int j = 0; j < i; ++j)
          res.data[j] = ba1.data[j];
        // res.data[i] = (uint8_t)ceil((double)(ba2.data[i]+ba1.data[i])/2);
        res.data[i] = (ba2.data[i]+ba1.data[i]+1)/2;
      }
      else{
        //increment
        res = incrementByteArray(ba1);
      }
      break;
    }
  }
  // printByteArray(res);
  assert(lessThan(ba1, res));
  assert(lessThan(res, ba2));
  if(withCompression)
    res = compress(res);
  return res;
}

ByteArray ByteArray_GenerateBetween(ByteArray ba1, ByteArray ba2){
  return ByteArray_GenerateBetweenX(ba1, ba2, 0);
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

struct node {
    ByteArray ba;
    struct node *next;
};
typedef struct node node;

/* linked list functions */
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
    ByteArray ba = ByteArray_GenerateBetweenX(previous->ba, last->ba, 1);
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
 
void Traverse(node * head){
    node * current = head;
    while (current != NULL){
        //printf("%d ", current->ba);
        printByteArray(current->ba);
        current = current->next;
    }
    printf("\n");
}

void printSeqSize(node * head){
  int a[10]= {0,0,0,0,0,0,0,0,0,0};
  node * current = head->next;
  while (current != NULL){
    a[current->ba.len-1]++;
    current = current->next;
  }
  for (int i = 0; i < 10; ++i)
    printf("ids of size %d Byte(s): %d\n", i+1, a[i]);
}

void testDecompress(){
  ByteArray ba;
  ba.len = 3;
  ba.data = malloc(ba.len);
  ba.data[0] = 0x03;
  ba.data[1] = 0x8f;
  ba.data[2] = 0x01;

  printByteArray(ba);
  printByteArray(decompress(ba));
}

void testPushFront(){
  node* seq = createList();
  Traverse(seq);
  for (int i = 0; i < 10; ++i)
    pushFront(seq);
  Traverse(seq);
  printSeqSize(seq);
}

void testPushBack(){
  node* seq = createList();
  Traverse(seq);
  for (int i = 0; i < 10000; ++i)
    pushBack(seq);
  Traverse(seq);
  printSeqSize(seq);
}

void testInsertAfter(){
  node* seq = createList();
  Traverse(seq);
  node * next = InsertAfter(seq);
  for (int i = 0; i < 5; ++i){
    pushFront(seq);
    next = InsertAfter(next);
  }
  Traverse(seq);
  printSeqSize(seq);
}

void testGenerateBetween(){
  ByteArray ba1;
  ba1.len = 2;
  ba1.data = malloc(ba1.len);
  ba1.data[0] = 0x3e;
  ba1.data[1] = 0x0e;
  printByteArray(ba1);

  ByteArray ba2;
  ba2.len = 1;
  ba2.data = malloc(ba2.len);
  ba2.data[0] = 0x3f;
  printByteArray(ba2);

  ByteArray res = ByteArray_GenerateBetween(ba1,ba2);
  printByteArray(res);
}

void testCompress(){
  ByteArray ba;
  ba.len = 130;
  ba.data = malloc(ba.len);
  ba.data[0] = 0x03;
  for (int i = 1; i < 129; ++i)
    ba.data[i] = 0x7f;
  ba.data[129] = 0x21;

  printByteArray(ba);
  printByteArray(compress(ba));
  printByteArray(decompress(compress(ba)));
}

int main(int argc, char **argv) {
  // testCompress();
  // testDecompress();
  // testGenerateBetween();
  // testInsertAfter();
  testPushBack();
  // testPushFront();
  return 0;
}