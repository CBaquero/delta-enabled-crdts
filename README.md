delta-enabled-crdts
===================

Reference implementations of state-based CRDTs that offer deltas for all mutations.

Current datatypes are:

GSet        -- A grow only set 
2PSet       -- A two phase set that supports removing an element for ever
GCounter    -- A grow only counter
AWORSet     -- A add-wins optimized observed-remove set that allows adds and removes
MVRegister  -- An optimized multi-value register

Each datatype depicts some mutation methods and some access methods. Mutations will inflate the state in the join semi-lattice and also return a state with a delta mutation. The delta mutations are intended to be much smaller that the whole state and can be joined together or to full states to synchronize states.  
