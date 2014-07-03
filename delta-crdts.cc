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

