delta-enabled-crdts
===================

Reference implementations of state-based CRDTs that offer deltas for all mutations.

Current datatypes are:

GSet: A grow only set 

2PSet: A two phase set that supports removing an element for ever

GCounter: A grow only counter

PNCounter: A counter supporting increment and decrement

AWORSet: A add-wins optimized observed-remove set that allows adds and removes

MVRegister: An optimized multi-value register (new unpublished datatype)

Each datatype depicts some mutation methods and some access methods. Mutations will inflate the state in the join semi-lattice and also return a state with a delta mutation. The delta mutations are intended to be much smaller that the whole state and can be joined together or to full states to synchronize states.  

Check the delta-tests.cc file to get a rough idea on how to use the datatypes.  
The code is still in a very early state and was not properly tested yet. 

For more information on CRDTs and the newer concept of delta mutation consider checking the following papers:

A comprehensive study of Convergent and Commutative Replicated Data Types

http://hal.inria.fr/docs/00/55/55/88/PDF/techreport.pdf

Efficient State-based CRDTs by Delta-Mutation

http://gsd.di.uminho.pt/members/cbm/ps/delta-crdt-draft16may2014.pdf

Conflict-free Replicated Data Types

http://gsd.di.uminho.pt/members/cbm/members/cbm/ps/sss2011.pdf


