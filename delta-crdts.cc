//-------------------------------------------------------------------
//
// File:      delta-crdts.cc
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
//   Reference implementation in C++ of delta enabled CRDTs
// @end  
//
//
//-------------------------------------------------------------------

#include <set>
#include <unordered_set>
#include <map>
#include <list>
#include <string>
#include <iostream>
#include <type_traits>

using namespace std;

//template<typename T> // Join two objects, deriving a new one
//T join(const T& l, const T& r) // assuming copy constructor
//{
//  T res;
//  res=l;
//  res.join(r);
//  return res;
//}

//int join(const int & l, const int & r)
//{
//  return max(l,r);
//}

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

template<typename T>
class gset
{
private:
  set<T> s;

public:

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

template<typename T>
class twopset
{
private:
  set<T> s;
  set<T> t;  // removed elements are added to t and removed from s

public:

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


  V read() // get counter value
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

// Autonomous causal context, for future map embeding
template<typename K>
class dotcontext
{
public:
  map<K,int> cc; // Compact causal context
  set<pair<K,int> > dc; // Dot cloud

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

  void insertdot(const pair<K,int> & d)
  {
    // Set
    dc.insert(d);
    compact();
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
      dc.insert(e);
    // List
    //for (const auto & e : o.dc)
    //  dc.push_front(e);

    compact();

  }

};

template <typename T, typename K>
class dotkernel
{
public:

  map<pair<K,int>,T> ds;  // Map of dots to vals

  /*
  map<K,int> cc; // Compact causal context
  set<pair<K,int> > dc; // Dot cloud
  */

  dotcontext<K> c;

  dotkernel() {} 

  friend ostream &operator<<( ostream &output, const dotkernel<T,K>& o)
  { 
    output << "Kernel: DS ( ";
    for (const auto & dv : o.ds)
      output <<  dv.first.first << ":" << dv.first.second << 
        "->" << dv.second << " ";

    output << ")";
    /*
    output << " CC ( ";
    for (const auto & ki : o.cc)
      output << ki.first << ":" << ki.second << " ";
    output << ")";
    output << " DC ( ";
    for (const auto & ki : o.dc)
      output << ki.first << ":" << ki.second << " ";
    output << ")";
    */
    cout << o.c;
    return output;            
  }

  /*
  bool dotin (const pair<K,int> & d) const
  {
    const auto itm = cc.find(d.first);
    if (itm != cc.end() && d.second <= itm->second) return true;
    if (dc.count(d)!=0) return true;
    return false;
  }
  */

  /*
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
  */

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
        // cout << "ds one\n";
        // dot only at this
        if (o.c.dotin(it->first)) // other knows dot, must delete here 
          ds.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.ds.end() && ( it == ds.end() || ito->first < it->first))
      {
        //cout << "ds two\n";
        // dot only at other
        if(! c.dotin(ito->first)) // If I dont know, import
          ds.insert(*ito);
        ++ito;
      }
      else if ( it != ds.end() && ito != o.ds.end() )
      {
        // cout << "ds three\n";
        // in both
        ++it; ++ito;
      }
    } while (it != ds.end() || ito != o.ds.end() );
    // CC
    c.join(o.c);
    /*
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
    dc.insert(o.dc.begin(),o.dc.end());

    compact();
    */
  }

  /*
  pair<K,int> makedot(const K & id)
  {
    // On a valid dot generator, all dots should be compact on the used id
    // Making the new dot, updates the dot generator and returns the dot
    // pair<typename map<K,int>::iterator,bool> ret;
    auto kib=cc.insert(pair<K,int>(id,1));
    if (kib.second==false) // already there, so update it
      (kib.first->second)+=1;
    //  cc.at(id)+=1; 
    // new dot is at id now: (id,cc.at(id))
    //return pair<K,int>(id,cc.at(id));
    return pair<K,int>(*kib.first);
  }
  */

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

  dotkernel<T,K> rmv (const T& val)  // remove all dots matching value
  {
    dotkernel<T,K> res;
    //typename  map<pair<K,int>,T>::iterator dsit;
    for(auto dsit=ds.begin(); dsit != ds.end();)
    {
      if (dsit->second == val) // match
      {
        res.c.insertdot(dsit->first); // result knows removed dots
        ds.erase(dsit++);
      }
      else
        ++dsit;
    }
    res.c.compact(); // Maybe several dots there, so atempt compactation
    return res;
  }

  dotkernel<T,K> rmv ()  // remove all dots 
  {
    dotkernel<T,K> res;
    /*
    for(auto dsit=ds.begin(); dsit != ds.end();)
    {
      res.dc.insert(dsit->first);
      ds.erase(dsit++);
    }
    */
    for (const auto & dv : ds) // Inserting by order, use that
      res.c.insertdot(dv.first);
    ds.clear(); // Clear the payload, but remember context
    return res;
  }

};

template<typename E, typename K=string>
class aworset    // Add-Wins Observed-Remove Set
{
private:
  dotkernel<E,K> dk; // Dot kernel
  K id;

public:
  aworset() {} // Only for deltas and those should not be mutated
  aworset(K k) : id(k) {} // Mutable replicas need a unique id

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
    /*
    typename map<pair<K,int>,E>::iterator dsit;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      res.insert(dsit->second);
    }
    */
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

  void join (aworset<E,K> o)
  {
    dk.join(o.dk);
    // Further optimization can be done by keeping for val x and id A 
    // only the highest dot from A supporting x. 
  }
};

template<typename E, typename K=string>
class rworset    // Remove-Wins Observed-Remove Set
{
private:
  dotkernel<pair<E,bool>,K> dk; // Dot kernel
  K id;

public:
  rworset() {} // Only for deltas and those should not be mutated
  rworset(K k) : id(k) {} // Mutable replicas need a unique id

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
    typename  map<pair<K,int>,V>::iterator dsit;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      s.insert(dsit->second);
    }
    return s;
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


