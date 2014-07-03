//-------------------------------------------------------------------
//
// File:      delta-tests.cc
//
// @author    Carlos Baquero <cbm@di.uminho.pt>
//
// @copyright 2014 Carlos Baquero
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
// @doc  
//   Simple tests for the datatypes in delta-crdts.cc
// @end  
//
//
//-------------------------------------------------------------------

#include <set>
#include <map>
#include <string>
#include <iostream>
#include "delta-crdts.cc"

using namespace std;

void test_gset()
{
  gset<int> o1,o2,do1,do2;

  do1.join(o1.add(1)); 
  do1.join(o1.add(2)); 

  do2.join(o2.add(2)); 
  do2.join(o2.add(3)); 

  gset<int> o3 = join(o1,o2);
  gset<int> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in(1) << o3.in(0) << endl;

  gset<string> o5;
  o5.add("hello");
  o5.add("world");
  o5.add("my");
  cout << o5 << endl;
}

void test_twopset()
{
  twopset<int> o1,o2,do1,do2;

  do1.join(o1.add(1)); 
  do1.join(o1.add(2)); 

  do2.join(o2.add(2)); 
  do2.join(o2.rmv(2)); 

  twopset<int> o3 = join(o1,o2);
  twopset<int> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in(1) << o3.in(2) << endl;

  twopset<string> o5;
  o5.add("hello");
  o5.add("world");
  o5.add("my");
  o5.rmv("my");
  o5.rmv("my");
  cout << o5 << endl;
}

void test_gcounter()
{
  gcounter o1,o2,do1,do2;

  do1.join(o1.inc("idx"));
  do1.join(o1.inc("idx",4));

  do2.join(o2.inc("idy"));
  do2.join(o2.inc("idy"));

  gcounter o3 = join(o1,o2);
  gcounter o4 = join(join(o1,do1),join(o2,do1));

  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.read() << endl;
}


int main(int argc, char * argv[])
{
  test_gset();
  test_twopset();
  test_gcounter();
}
