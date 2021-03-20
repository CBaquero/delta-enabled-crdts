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
#include <vector>
#include <iostream>
#include <chrono>
//#define NDEBUG  // Uncoment do stop testing asserts
#include <assert.h>
#include <crdts/delta-crdts.h>

using namespace std;
using namespace crdts;


void test_gset()
{
  cout << "--- Testing: gset --\n";
  GSet<int> o1,o2,do1,do2;

  do1.join(o1.add(1)); 
  do1.join(o1.add(2)); 

  do2.join(o2.add(2)); 
  do2.join(o2.add(3)); 

  GSet<int> o3 = join(o1,o2);
  GSet<int> o4 = join(join(o1,do2),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in(1) << o3.in(0) << endl;

  GSet<string> o5;
  o5.add("hello");
  o5.add("world");
  o5.add("my");
  cout << o5 << endl;
}

void test_twopset()
{
  cout << "--- Testing: twopset --\n";
  TwoPSet<int> o1,o2,do1,do2;

  do1.join(o1.add(1)); 
  do1.join(o1.add(2)); 

  do2.join(o2.add(2)); 
  do2.join(o2.rmv(2)); 

  TwoPSet<int> o3 = join(o1, o2);
  TwoPSet<int> o4 = join(join(o1, do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in(1) << o3.in(2) << endl;

  TwoPSet<string> o5;
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
  GCounter<> o1("idx");
  GCounter<> o2("idy");
  GCounter<> do1, do2;

  do1.join(o1.inc());
  do1.join(o1.inc(4));

  do2.join(o2.inc());
  do2.join(o2.inc());

  GCounter<> o3 = join(o1,o2);
  GCounter<> o4 = join(join(o1,do1),join(o2,do1));

  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.read() << endl;
}


void test_pncounter()
{
  cout << "--- Testing: pncounter --\n";
  // counter with ints in keys and floats in values
  PNCounter<float,int> o1(2);
  PNCounter<float,int> o2(5);
  PNCounter<float,int> do1,do2;

  do1.join(o1.inc(3.5));
  do1.join(o1.dec(2));

  do2.join(o2.inc());
  do2.join(o2.inc(5));

  PNCounter<float,int> o3 = join(o1,o2);
  PNCounter<float,int> o4 = join(join(o1,do2),join(o2,do1));

  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.read() << endl;
}

void test_lexcounter()
{
  cout << "--- Testing: lexcounter --\n";
  LexCounter<int,char> o1('a');
  LexCounter<int,char> o2('b');
  LexCounter<int,char> do1,do2;

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
  AWORSet<char> o1("idx"),o2("idy"),do1,do2;

  do1.join(o1.add('a')); 
  do1.join(o1.add('b')); 

  do2.join(o2.add('b')); 
  do2.join(o2.add('c')); 
  do2.join(o2.rmv('b')); 

  AWORSet<char> o3 = join(o1,o2);
  AWORSet<char> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  cout << o3.in('c') << o3.in('b') << endl;

  assert (o3.in('c') == true &&  o3.in('b') == true);

  AWORSet<string> o5("idz");
  o5.add("hello");
  o5.add("world");
  o5.add("my");
  cout << o5 << endl;
}

void test_rworset()
{
  cout << "--- Testing: rworset --\n";
  RWORSet<char> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.add('a')); 
  do1.join(o1.add('b')); 

  do2.join(o2.add('b')); 
  do2.join(o2.add('c')); 
  do2.join(o2.rmv('b')); 

  RWORSet<char> o3 = join(o1,o2);
  RWORSet<char> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;

  cout << o4.read() << endl;
  cout << o3.in('a') << o3.in('b') << endl;
}

void test_mvreg()
{
  cout << "--- Testing: mvreg --\n";
  MVReg<string> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.write("hello")); 
  do1.join(o1.write("world")); 

  do2.join(o2.write("world")); 
  do2.join(o2.write("hello")); 

  MVReg<string> o3 = join(o1,o2);
  MVReg<string> o4 = join(join(o1,do1),join(o2,do1));
  cout << o3 << endl;
  cout << o4 << endl;
  o3.write("hello world");
  o4.join(o3);
  cout << o4 << endl;

  cout << "--- Testing: mvreg with reduce --\n";

  MVReg<int> o5("id x"),o6("id y"),o7("id z");

  o5.write(3);
  o6.write(5);
  o7.write(2);

  o5.join(o6);
  o5.join(o7);
  cout << o5.read() << endl;

  cout << o5.resolve() << endl;
  cout << o5.read() << endl;

  MVReg<pair<int,int>> o8("id x"),o9("id y"),o10("id z");

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
  AWORSet<string> sx("x"),sy("y");

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
  RWORSet<string,char> sx('x'),sy('y');

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
  GSet<int> sx;

  // Node x does initial operations
  sx.add(1); sx.add(4);

  // Replicate full state in sy;
  GSet<int> sy=sx;

  // Node y records operations in delta
  GSet<int> dy;
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
  pair<int,GSet<int> > a, b, c, d;
  a.first=1;
  a.second.add(0);
  b.first=0;
  b.second.add(1);
  c=join(a,b);
  cout << c << endl;
  d=lexjoin(a,b);
  cout << d << endl;
  pair<float,TwoPSet<char> > e;
}

void test_lwwreg()
{
  cout << "--- Testing: lwwreg --\n";
  LWWReg<int,string> r;

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
  RWLWWSet<int,string> s;
  s.add(1,"a");
  s.add(1,"b");
  s.add(10000,"e");
  s.add(2,"b");
  cout << s << endl;
  cout << s.in("b") << endl;
  RWLWWSet<int,string> t;
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
  EWFlag<> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.enable()); 

  do2.join(o2.enable()); 
  do2.join(o2.enable()); // re-enable is fine

  EWFlag<> o3 = join(o1,o2);
  EWFlag<> o4 = join(join(o1,do1),join(o2,do1));
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
  DWFlag<> o1("id x"),o2("id y"),do1,do2;

  do1.join(o1.disable()); 

  do2.join(o2.disable()); 
  do2.join(o2.disable()); // re-disable is fine

  DWFlag<> o3 = join(o1,o2);
  DWFlag<> o4 = join(join(o1,do1),join(o2,do1));
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
  ORMap<string,TwoPSet<string>> m1,m2;
  m1["color"].add("red");
  m1["color"].add("blue");
  m2["taste"].add("bitter");
  m2["color"].add("green");
  cout << m2["taste"] << endl;
  m1.join(m2);
  cout << m1["color"] << endl;
  m1.erase("taste");
  cout << m1["taste"] << endl;
 
 

  DotContext<string> dc;
  AWORSet<int> s1("x",dc),s2("x",dc);
  s1.add(1); s2.add(2);
  cout << s1 << endl;
  cout << s2 << endl;

  DotContext<string> dc2;
  ORMap<string,AWORSet<string>> m3("x",dc),m4("y",dc2);
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

  ORMap<string,AWORSet<string>> mx("x"),d1,d2;
  mx["color"].add("red");
  mx["color"].add("blue");

  // Now make some deltas, d1 and d2

  d1=mx.erase("color");

  d2["color"].join(mx["color"].add("black"));

  cout << d1 << endl; // Will erase observed dots in the "color" entry
  cout << d2 << endl; // Will add a dot (x:3) tagged "black" entry under "color"

  CausalCounter<int> cc1("x"),cc2("y");
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
  ORMap<string,RWORSet<string>> m5("x"),m6("y");
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

  ORMap<int,ORMap<string,AWORSet<string>>> m7("x");
  m7[2]["color"].add("red");
  cout << m7 << endl;


}

void benchmark1()
{
  AWORSet<int,char> g('i');
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
  GSet<string> a,b;

  a.add("red");
  b.add("blue");

  cout << join(a,b) << endl; // GSet: ( blue red )
}

void example_twopset()
{
  TwoPSet<float> a,b;

  a.add(3.1415);
  a.rmv(3.1415);
  b.add(42);
  b.add(3.1415);

  cout << join(a,b) << endl; // 2PSet: S( 42 ) T ( 3.1415 )

  GSet<float> c;
  c.add(42);

  cout << ( join(a,b).read() == c.read() ) << endl; // true
}

void example_pair()
{
  pair<GSet<int>, GSet<char>> a,b,c;

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
  GCounter<unsigned int> x("x"),y("y"),z("z");

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
  PNCounter<int,char> x('a'), y('b');

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
}

void example_lexcounter()
{
  LexCounter<int> x("a"), y("b");

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
}

void example_ccounter()
{
  CausalCounter<int> x("a"), y("b");

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
  AWORSet<float> x("a"), y("b");

  x.add(3.14); x.add(2.718); x.rmv(3.14);
  y.add(3.14);

  x.join(y);

  cout << x.read() << endl; // Both 3.14 and 2.718 are there

  x.reset(); x.join(y);

  cout << x.read() << endl; // Empty, since 3.14 adition from "b" was visible
}

void example_rworset()
{
  RWORSet<float> x("a"), y("b");

  x.add(3.14); x.add(2.718); x.rmv(3.14);
  y.add(3.14);

  x.join(y);

  cout << x.read() << endl; // Only 2.718 is there, since rmv wins

  x.reset(); x.join(y);

  cout << x.read() << endl; // Empty
}


void example_ormap()
{
  ORMap<string, AWORSet<string>> mx("x"),my("y");

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

  ORMap<int,ORMap<string,AWORSet<string>>> ma("alice"), mb("bob");

  ma[23]["color"].add("red at 23");
  ma[44]["color"].add("blue at 44");
  mb[44]["sound"].add("soft at 44");


  ma.join(mb);

  cout << ma << endl; // Map with two map entries, "44" with two entries
}

void example_gmap()
{
  GMap<char,int> gmx, gmy;

  gmx['a']=1; 
  gmx['b']=0; 
  gmy['a']=3; 
  gmy['c']=0; 

  gmx.join(gmy);
  cout << gmx << endl;
  gmx.join(gmy);
  cout << gmx << endl;
}

void example_bcounter()
{
  BCounter<int,char> bcx('a'), bcy('b');

  bcx.inc(10);
  cout << bcx << endl;
  bcy.inc(3);

  cout << bcx.read() << endl;
  cout << bcy.read() << endl;

  bcy.mv(1,'a');
  bcy.mv(1,'a');

  bcx.join(bcy);
  cout << bcx << endl;
  cout << bcx.read() << endl;
  cout << bcx.local() << endl;
  cout << bcy << endl;
  cout << bcy.read() << endl;
  cout << bcy.local() << endl;

  bcx.mv(10,'b');
  cout << bcx << endl;
  cout << bcx.read() << endl;
  cout << bcx.local() << endl;
}

void example_orseq()
{
  vector<bool> bl,br;
  bl.push_back(false);
  bl.push_back(true);
  bl.push_back(false);
  bl.push_back(true);
//  bl.push_back(true);
//  bl.push_back(false);
//  bl.push_back(true);
  br.push_back(false);
  br.push_back(true);
  br.push_back(true);
//  br.push_back(false);
  br.push_back(true);
  
  cout << bl << endl;
  cout << br << endl;
  cout << "size " << bl.size() << endl;

  cout << (bl < br) << endl;
  cout << among(bl,br) << endl;

  // Simple ORSEQ

  ORSeq<> seq("rid");
  seq.pushBack('a');
  cout << seq << endl;
  seq.pushBack('b');
  cout << seq << endl;
  seq.pushBack('c');
  seq.pushFront('0');
  seq.pushFront('1');
  cout << seq << endl;

  auto i = seq.begin();
  i++;
  seq.insert(i,'x');
  cout << seq << endl;

  ORSeq<> seq2("b");
  seq2.pushBack('y');
  cout << seq2 << endl;

  seq.join(seq2);
  cout << seq << endl;
  seq2.erase(seq2.begin());
  seq.join(seq2);
  cout << seq << endl;
  seq.reset();
  cout << seq << endl;

  // Map with a ORSEQ inside

  ORMap<string,ORSeq<char>> ms1("id1"),ms2("id2");
  ms1["upper"].pushBack('a');
  ms2["upper"].pushFront('b');
  ms2["lower"].pushFront('c');
  ms1.join(ms2);
  cout << ms1 << endl;
  ms2.erase("upper");
  ms1.join(ms2);
  cout << ms1 << endl;

  // Metadata growth, insertions and deletions of added elements,
  // while keeping the first element there

  ORSeq<> seq3("s3");
  seq3.pushBack('a');
  cout << seq3 << endl;
  for (int ops=0; ops < 1000; ops++)
  {
    seq3.pushFront('d');
    seq3.erase(seq3.begin());
  }
  cout << seq3 << endl;

  // Metadata growth, insertions and deletions of added elements,
  // while keeping the last added element there

  ORSeq<> seq4("s4");
  seq4.pushBack('a');
  cout << seq4 << endl;
  for (int ops=0; ops < 1000; ops++)
  {
    seq4.pushBack('d');
    seq4.erase(seq4.begin());
  }
  cout << seq4 << endl;
  seq4.erase(seq4.begin());
  cout << seq4 << endl;


}

void  example_mvreg()
{
  MVReg<string> x("uid-x"),y("uid-y");

  x.write("hello"); x.write("world"); 

  y.write("world"); y.write("hello"); 

  y.join(x);

  cout << y.read() << endl; // Output is ( hello world )

  y.write("mars");
  x.join(y);

  cout << x.read() << endl; // Output is ( mars )

  MVReg<int> a("uid-a"), b("uid-b");

  a.write(0); b.write(3); a.join(b); 

  cout << a.read() << endl; // Output is ( 0 3 )
  
  a.resolve();

  cout << a.read() << endl; // Output is ( 3 )

  a.write(1); // Value can go down again

  cout << a.read() << endl; // Output is ( 1 )

  MVReg<pair<int,int>> j("uid-j"),k("uid-k"),l("uid-l");

  j.write(pair<int,int>(0,0));
  k.write(pair<int,int>(1,0));
  l.write(pair<int,int>(0,1));

  j.join(k); j.join(l); j.resolve();

  cout << j.read() << endl; // Output is ( (0,1) (1,0) )


}

void test_bag()
{
  Bag<pair<int,int>> b("i");
  Bag<pair<int,int>> c("j");

  b.mydata().first=1;
  cout << b.mydata() << endl;
  cout << b << endl;
  c.join(b);
  b.mydata().first=3;
  b.join(c);
  cout << b << endl;
  c.reset();
  b.join(c);
  cout << b << endl;

  // Now inside a map
  ORMap<string,Bag<pair<int,int>>> ma("y");

  cout << ma["a"] << endl;
  ma["a"].fresh();
  cout << ma["a"] << endl;
  cout << ma["a"].mydata() << endl;
  ma["a"].mydata().first+=1;
  cout << ma["a"].mydata() << endl;
  cout << ma << endl;
}

void test_rwcounter()
{
  RWCounter<int> rwc1("i"),rwc2("j");

  rwc1.inc();
  rwc1.inc(2);
  rwc1.dec();
  rwc2.inc(5);
  cout << rwc1 << endl;
  cout << rwc2 << endl;
  rwc1.join(rwc2);
  cout << rwc1 << endl;
  cout << rwc1.read() << endl;
  cout << "Reset:" << rwc2.reset() << endl;
  cout << "Delta:" << rwc2.inc(1) << endl;
  rwc1.join(rwc2);
  cout << rwc1 << endl;
  cout << rwc1.read() << endl;
  rwc2.join(rwc1);
  rwc2.reset();
  rwc1.fresh();
  cout << rwc1 << endl;
  rwc1.inc();
  cout << rwc1 << endl;
  rwc1.join(rwc2);
  cout << rwc1 << endl;
  cout << rwc1.read() << endl;

  ORMap<string, RWCounter<float>> mx("x");

  cout << mx["adds"] << endl;
  cout << mx["adds"] << endl;
  cout << mx["adds"].inc() << endl;
  mx["prints"].inc(5);
  cout << "Delta:" << mx["prints"].inc(6) << endl;
  mx["adds"].inc();

  cout << mx["adds"] << endl;
  cout << mx["adds"].read() << endl;
  cout << mx << endl;

  ORMap<string, RWCounter<float>> my("y");

  my.join(mx);
  my.erase("prints");
  mx["prints"].fresh(); // without this fresh concurrent inc is lost
  mx["prints"].inc(5);
  mx.join(my);
  cout << mx << endl;


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
  test_bag();
  test_rwcounter();

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
  example_gmap();
  example_bcounter();
  example_orseq();
  example_mvreg();


//  ormap<string,aworset<string>> m1("dev1"),m2("dev2");
//
//  m1["friend"].add("alice");
//  m2.join(m1); m2.erase("friend");
//  m1["friend"].add("bob");
//  
//  cout << join(m1,m2) << endl; // shows: friend -> {bob}

  ORMap<string, RWCounter<int>> m1("dev1"),m2("dev2");

  m1["friend"].inc(2);
  m2.join(m1); m2.erase("friend");
  m1["friend"].fresh(); m1["friend"].inc(3);
  
  cout << join(m1,m2)["friend"].read() << endl; // shows a total of 3

}
