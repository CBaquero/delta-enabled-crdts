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
  * AWORSet: A add-wins optimized observed-remove set that allows adds and removes
  * RWORSet: A remove-wins optimized observed-remove set that allows adds and removes
  * MVRegister: An optimized multi-value register (new unpublished datatype)
  * MaxOrder: Keeps the maximum value in an ordered payload type
  * MinOrder: Keeps the minimum value in an ordered payload type

Each datatype depicts some mutation methods and some access methods. Mutations will inflate the state in the join semi-lattice and also return a state with a delta mutation. The delta mutations are intended to be much smaller that the whole state and can be joined together or to full states to synchronize states.  

Check the delta-tests.cc file to get a rough idea on how to use the datatypes.  
The code is still in a very early state and was not properly tested yet. 

Simple Example
--------------

Lets make two replicas of an add-wins or-set of strings. Node `x` uses replica `sx` and node `y` uses `sy` (in practice you would like to run this in diferent nodes, and serialize state to move it between nodes). The first node will add and them remove a given string and the other node will add that same string and a diferent one. Finally we will join the two states and see the result. 

```cpp
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
```

The output will be: `( apple juice )`

With a remove-wins or-set the `rmv` needs to supply a node id. 

```cpp
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

Have fun ...

Additional information
----------------------

For more information on CRDTs and the newer concept of delta mutation consider checking the following papers:

  1. A comprehensive study of Convergent and Commutative Replicated Data Types
http://hal.inria.fr/docs/00/55/55/88/PDF/techreport.pdf

  2. Efficient State-based CRDTs by Delta-Mutation
http://gsd.di.uminho.pt/members/cbm/ps/delta-crdt-draft16may2014.pdf

  3. Conflict-free Replicated Data Types
http://gsd.di.uminho.pt/members/cbm/members/cbm/ps/sss2011.pdf


