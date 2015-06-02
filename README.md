delta-enabled-crdts
===================

Reference implementations of state-based CRDTs that offer deltas for all mutations.

Datatypes
---------

Current datatypes are:

  * GSet: A grow only set 
  * 2PSet: A two phase set that supports removing an element for ever
  * GCounter: A grow only counter
  * PNCounter: A counter supporting increment and decrement
  * LexCounter: A counter supporting increment and decrement (Cassandra inspired)
  * AWORSet: A add-wins optimized observed-remove set that allows adds and removes
  * RWORSet: A remove-wins optimized observed-remove set that allows adds and removes
  * MVRegister: An optimized multi-value register (new unpublished datatype)
  * RWLWWSet: Last-writer-wins set with remove wins bias (SoundCloud inspired)
  * LWWReg: Last-writer-wins register
  * EWFlag: Flag with enable/disable. Enable wins (Riak Flag inspired)
  * DWFlag: Flag with enable/disable. Disable wins (Riak Flag inspired)

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

The GCounter is basically a counter that starts at 0 and can only be incremented (and thus stays always positive). It is implementing by storing one number per each active replica, and thus the programmer must decide a type for the replica/actor) id (string by default) and indicate the type for the value itself. Increments have a default of +1 but they can be changed. In the example we create three replicas 'x', 'y', 'z' and do some concurrent increments and read the result. We also show that join is idempotent.  

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


Keep tuned for more datatype examples soon ...

Additional information
----------------------

For more information on CRDTs and the newer concept of delta mutation consider checking the following papers:

  1. A comprehensive study of Convergent and Commutative Replicated Data Types
http://hal.inria.fr/docs/00/55/55/88/PDF/techreport.pdf

  2. Efficient State-based CRDTs by Delta-Mutation
http://arxiv.org/pdf/1410.2803v1.pdf

  3. Conflict-free Replicated Data Types
http://gsd.di.uminho.pt/members/cbm/members/cbm/ps/sss2011.pdf


