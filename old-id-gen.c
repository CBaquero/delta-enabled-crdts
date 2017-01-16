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

ByteArray ByteArray_CreateWithInt(const int n)
{
  ByteArray ba;
  int BytesNum = 0, num = n;// ceil((double)n/255);
  while (num != 0) {
    num >>= 8;
    BytesNum ++;
  }
  printf("BytesNum %d\n", BytesNum);
  ba.len = 1 + ceil((double)((BytesNum*8)-6)/7);
  if(n<=64)
    ba.len=1;
  printf("len %zu\n", ba.len);
  ba.data = malloc(ba.len);
  for (size_t i = 0; i < ba.len; i++) {
    if (i == 0 && ba.len) {
      // little indian
      ba.data[i] = n & 0x3F;
    }
    else {
      ba.data[i] = (n >> (7*(i-1) + 6)) & 0x7F;
    }
    //printf("%x\n", ba.data[i]);
    //ba.data[i] |= (1u << 7); //put highest bit to 1
    //ba.data[i] &= ~(1u << 7); //put highest bit to 0
    //printf("%x\n", ba.data[i]);
  }
  return ba;
}

unsigned long ByteArray_GetIntValue(ByteArray ba) {
  ByteArray batemp;
  batemp.len = ba.len;
  batemp.data = malloc(ba.len);
  for (int i = 0; i < ba.len; ++i){
    if (i == 0)
    {
      batemp.data[ba.len-1] = ba.data[i] & 0x3F;
      //printf("%x\n", batemp.data[ba.len-1]);
      if ((ba.data[i+1]  & 0x01) == 1)
        batemp.data[ba.len-1] |= (1u << 6);
      //printf("%x\n", batemp.data[ba.len-1]);
      if (((ba.data[i+1] >> 1)  & 0x01) == 1)
        batemp.data[ba.len-1] |= (1u << 7);
      //printf("%x\n", batemp.data[ba.len-1]);
    }
    else
    {   
      batemp.data[ba.len-1-i] = (ba.data[i] & 0x7F) >> (i+1);

      for (int j = 0; j < i+2; ++j)
        if (((ba.data[i+1] >> j)  & 0x01) == 1)
          batemp.data[ba.len-1-i] |= (1u << (6-i+j));

    }
    printf("%x\n", batemp.data[i]);
  }
  unsigned long val = 0;
  for (int i = 0; i < batemp.len; ++i)
  {
    val = val + (batemp.data[i] << (8*(batemp.len-i-1)));
    printf("val %lu\n", val);
  }
  return val;
}

ByteArray ByteArray_GetBinValue(ByteArray ba) {
  ByteArray batemp;
  batemp.len = ba.len;
  batemp.data = malloc(ba.len);
  for (int i = 0; i < ba.len; ++i){
    if (i == 0)
    {
      batemp.data[ba.len-1] = ba.data[i] & 0x3F;
      //printf("%x\n", batemp.data[ba.len-1]);
      if ((ba.data[i+1]  & 0x01) == 1)
        batemp.data[ba.len-1] |= (1u << 6);
      //printf("%x\n", batemp.data[ba.len-1]);
      if (((ba.data[i+1] >> 1)  & 0x01) == 1)
        batemp.data[ba.len-1] |= (1u << 7);
      //printf("%x\n", batemp.data[ba.len-1]);
    }
    else
    {   
      batemp.data[ba.len-1-i] = (ba.data[i] & 0x7F) >> (i+1);

      for (int j = 0; j < i+2; ++j)
        if (((ba.data[i+1] >> j)  & 0x01) == 1)
          batemp.data[ba.len-1-i] |= (1u << (6-i+j));

    }
    printf("%x\n", batemp.data[i]);
  }
  return batemp;
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

int lessThan(ByteArray a, ByteArray b)
{
  return compare(a, b) == -1;
}

int main(int argc, char **argv) {
  ByteArray a = ByteArray_CreateWithInt(65535);
  ByteArray abin =ByteArray_GetBinValue(a);
  for (int i = 0; i < abin.len; ++i)
    printf("%x ", abin.data[i]);
  unsigned long ssa=ByteArray_GetIntValue(a);
  printf("sadad %lu\n", ssa);
  ByteArray b = ByteArray_CreateWithInt(123456);
  ByteArray bbin =ByteArray_GetBinValue(b);
  for (int i = 0; i < bbin.len; ++i)
    printf("%x ", bbin.data[i]);
  unsigned long ssb=ByteArray_GetIntValue(b);
  printf("sadad %lu\n", ssb);
  int c = compare(abin, bbin);
  printf("%d\n", c);
  ByteArray z = ByteArray_GenerateBetween(abin, bbin);
  // ByteArray ba1 = ByteArray_CreateWithInt(1234567891);
  // ByteArray ba2 = ByteArray_CreateWithInt(1234567891);


  return 0;
}