delta-enabled-crdts
===================

Reference implementations of state-based CRDTs that offer deltas for all mutations.

Datatypes
---------

Current datatypes are:

  * GSet: A grow only set 
  * 2PSet: A two phase set that supports removing an element for ever
  * Pair: A pair of CRDTs, first and second. 
  * GCounter: A grow only counter
  * PNCounter: A counter supporting increment and decrement
  * LexCounter: A counter supporting increment and decrement (Cassandra inspired)
  * DotKernel: (Auxiliary datatype for building causal based datatypes)
  * CCounter: A (causal) counter for map embedding (Optimization over Riak EMCounter)
  * AWORSet: A add-wins optimized observed-remove set that allows adds and removes
  * RWORSet: A remove-wins optimized observed-remove set that allows adds and removes
  * MVRegister: An optimized multi-value register (new unpublished datatype)
  * EWFlag: Flag with enable/disable. Enable wins (Riak Flag inspired)
  * DWFlag: Flag with enable/disable. Disable wins (Riak Flag inspired)
  * GMap: Grow only map of keys to CRDTs
  * ORMap: Map of keys to causal CRDTs. (spec in common with the Riak Map)
  * RWLWWSet: Last-writer-wins set with remove wins bias (SoundCloud inspired)
  * LWWReg: Last-writer-wins register
  * BCounter: Non negative bounder counter. (by Valter, et all. SRDS 2015)
  * ORSeq: A causal sequence prototype. (Treedoc inspired)

Each datatype depicts some mutation methods and some access methods. Mutations will inflate the state in the join semi-lattice and also return a state with a delta mutation. The delta mutations are intended to be much smaller that the whole state and can be joined together or to full states to synchronize states.  

If the return of mutation methods is ignored, the datatypes behave as standard state based CRDTs. The use of deltas is strictly optional. 

Check the delta-tests.cc file to get a rough idea on how to use the datatypes.  
The code is still in a very early state and was not properly tested yet. 

Simple Example
--------------

Lets make two replicas of an add-wins or-set of strings. Node `x` uses replica `sx` and node `y` uses `sy` (in practice you would like to run this in different nodes, and serialize state to move it between nodes). The first node will add and them remove a given string and the other node will add that same string and a diferent one. Finally we will join the two states and see the result. 

```cpp
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
```

The output will be: `( apple juice )`

Now, the same example, with a remove-wins or-set and using chars for node ids.
The default template type for node ids is string, so we need to change it to char in the second template argument. 

```cpp
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
```

The output will be: `( juice )`

Example with delta-mutations
----------------------------

The above example show how to operate with standard state-based CRDTs. One
drawback is that we need to ship and join full states. It would be much better
if we  could ship instead only the parts of the state that changed. We call
these parts deltas. 

In the next example, using a simple grow-only set of integers `gset<int>`, node
`x` will create a replica and replicate it to node `y`. Afterwards we will do
some new operations in `y`, collect the deltas and ship them back and merge
them to `x` replica. 

```cpp
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
```

Datatype Example Catalog
------------------------

In construction is a catalog of simple examples for each datatype. To keep the examples simple, deltas are not collected. 

GSet
----

Grow only sets do not require node ids and can store any type that is storable in a C++ std::set. These sets can only grow and do not support removal of elements. Join is by set union. 

```cpp
  gset<string> a,b;

  a.add("red");
  b.add("blue");

  cout << join(a,b) << endl; // GSet: ( blue red )
```

TwoPSet
-------

Two phase sets can both add and remove elements, but removed elements cannot be re-added. Both GSets and TwoPSets can be read to return a std::set with the payload. 

```cpp
  twopset<float> a,b;

  a.add(3.1415);
  a.rmv(3.1415);
  b.add(42);
  b.add(3.1415);

  cout << join(a,b) << endl; // 2PSet: S( 42 ) T ( 3.1415 )

  gset<float> c;
  c.add(42);

  cout << ( join(a,b).read() == c.read() ) << endl; // true
```

Pair
----

All CRDTs here can be composed in a pair using the std::pair construction. Join is defined for pairs and applies independently a join to the first and second components of the pair. More formally, pair is the composition by product.  

The example bellow uses the GSets, but any other valid CRDT types could be composed (including other pairs). 

```cpp
  pair<gset<int>,gset<char>> a,b,c;

  a.first.add(0); 
  b.first.add(1);
  a.second.add('a'); 
  b.second.add('x'); b.second.add('y');

  c=join(a,b);

  cout << c << endl; // (GSet: ( 0 1 ),GSet: ( a x y ))
```

A special use of pairs is when the first elements are comparable in a total order (int, float, bool, double, ...). In that case a special lexicographic join can be used as alternative to join. It compares the first elements and when one is higher it dictates the whole winning pair. On ties a join is performed on the second elements. The example also shows that primitive type numbers can be joined (by taking their max). 

```cpp
  pair<int,float> lww_a(12,42), lww_b(20,3.1415);

  cout << join(lww_a,lww_b) << endl; // (20,42)
  cout << lexjoin(lww_a,lww_b) << endl; // (20,3.1415)
```

GCounter
--------

The GCounter is basically a counter that starts at 0 and can only be incremented (and thus stays always positive). It is implemented by storing one number per each active replica, and thus the programmer must decide a type for the replica/actor) id (string by default) and indicate the type for the value itself (int by default). Increments have a default of +1 but they can be changed. In the example we create three replicas 'x', 'y', 'z' and do some concurrent and sequential increments and read the result. We also show that join is idempotent.  

```cpp
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
```

PNCounter
---------

The PNCounter allows increments and decrements, by keeping a separate account of increments and decrements. You can think of it as two GCounters.  The reported value is the number of increments minus the number of decrements. 

In the example bellow we declare two instances of counters for integers and change the default key type to be char (instead of string). Since among the two instances we did a total of 4 increments and 2 decrements, the overall value after should be 2. 

```cpp 
  pncounter<int,char> x('a'), y('b');

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
```

LexCounter
---------

The Lexicographic Counter is similar to the counters used in Cassandra (tough earlier specs existed). It is an alternative to the PNCounter and is based on a lexicographic pair, per actor, of a grow-only version number and count produced by that actor. Its use is just like a PNCounter, so we use a similar example bellow. To make the example slightly different we use the default string keys and no longer mention a char in the second template argument.

```cpp 
  lexcounter<int> x("a"), y("b");

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
```

DotKernel
---------

While the DotKernel is a proper CRDT, with mutations and providing a join, its purpose is not direct use but instead serve as a basis for all the datatypes that use dot based causality tracking (namely AWORSet, RWORSet, MVRegister, EWFlag, DWFlag and ORMap). All the supported datatypes do not need to define a specialized join and simply use the DotKernel generic join. 

The DotKernel can be used to locally create unique tags/dots (by consulting information on a causal context variable cc) and store in its local datastore (a map variable named ds) associations from that tag to an instance of a given payload type. 

If a mapping is removed, the tag/dot is still remembered (in a compact form) on the causal context and this allows the join to be efficient in the propagation of removes without resorting to more space demanding tombstones. In short, it implements the theory behind Optimized OR-Sets (a.k.a. ORSWOT) and offers a more general use for other similar datatypes. 

CCounter
--------

A Causal Counter is a variation of a LexCounter that can be used inside ORMaps. 
Each time a given actor wants to increase or decrease a count, it consults its last updated count, changes it and stores it under a new dot entry after deleting its previous entry. As required for all ORMap embeddable datatypes a reset is supported, but it is not a perfect observed reset. 

We show a simple example with increments and decrements

```cpp
  ccounter<int> x("a"), y("b");

  x.inc(4); x.dec();
  y.dec();

  cout << (x.read() == y.read()) << endl; // value is diferent

  x.join(y); y.join(x);

  cout << (x.read() == y.read()) << endl; // value is the same, both are 2
  
  x.reset();

  cout << x.read() << endl; // you guessed correctly, its 0
}
```

AWORSet
-------

An Add Wins Observed Remove Set is a set that allows element additions and removals. The removals only affect the elements that are visible locally, so that a concurrent removal and addition of the same element will result in the element still being present after joining the sets. The implementation used the DotKernel and is optimized, it is also known as ORSWOT in the early literature.

The example bellow will show a concurrent add and remove and finally a reset. 

```cpp
  aworset<float> x("a"), y("b");

  x.add(3.14); x.add(2.718); x.rmv(3.14);
  y.add(3.14);

  x.join(y);

  cout << x.read() << endl; // Both 3.14 and 2.718 are there

  x.reset(); x.join(y);

  cout << x.read() << endl; // Empty, since 3.14 adition from "b" was visible
```

RWORSet
-------

A Removed Wins Observed Remove Set is the dual implementation to AWORSet where concurrent removes win over adds. The implementation is slightly more complex but still optimized and based on the DotKernel. 

We show the same example as above, but with a different outcome. 

```cpp
  rworset<float> x("a"), y("b");

  x.add(3.14); x.add(2.718); x.rmv(3.14);
  y.add(3.14);

  x.join(y);

  cout << x.read() << endl; // Only 2.718 is there, since remove wins

  x.reset(); x.join(y);

  cout << x.read() << endl; // Empty
```

MVReg
-----

A Multi-Value Register is a simple data type that allows read and write operations of a given payload. Writes will overwrite all locally visible values, but concurrent writes will create alternative values tat are represented as siblings. Thus, unlike a sequential register, reads may return multiple values.   

In the example bellow, we do some sequential writes that overwrite the initial writes, and then we join two concurrent writes to show a pair of siblings. Finally we overwrite everything with the string "mars", in replica y, and show that this effect propagates on join to replica x. 

```cpp
  mvreg<string> x("uid-x"),y("uid-y");

  x.write("hello"); x.write("world"); 

  y.write("world"); y.write("hello"); 

  y.join(x);

  cout << y.read() << endl; // Output is ( hello world )

  y.write("mars");

  x.join(y);

  cout << x.read() << endl; // Output is ( mars )
```

Multi-value registers can store any type of "opaque" payload. However if the payload supports a partial order, it is possible to use a resolve method to reduce the number of siblings to only those that are maximal elements in the order. In other words, if any sibling has another sibling that is greater than it, it is removed. A special case is when the stored payload is a total order, implying that resolve will always produce a register with a single element, the maximal element. 

```cpp
  mvreg<int> a("uid-a"), b("uid-b");

  a.write(0); b.write(3); a.join(b); 

  cout << a.read() << endl; // Output is ( 0 3 )
  
  a.resolve();

  cout << a.read() << endl; // Output is ( 3 )

  a.write(1); // Value can go down again

  cout << a.read() << endl; // Output is ( 1 )
```

The last example, bellow, shows a payload that reflects a partial order, and can thus allow for concurrent maximals after resolve. 

```cpp
  mvreg<pair<int,int>> j("uid-j"),k("uid-k"),l("uid-l");

  j.write(pair<int,int>(0,0));
  k.write(pair<int,int>(1,0));
  l.write(pair<int,int>(0,1));

  j.join(k); j.join(l); j.resolve();

  cout << j.read() << endl; // Output is ( (0,1) (1,0) )
```

ORMap
-----

An Observed Remove Map can be used to compose a map of keys to CRDTs. Not all CRDTs make sense inside such a map, so currently it is restricted to AWORSet, RWORSet, MVRegister, EWFlag, DWFlag, CCounter and the ORMap (enabling recursive composition). 

Lets start with a simple example of a map that maps strings to AWORSets. 
We have two map replicas that will have a key in common and be joined. Later we remove content from one of the keys and see the effect after joining again. 

```cpp
  ormap<string,rworset<string>> mx("x"),my("y");

  mx["paint"].add("blue");
  mx["sound"].add("loud");  mx["sound"].add("soft");
  my["paint"].add("red");
  my["number"].add("42");

  mx.join(my);

  cout << mx << endl; // this map includes all added elements

  my["number"].rmv("42");
  mx.join(my);

  cout << mx << endl; // number set is now empty also in mx
```

If a key is erased in a map, this is equivalent to resenting the whole CRDT that it points to. If its a set it will become empty, a counter goes back to 0 and so forth. 

```cpp
  mx.erase("paint");
  my["paint"].add("green");

  my.join(mx);

  cout << my << endl; // in the "paint" key there is only "green" 
```

Above we see that if new operations are done concurrently with the erase they still take place. The reset is an observed reset and it only affects data that is already present. 

The next example illustrates that maps can hold other maps and that the key type can also be chosen. 

```cpp
  ormap<int,ormap<string,aworset<string>>> ma("alice"), mb("bob");

  ma[23]["color"].add("red at 23");
  ma[44]["color"].add("blue at 44");
  mb[44]["sound"].add("soft at 44");


  ma.join(mb);

  cout << ma << endl; // Map with two map entries, inner map 44 with two entries
```

In a map, capturing a delta is done in the usual way, with the exception of the key access operator. This is due to the access returning a reference to the stored value, and thus its delta mutations will be of that type and must be converted to a suitable map type. 

```cpp
  ormap<string,aworset<string>> mx("x"),d1,d2;
  mx["color"].add("red");
  mx["color"].add("blue");

  // Now make some deltas, d1 and d2

  d1=mx.erase("color");

  d2["color"].join(mx["color"].add("black"));

  cout << d1 << endl; // Will erase observed dots in the "color" entry
  cout << d2 << endl; // Will add a dot (x:3) for "black" entry under "color"
```

Keep tuned for more datatype examples soon ...

Acknowledgments
---------------

This work was partially supported by projects: FP7 SyncFree under grant 609551, and ON2 project NORTE-07-0124-FEDER-000058. 

Disclaimer
----------

The provided implementations are still in an alpha stage. The present focus is
establishing the main operational properties of the targeted CRDTs, that
matches the theory supporting them, and not the extensive programming of a rich
API for each, or ensuring production quality code. Only a minimal amount of
example-based testing was conducted and no serious benchmarking was done.  If
you are a developer and have a product that can benefit from these datatypes,
you should either ensure you understand the code and can improve it to meet
your needs, or you can contact me and work out a way to allocate the resources
for product improvement. 

Additional information
----------------------

For more information on CRDTs and the newer concept of delta mutation consider checking the following papers and video presentation:

  1. A comprehensive study of Convergent and Commutative Replicated Data Types
http://hal.inria.fr/docs/00/55/55/88/PDF/techreport.pdf

  2. Efficient State-based CRDTs by Delta-Mutation
http://arxiv.org/pdf/1410.2803v1.pdf

  3. Conflict-free Replicated Data Types
http://gsd.di.uminho.pt/members/cbm/members/cbm/ps/sss2011.pdf

  4. Implementation of CRDTs with δ-mutators
  https://www.youtube.com/watch?v=jwCx1A181vY&list=PL9Jh2HsAWHxIc7Tt2M6xez_TOP21GBH6M&index=15 
