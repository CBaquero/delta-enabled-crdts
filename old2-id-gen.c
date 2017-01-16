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

typedef struct _ByteArray {
  size_t len; /**< Number of bytes in the `data` field. */
  uint8_t* data; /**< Pointer to an allocated array of data bytes. */
} ByteArray;

ByteArray ByteArray_CreateWithCString (const char* str)
{
  return (ByteArray) {
    .data = (uint8_t*)str, .len = strlen(str)
  };
}

ByteArray ByteArray_CreateWithInt(const unsigned int n)
{
  ByteArray ba;
  int BytesNum = 0;
  unsigned int num = n;
  while (num != 0) {
    num >>= 8;
    BytesNum ++;
  }
  //printf("BytesNum %d\n", BytesNum);
  ba.len = ceil((double)(BytesNum*8)/7);
  //printf("len %zu\n", ba.len);
  ba.data = malloc(ba.len);
  for (size_t i = 0; i < ba.len-1; i++)
  {
    ba.data[i] = (n >> ((BytesNum * 8)-7*(i+1))) & 0x7F;
    //printf("%x\n", ba.data[i]);
  }
  ba.data[ba.len-1] = n << (9-ba.len);
  ba.data[ba.len-1] = ba.data[ba.len-1] >> (9-ba.len);
  //printf("%x\n", ba.data[ba.len-1]);
  return ba;
}

unsigned int ByteArray_GetIntValue(ByteArray ba) {
  ByteArray batemp;
  batemp.len = floor((double)ba.len*7/8);
  if (batemp.len < 1)
    batemp.len = 1;
  batemp.data = malloc(batemp.len);
  //printf("len %zu\n", batemp.len);

  for (int i = 0; i < batemp.len-1; ++i)
  {
    batemp.data[i] = ba.data[i] << (i+1);
    for (int j = 0; j < i+1; ++j)
      if (((ba.data[i+1] >> (6-j))  & 0x01) == 1)
        batemp.data[i] |= (1u << (i-j));
    //printf("%x\n", batemp.data[i]);
  }

  batemp.data[batemp.len-1] = ba.data[batemp.len-1] << (batemp.len);
  for (int j = 0; j < batemp.len; ++j)
    if (((ba.data[batemp.len] >> j)  & 0x01) == 1)
      batemp.data[batemp.len-1] |= (1u << j);
  //printf("%x\n", batemp.data[batemp.len-1]);

  unsigned int val = 0;
  for (int i = 0; i < batemp.len; ++i)
  {
    val = val + (batemp.data[i] << (8*(batemp.len-i-1)));
    //printf("val %u\n", val);
  }
  return val;
}

ByteArray ByteArray_GenerateBetween(ByteArray ba1, ByteArray ba2)
{
  assert(lessThan(ba1, ba2));

return ba1;
  // if(ba1.len == ba1.len) {
  //   s
  // }
  // else if (ba1.len < ba1.len)
  // {
  //   s
  // }
  // else {
  //   s
  // }
}

int compare(ByteArray a, ByteArray b)
{
  ByteArray longArray, shortArray;
  int order = 1;
  if (a.len >= b.len){
    longArray = a;
    shortArray = b;
  }
  else {
    longArray = b;
    shortArray = a;
    order = -1;
  }
  int deltaLen = longArray.len - shortArray.len;
  for (int i = 0; i < deltaLen; ++i)
    if(longArray.data[i] > 0)
      return order*1;
  for (int i = 0; i < shortArray.len; ++i)
    if(longArray.data[i+deltaLen] > shortArray.data[i])
      return order*1;
    else if(shortArray.data[i] > longArray.data[i+deltaLen])
      return order*-1;
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

void printByteArray(ByteArray ba){
  printf("Byte array is: ");
  for (int i = 0; i < ba.len; ++i)
    printf("%x ", ba.data[i]);
  printf("\n");
}

ByteArray IncPrintByteArray(ByteArray ba){
  printByteArray(ba);
  ba.data[ba.len-1]++;
  printByteArray(ba);
  return ba;
}

int main(int argc, char **argv) {
  ByteArray a1 = ByteArray_CreateWithInt(233);
  printByteArray(a1);
  unsigned int s1 = ByteArray_GetIntValue(a1);
  printf("Int is %u\n", s1);
  ByteArray a2 = ByteArray_CreateWithInt(234);
  printByteArray(a2);
  unsigned int s2 = ByteArray_GetIntValue(a2);
  printf("Int is %u\n", s2);
  ByteArray a3 = ByteArray_GenerateBetween(a1,a2);
  printByteArray(a3);
  unsigned int s3 = ByteArray_GetIntValue(a3);
  printf("Int is %u\n", s3);
  ByteArray a4 = IncPrintByteArray(a3);
  unsigned int s4 = ByteArray_GetIntValue(a4);
  printf("Int is %u\n", s4);
  return 0;
}