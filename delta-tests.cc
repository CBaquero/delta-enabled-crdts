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

#include <ctime>
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

void test_pncounter()
{
  pncounter o1,o2,do1,do2;

  do1.join(o1.inc("idx",3));
  do1.join(o1.dec("idx"));

  do2.join(o2.inc("idy"));
  do2.join(o2.inc("idy"));

  pncounter o3 = join(o1,o2);
  pncounter o4 = join(join(o1,do1),join(o2,do1));

  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.read() << endl;
}


void test_aworset()
{
  aworset<char> o1,o2,do1,do2;

  do1.join(o1.add("idx",'a')); 
  do1.join(o1.add("idx",'b')); 

  do2.join(o2.add("idy",'b')); 
  do2.join(o2.add("idy",'c')); 
  do2.join(o2.rmv('b')); 

  aworset<char> o3 = join(o1,o2);
  aworset<char> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in('c') << o3.in('b') << endl;

  aworset<string> o5;
  o5.add("idz","hello");
  o5.add("idz","world");
  o5.add("idz","my");
  cout << o5 << endl;
}

void test_rworset()
{
  rworset<char> o1,o2,do1,do2;

  do1.join(o1.add("idx",'a')); 
  do1.join(o1.add("idx",'b')); 

  do2.join(o2.add("idy",'b')); 
  do2.join(o2.add("idy",'c')); 
  do2.join(o2.rmv("idy",'b')); 

  rworset<char> o3 = join(o1,o2);
  rworset<char> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;

  cout << o4.read() << endl;
  cout << o3.in('a') << o3.in('b') << endl;
}

void test_mvreg()
{
  mvreg<string> o1,o2,do1,do2;

  do1.join(o1.write("idx","hello")); 
  do1.join(o1.write("idx","world")); 

  do2.join(o2.write("idy","world")); 
  do2.join(o2.write("idy","hello")); 

  mvreg<string> o3 = join(o1,o2);
  mvreg<string> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  o3.write("idz","hello world");
  o4.join(o3);
  cout << o4 << endl;
}

void test_maxord()
{
  maxord<int> o1,o2,do1,do2;
  do1.join(o1.write(6)); 
  do1.join(o1.write(3)); 

  do2.join(o2.write(5)); 
  do2.join(o2.write(10)); 

  maxord<int> o3 = join(o1,o2);
  maxord<int> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;

  maxord<bool> o5;
  cout << o5 << endl;
  o5.write(true);
  o5.write(false); // Once it goes up, it cant go back
  cout << o5 << endl;
}

void example1()
{
  aworset<string> sx,sy;

  // Node x
  sx.add("x","apple");
  sx.rmv("apple");
  // Node y
  sy.add("y","juice");
  sy.add("y","apple");

  // Join into one object and show it 
  sx.join(sy);
  cout << sx.read() << endl;
}

void example2()
{
  rworset<string> sx,sy;

  // Node x
  sx.add("x","apple");
  sx.rmv("x","apple");
  // Node y
  sy.add("y","juice");
  sy.add("y","apple");

  // Join into one object and show it 
  sx.join(sy);
  cout << sx.read() << endl;
}

void example3()
{
  gset<int> sx;

  // Node x does initial operations
  sx.add(1); sx.add(4);

  // Replicate full state in sy;
  gset<int> sy=sx;

  // Node y records operations in delta
  gset<int> dy;
  dy=sy.add(2);
  dy.join(sy.add(3));  // Join delta to delta

  cout << sy.read() << endl;  // ( 1 2 3 4 )

  // Merge deltas ( 2 3 ) to node x
  cout << dy.read() << endl;  // ( 2 3 )
  cout << sx.read() << endl;  // ( 1 4 )
  sx.join(dy);
  cout << sx.read() << endl;  // ( 1 2 3 4 )
}

void test_maxpairs()
{
  pair<maxord<int>,gset<int> > a, b, c, d;
  a.first.write(1);
  a.second.add(0);
  b.first.write(0);
  b.second.add(1);
  c=join(a,b);
  cout << c << endl;
  d=lexjoin(a,b);
  cout << d << endl;
  pair<maxord<float>,twopset<char> > e;
}

void test_lwwreg()
{
  lwwreg<int,string> r;

  r.write(1,"Hello");
  r.write(0,"My"); 
  r.write(3,"World");

  cout << r << endl;
  cout << r.write(2,"a") << endl;
  cout << r.read() << endl;

}

void test_lwwset()
{
  lwwset<int,string> s;
  s.add(1,"a");
  s.add(1,"b");
  s.add(2,"b");
  cout << s.in("b") << endl;
  lwwset<int,string> t;
  t.rmv(2,"b");
  t.add(1,"c");
  s.join(t);
  cout << s.in("b") << endl;
  cout << s << endl;
}

void benchmark1()
{
  aworset<int> g;

  const long double TimeBefore = time(0);

  for(int i=1; i < 100000; i++) // 100k
  {
    g.add("id",i);
  }

  cout << g.in(0) << endl;
  cout << g.in(10) << endl;

  const long double TimeAfter = time(0);

  cout << "Elapsed System Time in Seconds is " << TimeAfter-TimeBefore << "." << endl;
}

int main(int argc, char * argv[])
{
  test_gset();
  test_twopset();
  test_gcounter();
  test_pncounter();
  test_aworset();
  test_rworset();
  test_mvreg();
  test_maxord();
  test_maxpairs();
  test_lwwreg();
  test_lwwset();

  example1();
  example2();
  example3();

  // benchmark1();


}
