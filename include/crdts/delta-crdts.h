//-------------------------------------------------------------------
//
// File:      delta-crdts.h
//
// @author    Carlos Baquero <cbm@di.uminho.pt>
//
// @copyright 2014-2016 Carlos Baquero
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
//   Reference implementation in C++ of delta enabled CRDTs
// @end  
//
//
//-------------------------------------------------------------------
#ifndef _DELTA_ENABLED_CRDTS_H_
#define _DELTA_ENABLED_CRDTS_H_


#include <set>
#include <unordered_set>
#include <map>
#include <list>
#include <tuple>
#include <vector>
#include <string>
#include <iostream>
#include <type_traits>

using namespace std;

namespace crdts
{
  // Join with C++ traits
  // Join two objects, deriving a new one
  // assuming copy constructor
  template<typename T> 
  T join( const T& l, const T& r );


  // Join two pairs of objects
  template<typename A, typename B> 
  pair<A, B> join( const pair<A, B>& l, const pair<A, B>& r);

  // Join lexicographic of two pairs of objects
  template<typename A, typename B> 
  pair<A, B> lexjoin(const pair<A, B>& l, const pair<A, B>& r);

  // Output a pair
  template<typename A, typename B> 
  ostream& operator<<( ostream &output, const pair<A, B>& o );

  // Output a set
  template<typename T> 
  ostream& operator<<( ostream &output, const set<T>& o);

  // get a point among two points
  template<typename T> 
  vector<T> among( const vector<T>& l, const vector<T>& r, int j = 0 );

  template<typename T> // Output a vector
  ostream& operator<<( ostream &output, const vector<T>& o);

  // Autonomous causal context, for context sharing in maps
  template<typename K>
  class DotContext
  {
  public:
    DotContext<K>& operator=( const DotContext<K>& o );
    friend ostream& operator<<( ostream& output, const DotContext<K>& o );
    bool dotin( const pair<K, int>& d ) const;
    void compact();
    pair<K, int> makeDot( const K& id );
    void insertDot( const pair<K, int>& d, bool compactnow = true );
    void join( const DotContext<K>& o );

    map<K, int>       cc; // Compact causal context
    set<pair<K, int>> dc; // Dot cloud
  };


  template <typename T, typename K>
  class DotKernel
  {
  public:
    // if no causal context supplied, used base one
    DotKernel() : c(cbase) {} 
    // if supplied, use a shared causal context
    DotKernel( DotContext<K>& jointc ) : c( jointc ) {} 
    //  DotKernel(const DotKernel<T,K> &adk) : c(adk.c), ds(adk.ds) {}

    DotKernel<T, K>& operator=( const DotKernel<T, K>& adk );
    friend ostream& operator<<( ostream &output, const DotKernel<T, K>& o);
    void join( const DotKernel<T, K>& o );
    void deepJoin( const DotKernel<T, K>& o );
    DotKernel<T, K> add( const K& id, const T& val );
    // Add that returns the added dot, instead of kernel delta
    pair<K, int> dotAdd( const K& id, const T& val );
    // remove all dots matching value
    DotKernel<T, K> remove( const T& val );
    // remove a dot 
    DotKernel<T, K> remove( const pair<K, int>& dot );
    // remove all dots 
    DotKernel<T, K> remove();

    map<pair<K, int>, T>  ds;  // Map of dots to vals
    DotContext<K>         cbase;
    DotContext<K>&        c;
  };


  template<typename V = int, typename K = string>
  class GCounter
  {
  public:
    // Only for deltas and those should not be mutated
    GCounter() {} 
    
    // Mutable replicas need a unique id
    GCounter(K a) : id(a) {} 

    // argument is optional
    GCounter inc( V tosum = {1} );

    bool operator==( const GCounter<V, K>& o ) const;

    // get local counter value // CBM make this const
    V local();
    
    // get counter value
    V read() const;

    void join( const GCounter<V, K>& o );

    friend ostream& operator<<( ostream &output, const GCounter<V, K>& o);

  private:
    map<K, V>   m;
    K           id;
  };


  template<typename V = int, typename K = string>
  class PNCounter
  {
  public:
    PNCounter() {} // Only for deltas and those should not be mutated
    PNCounter( K a ) : p(a), n(a) {} // Mutable replicas need a unique id

    // Argument is optional
    PNCounter inc( V tosum = {1} );

    // Argument is optional
    PNCounter dec( V tosum = {1} );

    // get local counter value
    V local();

    // get counter value
    V read() const;

    void join(const PNCounter& o);

    friend ostream &operator<<( ostream &output, const PNCounter<V, K>& o);

  private:
    GCounter<V, K> p, n;
  };


  template <typename V = int, typename K = string>
  class LexCounter
  {
  public:
    LexCounter() {} // Only for deltas and those should not be mutated
    LexCounter(K a) : id(a) {} // Mutable replicas need a unique id

    // Argument is optional
    LexCounter inc(V tosum = 1);

    // Argument is optional
    LexCounter dec(V tosum = 1);
    // get counter value
    V read() const;

    void join( const LexCounter<V, K>& o );

    friend ostream& operator<<( ostream& output, const LexCounter<V, K>& o);

  private:
    map<K, pair<int, V>> m;
    K id;
  };

  template<typename V, typename K = string>
  class CausalCounter    // Causal counter, variation of Riak_dt_emcntr and LexCounter 
  {
  public:
    CausalCounter() {} // Only for deltas and those should not be mutated
    CausalCounter(K k) : id(k) {} // Mutable replicas need a unique id
    CausalCounter(K k, DotContext<K> &jointc) : id(k), dk(jointc) {} 

    DotContext<K>& context();

    friend ostream& operator<<( ostream &output, const CausalCounter<V, K>& o);

    CausalCounter<V, K> inc( const V& val = 1 );

    CausalCounter<V, K> dec( const V& val = 1 );
    
    // Other nodes might however upgrade their counts
    CausalCounter<V, K> reset();

    V read();

    void join( CausalCounter<V, K> o );

  private:
    // To re-use the kernel there is an artificial need for dot-tagged bool payload
    DotKernel<V, K>   dk; // Dot kernel
    K                 id;
  };


  template<typename T>
  class GSet
  {
  public:
    GSet() {}
      //  GSet(string id, DotContext<K> &jointdc) {}

    // For map compliance reply with an empty context
      //  DotContext<K> context();

    set<T> read() const;

    bool operator ==( const GSet<T>& o ) const;

    bool in( const T& val );

    friend ostream& operator<<( ostream& output, const GSet<T>& o );

    GSet<T> add( const T& val );

    void join( const GSet<T>& o );

  private:
    set<T> s;
  };


  // Map embedable datatype
  template<typename T, typename K = string> 
  class TwoPSet
  {
  public:

    TwoPSet() {}
    TwoPSet( string id, DotContext<K>& jointdc ) {}
    // For map compliance reply with an empty context
    DotContext<K> context();

    set<T> read ();

    bool operator ==( const TwoPSet<T>& o ) const;

    bool in( const T& val );

    friend ostream& operator<<( ostream& output, const TwoPSet<T, K>& o);

    TwoPSet<T> add( const T& val );

    TwoPSet<T> rmv( const T& val );

    TwoPSet<T> reset();

    void join( const TwoPSet<T>& o );

  private:
    set<T> s;
    set<T> t;  // removed elements are added to t and removed from s
  };


  // Map embedable datatype
  template<typename E, typename K = string> 
  class AWORSet    // Add-Wins Observed-Remove Set
  {
  public:
      AWORSet() {}            // Only for deltas and those should not be mutated
      AWORSet( K k ) : id(k) {} // Mutable replicas need a unique id
      AWORSet( K k, DotContext<K>& jointc ) : id( k ), dk( jointc ) {} 

      DotContext<K>& context();

      friend ostream& operator<<( ostream& output, const AWORSet<E, K>& o );

      set<E> read();

      bool in(const E& val);

      AWORSet<E, K> add( const E& val );

      AWORSet<E, K> rmv( const E& val );

      AWORSet<E,K> reset();

      void join( AWORSet<E, K> o );

  private:
      DotKernel<E, K>   dk; // Dot kernel
      K                 id;
  };


  // Map embedable datatype
  template<typename E, typename K = string> 
  class RWORSet    // Remove-Wins Observed-Remove Set
  {
  public:
      RWORSet() {} // Only for deltas and those should not be mutated
      RWORSet( K k ) : id(k) {} // Mutable replicas need a unique id
      RWORSet( K k, DotContext<K>& jointc ) : id( k ), dk( jointc ) {} 

      DotContext<K> & context();

      friend ostream &operator<<( ostream &output, const RWORSet<E, K>& o );

      set<E> read();
      // Could
      bool in(const E& val);

      RWORSet<E, K> add( const E& val );

      RWORSet<E, K> rmv( const E& val );

      RWORSet<E, K> reset();

      void join( RWORSet<E, K> o );

  private:
      DotKernel<pair<E, bool>, K> dk; // Dot kernel
      K id;
  };



  // Multi-value register, Optimized
  template<typename V, typename K = string>
  class MVReg    
  {
  public:
      MVReg() {} // Only for deltas and those should not be mutated
      MVReg( K k ) : id(k) {} // Mutable replicas need a unique id
      MVReg( K k, DotContext<K> &jointc ) : id( k ), dk( jointc ) {} 

      DotContext<K>& context();

      friend ostream& operator<<( ostream &output, const MVReg<V, K>& o);

      MVReg<V, K> write( const V& val );

      set<V> read( );

      MVReg<V, K> reset( );

      MVReg<V, K> resolve( );

      void join( MVReg<V, K> o );

  private:
      DotKernel<V, K> dk; // Dot kernel
      K               id;
  };


  template<typename K = string>
  class EWFlag    // Enable-Wins Flag
  {
  private:
      // To re-use the kernel there is an artificial need for dot-tagged bool payload
      DotKernel<bool, K> dk; // Dot kernel
      K id;

  public:
      EWFlag( ) {} // Only for deltas and those should not be mutated
      EWFlag( K k ) : id(k) {} // Mutable replicas need a unique id
      EWFlag( K k, DotContext<K>& jointc ) : id( k ), dk( jointc ) {} 

      DotContext<K>& context();

      friend ostream& operator<<( ostream &output, const EWFlag<K>& o );

      bool read();

      EWFlag<K> enable();

      EWFlag<K> disable();

      EWFlag<K> reset();

      void join(EWFlag<K> o);
  };

  template<typename K = string>
  class DWFlag    // Disable-Wins Flag
  {
  public:
      DWFlag() {} // Only for deltas and those should not be mutated
      DWFlag(K k) : id(k) {} // Mutable replicas need a unique id
      DWFlag(K k, DotContext<K> &jointc) : id(k), dk(jointc) {} 

      DotContext<K> & context();

      friend ostream &operator<<( ostream &output, const DWFlag<K>& o);

      bool read();

      DWFlag<K> disable();

      DWFlag<K> enable();

      DWFlag<K> reset();

      void join (DWFlag<K> o);

  private:
      // To re-use the kernel there is an artificial need for dot-tagged bool payload
      DotKernel<bool,K>   dk; // Dot kernel
      K                   id;
  };

  // U is timestamp, T is payload
  template<typename U, typename T>
  class RWLWWSet // remove wins bias for identical timestamps
  {
  public:
      friend ostream& operator<<( ostream &output, const RWLWWSet<U, T>& o );

      RWLWWSet<U, T> add( const U& ts, const T& val );

      RWLWWSet<U, T> rmv( const U& ts, const T& val );

      bool in(const T& val);

      void join( const RWLWWSet<U, T>& o );

  private:
      RWLWWSet<U, T> addrmv( const U& ts, const T& val, bool b );  

      map<T, pair<U, bool>> s;
  };


  // U must be comparable
  template<typename U, typename T>
  class LWWReg  
  {
  public:
      friend ostream& operator<<( ostream& output, const LWWReg<U, T>& o);

      void join(const LWWReg<U, T>& o);

      LWWReg<U, T> write( const U& ts, const T& val );

      T read();

  private:
      pair<U, T> r;
  };



  template<typename N, typename V, typename K = string>
  class ORMap
  {
  public:
      // if no causal context supplied, use base one
      ORMap() : c( cbase ) {} 
      ORMap(K i) : id( i ), c( cbase ) {} 
      // if supplied, use a shared causal context
      ORMap(K i, DotContext<K> &jointc) : id(i), c(jointc) {} 

      //  ORMap( const ORMap<N,V,K>& o ) :  id(o.id), m(o.m), c(o.c) {}

      ORMap<N, V, K>& operator=( const ORMap<N, V, K> & o );

      DotContext<K>& context() const;

      friend ostream& operator<<( ostream &output, const ORMap<N, V, K>& o );

      // Need to find a way to collect deltas for this interface
      V& operator[]( const N& n );

      ORMap<N, V, K> erase( const N & n );

      ORMap<N, V, K> reset();

      void join( const ORMap<N, V> & o );

  public:
      map<N, V>       m;  
      DotContext<K>   cbase;
      DotContext<K>&  c;
      K               id;
  };


  // A bag is similar to an RWSet, but allows for CRDT payloads
  template<typename V, typename K = string>
  class Bag 
  {
  public:

      Bag() {} // Only for deltas and those should not be mutated
      Bag( K k ) : id(k) {} // Mutable replicas need a unique id
      Bag( K k, DotContext<K>& jointc ) : id( k ), dk( jointc ) {} 

      Bag<V, K>& operator=( const Bag<V, K>& o);


      DotContext<K> & context();

      void insert( pair<pair<K, int>, V> t );

      friend ostream& operator<<(ostream& output, const Bag<V, K>& o);

      typename map<pair<K, int>, V>::iterator begin();

      typename map<pair<K, int>, V>::iterator end();

      pair<K, int> mydot( );

      V& mydata();

      // To protect from concurrent removes, create fresh dot for self
      void fresh( );

      Bag<V, K> reset( );

      // Using the deep join will try to join different payloads under same dot
      void join( const Bag<V, K>& o );

  private:
      DotKernel<V, K> dk; // Dot kernel
      K               id;
  };


  // Inspired by designs from Carl Lerche and Paulo S. Almeida
  template<typename V, typename K = string>
  class RWCounter    //  Reset Wins Counter
  {
  public:
      RWCounter() {} // Only for deltas and those should not be mutated
      RWCounter( K k ) : id(k), b(k) {} // Mutable replicas need a unique id
      RWCounter( K k, DotContext<K>& jointc ) : id(k), b(k, jointc) {} 

      RWCounter<V, K>& operator=( const RWCounter<V, K>& o);

      DotContext<K>& context();

      friend ostream& operator<<( ostream &output, const RWCounter<V, K>& o);
      
      RWCounter<V, K> inc( const V& val = 1 );

      RWCounter<V, K> dec( const V& val = 1 );

      RWCounter<V, K> reset( );

      void fresh( );

      V read( );

      void join( const RWCounter<V, K> & o );

  private:
      Bag<pair<V, V>, K>  b; // Bag of pairs
      K                   id;
  };



  template<typename N, typename V>
  class GMap
  {
  public:
      // later make m private by adding a begin() for iterators 
      map<N, V> m;  

      friend ostream& operator<<( ostream &output, const GMap<N, V>& o);

      // Need to find a better way to collect deltas for this interface
      V& operator[]( const N& n );

      void join( const GMap<N, V>& o );
  };


  template <typename V = int, typename K = string>
  class BCounter
  {
  public:
      BCounter() {} // Only for deltas and those should not be mutated
      BCounter(K a) : id( a ), c( a ) {} // Mutable replicas need a unique id

      // Argument is optional
      BCounter inc( V tosum = {1} );

      // Argument is optional
      BCounter dec(V todec = {1} );

      // Quantity V to node id K
      BCounter mv( V q, K to );

      // get global counter value
      V read();

      // get local counter available value
      V local();

      void join( const BCounter& o );

      friend ostream& operator<<( ostream& output, const BCounter<V, K>& o);

  private:
      PNCounter<V, K>         c;
      GMap<pair<K, K>, int>   m; 
      K                       id;
  };


  template<typename T = char, typename I = string>
  class ORSeq
  {
  public:
      // if no causal context supplied, used base one
      ORSeq() : c( cbase ) {}  // Only for deltas and those should not be mutated
      ORSeq( I i ) : id( i ), c( cbase ) {} 
      // if supplied, use a shared causal context
      ORSeq( I i, DotContext<I>& jointc ) : id( i ), c( jointc ) {} 

      ORSeq<T, I>& operator=( const ORSeq<T, I>& aos );

      friend ostream& operator<<( ostream &output, const ORSeq<T, I>& o);

      typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator begin();

      typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator end();

      ORSeq<T, I> erase(typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator i);

      DotContext<I>& context();

      ORSeq<T, I> reset();

      ORSeq<T, I> insert( typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator i, const T& val );

      // add 1st element
      ORSeq<T, I> makeFirst( const T& val );

      ORSeq<T, I> pushBack( const T& val );

      ORSeq<T, I> pushFront( const T& val );

      void join( const ORSeq<T, I>& o );

  private:
      // List elements are: (position,dot,payload)
      list<tuple<vector<bool>, pair<I, int>, T>>  l;
      I                                           id;  
      DotContext<I>                               cbase;
      DotContext<I>&                              c;
  };
}


#endif // _DELTA_ENABLED_CRDTS_H_