//-------------------------------------------------------------------
//
// File:      delta-crdts.cc
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

template< bool b > 
struct join_selector { 
  template< typename T > 
  static T join( const T& l, const T& r ) 
  { 
    T res;
    res=l;
    res.join(r);
    return res;
  } 
};

template<> 
struct join_selector < true > { 
  template< typename T > 
  static T join( const T& l, const T& r ) 
  { 
    T res;
    res=max(l,r);
    return res;
  } 
};


// Join with C++ traits
template<typename T> // Join two objects, deriving a new one
T join(const T& l, const T& r) // assuming copy constructor
{
  return join_selector< is_arithmetic<T>::value >::join(l,r);
}

template<typename A, typename B> // Join two pairs of objects
pair<A,B> join(const pair<A,B>& l, const pair<A,B>& r)
{
  pair<A,B> res;
  res.first=join(r.first,l.first);
  res.second=join(r.second,l.second);
  return res;
}

template<typename A, typename B> // Join lexicographic of two pairs of objects
pair<A,B> lexjoin(const pair<A,B>& l, const pair<A,B>& r)
{
  pair<A,B> res;
  if (r==l) return res=r;
  if (l.first > r.first) return res=l;
  if (r.first > l.first) return res=r;
  // Left is equal, so join right
  if (r.first == l.first)
  {
    res.first=r.first;
    res.second=join(r.second,l.second);
    return res;
  }
  // Otherwise A is not a total order so keep res empty to signal error
  return res;
}

template<typename A, typename B> // Output a pair
ostream &operator<<( ostream &output, const pair<A,B>& o)
{
  output << "(" << o.first << "," << o.second << ")";
  return output;
}

template<typename T> // Output a set
ostream &operator<<( ostream &output, const set<T>& o)
{
  output << "( ";
  for (const auto& e : o) output << e << " ";
  output << ")";
  return output;
}

template<typename T> // get a point among two points
vector<T> among(const vector<T> & l, const vector<T> & r, int j=0)
{
  // Overall strategy is to first try wide advances to the right, 
  // as compact as possible. If that fails, go with fine grain (less compact)
  // advances until eventually succeed. 
  assert (l < r);
  vector<T> res;
  // adjust res as forwardly compact as possible
  for (int is = 0; is <= l.size(); is++)
  {
    res.assign(l.begin(),l.begin()+is); // get initial segment
    if ( is < l.size() ) // if partial segment, try appending one
    {
      res.push_back(true);
      if ( res >= l && res < r ) break; // see if we are there 
    }
  }
  assert (res >= l && res < r);
  if (res > l) return res;
  //vector<T> res=l;
  // forward finer and finer
  for(int i = 0; i < j; i++){
    res.push_back(false);
  }
  res.push_back(true);    
  
  while (res >= r)
  {
    res.back()=false;
    for(int i = 0; i < j; i++){
      res.push_back(false);
    }
    res.push_back(true);    
  }
  assert (res > l && res < r);
  return res;
}

template<typename T> // Output a vector
ostream &operator<<( ostream &output, const vector<T>& o)
{
  output << "[";
  for (const auto & e : o) output << e;
  output << "]";
  return output;
}

// Autonomous causal context, for context sharing in maps
template<typename K>
class dotcontext
{
public:
  map<K,int> cc; // Compact causal context
  set<pair<K,int> > dc; // Dot cloud

  dotcontext<K> & operator=(const dotcontext<K> & o)
  {
    if (&o == this) return *this;
    cc=o.cc; dc=o.dc;
    return *this;
  }

  friend ostream &operator<<( ostream &output, const dotcontext<K>& o)
  { 
    output << "Context:";
    output << " CC ( ";
    for (const auto & ki : o.cc)
      output << ki.first << ":" << ki.second << " ";
    output << ")";
    output << " DC ( ";
    for (const auto & ki : o.dc)
      output << ki.first << ":" << ki.second << " ";
    output << ")";
    return output;            
  }

  bool dotin (const pair<K,int> & d) const
  {
    const auto itm = cc.find(d.first);
    if (itm != cc.end() && d.second <= itm->second) return true;
    if (dc.count(d)!=0) return true;
    return false;
  }

  void compact()
  {
    // Compact DC to CC if possible
    //typename map<K,int>::iterator mit;
    //typename set<pair<K,int> >::iterator sit;
    bool flag; // may need to compact several times if ordering not best
    do
    {
      flag=false;
      for(auto sit = dc.begin(); sit != dc.end();)
      {
        auto mit=cc.find(sit->first); 
        if (mit==cc.end()) // No CC entry
          if (sit->second == 1) // Can compact
          {
            cc.insert(*sit);
            dc.erase(sit++);
            flag=true;
          }
          else ++sit;
        else // there is a CC entry already
          if (sit->second == cc.at(sit->first) + 1) // Contiguous, can compact
          {
            cc.at(sit->first)++;
            dc.erase(sit++);
            flag=true;
          }
          else 
            if (sit->second <= cc.at(sit->first)) // dominated, so prune
            {
              dc.erase(sit++);
              // no extra compaction oportunities so flag untouched
            }
            else ++sit;
      }
    }
    while(flag==true);
  }

  pair<K,int> makedot(const K & id)
  {
    // On a valid dot generator, all dots should be compact on the used id
    // Making the new dot, updates the dot generator and returns the dot
    // pair<typename map<K,int>::iterator,bool> ret;
    auto kib=cc.insert(pair<K,int>(id,1));
    if (kib.second==false) // already there, so update it
      (kib.first->second)+=1;
    //return dot;
    return pair<K,int>(*kib.first);
  }

  void insertdot(const pair<K,int> & d, bool compactnow=true)
  {
    // Set
    dc.insert(d);
    if (compactnow) compact();
  }


  void join (const dotcontext<K> & o)
  {
    if (this == &o) return; // Join is idempotent, but just dont do it.
    // CC
    //typename  map<K,int>::iterator mit;
    //typename  map<K,int>::const_iterator mito;
    auto mit=cc.begin(); auto mito=o.cc.begin();
    do 
    {
      if (mit != cc.end() && (mito == o.cc.end() || mit->first < mito->first))
      {
        // cout << "cc one\n";
        // entry only at here
        ++mit;
      }
      else if (mito != o.cc.end() && (mit == cc.end() || mito->first < mit->first))
      {
        // cout << "cc two\n";
        // entry only at other
        cc.insert(*mito);
        ++mito;
      }
      else if ( mit != cc.end() && mito != o.cc.end() )
      {
        // cout << "cc three\n";
        // in both
        cc.at(mit->first)=max(mit->second,mito->second);
        ++mit; ++mito;
      }
    } while (mit != cc.end() || mito != o.cc.end());
    // DC
    // Set
    for (const auto & e : o.dc)
      insertdot(e,false);

    compact();

  }

};

template <typename T, typename K>
class dotkernel
{
public:

  map<pair<K,int>,T> ds;  // Map of dots to vals

  dotcontext<K> cbase;
  dotcontext<K> & c;

  // if no causal context supplied, used base one
  dotkernel() : c(cbase) {} 
  // if supplied, use a shared causal context
  dotkernel(dotcontext<K> &jointc) : c(jointc) {} 
//  dotkernel(const dotkernel<T,K> &adk) : c(adk.c), ds(adk.ds) {}

  dotkernel<T,K> & operator=(const dotkernel<T,K> & adk)
  {
    if (&adk == this) return *this;
    if (&c != &adk.c) c=adk.c; 
    ds=adk.ds;
    return *this;
  }

  friend ostream &operator<<( ostream &output, const dotkernel<T,K>& o)
  { 
    output << "Kernel: DS ( ";
    for (const auto & dv : o.ds)
      output <<  dv.first.first << ":" << dv.first.second << 
        "->" << dv.second << " ";
    output << ") ";

    cout << o.c;

    return output;            
  }

  void join (const dotkernel<T,K> & o)
  {
    if (this == &o) return; // Join is idempotent, but just dont do it.
    // DS
    // will iterate over the two sorted sets to compute join
    //typename  map<pair<K,int>,T>::iterator it;
    //typename  map<pair<K,int>,T>::const_iterator ito;
    auto it=ds.begin(); auto ito=o.ds.begin();
    do 
    {
      if ( it != ds.end() && ( ito == o.ds.end() || it->first < ito->first))
      {
        // dot only at this
        if (o.c.dotin(it->first)) // other knows dot, must delete here 
          ds.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.ds.end() && ( it == ds.end() || ito->first < it->first))
      {
        // dot only at other
        if(! c.dotin(ito->first)) // If I dont know, import
          ds.insert(*ito);
        ++ito;
      }
      else if ( it != ds.end() && ito != o.ds.end() )
      {
        // dot in both
        ++it; ++ito;
      }
    } while (it != ds.end() || ito != o.ds.end() );
    // CC
    c.join(o.c);
  }

  void deepjoin (const dotkernel<T,K> & o)
  {
    if (this == &o) return; // Join is idempotent, but just dont do it.
    // DS
    // will iterate over the two sorted sets to compute join
    //typename  map<pair<K,int>,T>::iterator it;
    //typename  map<pair<K,int>,T>::const_iterator ito;
    auto it=ds.begin(); auto ito=o.ds.begin();
    do 
    {
      if ( it != ds.end() && ( ito == o.ds.end() || it->first < ito->first))
      {
        // dot only at this
        if (o.c.dotin(it->first)) // other knows dot, must delete here 
          ds.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.ds.end() && ( it == ds.end() || ito->first < it->first))
      {
        // dot only at other
        if(! c.dotin(ito->first)) // If I dont know, import
          ds.insert(*ito);
        ++ito;
      }
      else if ( it != ds.end() && ito != o.ds.end() )
      {
        // dot in both
        // check it payloads are diferent 
        if (it->second != ito->second)
        {
          // if payloads are not equal, they must be mergeable
          // use the more general binary join
          it->second=::join(it->second,ito->second);
        }
        ++it; ++ito;
      }
    } while (it != ds.end() || ito != o.ds.end() );
    // CC
    c.join(o.c);
  }


  dotkernel<T,K> add (const K& id, const T& val) 
  {
    dotkernel<T,K> res;
    // get new dot
    pair<K,int> dot=c.makedot(id);
    // add under new dot
    ds.insert(pair<pair<K,int>,T>(dot,val));
    // make delta
    res.ds.insert(pair<pair<K,int>,T>(dot,val));
    res.c.insertdot(dot);
    return res;
  }

  // Add that returns the added dot, instead of kernel delta
  pair<K,int> dotadd (const K& id, const T& val)
  {
    // get new dot
    pair<K,int> dot=c.makedot(id);
    // add under new dot
    ds.insert(pair<pair<K,int>,T>(dot,val));
    return dot;
  }

  dotkernel<T,K> rmv (const T& val)  // remove all dots matching value
  {
    dotkernel<T,K> res;
    //typename  map<pair<K,int>,T>::iterator dsit;
    for(auto dsit=ds.begin(); dsit != ds.end();)
    {
      if (dsit->second == val) // match
      {
        res.c.insertdot(dsit->first,false); // result knows removed dots
        ds.erase(dsit++);
      }
      else
        ++dsit;
    }
    res.c.compact(); // Maybe several dots there, so atempt compactation
    return res;
  }

  dotkernel<T,K> rmv (const pair<K,int>& dot)  // remove a dot 
  {
    dotkernel<T,K> res;
    auto dsit=ds.find(dot);
    if (dsit != ds.end()) // found it
    {
      res.c.insertdot(dsit->first,false); // result knows removed dots
      ds.erase(dsit++);
    }
    res.c.compact(); // Atempt compactation
    return res;
  }

  dotkernel<T,K> rmv ()  // remove all dots 
  {
    dotkernel<T,K> res;
    for (const auto & dv : ds) 
      res.c.insertdot(dv.first,false);
    res.c.compact();
    ds.clear(); // Clear the payload, but remember context
    return res;
  }

};

template <typename V=int, typename K=string>
class gcounter
{
private:
  map<K,V> m;
  K id;

public:
  gcounter() {} // Only for deltas and those should not be mutated
  gcounter(K a) : id(a) {} // Mutable replicas need a unique id

  gcounter inc(V tosum={1}) // argument is optional
  {
    gcounter<V,K> res;
    m[id]+=tosum;
    res.m[id]=m[id];
    return res;
  }

  bool operator == ( const gcounter<V,K>& o ) const 
  { 
    return m==o.m; 
  }

  V local() // get local counter value // CBM make this const
  {
    V res=0;
    res += m[id];
    return res;
  }

  V read() const // get counter value
  {
    V res=0;
    for (const auto& kv : m) // Fold+ on value list
      res += kv.second;
    return res;
  }

  void join(const gcounter<V,K>& o)
  {
    for (const auto& okv : o.m)
      m[okv.first]=max(okv.second,m[okv.first]);
  }

  friend ostream &operator<<( ostream &output, const gcounter<V,K>& o)
  { 
    output << "GCounter: ( ";
    for (const auto& kv : o.m)
      output << kv.first << "->" << kv.second << " ";
    output << ")";
    return output;            
  }

};

template <typename V=int, typename K=string>
class pncounter
{
private:
  gcounter<V,K> p,n;

public:
  pncounter() {} // Only for deltas and those should not be mutated
  pncounter(K a) : p(a), n(a) {} // Mutable replicas need a unique id

  pncounter inc(V tosum={1}) // Argument is optional
  {
    pncounter<V,K> res;
    res.p = p.inc(tosum); 
    return res;
  }

  pncounter dec(V tosum={1}) // Argument is optional
  {
    pncounter<V,K> res;
    res.n = n.inc(tosum); 
    return res;
  }

  V local() // get local counter value
  {
    V res=p.local()-n.local();
    return res;
  }

  V read() const // get counter value
  {
    V res=p.read()-n.read();
    return res;
  }

  void join(const pncounter& o)
  {
    p.join(o.p);
    n.join(o.n);
  }

  friend ostream &operator<<( ostream &output, const pncounter<V,K>& o)
  { 
    output << "PNCounter:P:" << o.p << " PNCounter:N:" << o.n;
    return output;            
  }

};

template <typename V=int, typename K=string>
class lexcounter
{
private:
  map<K,pair<int,V> > m;
  K id;

public:
  lexcounter() {} // Only for deltas and those should not be mutated
  lexcounter(K a) : id(a) {} // Mutable replicas need a unique id

  lexcounter inc(V tosum=1) // Argument is optional
  {
    lexcounter<V,K> res;

//    m[id].first+=1; // optional
    m[id].second+=tosum;
    res.m[id]=m[id];

    return res;
  }

  lexcounter dec(V tosum=1) // Argument is optional
  {
    lexcounter<V,K> res;

    m[id].first+=1; // mandatory
    m[id].second-=tosum;
    res.m[id]=m[id];

    return res;
  }

  V read() const // get counter value
  {
    V res=0;
    for (const auto& kv : m) // Fold+ on value list
      res +=  kv.second.second;
    return res;
  }

  void join(const lexcounter<V,K>& o)
  {
    for (const auto& okv : o.m)
      m[okv.first]=lexjoin(okv.second,m[okv.first]); 
  }

  friend ostream &operator<<( ostream &output, const lexcounter<V,K>& o)
  { 
    output << "LexCounter: ( ";
    for (const auto& kv : o.m)
      output << kv.first << "->" << kv.second << " ";
    output << ")";
    return output;            
  }

};

template<typename V, typename K=string>
class ccounter    // Causal counter, variation of Riak_dt_emcntr and lexcounter 
{
private:
  // To re-use the kernel there is an artificial need for dot-tagged bool payload
  dotkernel<V,K> dk; // Dot kernel
  K id;

public:
  ccounter() {} // Only for deltas and those should not be mutated
  ccounter(K k) : id(k) {} // Mutable replicas need a unique id
  ccounter(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  dotcontext<K> & context()
  {
    return dk.c;
  }

  friend ostream &operator<<( ostream &output, const ccounter<V,K>& o)
  { 
    output << "CausalCounter:" << o.dk;
    return output;            
  }

  ccounter<V,K> inc (const V& val=1) 
  {
    ccounter<V,K> r;
    set<pair<K,int>> dots; // dots to remove, should be only 1
    V base = {}; // typically 0
    for(const auto & dsit : dk.ds)
    {
      if (dsit.first.first == id) // there should be a single one such
      {
        base=max(base,dsit.second);
        dots.insert(dsit.first);
      }
    }
    for(const auto & dot : dots)
      r.dk.join(dk.rmv(dot));
    r.dk.join(dk.add(id,base+val));
    return r;
  }

  ccounter<V,K> dec (const V& val=1) 
  {
    ccounter<V,K> r;
    set<pair<K,int>> dots; // dots to remove, should be only 1
    V base = {}; // typically 0
    for(const auto & dsit : dk.ds)
    {
      if (dsit.first.first == id) // there should be a single one such
      {
        base=max(base,dsit.second);
        dots.insert(dsit.first);
      }
    }
    for(const auto & dot : dots)
      r.dk.join(dk.rmv(dot));
    r.dk.join(dk.add(id,base-val));
    return r;
  }

  ccounter<V,K> reset () // Other nodes might however upgrade their counts
  {
    ccounter<V,K> r;
    r.dk=dk.rmv(); 
    return r;
  }

  V read ()
  {
    V v = {}; // Usually 0
    for (const auto & dse : dk.ds)
      v+=dse.second;
    return v;
  }

  void join (ccounter<V,K> o)
  {
    dk.join(o.dk);
  }

};


template<typename T>
class gset
{
private:
  set<T> s;

public:

  gset() {}
//  gset(string id, dotcontext<K> &jointdc) {}

  // For map compliance reply with an empty context
//  dotcontext<K> context()
//  {
//    return dotcontext<K>();
//  }

  set<T> read () const { return s; }

  bool operator == ( const gset<T>& o ) const { return s==o.s; }

  bool in (const T& val) 
  { 
    return s.count(val);
  }

  friend ostream &operator<<( ostream &output, const gset<T>& o)
  { 
    output << "GSet: " << o.s;
    return output;            
  }

  gset<T> add (const T& val) 
  { 
    gset<T> res;
    s.insert(val); 
    res.s.insert(val); 
    return res; 
  }

  void join (const gset<T>& o)
  {
    s.insert(o.s.begin(), o.s.end());
  }

};


template<typename T, typename K=string> // Map embedable datatype
class twopset
{
private:
  set<T> s;
  set<T> t;  // removed elements are added to t and removed from s

public:

  twopset() {}
  twopset(string id, dotcontext<K> &jointdc) {}
  // For map compliance reply with an empty context
  dotcontext<K> context()
  {
    return dotcontext<K>();
  }

  set<T> read () { return s; }

  bool operator == ( const twopset<T>& o ) const 
  { 
    return s==o.s && t==o.t; 
  }

  bool in (const T& val) 
  { 
    return s.count(val);
  }

  friend ostream &operator<<( ostream &output, const twopset<T>& o)
  { 
    output << "2PSet: S" << o.s << " T " << o.t;
    return output;            
  }

  twopset<T> add (const T& val) 
  { 
    twopset<T> res;
    if (t.count(val) == 0) // only add if not in tombstone set
    {
      s.insert(val);
      res.s.insert(val); 
    }
    return res; 
  }

  twopset<T> rmv (const T& val) 
  { 
    twopset<T> res;
    s.erase(val);
    t.insert(val); // add to tombstones
    res.t.insert(val); 
    return res; 
  }

  twopset<T> reset ()
  {
    twopset<T> res;
    for (auto const & val : s)
    {
      t.insert(val);
      res.t.insert(val); 
    }
    s.clear();
    return res; 
  }

  void join (const twopset<T>& o)
  {
    for (const auto& ot : o.t) // see other tombstones
    {
      t.insert(ot); // insert them locally
      if (s.count(ot) == 1) // remove val if present
        s.erase(ot);
    }
    for (const auto& os : o.s) // add other vals, if not tombstone
    {
      if (t.count(os) == 0) s.insert(os);
    }
  }
};


template<typename E, typename K=string> // Map embedable datatype
class aworset    // Add-Wins Observed-Remove Set
{
private:
  dotkernel<E,K> dk; // Dot kernel
  K id;

public:
  aworset() {} // Only for deltas and those should not be mutated
  aworset(K k) : id(k) {} // Mutable replicas need a unique id
  aworset(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  dotcontext<K> & context()
  {
    return dk.c;
  }

  friend ostream &operator<<( ostream &output, const aworset<E,K>& o)
  { 
    output << "AWORSet:" << o.dk;
    return output;            
  }


  set<E> read ()
  {
    set<E> res;
    for (const auto &dv : dk.ds)
      res.insert(dv.second);
    return res;
  }

  bool in (const E& val) 
  { 
    typename map<pair<K,int>,E>::iterator dsit;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      if (dsit->second == val)
        return true;
    }
    return false;
  }


  aworset<E,K> add (const E& val) 
  {
    aworset<E,K> r;
    r.dk=dk.rmv(val); // optimization that first deletes val
    r.dk.join(dk.add(id,val));
    return r;
  }

  aworset<E,K> rmv (const E& val)
  {
    aworset<E,K> r;
    r.dk=dk.rmv(val); 
    return r;
  }
  
  aworset<E,K> reset()
  {
    aworset<E,K> r;
    r.dk=dk.rmv(); 
    return r;
  }

  void join (aworset<E,K> o)
  {
    dk.join(o.dk);
    // Further optimization can be done by keeping for val x and id A 
    // only the highest dot from A supporting x. 
  }
};

template<typename E, typename K=string> // Map embedable datatype
class rworset    // Remove-Wins Observed-Remove Set
{
private:
  dotkernel<pair<E,bool>,K> dk; // Dot kernel
  K id;

public:
  rworset() {} // Only for deltas and those should not be mutated
  rworset(K k) : id(k) {} // Mutable replicas need a unique id
  rworset(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  dotcontext<K> & context()
  {
    return dk.c;
  }


  friend ostream &operator<<( ostream &output, const rworset<E,K>& o)
  { 
    output << "RWORSet:" << o.dk;
    return output;            
  }

  set<E> read ()
  {
    set<E> res;
    map<E,bool> elems;
    typename map<pair<K,int>,pair<E,bool> >::iterator dsit;
    pair<typename map<E,bool>::iterator,bool> ret;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      ret=elems.insert(pair<E,bool>(dsit->second));
      if (ret.second==false) // val already exists
      {
        elems.at(ret.first->first) &= dsit->second.second; // Fold by &&
      }
    }
    typename map<E,bool>::iterator mit;
    for (mit=elems.begin(); mit != elems.end(); ++mit)
    {
      if (mit->second == true) res.insert(mit->first);
    }
    return res;
  }

  bool in (const E& val) // Could
  { 
    // Code could be slightly faster if re-using only part of read code
    set<E> s=read();
    if ( s.find(val) != s.end() ) return true;
    return false;
  }


  rworset<E,K> add (const E& val) 
  {
    rworset<E,K> r;
    r.dk=dk.rmv(pair<E,bool>(val,true));  // Remove any observed add token
    r.dk.join(dk.rmv(pair<E,bool>(val,false))); // Remove any observed remove token
    r.dk.join(dk.add(id,pair<E,bool>(val,true)));
    return r;
  }

  rworset<E,K> rmv (const E& val)
  {
    rworset<E,K> r;
    r.dk=dk.rmv(pair<E,bool>(val,true));  // Remove any observed add token
    r.dk.join(dk.rmv(pair<E,bool>(val,false))); // Remove any observed remove token
    r.dk.join(dk.add(id,pair<E,bool>(val,false)));
    return r;
  }

  rworset<E,K> reset()
  {
    rworset<E,K> r;
    r.dk=dk.rmv(); 
    return r;
  }


  void join (rworset<E,K> o)
  {
    dk.join(o.dk);
  }
};



template<typename V, typename K=string>
class mvreg    // Multi-value register, Optimized
{
private:
  dotkernel<V,K> dk; // Dot kernel
  K id;

public:
  mvreg() {} // Only for deltas and those should not be mutated
  mvreg(K k) : id(k) {} // Mutable replicas need a unique id
  mvreg(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  dotcontext<K> & context()
  {
    return dk.c;
  }

  friend ostream &operator<<( ostream &output, const mvreg<V,K>& o)
  { 
    output << "MVReg:" << o.dk;
    return output;            
  }

  mvreg<V,K> write (const V& val) 
  {
    mvreg<V,K> r,a;
    r.dk=dk.rmv(); 
    a.dk=dk.add(id,val);
    r.join(a);
    return r;
  }

  set<V> read ()
  {
    set<V> s;
    for (const auto & dse : dk.ds)
      s.insert(dse.second);
    return s;
  }

  mvreg<V,K> reset()
  {
    mvreg<V,K> r;
    r.dk=dk.rmv(); 
    return r;
  }

  mvreg<V,K> resolve()
  {
    mvreg<V,K> r,v;
    set<V> s; // collect all values that are not maximals
    for (const auto & dsa : dk.ds) // Naif quadratic comparison
      for (const auto & dsb : dk.ds)
        if ( dsa.second != dsb.second && 
            ::join(dsa.second,dsb.second) == dsb.second ) // < based on join
        // if (dsa.second < dsb.second) // values must implement operator<
          s.insert(dsa.second);
    // remove all non maximals and register deltas for those removals
    for (const auto & val : s)
    {
      v.dk=dk.rmv(val);
      r.join(v);
    }
    return r;
  }

  void join (mvreg<V,K> o)
  {
    dk.join(o.dk);
  }
};


template<typename K=string>
class ewflag    // Enable-Wins Flag
{
private:
  // To re-use the kernel there is an artificial need for dot-tagged bool payload
  dotkernel<bool,K> dk; // Dot kernel
  K id;

public:
  ewflag() {} // Only for deltas and those should not be mutated
  ewflag(K k) : id(k) {} // Mutable replicas need a unique id
  ewflag(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  dotcontext<K> & context()
  {
    return dk.c;
  }

  friend ostream &operator<<( ostream &output, const ewflag<K>& o)
  { 
    output << "EWFlag:" << o.dk;
    return output;            
  }

  bool read ()
  {
    typename map<pair<K,int>,bool>::iterator dsit;
    if ( dk.ds.begin() == dk.ds.end()) 
      // No active dots
      return false;
    else
      // Some dots
      return true;
  }

  ewflag<K> enable () 
  {
    ewflag<K> r;
    r.dk=dk.rmv(true); // optimization that first deletes active dots
    r.dk.join(dk.add(id,true));
    return r;
  }

  ewflag<K> disable ()
  {
    ewflag<K> r;
    r.dk=dk.rmv(true); 
    return r;
  }

  ewflag<K> reset()
  {
    ewflag<K> r;
    r.dk=dk.rmv(); 
    return r;
  }

  void join (ewflag<K> o)
  {
    dk.join(o.dk);
  }
};

template<typename K=string>
class dwflag    // Disable-Wins Flag
{
private:
  // To re-use the kernel there is an artificial need for dot-tagged bool payload
  dotkernel<bool,K> dk; // Dot kernel
  K id;

public:
  dwflag() {} // Only for deltas and those should not be mutated
  dwflag(K k) : id(k) {} // Mutable replicas need a unique id
  dwflag(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  dotcontext<K> & context()
  {
    return dk.c;
  }

  friend ostream &operator<<( ostream &output, const dwflag<K>& o)
  { 
    output << "DWFlag:" << o.dk;
    return output;            
  }

  bool read ()
  {
    typename map<pair<K,int>,bool>::iterator dsit;
    if ( dk.ds.begin() == dk.ds.end()) 
      // No active dots
      return true;
    else
      // Some dots
      return false;
  }

  dwflag<K> disable () 
  {
    dwflag<K> r;
    r.dk=dk.rmv(false); // optimization that first deletes active dots
    r.dk.join(dk.add(id,false));
    return r;
  }

  dwflag<K> enable ()
  {
    dwflag<K> r;
    r.dk=dk.rmv(false); 
    return r;
  }

  dwflag<K> reset()
  {
    dwflag<K> r;
    r.dk=dk.rmv(); 
    return r;
  }

  void join (dwflag<K> o)
  {
    dk.join(o.dk);
  }
};

// U is timestamp, T is payload
template<typename U, typename T>
class rwlwwset // remove wins bias for identical timestamps
{
private:
  map<T,pair<U,bool> > s;

  rwlwwset<U,T> addrmv(const U& ts, const T& val, bool b)
  {
    rwlwwset<U,T> res;
    pair<U,bool> a(ts,b);
    res.s.insert(pair<T,pair<U,bool> >(val,a));
    pair<typename map<T,pair<U,bool> >::iterator,bool> ret;
    ret=s.insert(pair<T,pair<U,bool> >(val,a));
    if (ret.second == false ) // some value there
    {
        s.at(ret.first->first)=lexjoin(ret.first->second,a);
    }
    return res;
  }

public:

  friend ostream &operator<<( ostream &output, const rwlwwset<U,T>& o)
  { 
    output << "RW LWWSet: ( ";
    for(typename  map< T,pair<U,bool> >::const_iterator it=o.s.begin(); it != o.s.end(); ++it)
    {
      if( it->second.second == false)
        output << it->first << " ";
    }
    output << ")" << endl;
    return output;            
  }

  rwlwwset<U,T> add(const U& ts, const T& val)
  {
    return addrmv(ts,val,false);
  }

  rwlwwset<U,T> rmv(const U& ts, const T& val)
  {
    return addrmv(ts,val,true);
  }


  bool in (const T& val) 
  { 
    typename  map<T,pair<U,bool> >::const_iterator it=s.find(val); 
    if ( it == s.end() || it->second.second == true)
      return false;
    else
      return true;
  }

  void join (const rwlwwset<U,T> & o)
  {
    if (this == &o) return; // Join is idempotent, but just dont do it.
    // will iterate over the two sorted sets to compute join
    typename  map< T, pair<U,bool> >::iterator it; 
    typename  map< T, pair<U,bool> >::const_iterator ito; 
    it=s.begin(); ito=o.s.begin();
    do 
    {
      if ( it != s.end() && ( ito == o.s.end() || it->first < ito->first))
      {
        // entry only at this
        // keep it
        ++it;
      }
      else if ( ito != o.s.end() && ( it == s.end() || ito->first < it->first))
      {
        // entry only at other
        // import it
        s.insert(*ito);
        ++ito;
      }
      else if ( it != s.end() && ito != o.s.end() )
      {
        // in both
        // merge values by lex operator
        s.at(it->first)=lexjoin(it->second,ito->second);
        ++it; ++ito;
      }
    } while (it != s.end() || ito != o.s.end() );
  }


};

template<typename U, typename T>
class lwwreg // U must be comparable 
{
private:
  pair<U, T> r;

public:

  friend ostream &operator<<( ostream &output, const lwwreg<U,T>& o)
  { 
    output << "LWWReg: " << o.r;
    return output;            
  }

  void join (const lwwreg<U,T>& o)
  {
    if (o.r.first > r.first)
    {
      r=o.r;
    }
  }

  lwwreg<U,T> write (const U& ts, const T& val)
  {
    lwwreg<U,T> res;
    res.r.first=ts;
    res.r.second=val;
    join(res);  // Will only update if ts is greater
    return res;
  }

  T read()
  {
    return r.second;
  }
};

template<typename N, typename V, typename K=string>
class ormap
{
  map<N,V> m;  
  
  dotcontext<K> cbase;
  dotcontext<K> & c;
  K id;

  public:
  // if no causal context supplied, use base one
  ormap() : c(cbase) {} 
  ormap(K i) : id(i), c(cbase) {} 
  // if supplied, use a shared causal context
  ormap(K i, dotcontext<K> &jointc) : id(i), c(jointc) {} 

//  ormap( const ormap<N,V,K>& o ) :  id(o.id), m(o.m), c(o.c) {}

  ormap<N,V,K> & operator=(const ormap<N,V,K> & o)
  {
    if (&o == this) return *this;
    if (&c != &o.c) c=o.c; 
    m=o.m; id=o.id;
    return *this;
  }

  dotcontext<K> & context() const
  {
    return c;
  }

  friend ostream &operator<<( ostream &output, const ormap<N,V,K>& o)
  { 
    output << "Map:" << o.c << endl;
    for (const auto & kv : o.m)
      cout << kv.first << "->" << kv.second << endl;
    return output;            
  }

  // Need to find a way to collect deltas for this interface
  V& operator[] (const N& n)
  {
    auto i = m.find(n);
    if (i == m.end()) // 1st key access
    {
      auto ins = m.insert(i,pair<N,V>(n,V(id,c)));
      return ins->second;

    }
    else
    {
      return i->second;
    }
  }

  ormap<N,V,K> erase(const N & n)
  {
    ormap<N,V,K> r;
    if (m.count(n) != 0)
    {
      // need to collect erased dots, and list then in r context
      V v;
      v=m[n].reset();
      r.c=v.context();
      // now erase
      m.erase(n);
    }
    return r;
  }

  ormap<N,V,K> reset()
  {
    ormap<N,V,K> r;
    if (! m.empty())
    {
      // need to collect erased dots, and list then in r context
      for (auto & kv : m)
      {
        V v;
        v=kv.second.reset();
        r.c.join(v.context());
      }
      m.clear();
    }
    return r;
  }


  void join (const ormap<N,V> & o)
  {
    const dotcontext<K> ic=c; // need access to an immutable context

    // join all keys
    auto mit=m.begin(); auto mito=o.m.begin();
    do 
    {
      // ---- debug
      /*
      cout << "key left ";
      if (mit != m.end()) 
        cout << mit->first;
      else
        cout << "[empty]";
      cout << ", key rigth ";
      if (mito != o.m.end()) 
        cout << mito->first;
      else
        cout << "[empty]";
      cout << endl;
      */
      if (mit != m.end() && (mito == o.m.end() || mit->first < mito->first))
      {
        //cout << "entry left\n";
        // entry only at here
        
        // creaty and empty payload with the other context, since it might   
        // obsolete some local entries. 
        V empty(id,o.context());
        mit->second.join(empty);
        c=ic;

        ++mit;
      }
      else if (mito != o.m.end() && (mit == m.end() || mito->first < mit->first))
      {
        // cout << "entry right\n";
        // entry only at other

        (*this)[mito->first].join(mito->second);
        c=ic;


        ++mito;
      }
      else if ( mit != m.end() && mito != o.m.end() )
      {
        // cout << "entry both\n";
        // in both
        (*this)[mito->first].join(mito->second);
        c=ic;

        ++mit; ++mito;
      }
    } while (mit != m.end() || mito != o.m.end());
    c.join(o.c);

  }


};

// A bag is similar to an RWSet, but allows for CRDT payloads
template<typename V, typename K=string>
class bag 
{
private:
  dotkernel<V,K> dk; // Dot kernel
  K id;

public:

  bag() {} // Only for deltas and those should not be mutated
  bag(K k) : id(k) {} // Mutable replicas need a unique id
  bag(K k, dotcontext<K> &jointc) : id(k), dk(jointc) {} 

  bag<V,K> & operator=(const bag<V,K> & o)
  {
    if (&o == this) return *this;
    if (&dk != &o.dk) dk=o.dk; 
    id=o.id;
    return *this;
  }


  dotcontext<K> & context()
  {
    return dk.c;
  }

  void insert(pair<pair<K,int>,V> t)
  {
    dk.ds.insert(pair<pair<K,int>,V>(t));
    dk.c.insertdot(t.first);
  }

  friend ostream &operator<<( ostream &output, const bag<V,K>& o)
  { 
    output << "Bag:" << o.dk;
    return output;            
  }

  typename map<pair<K,int>,V>::iterator begin()
  {
    return dk.ds.begin();
  }

  typename map<pair<K,int>,V>::iterator end()
  {
    return dk.ds.end();
  }

  pair<K,int> mydot()
  {
    auto me = dk.ds.end();
    for (auto it=dk.ds.begin(); it!=dk.ds.end(); ++it)
    {
      if (it->first.first == id) // a candidate
      {
        if (me==dk.ds.end()) // pick at least one valid
          me=it;
        else // need to switch if more recent
        {
          if (it->first.second > me->first.second)
            me=it;
        }
      }
    }
    if (me != dk.ds.end())
      return me->first;
    else 
    {
      fresh();
      return mydot(); // After a fresh it must be found
    }
  }

  V & mydata()
  {
    auto me = dk.ds.end();
    for (auto it=dk.ds.begin(); it!=dk.ds.end(); ++it)
    {
      if (it->first.first == id) // a candidate
      {
        if (me==dk.ds.end()) // pick at least one valid
          me=it;
        else // need to switch if more recent
        {
          if (it->first.second > me->first.second)
            me=it;
        }
      }
    }
    if (me != dk.ds.end())
      return me->second;
    else 
    {
      fresh();
      return mydata(); // After a fresh it must be found
    }
  }

  // To protect from concurrent removes, create fresh dot for self
  void fresh()
  {
    dk.add(id,V());
  }

  bag<V,K> reset()
  {
    bag<V,K> r;
    r.dk=dk.rmv(); 
    return r;
  }

  // Using the deep join will try to join different payloads under same dot
  void join (const bag<V,K> & o)
  {
    dk.deepjoin(o.dk);
  }
};

// Inspired by designs from Carl Lerche and Paulo S. Almeida
template<typename V, typename K=string>
class rwcounter    //  Reset Wins Counter
{
private:
  bag<pair<V,V>,K> b; // Bag of pairs
  K id;

public:
  rwcounter() {} // Only for deltas and those should not be mutated
  rwcounter(K k) : id(k), b(k) {} // Mutable replicas need a unique id
  rwcounter(K k, dotcontext<K> &jointc) : id(k), b(k,jointc) {} 

  rwcounter<V,K> & operator=(const rwcounter<V,K> & o)
  {
    if (&o == this) return *this;
    if (&b != &o.b) b=o.b; 
    id=o.id; 
    return *this;
  }

  dotcontext<K> & context()
  {
    return b.context();
  }

  friend ostream &operator<<( ostream &output, const rwcounter<V,K>& o)
  { 
    output << "ResetWinsCounter:" << o.b;
    return output;            
  }
  
  rwcounter<V,K> inc (const V& val=1) 
  {
    rwcounter<V,K> r;
    b.mydata().first+=val;
    r.b.insert(pair<pair<K,int>,pair<V,V>>(b.mydot(),b.mydata()));
    return r;
  }

  rwcounter<V,K> dec (const V& val=1) 
  {
    rwcounter<V,K> r;
    b.mydata().second+=val;
    r.b.insert(pair<pair<K,int>,pair<V,V>>(b.mydot(),b.mydata()));
    return r;
  }

  rwcounter<V,K> reset()
  {
    rwcounter<V,K> r;
    r.b=b.reset();
    return r;
  }

  void fresh()
  {
    b.fresh();
  }

  V read()
  {
    pair<V,V> ac;
    for (const auto & dv : b)
    {
      ac.first+=dv.second.first;
      ac.second+=dv.second.second;
    }
    return ac.first - ac.second;
  }

  void join(const rwcounter<V,K> & o)
  {
    b.join(o.b);
  }

};

template<typename N, typename V>
class gmap
{
  
  public:
  // later make m private by adding a begin() for iterators 
  map<N,V> m;  

  friend ostream &operator<<( ostream &output, const gmap<N,V>& o)
  { 
    output << "GMap:" << endl;
    for (const auto & kv : o.m)
      cout << kv.first << "->" << kv.second << endl;
    return output;            
  }

  // Need to find a better way to collect deltas for this interface
  V& operator[] (const N& n)
  {
    auto i = m.find(n);
    if (i == m.end()) // 1st key access
    {
      auto ins = m.insert(i,pair<N,V>(n,V()));
      return ins->second;

    }
    else
    {
      return i->second;
    }
  }

  void join (const gmap<N,V> & o)
  {
    // join all keys
    auto mit=m.begin(); auto mito=o.m.begin();
    do 
    {
      if (mit != m.end() && (mito == o.m.end() || mit->first < mito->first))
      {
        //cout << "entry left\n";
        // entry only at here

        ++mit;
      }
      else if (mito != o.m.end() && (mit == m.end() || mito->first < mit->first))
      {
        // cout << "entry right\n";
        // entry only at other

        (*this)[mito->first]=mito->second;

        ++mito;
      }
      else if ( mit != m.end() && mito != o.m.end() )
      {
        // cout << "entry both\n";
        // in both
        (*this)[mito->first]=::join((*this)[mito->first],mito->second);

        ++mit; ++mito;
      }
    } while (mit != m.end() || mito != o.m.end());

  }

};


template <typename V=int, typename K=string>
class bcounter
{
private:
  pncounter<V,K> c;
  gmap<pair<K,K>,int> m; 
  K id;

public:

  bcounter() {} // Only for deltas and those should not be mutated
  bcounter(K a) : id(a), c(a) {} // Mutable replicas need a unique id

  bcounter inc(V tosum={1}) // Argument is optional
  {
    bcounter<V,K> res;
    res.c = c.inc(tosum); 
    return res;
  }

  bcounter dec(V todec={1}) // Argument is optional
  {
    bcounter<V,K> res;
    if (todec <= local()) // Check local capacity
      res.c = c.dec(todec); 
    return res;
  }

  bcounter mv(V q, K to) // Quantity V to node id K
  {
    bcounter<V,K> res;
    if (q <= local()) // Check local capacity
    {
      m[pair<K,K>(id,to)]+=q; 
      res.m[pair<K,K>(id,to)]=m[pair<K,K>(id,to)];
    }
    return res;
  }

  V read() // get global counter value
  {
    V res=c.read();
    return res;
  }

  V local() // get local counter available value
  {
    V res=c.local();
    // Sum incoming flows
    for (const auto & kv : m.m)
      if (kv.first.second==id) res+=kv.second;
    // Subtract outgoing flows
    for (const auto & kv : m.m)
      if (kv.first.first==id) res-=kv.second;
    return res;
  }

  void join(const bcounter& o)
  {
    c.join(o.c);
    m.join(o.m);
  }

  friend ostream &operator<<( ostream &output, const bcounter<V,K>& o)
  { 
    output << "BCounter:C:" << o.c << "BCounter:M:" << o.m;
    return output;            
  }

};

template<typename T=char, typename I=string>
class orseq
{
private:

  // List elements are: (position,dot,payload)
  list<tuple<vector<bool>,pair<I,int>,T>> l;
  I id;  

  dotcontext<I> cbase;
  dotcontext<I> & c;

public:

  // if no causal context supplied, used base one
  orseq() : c(cbase) {}  // Only for deltas and those should not be mutated
  orseq(I i) : id(i), c(cbase) {} 
  // if supplied, use a shared causal context
  orseq(I i,dotcontext<I> &jointc) : id(i), c(jointc) {} 

  orseq<T,I> & operator=(const orseq<T,I> & aos)
  {
    if (&aos == this) return *this;
    if (&c != &aos.c) c=aos.c; 
    l=aos.l;
    id=aos.id;
    return *this;
  }

  friend ostream &operator<<( ostream &output, const orseq<T,I>& o)
  { 
    output << "ORSeq: " << o.c;
    output << " List:"; 
    for (const auto & t : o.l)
      output << "(" << get<0>(t) << " " << get<1>(t) 
        << " " << get<2>(t) << ")";
    return output;            
  }

  typename list<tuple<vector<bool>,pair<I,int>,T>>::iterator begin() 
  {
    return l.begin();
  }

  typename list<tuple<vector<bool>,pair<I,int>,T>>::iterator end() 
  {
    return l.end();
  }

  orseq<T,I> erase (typename list<tuple<vector<bool>,pair<I,int>,T>>::iterator i)
  {
    orseq<T,I> res;
    if (i != l.end())
    {
      res.c.insertdot(get<1>(*i));
      l.erase(i);
    }
    return res;
  }

  dotcontext<I> & context()
  {
    return c;
  }

  orseq<T,I> reset ()
  {
    orseq<T,I> res;
    for (auto const & t : l)
      res.c.insertdot(get<1>(t));
    l.clear();
    return res;
  }

  orseq<T,I> insert (typename list<tuple<vector<bool>,pair<I,int>,T>>::iterator i, const T & val)
  {
    orseq<T,I> res;
    if (i == l.end())
      res=push_back(val);
    else
      if (i == l.begin())
        res=push_front(val);
      else
      {
        typename list<tuple<vector<bool>,pair<I,int>,T>>::iterator j=i;
        j--;
        vector<bool> bl,br,pos;
        bl=get<0>(*j);
        br=get<0>(*i);
        pos=among(bl,br);
        // get new dot
        auto dot=c.makedot(id);
        auto tuple=make_tuple(pos,dot,val);
        l.insert(i,tuple);
        // delta
        res.c.insertdot(dot);
        res.l.push_front(tuple);
      }
    return res;
  }

  // add 1st element
  orseq<T,I> makefirst(const T & val)
  {
    assert(l.empty());

    orseq<T,I> res;
    vector<bool> bl,br,pos;
    bl.push_back(false);
    br.push_back(true);
    pos=among(bl,br);
    // get new dot
    pair<I,int> dot=c.makedot(id);
    l.push_back(make_tuple(pos,dot,val));
    // delta
    res.c.insertdot(dot);
    res.l=l;
    return res;
  }

  orseq<T,I> push_back (const T & val)
  {
    orseq<T,I> res;
    if (l.empty())
      res=makefirst(val);
    else
    {
      vector<bool> bl,br,pos;
      bl=get<0>(l.back());
      br.push_back(true);
      pos=among(bl,br);
      // get new dot
      auto dot=c.makedot(id);
      auto tuple=make_tuple(pos,dot,val);
      l.push_back(tuple);
      // delta
      res.c.insertdot(dot);
      res.l.push_front(tuple);
    }
    return res;
  }

  orseq<T,I> push_front (const T & val)
  {
    orseq<T,I> res;
    if (l.empty())
      res=makefirst(val);
    else
    {
      vector<bool> bl,br,pos;
      br=get<0>(l.front());
      bl.push_back(false);
      pos=among(bl,br);
      // get new dot
      auto dot=c.makedot(id);
      auto tuple=make_tuple(pos,dot,val);
      l.push_front(tuple);
      // delta
      res.c.insertdot(dot);
      res.l.push_front(tuple);
    }
    return res;
  }

  void join (const orseq<T,I> & o)
  {
    if (this == &o) return; // Join is idempotent, but just don't do it.
    auto it=l.begin(); auto ito=o.l.begin();
    pair<vector<bool>,I> e,eo;
    do 
    {
      if ( it != l.end() )
      {
        e.first=get<0>(*it);
        e.second=get<1>(*it).first;
      }
      if ( ito != o.l.end() )
      {
        eo.first=get<0>(*ito);
        eo.second=get<1>(*ito).first;
      }
      if ( it != l.end() && ( ito == o.l.end() || e < eo ) )
      {
        // cout << "ds one\n";
        // entry only at this
        if (o.c.dotin(get<1>(*it))) // other knows dot, must delete here 
          l.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.l.end() && ( it == l.end() || eo < e ) )
      {
        //cout << "ds two\n";
        // entry only at other
        if(! c.dotin(get<1>(*ito))) // If I dont know, import
        {
          l.insert(it,*ito); // it keeps pointing to next
        }
        ++ito;
      }
      else if ( it != l.end() && ito != o.l.end() )
      {
        // cout << "ds three\n";
        // in both
        ++it; ++ito;
      }
    } while (it != l.end() || ito != o.l.end() );
    // CC
    c.join(o.c);

  }

};

