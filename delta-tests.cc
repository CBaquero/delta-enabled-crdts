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
#include <chrono>
//#define NDEBUG  // Uncoment do stop testing asserts
#include <assert.h>
#include "delta-crdts.cc"

using namespace std;

void test_gset()
{
  cout << "--- Testing: gset --\n";
  gset<int> o1,o2,do1,do2;

  do1.join(o1.add(1)); 
  do1.join(o1.add(2)); 

  do2.join(o2.add(2)); 
  do2.join(o2.add(3)); 

  gset<int> o3 = join(o1,o2);
  gset<int> o4 = join(join(o1,do2),join(o2,do1));
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
  cout << "--- Testing: twopset --\n";
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
  cout << "--- Testing: gcounter --\n";
  // default template type is string key and int value
  gcounter<> o1("idx");
  gcounter<> o2("idy");
  gcounter<> do1,do2;

  do1.join(o1.inc());
  do1.join(o1.inc(4));

  do2.join(o2.inc());
  do2.join(o2.inc());

  gcounter<> o3 = join(o1,o2);
  gcounter<> o4 = join(join(o1,do1),join(o2,do1));

  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.read() << endl;
}


void test_pncounter()
{
  cout << "--- Testing: pncounter --\n";
  // counter with ints in keys and floats in values
  pncounter<float,int> o1(2);
  pncounter<float,int> o2(5);
  pncounter<float,int> do1,do2;

  do1.join(o1.inc(3.5));
  do1.join(o1.dec(2));

  do2.join(o2.inc());
  do2.join(o2.inc(5));

  pncounter<float,int> o3 = join(o1,o2);
  pncounter<float,int> o4 = join(join(o1,do2),join(o2,do1));

  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.read() << endl;
}

void test_lexcounter()
{
  cout << "--- Testing: lexcounter --\n";
  lexcounter<int,char> o1('a');
  lexcounter<int,char> o2('b');
  lexcounter<int,char> do1,do2;

  o1.inc(3);
  o1.inc(2);
  o1.dec(1);
  o2.inc(1);

  cout << o1 << endl;
  cout << o2 << endl;

  o2.join(o1);
  cout << o2.read() << endl;
}


void test_aworset()
{
  cout << "--- Testing: aworset --\n";
  aworset<char> o1("idx"),o2("idy"),do1,do2;

  do1.join(o1.add('a')); 
  do1.join(o1.add('b')); 

  do2.join(o2.add('b')); 
  do2.join(o2.add('c')); 
  do2.join(o2.rmv('b')); 

  aworset<char> o3 = join(o1,o2);
  aworset<char> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in('c') << o3.in('b') << endl;

  assert (o3.in('c') == true &&  o3.in('b') == true);

  aworset<string> o5("idz");
  o5.add("hello");
  o5.add("world");
  o5.add("my");
  cout << o5 << endl;
}

void test_rworset()
{
  cout << "--- Testing: rworset --\n";
  rworset<char> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.add('a')); 
  do1.join(o1.add('b')); 

  do2.join(o2.add('b')); 
  do2.join(o2.add('c')); 
  do2.join(o2.rmv('b')); 

  rworset<char> o3 = join(o1,o2);
  rworset<char> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;

  cout << o4.read() << endl;
  cout << o3.in('a') << o3.in('b') << endl;
}

void test_mvreg()
{
  cout << "--- Testing: mvreg --\n";
  mvreg<string> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.write("hello")); 
  do1.join(o1.write("world")); 

  do2.join(o2.write("world")); 
  do2.join(o2.write("hello")); 

  mvreg<string> o3 = join(o1,o2);
  mvreg<string> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  o3.write("hello world");
  o4.join(o3);
  cout << o4 << endl;

  cout << "--- Testing: mvreg with reduce --\n";

  mvreg<int> o5("id x"),o6("id y"),o7("id z");

  o5.write(3);
  o6.write(5);
  o7.write(2);

  o5.join(o6);
  o5.join(o7);
  cout << o5.read() << endl;

  cout << o5.resolve() << endl;
  cout << o5.read() << endl;

  mvreg<pair<int,int>> o8("id x"),o9("id y"),o10("id z");

  // notice that the default order for pairs is lexicographic in C++
  // cout << (pair<int,int>(0,1) < pair<int,int>(1,0)) << endl;

  o8.write(pair<int,int>(0,0));
  o9.write(pair<int,int>(1,0));
  o10.write(pair<int,int>(0,1));

  o8.join(o9);
  o8.join(o10);
  cout << o8.read() << endl;

  cout << o8.resolve() << endl;
  cout << o8.read() << endl;
}

/*
void test_maxord()
{
  maxord<int> o1,o2,do1,do2;
  do1.join(o1.write(6)); 
  do1.join(o1.write(3)); 

  do2.join(o2=5); 
  do2.join(o2=10); 

  maxord<int> o3 = join(o1,o2);
  maxord<int> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;

  maxord<bool> o5;
  cout << o5 << endl;
  o5=true;
  o5=false; // Once it goes up, it cant go back
  cout << o5 << endl;

  
  maxord<double> a;
  maxord<double> b;
  cout << a << endl;
  a=1.1;
  cout << a << endl;
  b=(--a);
  cout << b << endl;
 

}
*/

void example1()
{
  aworset<string> sx("x"),sy("y");

  // Node x
  sx.add("apple");
  sx.rmv("apple");
  // Node y
  sy.add("juice");
  sy.add("apple");

  // Join into one object and show it 
  sx.join(sy);
  cout << sx.read() << endl;
}

void example2()
{
  rworset<string,char> sx('x'),sy('y');

  // Node x
  sx.add("apple");
  sx.rmv("apple");
  // Node y
  sy.add("juice");
  sy.add("apple");

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
  cout << "--- Testing: lexjoin on pairs --\n";
  pair<int,gset<int> > a, b, c, d;
  a.first=1;
  a.second.add(0);
  b.first=0;
  b.second.add(1);
  c=join(a,b);
  cout << c << endl;
  d=lexjoin(a,b);
  cout << d << endl;
  pair<float,twopset<char> > e;
}

void test_lwwreg()
{
  cout << "--- Testing: lwwreg --\n";
  lwwreg<int,string> r;

  r.write(1,"Hello");
  cout << r << endl;
  r.write(0,"My"); 
  cout << r << endl;
  r.write(3,"World");

  cout << r << endl;
  cout << r.write(2,"a") << endl;
  cout << r.read() << endl;

}

void test_rwlwwset()
{
  cout << "--- Testing: rwlwwset --\n";
  rwlwwset<int,string> s;
  s.add(1,"a");
  s.add(1,"b");
  s.add(10000,"e");
  s.add(2,"b");
  cout << s << endl;
  cout << s.in("b") << endl;
  rwlwwset<int,string> t;
  t.rmv(2,"b");
  t.rmv(6,"e");
  t.add(1,"c");
  s.join(t);
  cout << s.in("b") << endl;
  cout << s << endl;
}

void test_ewflag()
{
  cout << "--- Testing: ewflag --\n";
  ewflag<> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.enable()); 

  do2.join(o2.enable()); 
  do2.join(o2.enable()); // re-enable is fine

  ewflag<> o3 = join(o1,o2);
  ewflag<> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o4.read() << endl;
  o3.disable();
  o4.join(o3);
  cout << o4 << endl;
  cout << o4.read() << endl;
}

void test_dwflag()
{
  cout << "--- Testing: dwflag --\n";
  dwflag<> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.disable()); 

  do2.join(o2.disable()); 
  do2.join(o2.disable()); // re-disable is fine

  dwflag<> o3 = join(o1,o2);
  dwflag<> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o4.read() << endl;
  o3.enable();
  o4.join(o3);
  cout << o4 << endl;
  cout << o4.read() << endl;
}

void test_ormap()
{
  ormap<string,twopset<string>> m1,m2;
  m1["color"].add("red");
  m1["color"].add("blue");
  m2["taste"].add("bitter");
  m2["color"].add("green");
  cout << m2["taste"] << endl;
  m1.join(m2);
  cout << m1["color"] << endl;
  m1.erase("taste");
  cout << m1["taste"] << endl;
 
 

  dotcontext<string> dc;
  aworset<int> s1("x",dc),s2("x",dc);
  s1.add(1); s2.add(2);
  cout << s1 << endl;
  cout << s2 << endl;

  dotcontext<string> dc2;
  ormap<string,aworset<string>> m3("x",dc),m4("y",dc2);
  m3["color"].add("red");
  m3["color"].add("blue");
  m4["color"].add("green");
  cout << m3["color"] << endl;
  cout << m4["color"] << endl;
  m3.join(m4);
  cout << m3["color"] << endl;
  m3["color"].rmv("green");
  m3.join(m4);
  cout << m3["color"] << endl;

  ormap<string,aworset<string>> mx("x"),d1,d2;
  mx["color"].add("red");
  mx["color"].add("blue");

  // Now make some deltas, d1 and d2

  d1=mx.erase("color");

  d2["color"].join(mx["color"].add("black"));

  cout << d1 << endl; // Will erase observed dots in the "color" entry
  cout << d2 << endl; // Will add a dot (x:3) tagged "black" entry under "color"

  ccounter<int> cc1("x"),cc2("y");
  cc1.inc(10);
  cc2.join(cc1);
  cc2.inc(10);
  cout << cc1 << endl;
  cc1.inc();
  cout << cc1 << endl;
  cc1.dec();
  cout << cc1 << endl;
  cc1.reset();
  cout << cc1 << endl;
  cout << cc1.read() << endl;
  cc1.inc(5);
  cc1.join(cc2);
  cout << cc1 << endl;
  cout << cc1.read() << endl;
   
  cout << "--- Map I ---" << endl;
  ormap<string,rworset<string>> m5("x"),m6("y");
  m5["color"].add("red");
  m5["taste"].add("bitter");
  m6["sound"].add("loud");
  m6["color"].add("blue");
  cout << "m5 " << m5 << endl;
  cout << "m6 " << m6 << endl;
  m5.join(m6);
  cout << "m5 " << m5 << endl;
  m6.erase("sound");
  cout << "m6 " << m6 << endl;
  m5.join(m6);
  cout << "m5 " << m5 << endl;
  cout << m5.erase("color");
  cout << m5.reset();
  cout << "m5 " << m5 << endl;
  m5.join(m6);
  cout << "m5 " << m5 << endl;

  
  cout << "--- Map F ---" << endl;

  ormap<int,ormap<string,aworset<string>>> m7("x");
  m7[2]["color"].add("red");
  cout << m7 << endl;


}

void benchmark1()
{
  aworset<int,char> g('i');
//  twopset<int> g;

  using namespace std::chrono;

  steady_clock::time_point t1 = steady_clock::now();

  //const long double TimeBefore = time(0);

  for(int i=1; i < 1000; i++) // 1k
  {
    g.add(i);
  }
  for(int i=1; i < 1000; i+=2) // 1k
  {
    g.rmv(i);
  }
  for(int i=999; i > 0; i--) // 1k
  {
    g.add(i);
  }

  steady_clock::time_point t2 = steady_clock::now();

  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);

  std::cout << "It took me " << time_span.count() << " seconds.";
  std::cout << std::endl;

  cout << g.in(0) << endl;
  cout << g.in(10) << endl;

  /*
  const long double TimeAfter = time(0);

  cout << "Elapsed System Time in Seconds is " << TimeAfter-TimeBefore << "." << endl;
  */
}

void example_gset()
{
  gset<string> a,b;

  a.add("red");
  b.add("blue");

  cout << join(a,b) << endl; // GSet: ( blue red )
}

void example_twopset()
{
  twopset<float> a,b;

  a.add(3.1415);
  a.rmv(3.1415);
  b.add(42);
  b.add(3.1415);

  cout << join(a,b) << endl; // 2PSet: S( 42 ) T ( 3.1415 )

  gset<float> c;
  c.add(42);

  cout << ( join(a,b).read() == c.read() ) << endl; // true
}

void example_pair()
{
  pair<gset<int>,gset<char>> a,b,c;

  a.first.add(0); 
  b.first.add(1);
  a.second.add('a'); 
  b.second.add('x'); b.second.add('y');

  c=join(a,b);

  cout << c << endl; // (GSet: ( 0 1 ),GSet: ( a x y ))
}

void example_lexpair()
{
  pair<int,float> lww_a(12,42), lww_b(20,3.1415);

  cout << join(lww_a,lww_b) << endl; // (20,42)
  cout << lexjoin(lww_a,lww_b) << endl; // (20,3.1415)
}

void example_gcounter()
{
  gcounter<unsigned int> x("x"),y("y"),z("z");

  x.inc(); x.inc();
  y.inc(2);
  z.join(x); z.join(y);
  
  cout << z.read() << endl; // 4

  x.inc(2);
  z.inc(2);
  z.join(x);
  z.join(x);

  cout << z.read() << endl; // 8
  cout << z << endl; // GCounter: ( x->4 y->2 z->2 ) 
}

void example_pncounter()
{
  pncounter<int,char> x('a'), y('b');

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
}

void example_lexcounter()
{
  lexcounter<int> x("a"), y("b");

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
}

void example_ccounter()
{
  ccounter<int> x("a"), y("b");

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2

  x.reset();

  cout << x.read() << endl; // you guessed correctly, its 0
}

void example_aworset()
{
  aworset<float> x("a"), y("b");

  x.add(3.14); x.add(2.718); x.rmv(3.14);
  y.add(3.14);

  x.join(y);

  cout << x.read() << endl; // Both 3.14 and 2.718 are there

  x.reset(); x.join(y);

  cout << x.read() << endl; // Empty, since 3.14 adition from "b" was visible
}

void example_rworset()
{
  rworset<float> x("a"), y("b");

  x.add(3.14); x.add(2.718); x.rmv(3.14);
  y.add(3.14);

  x.join(y);

  cout << x.read() << endl; // Only 2.718 is there, since rmv wins

  x.reset(); x.join(y);

  cout << x.read() << endl; // Empty
}


void example_ormap()
{
  ormap<string,aworset<string>> mx("x"),my("y");

  mx["paint"].add("blue");
  mx["sound"].add("loud");  mx["sound"].add("soft");
  my["paint"].add("red");
  my["number"].add("42");

  mx.join(my);

  cout << mx << endl; // this map includes all added elements

  my["number"].rmv("42");
  mx.join(my);

  cout << mx << endl; // number set is now empty also in mx

  mx.erase("paint");
  my["paint"].add("green");

  my.join(mx);

  cout << my << endl; // in the "paint" key there is only "green" 

  ormap<int,ormap<string,aworset<string>>> ma("alice"), mb("bob");

  ma[23]["color"].add("red at 23");
  ma[44]["color"].add("blue at 44");
  mb[44]["sound"].add("soft at 44");


  ma.join(mb);

  cout << ma << endl; // Map with two map entries, "44" with two entries
}

int main(int argc, char * argv[])
{
  test_gset();
  test_twopset();
  test_gcounter();
  test_pncounter();
  test_lexcounter();
  test_aworset();
  test_rworset();
  test_mvreg();
//  test_maxord();
  test_maxpairs();
  test_lwwreg();
  test_rwlwwset();
  test_ewflag();
  test_dwflag();
  test_ormap();
  test_rwlwwset();

  example1();
  example2();
  example3();

  example_gset();
  example_twopset();
  example_pair();
  example_lexpair();
  example_gcounter();
  example_pncounter();
  example_lexcounter();
  example_ccounter();
  example_aworset();
  example_rworset();
  example_ormap();

}
