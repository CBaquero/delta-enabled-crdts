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

using namespace std;

template<typename T>
T join(const T& l, const T& r) // assuming copy constructor
{
  T res=l;
  res.join(r);
  return res;
}

template<typename T>
class gset
{
private:
  set<T> s;

public:

  set<T> read () { return s; }

  bool in (const T& val) 
  { 
    if ( s.find(val) == s.end() )
      return false;
    else
      return true;
  }

  friend ostream &operator<<( ostream &output, const gset<T>& o)
  { 
    typename set<T>::iterator it;
    output << "GSet: ( ";
    for (it=o.s.begin(); it!=o.s.end(); ++it)
      output << *it << " ";
    output << ")";
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

  bool in (const T& val) 
  { 
    if ( s.find(val) == s.end() )
      return false;
    else
      return true;
  }

  friend ostream &operator<<( ostream &output, const twopset<T>& o)
  { 
    typename set<T>::iterator it;
    output << "2PSet: S ( ";
    for (it=o.s.begin(); it!=o.s.end(); ++it)
      output << *it << " ";
    output << ")";
    output << " T ( ";
    for (it=o.t.begin(); it!=o.t.end(); ++it)
      output << *it << " ";
    output << ")";
    return output;            
  }

  twopset<T> add (const T& val) 
  { 
    twopset<T> res;
    if (t.find(val) == t.end()) // only add if not in tombstone set
    {
      s.insert(val);
      res.s.insert(val); 
    }
    return res; 
  }

  twopset<T> rmv (const T& val) 
  { 
    twopset<T> res;
    typename set<T>::iterator it;
    if ((it=s.find(val)) != s.end()) // remove from s if present
    {
      s.erase(it);
    }
    t.insert(val); // add to tombstones
    res.t.insert(val); 
    return res; 
  }

  void join (const twopset<T>& o)
  {
    typename set<T>::iterator soit;
    typename set<T>::iterator sit;
    for (soit=o.t.begin(); soit!=o.t.end(); ++soit) // see other tombstones
    {
      t.insert(*soit); // insert them locally
      if ((sit=s.find(*soit)) != s.end()) // remove val if present
        s.erase(sit);
    }
    for (soit=o.s.begin(); soit!=o.s.end(); ++soit) // add other vals 
      if (t.find(*soit) == t.end()) // only add if not in tombstone set
        s.insert(*soit); 
  }
};

class gcounter
{
private:
  map<string,int> m;

public:
  gcounter inc(string id, int tosum=1) // 2nd argument is optional
  {
    gcounter res;
    pair<map<string,int>::iterator,bool> ret;
    ret=m.insert(pair<string,int>(id,tosum));
    if (ret.second==false) // already there, so update it
      m.at(id)+=tosum;
    res.m.insert(pair<string,int>(id,m.at(id)));
    return res;
  }

  int read() // get counter value
  {
    int res=0;
    map<string,int>::iterator mit;
    for(mit=m.begin(); mit!=m.end(); ++mit) res+=mit->second;
    return res;
  }

  void join(const gcounter& o)
  {
    map<string,int>::const_iterator it;
    pair<map<string,int>::const_iterator,bool> ret;
    for (it=o.m.begin(); it!=o.m.end(); ++it)
    {
      ret=m.insert(*it);
      if (ret.second==false) // already there, so update it
        m.at(ret.first->first)=max(ret.first->second,it->second);
    }

  }

  friend ostream &operator<<( ostream &output, const gcounter& o)
  { 
    map<string,int>::const_iterator it;
    output << "GCounter: ( ";
    for (it=o.m.begin(); it!=o.m.end(); ++it)
      output << it->first << "->" << it->second << " ";
    output << ")";
    return output;            
  }

};

class pncounter
{
private:
  gcounter p,n;

public:
  pncounter inc(string id, int tosum=1) // 2nd argument is optional
  {
    pncounter res;
    res.p = p.inc(id,tosum); 
    return res;
  }

  pncounter dec(string id, int tosum=1) // 2nd argument is optional
  {
    pncounter res;
    res.n = n.inc(id,tosum); 
    return res;
  }


  int read() // get counter value
  {
    int res=p.read()-n.read();
    return res;
  }

  void join(const pncounter& o)
  {
    p.join(o.p);
    n.join(o.n);
  }

  friend ostream &operator<<( ostream &output, const pncounter& o)
  { 
    output << "PNCounter:P:" << o.p << " PNCounter:N:" << o.n;
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

  friend ostream &operator<<( ostream &output, dotkernel<T>& o)
  { 
    output << "Kernel: DS ( ";
    for(typename  map<pair<string,int>,T>::iterator it=o.ds.begin(); 
        it!=o.ds.end(); ++it)
      output <<  it->first.first << ":" << it->first.second << 
        "->" << it->second << " ";
    output << ")";
    output << " CC ( ";
    for(map<string,int>::iterator it=o.cc.begin(); it!=o.cc.end(); ++it)
      output << it->first << ":" << it->second << " ";
    output << ")";
    output << " DC ( ";
    for(set<pair<string,int> >::iterator it=o.dc.begin(); it!=o.dc.end(); ++it)
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
  friend ostream &operator<<( ostream &output, aworset<T>& o)
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
    aworset<T> r,a;
    r.dk=dk.rmv(val); // optimization that first deletes val
    a.dk=dk.add(id,val);
    r.join(a);
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
class mvreg    // Multi-value register, Optimized
{
private:
  dotkernel<T> dk; // Dot kernel

public:
  friend ostream &operator<<( ostream &output, mvreg<T>& o)
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


template<typename T>
class maxord // Keeps the max value in some total order 
{
private:
  T n,f; 

public:
  maxord(T i) : n(i), f(i) { }

  friend ostream &operator<<( ostream &output, const maxord<T>& o)
  { 
    output << "MaxOrder: " << o.n;
    return output;            
  }

  maxord<T> write(const T& val)
  {
    maxord<T> r(f);
    n=max(n,val);
    r.n=val;
    return r;
  }

  maxord<T> read() 
  { 
    return n; 
  }

  void join (maxord<T> o) // Join doesnt change initial f value
  {
    n=max(n,o.n);
  }
};
