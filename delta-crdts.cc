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
#include <map>
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

  twopset<T> read () { return s; }

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

template <typename K=string, typename V=int>
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
    gcounter<K,V> res;
    m[id]+=tosum;
    res.m[id]=m[id];
    return res;
  }

  bool operator == ( const gcounter<K,V>& o ) const 
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

  void join(const gcounter<K,V>& o)
  {
    for (const auto& okv : o.m)
      m[okv.first]=max(okv.second,m[okv.first]);
  }

  friend ostream &operator<<( ostream &output, const gcounter<K,V>& o)
  { 
    output << "GCounter: ( ";
    for (const auto& kv : o.m)
      output << kv.first << "->" << kv.second << " ";
    output << ")";
    return output;            
  }

};

template <typename K=string, typename V=int>
class pncounter
{
private:
  gcounter<K,V> p,n;

public:
  pncounter() {} // Only for deltas and those should not be mutated
  pncounter(K a) : p(a), n(a) {} // Mutable replicas need a unique id

  pncounter inc(V tosum={1}) // Argument is optional
  {
    pncounter<K,V> res;
    res.p = p.inc(tosum); 
    return res;
  }

  pncounter dec(V tosum={1}) // Argument is optional
  {
    pncounter<K,V> res;
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

  friend ostream &operator<<( ostream &output, const pncounter<K,V>& o)
  { 
    output << "PNCounter:P:" << o.p << " PNCounter:N:" << o.n;
    return output;            
  }

};

/*
template<typename T>
class maxord // Keeps the max  in a total order starting at default value 
{
private:
  T n {}; 

public:

  friend ostream &operator<<( ostream &output, const maxord<T>& o)
  { 
    output << "MaxOrder: " << o.n;
    return output;            
  }

  operator T& () { return n; }

  bool operator == ( const maxord<T>& o ) const { return n==o.n; }
  bool operator > ( const maxord<T>& o ) const { return n>o.n; }
  bool operator < ( const maxord<T>& o ) const { return n<o.n; }
  bool operator <= ( const maxord<T>& o ) const { return n<=o.n; }
  bool operator >= ( const maxord<T>& o ) const { return n>=o.n; }
  bool operator != ( const maxord<T>& o ) const { return n!=o.n; }

  maxord<T> write(const T& val) // method being deprecated, use =
  {
    maxord<T> r;
    n=max(n,val);
    r.n=n;
    return r;
  }

  const T & read() const
  {
    return n;
  }

  maxord<T> & operator=(const maxord<T>& o)
  {
    if (this != &o) // no need to self assign
      n=max(n,o.n);
    return *this;
  }

  maxord<T> & operator=(const T& t) // Can encapsulate in the type T
  {
    n=max(n,t);
    return *this;
  }

  maxord<T> & operator+=(const maxord<T> &o)
  {
    n=max(n,n+o.n); // cant go down
    return *this;
  }

  maxord<T> & operator+=(const T &t)
  {
    n=max(n,n+t); // cant go down
    return *this;
  }

  maxord<T> & operator-=(const maxord<T> &o)
  {
    n=max(n,n-o.n); // cant go down
    return *this;
  }

  maxord<T> & operator-=(const T &t)
  {
    n=max(n,n-t); // cant go down
    return *this;
  }

  maxord<T> & operator++()
  {
    n++;
    return *this;
  }

  maxord<T> & operator--()
  {
    return *this; // Really cant go down
  }

  void join (maxord<T> o) 
  {
    n=max(n,o.n);
  }
};
*/

template <typename K=string, typename V=int>
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
    lexcounter<K,V> res;

//    m[id].first+=1; // optional
    m[id].second+=tosum;
    res.m[id]=m[id];

    return res;
  }

  lexcounter dec(V tosum=1) // Argument is optional
  {
    lexcounter<K,V> res;

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

  void join(const lexcounter<K,V>& o)
  {
    for (const auto& okv : o.m)
      m[okv.first]=lexjoin(okv.second,m[okv.first]); 
//      CBM: fix maxord first to make it work
      /*
    {
        if (m[okv.first].first < okv.second.first) // update it
          m[okv.first]=okv.second;
        else if (m[okv.first].first == okv.second.first) // tie
        {
          m[okv.first].second=max(m[okv.first].second,okv.second.second);
        }
        // othwerwise (>) dont update
    }
    */
  }

  friend ostream &operator<<( ostream &output, const lexcounter<K,V>& o)
  { 
    output << "LexCounter: ( ";
    for (const auto& kv : o.m)
      output << kv.first << "->" << kv.second << " ";
    output << ")";
    return output;            
  }

};


template<typename T>
class dotkernel
{
public:

  map<pair<string,int>,T> ds;  // Map of dots to vals
  map<string,int> cc; // Compact causal context
  set<pair<string,int> > dc; // Dot cloud

  friend ostream &operator<<( ostream &output, const dotkernel<T>& o)
  { 
    output << "Kernel: DS ( ";
    for(typename  map<pair<string,int>,T>::const_iterator it=o.ds.begin(); 
        it!=o.ds.end(); ++it)
      output <<  it->first.first << ":" << it->first.second << 
        "->" << it->second << " ";
    output << ")";
    output << " CC ( ";
    for(map<string,int>::const_iterator it=o.cc.begin(); it!=o.cc.end(); ++it)
      output << it->first << ":" << it->second << " ";
    output << ")";
    output << " DC ( ";
    for(set<pair<string,int> >::const_iterator it=o.dc.begin(); it!=o.dc.end(); ++it)
      output << it->first << ":" << it->second << " ";
    output << ")";
    return output;            
  }

  bool dotin (const pair<string,int> & d) const
  {
    map<string,int>::const_iterator itm=cc.find(d.first);
    if (itm != cc.end() && d.second <= cc.at(d.first)) return true;
    set<pair<string,int> >::const_iterator its=dc.find(d);
    if (its != dc.end()) return true;
    return false;
  }

  void compact()
  {
    // Compact DC to CC if possible
    map<string,int>::iterator mit;
    set<pair<string,int> >::iterator sit;
    bool flag; // may need to compact several times if ordering not best
    do
    {
      flag=false;
      for(sit = dc.begin(); sit != dc.end();)
      {
        mit=cc.find(sit->first); 
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

  void join (const dotkernel<T> & o)
  {
    if (this == &o) return; // Join is idempotent, but just dont do it.
    // DS
    // will iterate over the two sorted sets to compute join
    typename  map<pair<string,int>,T>::iterator it;
    typename  map<pair<string,int>,T>::const_iterator ito;
    it=ds.begin(); ito=o.ds.begin();
    do 
    {
      if ( it != ds.end() && ( ito == o.ds.end() || it->first < ito->first))
      {
        // cout << "ds one\n";
        // dot only at this
        if (o.dotin(it->first)) // other knows dot, must delete here 
          ds.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.ds.end() && ( it == ds.end() || ito->first < it->first))
      {
        //cout << "ds two\n";
        // dot only at other
        if(! dotin(ito->first)) // If I dont know, import
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
    typename  map<string,int>::iterator mit;
    typename  map<string,int>::const_iterator mito;
    mit=cc.begin(); mito=o.cc.begin();
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
  }

  pair<string,int> makedot(string id)
  {
    // On a valid dot generator, all dots should be compact on the used id
    // Making the new dot, updates the dot generator and returns the dot
    pair<map<string,int>::iterator,bool> ret;
    ret=cc.insert(pair<string,int>(id,1));
    if (ret.second==false) // already there, so update it
      cc.at(id)+=1; 
    // new dot is at id now: (id,cc.at(id))
    return pair<string,int>(id,cc.at(id));
  }

  dotkernel<T> add (string id, const T& val) 
  {
    dotkernel<T> res;
    // get new dot
    pair<string,int> dot=makedot(id);
    // add under new dot
    ds.insert(pair<pair<string,int>,T>(dot,val));
    // make delta
    res.ds.insert(pair<pair<string,int>,T>(dot,val));
    res.dc.insert(dot);
    return res;
  }

  dotkernel<T> rmv (const T& val)  // remove all dots matching value
  {
    dotkernel<T> res;
    typename  map<pair<string,int>,T>::iterator dsit;
    for(dsit=ds.begin(); dsit != ds.end();)
    {
      if (dsit->second == val) // match
      {
        res.dc.insert(dsit->first);
        ds.erase(dsit++);
      }
      else
        ++dsit;
    }
    res.compact(); // Maybe several dots so atempt compactation
    return res;
  }

  dotkernel<T> rmv ()  // remove all dots 
  {
    dotkernel<T> res;
    typename  map<pair<string,int>,T>::iterator dsit;
    for(dsit=ds.begin(); dsit != ds.end();)
    {
      res.dc.insert(dsit->first);
      ds.erase(dsit++);
    }
    res.compact(); // Maybe several dots so atempt compactation
    return res;
  }

};

template<typename T>
class aworset    // Add-Wins Observed-Remove Set
{
private:
  dotkernel<T> dk; // Dot kernel

public:
  friend ostream &operator<<( ostream &output, const aworset<T>& o)
  { 
    output << "AWORSet:" << o.dk;
    return output;            
  }

  set<T> read ()
  {
    set<T> res;
    typename map<pair<string,int>,T>::iterator dsit;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      res.insert(dsit->second);
    }
    return res;
  }

  bool in (const T& val) 
  { 
    typename map<pair<string,int>,T>::iterator dsit;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      if (dsit->second == val)
        return true;
    }
    return false;
  }


  aworset<T> add (string id, const T& val) 
  {
    aworset<T> r;
    r.dk=dk.rmv(val); // optimization that first deletes val
    r.dk.join(dk.add(id,val));
    return r;
  }

  aworset<T> rmv (const T& val)
  {
    aworset<T> r;
    r.dk=dk.rmv(val); 
    return r;
  }

  void join (aworset<T> o)
  {
    dk.join(o.dk);
    // Further optimization can be done by keeping for val x and id A 
    // only the highest dot from A supporting x. 
  }
};

template<typename T>
class rworset    // Remove-Wins Observed-Remove Set
{
private:
  dotkernel<pair<T,bool> > dk; // Dot kernel

public:
  friend ostream &operator<<( ostream &output, const rworset<T>& o)
  { 
    output << "RWORSet:" << o.dk;
    return output;            
  }

  set<T> read ()
  {
    set<T> res;
    map<T,bool> elems;
    typename map<pair<string,int>,pair<T,bool> >::iterator dsit;
    pair<typename map<T,bool>::iterator,bool> ret;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      ret=elems.insert(pair<T,bool>(dsit->second));
      if (ret.second==false) // val already exists
      {
        elems.at(ret.first->first) &= dsit->second.second; // Fold by &&
      }
    }
    typename map<T,bool>::iterator mit;
    for (mit=elems.begin(); mit != elems.end(); ++mit)
    {
      if (mit->second == true) res.insert(mit->first);
    }
    return res;
  }

  bool in (const T& val) // Could
  { 
    // Code could be slightly faster if re-using only part of read code
    set<T> s=read();
    if ( s.find(val) != s.end() ) return true;
    return false;
  }


  rworset<T> add (string id, const T& val) 
  {
    rworset<T> r;
    r.dk=dk.rmv(pair<T,bool>(val,true));  // Remove any observed add token
    r.dk.join(dk.rmv(pair<T,bool>(val,false))); // Remove any observed remove token
    r.dk.join(dk.add(id,pair<T,bool>(val,true)));
    return r;
  }

  rworset<T> rmv (string id, const T& val)
  {
    rworset<T> r;
    r.dk=dk.rmv(pair<T,bool>(val,true));  // Remove any observed add token
    r.dk.join(dk.rmv(pair<T,bool>(val,false))); // Remove any observed remove token
    r.dk.join(dk.add(id,pair<T,bool>(val,false)));
    return r;
  }

  void join (rworset<T> o)
  {
    dk.join(o.dk);
  }
};



template<typename T>
class mvreg    // Multi-value register, Optimized
{
private:
  dotkernel<T> dk; // Dot kernel

public:
  friend ostream &operator<<( ostream &output, const mvreg<T>& o)
  { 
    output << "MVReg:" << o.dk;
    return output;            
  }

  mvreg<T> write (string id, const T& val) 
  {
    mvreg<T> r,a;
    r.dk=dk.rmv(); 
    a.dk=dk.add(id,val);
    r.join(a);
    return r;
  }

  set<T> read ()
  {
    set<T> s;
    typename  map<pair<string,int>,T>::iterator dsit;
    for(dsit=dk.ds.begin(); dsit != dk.ds.end();++dsit)
    {
      s.insert(dsit->second);
    }
    return s;
  }

  void join (mvreg<T> o)
  {
    dk.join(o.dk);
  }
};


class ewflag    // Enable-Wins Flag
{
private:
  // To re-use the kernel there is an artificial need for dot-tagged bool payload
  dotkernel<bool> dk; // Dot kernel

public:
  friend ostream &operator<<( ostream &output, const ewflag& o)
  { 
    output << "EWFlag:" << o.dk;
    return output;            
  }

  bool read ()
  {
    typename map<pair<string,int>,bool>::iterator dsit;
    if ( dk.ds.begin() == dk.ds.end()) 
      // No active dots
      return false;
    else
      // Some dots
      return true;
  }

  ewflag enable (string id) 
  {
    ewflag r;
    r.dk=dk.rmv(true); // optimization that first deletes active dots
    r.dk.join(dk.add(id,true));
    return r;
  }

  ewflag disable ()
  {
    ewflag r;
    r.dk=dk.rmv(true); 
    return r;
  }

  void join (ewflag o)
  {
    dk.join(o.dk);
  }
};

class dwflag    // Disable-Wins Flag
{
private:
  // To re-use the kernel there is an artificial need for dot-tagged bool payload
  dotkernel<bool> dk; // Dot kernel

public:
  friend ostream &operator<<( ostream &output, const dwflag& o)
  { 
    output << "DWFlag:" << o.dk;
    return output;            
  }

  bool read ()
  {
    typename map<pair<string,int>,bool>::iterator dsit;
    if ( dk.ds.begin() == dk.ds.end()) 
      // No active dots
      return true;
    else
      // Some dots
      return false;
  }

  dwflag disable (string id) 
  {
    dwflag r;
    r.dk=dk.rmv(false); // optimization that first deletes active dots
    r.dk.join(dk.add(id,false));
    return r;
  }

  dwflag enable ()
  {
    dwflag r;
    r.dk=dk.rmv(false); 
    return r;
  }

  void join (dwflag o)
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


