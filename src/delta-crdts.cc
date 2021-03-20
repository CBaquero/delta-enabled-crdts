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

#include "delta-crdts.h"


namespace crdts
{
  template<bool b> 
  struct JoinSelector 
  { 
    template<typename T> 
    static T join( const T& l, const T& r ) 
    { 
      T res;
      res = l;
      res.join(r);
      return res;
    } 
  };

  template<> 
  struct JoinSelector<true> 
  { 
    template<typename T> 
    static T join( const T& l, const T& r ) 
    { 
      T res;
      res = max( l, r );
      return res;
    } 
  };


  // Join with C++ traits
  // Join two objects, deriving a new one
  // assuming copy constructor
  template<typename T> 
  T join( const T& l, const T& r ) 
  {
    return join_selector< is_arithmetic<T>::value >::join( l, r );
  }


  // Join two pairs of objects
  template<typename A, typename B> 
  pair<A, B> join( const pair<A, B>& l, const pair<A, B>& r)
  {
    pair< A, B > res;
    res.first = join( r.first, l.first );
    res.second = join( r.second, l.second );
    return res;
  }

  // Join lexicographic of two pairs of objects
  template<typename A, typename B> 
  pair<A, B> lexjoin(const pair<A, B>& l, const pair<A, B>& r)
  {
    pair< A, B > res;
    if ( r == l ) return res = r;
    if ( l.first > r.first ) return res = l;
    if ( r.first > l.first ) return res = r;
    // Left is equal, so join right
    if ( r.first == l.first )
    {
      res.first = r.first;
      res.second = join( r.second, l.second );
      return res;
    }
    // Otherwise A is not a total order so keep res empty to signal error
    return res;
  }

  // Output a pair
  template<typename A, typename B> 
  ostream &operator<<( ostream &output, const pair<A, B>& o )
  {
    output << "(" << o.first << "," << o.second << ")";
    return output;
  }

  // Output a set
  template<typename T> 
  ostream &operator<<( ostream &output, const set<T>& o)
  {
    output << "( ";
    for (const auto& e : o) output << e << " ";
    output << ")";
    return output;
  }

  // get a point among two points
  template<typename T> 
  vector<T> among( const vector<T>& l, const vector<T>& r, int j)
  {
    // Overall strategy is to first try wide advances to the right, 
    // as compact as possible. If that fails, go with fine grain (less compact)
    // advances until eventually succeed. 
    assert(l < r);
    vector<T> res;
    // adjust res as forwardly compact as possible
    for ( int is = 0; is <= l.size(); is++ )
    {
      res.assign( l.begin(), l.begin() + is ); // get initial segment
      if ( is < l.size() ) // if partial segment, try appending one
      {
        res.push_back( true );
        if ( res >= l && res < r ) break; // see if we are there 
      }
    }

    assert(res >= l && res < r);
    if ( res > l ) return res;
    //vector<T> res=l;
    // forward finer and finer
    for( int i = 0; i < j; i++ )
    {
      res.push_back( false );
    }
    res.push_back( true );    
    
    while ( res >= r )
    {
      res.back() = false;
      for( int i = 0; i < j; i++ )
      {
        res.push_back( false );
      }
      res.push_back( true );    
    }

    assert( res > l && res < r );
    return res;
  }

  template<typename T> // Output a vector
  ostream& operator<<( ostream &output, const vector<T>& o)
  {
    output << "[";
    for (const auto & e : o) output << e;
    output << "]";
    return output;
  }

  // Autonomous causal context, for context sharing in maps
#pragma region DotContext

  
  template<typename K>
  DotContext<K>& DotContext<K>::operator=( const DotContext<K>& o )
  {
    if (&o == this) return *this;
    cc = o.cc; 
    dc = o.dc;
    return *this;
  }

  template<typename K>
  ostream& operator<<( ostream& output, const DotContext<K>& o )
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

  template<typename K>
  bool DotContext<K>::dotin( const pair<K, int>& d ) const
  {
    const auto itm = cc.find( d.first );
    if ( itm != cc.end() && d.second <= itm->second ) return true;
    if ( dc.count( d ) != 0 ) return true;
    return false;
  }

  template<typename K>
  void DotContext<K>::compact()
  {
    // Compact DC to CC if possible
    //typename map<K,int>::iterator mit;
    //typename set<pair<K,int> >::iterator sit;
    bool flag = true; // may need to compact several times if ordering not best
    while( flag == true )
    {
      flag = false;
      for( auto sit = dc.begin(); sit != dc.end(); )
      {
        auto mit = cc.find( sit->first ); 
        if ( mit == cc.end() ) // No CC entry
        {
          if ( sit->second == 1 ) // Can compact
          {
            cc.insert( *sit );
            dc.erase( sit++ );
            flag = true;
          }
          else ++sit;
        }
        else // there is a CC entry already
        {
          if ( sit->second == cc.at( sit->first ) + 1 ) // Contiguous, can compact
          {
            cc.at( sit->first )++;
            dc.erase( sit++ );
            flag = true;
          }
          else
          {
            if ( sit->second <= cc.at( sit->first ) ) // dominated, so prune
            {
              dc.erase( sit++ );
              // no extra compaction oportunities so flag untouched
            }
            else ++sit;
          }
        }
      }
    }
  }

  template<typename K>
  pair<K, int> DotContext<K>::makeDot( const K& id )
  {
    // On a valid dot generator, all dots should be compact on the used id
    // Making the new dot, updates the dot generator and returns the dot
    // pair<typename map<K,int>::iterator,bool> ret;
    auto kib = cc.insert( pair<K, int>( id, 1 ) );
    if ( kib.second == false ) // already there, so update it
      (kib.first->second) += 1;
    //return dot;
    return pair<K, int>(*kib.first);
  }

  template<typename K>
  void DotContext<K>::insertDot( const pair<K, int>& d, bool compactnow)
  {
    // Set
    dc.insert( d );
    if ( compactnow ) compact();
  }

  template<typename K>
  void DotContext<K>::join( const DotContext<K>& o )
  {
    if ( this == &o ) return; // Join is idempotent, but just dont do it.
    // CC
    //typename  map<K,int>::iterator mit;
    //typename  map<K,int>::const_iterator mito;
    auto mit = cc.begin(); auto mito = o.cc.begin();
    do 
    {
      if ( mit != cc.end() && ( mito == o.cc.end() || mit->first < mito->first ) )
      {
        // cout << "cc one\n";
        // entry only at here
        ++mit;
      }
      else if ( mito != o.cc.end() && ( mit == cc.end() || mito->first < mit->first ) )
      {
        // cout << "cc two\n";
        // entry only at other
        cc.insert( *mito );
        ++mito;
      }
      else if ( mit != cc.end() && mito != o.cc.end() )
      {
        // cout << "cc three\n";
        // in both
        cc.at( mit->first ) = max( mit->second, mito->second );
        ++mit; ++mito;
      }
    } while ( mit != cc.end() || mito != o.cc.end() );
    
    // DC
    // Set
    for (const auto & e : o.dc )
      insertDot( e, false );
    compact();
  }
#pragma endregion


#pragma region DotKernel
  template <typename T, typename K>
  DotKernel<T, K>& DotKernel<T, K>::operator=( const DotKernel<T, K>& adk )
  {
    if ( &adk == this ) return *this;
    if ( &c != &adk.c ) c = adk.c; 
    ds = adk.ds;
    return *this;
  }

  template <typename T, typename K>
  ostream& operator<<( ostream &output, const DotKernel<T, K>& o)
  { 
    output << "Kernel: DS ( ";
    
    for ( const auto & dv : o.ds )
      output <<  dv.first.first << ":" << dv.first.second << 
        "->" << dv.second << " ";
    
    output << ") ";

    cout << o.c;

    return output;            
  }

  template <typename T, typename K>
  void DotKernel<T, K>::join( const DotKernel<T, K>& o )
  {
    if ( this == &o ) return; // Join is idempotent, but just dont do it.
    // DS
    // will iterate over the two sorted sets to compute join
    //typename  map<pair<K,int>,T>::iterator it;
    //typename  map<pair<K,int>,T>::const_iterator ito;
    auto it = ds.begin(); auto ito = o.ds.begin();
    do 
    {
      if ( it != ds.end() && ( ito == o.ds.end() || it->first < ito->first ) )
      {
        // dot only at this
        if ( o.c.dotin( it->first ) ) // other knows dot, must delete here 
          ds.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.ds.end() && ( it == ds.end() || ito->first < it->first ) )
      {
        // dot only at other
        if( !c.dotin( ito->first ) ) // If I dont know, import
          ds.insert( *ito );
        ++ito;
      }
      else if ( it != ds.end() && ito != o.ds.end() )
      {
        // dot in both
        ++it; ++ito;
      }
    } while (it != ds.end() || ito != o.ds.end() );
    // CC
    c.join( o.c );
  }

  template <typename T, typename K>
  void DotKernel<T, K>::deepJoin( const DotKernel<T, K>& o )
  {
    if ( this == &o ) return; // Join is idempotent, but just dont do it.
    // DS
    // will iterate over the two sorted sets to compute join
    //typename  map<pair<K,int>,T>::iterator it;
    //typename  map<pair<K,int>,T>::const_iterator ito;
    auto it = ds.begin(); auto ito = o.ds.begin();
    do 
    {
      if ( it != ds.end() && ( ito == o.ds.end() || it->first < ito->first ) )
      {
        // dot only at this
        if ( o.c.dotin( it->first ) ) // other knows dot, must delete here 
          ds.erase( it++ );
        else // keep it
          ++it;
      }
      else if ( ito != o.ds.end() && ( it == ds.end() || ito->first < it->first))
      {
        // dot only at other
        if( !c.dotin( ito->first ) ) // If I dont know, import
          ds.insert( *ito );
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
          it->second = ::join( it->second, ito->second );
        }
        ++it; ++ito;
      }
    } while ( it != ds.end() || ito != o.ds.end() );
    // CC
    c.join( o.c );
  }

  template <typename T, typename K>
  DotKernel<T, K> DotKernel<T, K>::add( const K& id, const T& val ) 
  {
    DotKernel<T, K> res;
    // get new dot
    pair<K, int> dot = c.makeDot( id );
    // add under new dot
    ds.insert( pair<pair<K, int>, T>( dot, val ) );
    // make delta
    res.ds.insert( pair<pair<K, int>, T>( dot, val ) );
    res.c.insertDot( dot );
    return res;
  }

  // Add that returns the added dot, instead of kernel delta
  template <typename T, typename K>
  pair<K, int> DotKernel<T, K>::dotAdd( const K& id, const T& val )
  {
    // get new dot
    pair<K, int> dot = c.makeDot( id );
    // add under new dot
    ds.insert( pair<pair<K, int>, T>( dot, val ) );
    return dot;
  }

  // remove all dots matching value
  template <typename T, typename K>
  DotKernel<T, K> DotKernel<T, K>::remove( const T& val )  
  {
    DotKernel<T, K> res;
    //typename  map<pair<K,int>,T>::iterator dsit;
    for( auto dsit = ds.begin(); dsit != ds.end(); )
    {
      if ( dsit->second == val ) // match
      {
        res.c.insertDot( dsit->first, false ); // result knows removed dots
        ds.erase( dsit++ );
      }
      else
        ++dsit;
    }
    res.c.compact(); // Maybe several dots there, so atempt compactation
    return res;
  }

  // remove a dot 
  template <typename T, typename K>
  DotKernel<T, K> DotKernel<T, K>::remove( const pair<K, int>& dot )  
  {
    DotKernel<T, K> res;
    auto dsit = ds.find(dot);
    if (dsit != ds.end()) // found it
    {
      res.c.insertDot( dsit->first, false); // result knows removed dots
      ds.erase(dsit++);
    }
    res.c.compact(); // Atempt compactation
    return res;
  }

  // remove all dots 
  template <typename T, typename K>
  DotKernel<T, K> DotKernel<T, K>::remove()  
  {
    DotKernel<T, K> res;
    for ( const auto & dv : ds ) 
      res.c.insertDot( dv.first, false );
    
    res.c.compact();
    ds.clear(); // Clear the payload, but remember context
    return res;
  }

#pragma endregion

#pragma region GCounter
  // argument is optional
  template<typename V, typename K>
  GCounter<V, K> GCounter<V, K>::inc( V tosum) 
  {
    GCounter<V, K> res;
    m[id] += tosum;
    res.m[id] = m[id];
    return res;
  }

  template<typename V, typename K>
  bool GCounter<V, K>::operator==( const GCounter<V, K>& o ) const 
  { 
    return m == o.m; 
  }

  template<typename V, typename K>
  V GCounter<V, K>::local() // get local counter value // CBM make this const
  {
    V res = 0;
    res += m[id];
    return res;
  }

  template<typename V, typename K>
  V GCounter<V, K>::read() const // get counter value
  {
    V res=0;
    for ( const auto& kv : m ) // Fold+ on value list
      res += kv.second;
    return res;
  }

  template<typename V, typename K>
  void GCounter<V, K>::join( const GCounter<V, K>& o )
  {
    for (const auto& okv : o.m)
      m[okv.first] = max(okv.second,m[okv.first]);
  }

  template<typename V, typename K>
  ostream& operator<<( ostream &output, const GCounter<V, K>& o)
  { 
    output << "GCounter: ( ";
    for (const auto& kv : o.m)
      output << kv.first << "->" << kv.second << " ";
    
    output << ")";
    return output;            
  }

#pragma endregion

#pragma region PNCounter
  
  // Argument is optional
  template<typename V, typename K>
  PNCounter<V, K> PNCounter<V, K>::inc( V tosum) 
  {
    PNCounter<V, K> res;
    res.p = p.inc( tosum ); 
    return res;
  }

  // Argument is optional
  template<typename V, typename K>
  PNCounter<V, K> PNCounter<V, K>::dec( V tosum) 
  {
    PNCounter<V, K> res;
    res.n = n.inc( tosum ); 
    return res;
  }

  // get local counter value
  template<typename V, typename K>
  V PNCounter<V, K>::local() 
  {
    V res = p.local() - n.local();
    return res;
  }

  // get counter value
  template<typename V, typename K>
  V PNCounter<V, K>::read() const 
  {
    V res = p.read() - n.read();
    return res;
  }

  template<typename V, typename K>
  void PNCounter<V, K>::join(const PNCounter<V, K>& o)
  {
    p.join(o.p);
    n.join(o.n);
  }

  template<typename V, typename K>
  ostream& operator<<( ostream &output, const PNCounter<V, K>& o)
  { 
    output << "PNCounter:P:" << o.p << " PNCounter:N:" << o.n;
    return output;            
  }
#pragma endregion

#pragma region LexCounter
  // Argument is optional
  template <typename V, typename K>
  LexCounter<V, K> LexCounter<V, K>::inc(V tosum) 
  {
    LexCounter<V, K> res;

    // m[id].first+=1; // optional
    m[id].second += tosum;
    res.m[id] = m[id];

    return res;
  }

  // Argument is optional
  template <typename V, typename K>
  LexCounter<V, K> LexCounter<V, K>::dec(V tosum) 
  {
    LexCounter<V, K> res;

    m[id].first += 1; // mandatory
    m[id].second -= tosum;
    res.m[id] = m[id];

    return res;
  }

  // get counter value
  template <typename V, typename K>
  V LexCounter<V, K>::read() const // get counter value
  {
    V res = 0;
    for (const auto& kv : m) // Fold+ on value list
      res +=  kv.second.second;

    return res;
  }

  template <typename V, typename K>
  void LexCounter<V, K>::join( const LexCounter<V, K>& o )
  {
    for ( const auto& okv : o.m )
      m[okv.first] = lexjoin( okv.second, m[okv.first] ); 
  }

  template <typename V, typename K>
  ostream& operator<<( ostream& output, const LexCounter<V, K>& o)
  { 
    output << "LexCounter: ( ";
    for (const auto& kv : o.m)
      output << kv.first << "->" << kv.second << " ";
    
    output << ")";
    return output;            
  }

#pragma endregion


#pragma region CausalCounter
  template<typename V, typename K>
  DotContext<K>& CausalCounter<V, K>::context()
  {
    return dk.c;
  }

  template<typename V, typename K>
  ostream& operator<<( ostream &output, const CausalCounter<V, K>& o)
  { 
    output << "CausalCounter:" << o.dk;
    return output;            
  }

  template<typename V, typename K>
  CausalCounter<V, K> CausalCounter<V, K>::inc( const V& val) 
  {
    CausalCounter<V, K> r;
    set<pair<K, int>> dots; // dots to remove, should be only 1
    V base = {}; // typically 0
    for ( const auto & dsit : dk.ds )
    {
      if ( dsit.first.first == id) // there should be a single one such
      {
        base = max( base,dsit.second );
        dots.insert( dsit.first );
      }
    }

    for( const auto & dot : dots )
      r.dk.join( dk.remove( dot ) );

    r.dk.join( dk.add( id, base+val ) );
    return r;
  }

  template<typename V, typename K>
  CausalCounter<V, K> CausalCounter<V, K>::dec( const V& val) 
  {
    CausalCounter<V, K> r;
    set<pair<K, int>> dots; // dots to remove, should be only 1
    V base = {}; // typically 0
    for( const auto & dsit : dk.ds )
    {
      if ( dsit.first.first == id ) // there should be a single one such
      {
        base = max( base, dsit.second );
        dots.insert( dsit.first );
      }
    }

    for( const auto & dot : dots )
      r.dk.join( dk.remove( dot ) );

    r.dk.join( dk.add( id, base-val ) );
    return r;
  }

  // Other nodes might however upgrade their counts
  template<typename V, typename K>
  CausalCounter<V, K> CausalCounter<V, K>::reset()
  {
    CausalCounter<V, K> r;
    r.dk = dk.remove(); 
    return r;
  }

  template<typename V, typename K>
  V CausalCounter<V, K>::read()
  {
    V v = {}; // Usually 0
    for ( const auto & dse : dk.ds )
      v += dse.second;
    return v;
  }

  template<typename V, typename K>
  void CausalCounter<V, K>::join( CausalCounter<V, K> o )
  {
    dk.join( o.dk );
  }
#pragma endregion

#pragma region GSet
  //  GSet(string id, DotContext<K> &jointdc) {}

    // For map compliance reply with an empty context
  //  DotContext<K> context()
  //  {
  //    return DotContext<K>();
  //  }
    template<typename T>
    set<T> GSet<T>::read() const 
    { 
      return s; 
    }

    template<typename T>
    bool GSet<T>::operator==( const GSet<T>& o ) const 
    { 
      return s == o.s; 
    }

    template<typename T>
    bool GSet<T>::in( const T& val ) 
    { 
      return s.count( val );
    }

    template<typename T>
    ostream& operator<<( ostream& output, const GSet<T>& o )
    { 
      output << "GSet: " << o.s;
      return output;            
    }

    template<typename T>
    GSet<T> GSet<T>::add( const T& val ) 
    { 
      GSet<T> res;
      s.insert( val ); 
      res.s.insert( val ); 
      return res; 
    }

    template<typename T>
    void GSet<T>::join( const GSet<T>& o )
    {
      s.insert( o.s.begin(), o.s.end() );
    }

#pragma endregion


#pragma region TwoPSet
  // Map embedable datatype
  // For map compliance reply with an empty context
  template<typename T, typename K> 
  DotContext<K> TwoPSet<T, K>::context()
  {
    return DotContext<K>();
  }

  template<typename T, typename K> 
  set<T> TwoPSet<T, K>::read () 
  { 
    return s; 
  }

  template<typename T, typename K> 
  bool TwoPSet<T, K>::operator==( const TwoPSet<T>& o ) const 
  { 
    return s==o.s && t==o.t; 
  }

  template<typename T, typename K> 
  bool TwoPSet<T, K>::in( const T& val ) 
  { 
    return s.count( val );
  }

  template<typename T, typename K> 
  ostream& operator<<( ostream &output, const TwoPSet<T, K>& o)
  { 
    output << "2PSet: S" << o.s << " T " << o.t;
    return output;            
  }

  template<typename T, typename K> 
  TwoPSet<T> TwoPSet<T, K>::add( const T& val ) 
  { 
    TwoPSet<T> res;
    if ( t.count(val) == 0 ) // only add if not in tombstone set
    {
      s.insert( val );
      res.s.insert( val ); 
    }

    return res; 
  }

  template<typename T, typename K> 
  TwoPSet<T> TwoPSet<T, K>::rmv( const T& val ) 
  { 
    TwoPSet<T> res;
    s.erase( val );
    t.insert( val );    // add to tombstones
    res.t.insert( val ); 
    return res; 
  }

  template<typename T, typename K> 
  TwoPSet<T> TwoPSet<T, K>::reset()
  {
    TwoPSet<T> res;
    for ( auto const & val : s )
    {
      t.insert( val );
      res.t.insert( val ); 
    }

    s.clear();
    return res; 
  }

  template<typename T, typename K> 
  void TwoPSet<T, K>::join( const TwoPSet<T>& o )
  {
    for ( const auto& ot : o.t ) // see other tombstones
    {
      // insert them locally
      t.insert( ot ); 

      // remove val if present
      if ( s.count(ot) == 1 ) 
        s.erase(ot);
    }

    // add other vals, if not tombstone
    for (const auto& os : o.s) 
    {
      if (t.count(os) == 0) s.insert(os);
    }
  }
#pragma endregion

#pragma region AWORSet
  // Map embedable datatype
  template<typename E, typename K> 
  DotContext<K>& AWORSet<E, K>::context()
  {
    return dk.c;
  }

  template<typename E, typename K> 
  ostream& operator<<( ostream &output, const AWORSet<E, K>& o )
  { 
    output << "AWORSet:" << o.dk;
    return output;            
  }

  template<typename E, typename K> 
  set<E> AWORSet<E, K>::read()
  {
    set<E> res;
    for ( const auto &dv : dk.ds )
      res.insert( dv.second );

    return res;
  }

  template<typename E, typename K> 
  bool AWORSet<E, K>::in(const E& val) 
  { 
    typename map<pair<K, int>, E>::iterator dsit;
    for( dsit = dk.ds.begin(); dsit != dk.ds.end(); ++dsit)
    {
      if ( dsit->second == val )
        return true;
    }
    return false;
  }

  template<typename E, typename K> 
  AWORSet<E, K> AWORSet<E, K>::add( const E& val ) 
  {
    AWORSet<E, K> r;
    r.dk = dk.remove( val ); // optimization that first deletes val
    r.dk.join( dk.add( id, val ) );
    return r;
  }

  template<typename E, typename K> 
  AWORSet<E, K> AWORSet<E, K>::rmv( const E& val )
  {
    AWORSet<E, K> r;
    r.dk = dk.remove( val ); 
    return r;
  }
  
  template<typename E, typename K> 
  AWORSet<E,K> AWORSet<E, K>::reset()
  {
    AWORSet<E, K> r;
    r.dk = dk.remove(); 
    return r;
  }

  template<typename E, typename K> 
  void AWORSet<E, K>::join( AWORSet<E, K> o )
  {
    dk.join( o.dk );
    // Further optimization can be done by keeping for val x and id A 
    // only the highest dot from A supporting x. 
  }
#pragma endregion

#pragma region RWORSet
  // Map embedable datatype
  template<typename E, typename K> 
  DotContext<K> & RWORSet<E, K>::context()
  {
    return dk.c;
  }

  template<typename E, typename K> 
  ostream& operator<<( ostream &output, const RWORSet<E, K>& o )
  { 
    output << "RWORSet:" << o.dk;
    return output;            
  }

  template<typename E, typename K> 
  set<E> RWORSet<E, K>::read()
  {
    set<E> res;
    map<E, bool> elems;
    typename map<pair<K, int>, pair<E, bool>>::iterator dsit;
    pair<typename map<E, bool>::iterator, bool> ret;
    for( dsit = dk.ds.begin(); dsit != dk.ds.end(); ++dsit)
    {
      ret = elems.insert( pair<E, bool>(dsit->second ) );
      if (ret.second == false) // val already exists
      {
        elems.at( ret.first->first ) &= dsit->second.second; // Fold by &&
      }
    }

    typename map<E, bool>::iterator mit;
    for ( mit = elems.begin(); mit != elems.end(); ++mit )
    {
      if ( mit->second == true ) res.insert( mit->first );
    }
    return res;
  }

  template<typename E, typename K> 
  bool RWORSet<E, K>::in(const E& val) // Could
  { 
    // Code could be slightly faster if re-using only part of read code
    set<E> s = read();
    if ( s.find(val) != s.end() ) return true;
    return false;
  }

  template<typename E, typename K> 
  RWORSet<E, K> RWORSet<E, K>::add( const E& val ) 
  {
    RWORSet<E, K> r;
    // Remove any observed add token
    r.dk = dk.remove( pair<E, bool>( val, true ) );    
    // Remove any observed remove token    
    r.dk.join( dk.remove( pair<E,bool>( val, false ) ) );  
    r.dk.join( dk.add( id, pair<E,bool>( val, true ) ) );
    return r;
  }

  template<typename E, typename K> 
  RWORSet<E, K> RWORSet<E, K>::rmv( const E& val )
  {
    RWORSet<E, K> r;
    // Remove any observed add token
    r.dk = dk.remove( pair<E, bool>( val, true ) );  
    // Remove any observed remove token
    r.dk.join(dk.remove( pair<E, bool>( val, false ) ) ); 
    r.dk.join(dk.add( id, pair<E, bool>(val, false ) ) );
    return r;
  }

  template<typename E, typename K> 
  RWORSet<E, K> RWORSet<E, K>::reset()
  {
    RWORSet<E, K> r;
    r.dk = dk.remove(); 
    return r;
  }

  template<typename E, typename K> 
  void RWORSet<E, K>::join( RWORSet<E, K> o )
  {
    dk.join( o.dk );
  }
#pragma endregion


#pragma region MVReg
  // Multi-value register, Optimized
  template<typename V, typename K>
  DotContext<K>& MVReg<V, K>::context()
  {
    return dk.c;
  }

  template<typename V, typename K>
  ostream& operator<<( ostream &output, const MVReg<V, K>& o)
  { 
    output << "MVReg:" << o.dk;
    return output;            
  }

  template<typename V, typename K>
  MVReg<V, K> MVReg<V, K>::write( const V& val ) 
  {
    MVReg<V, K> r,a;
    r.dk = dk.remove(); 
    a.dk = dk.add( id, val );
    r.join( a );
    return r;
  }

  template<typename V, typename K>
  set<V> MVReg<V, K>::read( )
  {
    set<V> s;
    for ( const auto & dse : dk.ds )
      s.insert( dse.second );

    return s;
  }

  template<typename V, typename K>
  MVReg<V, K> MVReg<V, K>::reset( )
  {
    MVReg<V, K> r;
    r.dk = dk.remove( ); 
    return r;
  }

  template<typename V, typename K>
  MVReg<V, K> MVReg<V, K>::resolve( )
  {
    MVReg<V, K> r,v;
    set<V> s; // collect all values that are not maximals
    for ( const auto & dsa : dk.ds ) // Naif quadratic comparison
      for ( const auto & dsb : dk.ds )
        if ( dsa.second != dsb.second && 
            ::join( dsa.second, dsb.second ) == dsb.second ) // < based on join
        // if (dsa.second < dsb.second) // values must implement operator<
          s.insert( dsa.second );
    
    // remove all non maximals and register deltas for those removals
    for ( const auto & val : s )
    {
      v.dk = dk.remove( val );
      r.join(v);
    }

    return r;
  }

  template<typename V, typename K>
  void MVReg<V, K>::join( MVReg<V, K> o )
  {
    dk.join( o.dk );
  }
#pragma endregion

#pragma region EWFlag
  template<typename K>
  DotContext<K>& EWFlag<K>::context()
  {
    return dk.c;
  }

  template<typename K>
  ostream& operator<<( ostream &output, const EWFlag<K>& o )
  { 
    output << "EWFlag:" << o.dk;
    return output;            
  }

  template<typename K>
  bool EWFlag<K>::read()
  {
    typename map<pair<K, int>, bool>::iterator dsit;
    if ( dk.ds.begin() == dk.ds.end( ) ) 
      // No active dots
      return false;
    else
      // Some dots
      return true;
  }

  template<typename K>
  EWFlag<K> EWFlag<K>::enable() 
  {
    EWFlag<K> r;
    r.dk = dk.remove( true ); // optimization that first deletes active dots
    r.dk.join( dk.add( id, true ) );
    return r;
  }

  template<typename K>
  EWFlag<K> EWFlag<K>::disable()
  {
    EWFlag<K> r;
    r.dk = dk.remove( true ); 
    return r;
  }

  template<typename K>
  EWFlag<K> EWFlag<K>::reset()
  {
    EWFlag<K> r;
    r.dk = dk.remove( ); 
    return r;
  }

  template<typename K>
  void EWFlag<K>::join(EWFlag<K> o)
  {
    dk.join(o.dk);
  }
#pragma endregion

#pragma region DWFlag
  template<typename K>
  DotContext<K>& DWFlag<K>::context()
  {
    return dk.c;
  }

  template<typename K>
  ostream& operator<<( ostream &output, const DWFlag<K>& o)
  { 
    output << "DWFlag:" << o.dk;
    return output;            
  }

  template<typename K>
  bool DWFlag<K>::read()
  {
    typename map<pair<K, int>, bool>::iterator dsit;
    if ( dk.ds.begin() == dk.ds.end()) 
      // No active dots
      return true;
    else
      // Some dots
      return false;
  }

  template<typename K>
  DWFlag<K> DWFlag<K>::disable() 
  {
    DWFlag<K> r;
    r.dk = dk.remove( false ); // optimization that first deletes active dots
    r.dk.join( dk.add( id, false ) );
    return r;
  }

  template<typename K>
  DWFlag<K> DWFlag<K>::enable()
  {
    DWFlag<K> r;
    r.dk = dk.remove( false ); 
    return r;
  }

  template<typename K>
  DWFlag<K> DWFlag<K>::reset()
  {
    DWFlag<K> r;
    r.dk = dk.remove(); 
    return r;
  }

  template<typename K>
  void DWFlag<K>::join(DWFlag<K> o)
  {
    dk.join( o.dk );
  }

#pragma endregion


#pragma region RWLWWSet
  // U is timestamp, T is payload
  template<typename U, typename T>
  RWLWWSet<U, T> RWLWWSet<U, T>::addrmv( const U& ts, const T& val, bool b )
  {
    RWLWWSet<U, T> res;
    pair<U, bool> a(ts, b);
    res.s.insert( pair<T, pair<U, bool>>( val, a ) );
    pair<typename map<T, pair<U, bool>>::iterator, bool> ret;
    ret = s.insert(pair<T, pair<U, bool>>( val, a ) );
    // some value there 
    if ( ret.second == false ) 
    {
        s.at( ret.first->first) = lexjoin( ret.first->second, a );
    }
    return res;
  }

  template<typename U, typename T>
  ostream& operator<<( ostream &output, const RWLWWSet<U, T>& o )
  { 
    output << "RW LWWSet: ( ";
    for( typename  map<T, pair<U, bool>>::const_iterator it = o.s.begin(); it != o.s.end(); ++it )
    {
      if( it->second.second == false )
        output << it->first << " ";
    }
    output << ")" << endl;
    return output;            
  }

  template<typename U, typename T>
  RWLWWSet<U, T> RWLWWSet<U, T>::add( const U& ts, const T& val )
  {
    return addrmv( ts, val, false );
  }

  template<typename U, typename T>
  RWLWWSet<U, T> RWLWWSet<U, T>::rmv( const U& ts, const T& val )
  {
    return addrmv( ts, val, true );
  }

  template<typename U, typename T>
  bool RWLWWSet<U, T>::in(const T& val) 
  { 
    typename  map<T, pair<U, bool>>::const_iterator it = s.find( val ); 
    if ( it == s.end() || it->second.second == true )
      return false;
    else
      return true;
  }

  template<typename U, typename T>
  void RWLWWSet<U, T>::join( const RWLWWSet<U, T>& o )
  {
    // Join is idempotent, but just dont do it.
    if ( this == &o ) return; 

    // will iterate over the two sorted sets to compute join
    typename  map<T, pair<U, bool>>::iterator it; 
    typename  map<T, pair<U, bool>>::const_iterator ito; 
    it = s.begin(); ito = o.s.begin();
    do 
    {
      if ( it != s.end() && ( ito == o.s.end() || it->first < ito->first ) )
      {
        // entry only at this
        // keep it
        ++it;
      }
      else if ( ito != o.s.end() && ( it == s.end() || ito->first < it->first ) )
      {
        // entry only at other
        // import it
        s.insert( *ito );
        ++ito;
      }
      else if ( it != s.end() && ito != o.s.end() )
      {
        // in both
        // merge values by lex operator
        s.at( it->first ) = lexjoin( it->second,ito->second );
        ++it; ++ito;
      }
    } while ( it != s.end() || ito != o.s.end() );
  }
#pragma endregion

  // U must be comparable
#pragma region LWWReg
  template<typename U, typename T>
  ostream& operator<<( ostream& output, const LWWReg<U, T>& o)
  { 
    output << "LWWReg: " << o.r;
    return output;            
  }

  template<typename U, typename T>
  void LWWReg<U, T>::join(const LWWReg<U, T>& o)
  {
    if ( o.r.first > r.first )
    {
      r = o.r;
    }
  }

  template<typename U, typename T>
  LWWReg<U, T> LWWReg<U, T>::write( const U& ts, const T& val )
  {
    LWWReg<U, T> res;
    res.r.first = ts;
    res.r.second = val;
    join( res );  // Will only update if ts is greater
    return res;
  }

  template<typename U, typename T>
  T LWWReg<U, T>::read()
  {
    return r.second;
  }
#pragma endregion


#pragma region ORMap

  template<typename N, typename V, typename K>
  ORMap<N, V, K>& ORMap<N, V, K>::operator=( const ORMap<N, V, K>& o )
  {
    if ( &o == this ) return *this;
    if ( &c != &o.c ) c = o.c; 
    m = o.m; id = o.id;
    return *this;
  }

  template<typename N, typename V, typename K>
  DotContext<K>& ORMap<N, V, K>::context() const
  {
    return c;
  }

  template<typename N, typename V, typename K>
  ostream& operator<<( ostream &output, const ORMap<N, V, K>& o )
  { 
    output << "Map:" << o.c << endl;
    for ( const auto & kv : o.m )
      cout << kv.first << "->" << kv.second << endl;

    return output;            
  }

  // Need to find a way to collect deltas for this interface
  template<typename N, typename V, typename K>
  V& ORMap<N, V, K>::operator[]( const N& n )
  {
    auto i = m.find(n);
    if ( i == m.end() ) // 1st key access
    {
      auto ins = m.insert( i, pair<N, V>(n, V(id, c) ) );
      return ins->second;
    }
    else
    {
      return i->second;
    }
  }

  template<typename N, typename V, typename K>
  ORMap<N, V, K> ORMap<N, V, K>::erase( const N& n )
  {
    ORMap<N, V, K> r;
    if ( m.count(n) != 0 )
    {
      // need to collect erased dots, and list then in r context
      V v;
      v = m[n].reset();
      r.c = v.context();
      // now erase
      m.erase( n );
    }

    return r;
  }

  template<typename N, typename V, typename K>
  ORMap<N, V, K> ORMap<N, V, K>::reset()
  {
    ORMap<N, V, K> r;
    if ( !m.empty() )
    {
      // need to collect erased dots, and list then in r context
      for ( auto & kv : m )
      {
        V v;
        v = kv.second.reset();
        r.c.join( v.context() );
      }

      m.clear();
    }

    return r;
  }

  template<typename N, typename V, typename K>
  void ORMap<N, V, K>::join( const ORMap<N, V>& o )
  {
    const DotContext<K> ic = c; // need access to an immutable context

    // join all keys
    auto mit = m.begin(); auto mito = o.m.begin();
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
      if( mit != m.end() && (mito == o.m.end() || mit->first < mito->first ) )
      {
        //cout << "entry left\n";
        // entry only at here
        
        // creaty and empty payload with the other context, since it might   
        // obsolete some local entries. 
        V empty(id,o.context());
        mit->second.join(empty);
        c = ic;

        ++mit;
      }
      else if ( mito != o.m.end() && ( mit == m.end() || mito->first < mit->first ) )
      {
        // cout << "entry right\n";
        // entry only at other

        (*this)[mito->first].join( mito->second );
        c = ic;

        ++mito;
      }
      else if ( mit != m.end() && mito != o.m.end() )
      {
        // cout << "entry both\n";
        // in both
        (*this)[mito->first].join( mito->second );
        c = ic;

        ++mit; ++mito;
      }
    } while (mit != m.end() || mito != o.m.end());
    
    c.join(o.c);
  }
#pragma endregion

#pragma region Bag
  // A bag is similar to an RWSet, but allows for CRDT payloads
  template<typename V, typename K>
  Bag<V, K>& Bag<V, K>::operator=( const Bag<V, K>& o)
  {
    if ( &o == this ) return *this;
    if ( &dk != &o.dk ) dk = o.dk; 
    id = o.id;
    return *this;
  }

  template<typename V, typename K>
  DotContext<K>& Bag<V, K>::context()
  {
    return dk.c;
  }

  template<typename V, typename K>
  void Bag<V, K>::insert( pair<pair<K, int>, V> t )
  {
    dk.ds.insert( pair<pair<K, int>, V>( t ) );
    dk.c.insertDot( t.first );
  }

  template<typename V, typename K>
  ostream& operator<<(ostream& output, const Bag<V, K>& o)
  { 
    output << "Bag:" << o.dk;
    return output;            
  }

  template<typename V, typename K>
  typename map<pair<K, int>, V>::iterator Bag<V, K>::begin()
  {
    return dk.ds.begin();
  }

  template<typename V, typename K>
  typename map<pair<K, int>, V>::iterator Bag<V, K>::end()
  {
    return dk.ds.end();
  }

  template<typename V, typename K>
  pair<K, int> Bag<V, K>::mydot( )
  {
    auto me = dk.ds.end();
    for ( auto it = dk.ds.begin(); it != dk.ds.end(); ++it)
    {
      if ( it->first.first == id ) // a candidate
      {
        if ( me == dk.ds.end() ) // pick at least one valid
        {
          me = it;
        }
        else // need to switch if more recent
        {
          if ( it->first.second > me->first.second )
            me = it;
        }
      }
    }

    if ( me != dk.ds.end() )
      return me->first;
    else 
    {
      fresh();
      return mydot(); // After a fresh it must be found
    }
  }

  template<typename V, typename K>
  V& Bag<V, K>::mydata()
  {
    auto me = dk.ds.end();
    for ( auto it = dk.ds.begin(); it != dk.ds.end(); ++it )
    {
      if ( it->first.first == id ) // a candidate
      {
        if ( me == dk.ds.end() ) // pick at least one valid
          me = it;
        else // need to switch if more recent
        {
          if ( it->first.second > me->first.second )
            me = it;
        }
      }
    }

    if ( me != dk.ds.end() )
      return me->second;
    else 
    {
      fresh();
      return mydata(); // After a fresh it must be found
    }
  }

  // To protect from concurrent removes, create fresh dot for self
  template<typename V, typename K>
  void Bag<V, K>::fresh( )
  {
    dk.add( id, V() );
  }

  template<typename V, typename K>
  Bag<V, K> Bag<V, K>::reset( )
  {
    Bag<V, K> r;
    r.dk = dk.remove(); 
    return r;
  }

  // Using the deep join will try to join different payloads under same dot
  template<typename V, typename K>
  void Bag<V, K>::join( const Bag<V, K>& o )
  {
    dk.deepJoin( o.dk );
  }
#pragma endregion

#pragma region RWCounter
  // Inspired by designs from Carl Lerche and Paulo S. Almeida
  template<typename V, typename K>
  RWCounter<V, K>& RWCounter<V, K>::operator=( const RWCounter<V, K>& o)
  {
    if ( &o == this ) return *this;
    if ( &b != &o.b ) b = o.b; 
    id = o.id; 
    return *this;
  }

  template<typename V, typename K>
  DotContext<K>& RWCounter<V, K>::context()
  {
    return b.context();
  }

  template<typename V, typename K>
  ostream& operator<<( ostream &output, const RWCounter<V, K>& o)
  { 
    output << "ResetWinsCounter:" << o.b;
    return output;            
  }
  
  template<typename V, typename K>
  RWCounter<V, K> RWCounter<V, K>::inc( const V& val) 
  {
    RWCounter<V, K> r;
    b.mydata().first += val;
    r.b.insert(pair<pair<K, int>, pair<V, V>>( b.mydot(), b.mydata() ) );
    return r;
  }

  template<typename V, typename K>
  RWCounter<V, K> RWCounter<V, K>::dec( const V& val) 
  {
    RWCounter<V, K> r;
    b.mydata().second += val;
    r.b.insert(pair<pair<K, int>, pair<V, V>>( b.mydot(), b.mydata() ) );
    return r;
  }

  template<typename V, typename K>
  RWCounter<V, K> RWCounter<V, K>::reset( )
  {
    RWCounter<V, K> r;
    r.b = b.reset();
    return r;
  }

  template<typename V, typename K>
  void RWCounter<V, K>::fresh( )
  {
    b.fresh();
  }

  template<typename V, typename K>
  V RWCounter<V, K>::read( )
  {
    pair<V, V> ac;
    for ( const auto & dv : b )
    {
      ac.first += dv.second.first;
      ac.second += dv.second.second;
    }
    return ac.first - ac.second;
  }

  template<typename V, typename K>
  void RWCounter<V, K>::join( const RWCounter<V, K> & o )
  {
    b.join( o.b );
  }
#pragma endregion


#pragma region GMap
  template<typename N, typename V>
  ostream& operator<<( ostream &output, const GMap<N, V>& o)
  { 
    output << "GMap:" << endl;
    for ( const auto & kv : o.m )
      cout << kv.first << "->" << kv.second << endl;

    return output;            
  }

  // Need to find a better way to collect deltas for this interface
  template<typename N, typename V>
  V& GMap<N, V>::operator[]( const N& n )
  {
    auto i = m.find(n);
    if ( i == m.end() ) // 1st key access
    {
      auto ins = m.insert( i, pair<N, V>(n, V() ) );
      return ins->second;
    }
    else
    {
      return i->second;
    }
  }

  template<typename N, typename V>
  void GMap<N, V>::join( const GMap<N, V>& o )
  {
    // join all keys
    auto mit = m.begin(); auto mito = o.m.begin();
    do 
    {
      if ( mit != m.end() && ( mito == o.m.end() || mit->first < mito->first ) )
      {
        //cout << "entry left\n";
        // entry only at here

        ++mit;
      }
      else if ( mito != o.m.end() && (mit == m.end() || mito->first < mit->first))
      {
        // cout << "entry right\n";
        // entry only at other

        (*this)[mito->first] = mito->second;

        ++mito;
      }
      else if ( mit != m.end() && mito != o.m.end() )
      {
        // cout << "entry both\n";
        // in both
        (*this)[mito->first] = ::join((*this)[mito->first],mito->second);

        ++mit; ++mito;
      }
    } while (mit != m.end() || mito != o.m.end());
  }
#pragma endregion


#pragma region BCounter
  // Argument is optional
  template <typename V, typename K>
  BCounter<V, K> BCounter<V, K>::inc( V tosum) 
  {
    BCounter<V, K> res;
    res.c = c.inc( tosum ); 
    return res;
  }

  // Argument is optional
  template <typename V, typename K>
  BCounter<V, K> BCounter<V, K>::dec(V todec) 
  {
    BCounter<V, K> res;
    if ( todec <= local() ) // Check local capacity
      res.c = c.dec( todec ); 
    return res;
  }

  // Quantity V to node id K
  template <typename V, typename K>
  BCounter<V, K> BCounter<V, K>::mv( V q, K to ) 
  {
    BCounter<V, K> res;
    if ( q <= local() ) // Check local capacity
    {
      m[pair<K, K>(id, to)] += q; 
      res.m[pair<K,K>(id, to)] = m[pair<K, K>(id, to)];
    }
    return res;
  }

  // get global counter value
  template <typename V, typename K>
  V BCounter<V, K>::read() 
  {
    V res = c.read();
    return res;
  }

  // get local counter available value
  template <typename V, typename K>
  V BCounter<V, K>::local() 
  {
    V res = c.local();
    // Sum incoming flows
    for ( const auto & kv : m.m )
      if ( kv.first.second == id ) res += kv.second;
    
    // Subtract outgoing flows
    for ( const auto & kv : m.m )
      if ( kv.first.first == id ) res -= kv.second;

    return res;
  }

  template <typename V, typename K>
  void BCounter<V, K>::join( const BCounter<V, K>& o )
  {
    c.join( o.c );
    m.join( o.m );
  }

  template <typename V, typename K>
  ostream& operator<<( ostream& output, const BCounter<V, K>& o)
  { 
    output << "BCounter:C:" << o.c << "BCounter:M:" << o.m;
    return output;            
  }
#pragma endregion


#pragma region ORSeq
  template<typename T, typename I>
  ORSeq<T, I>& ORSeq<T, I>::operator=( const ORSeq<T, I>& aos )
  {
    if ( &aos == this ) return *this;
    if ( &c != &aos.c ) c = aos.c; 
    l = aos.l;
    id = aos.id;
    return *this;
  }

  template<typename T, typename I>
  ostream& operator<<( ostream &output, const ORSeq<T, I>& o)
  { 
    output << "ORSeq: " << o.c;
    output << " List:"; 
    for (const auto & t : o.l)
      output << "(" << get<0>(t) << " " << get<1>(t) 
        << " " << get<2>(t) << ")";
    return output;            
  }

  template<typename T, typename I>
  typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator ORSeq<T, I>::begin() 
  {
    return l.begin();
  }

  template<typename T, typename I>
  typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator ORSeq<T, I>::end() 
  {
    return l.end();
  }

  template<typename T, typename I>
  ORSeq<T, I> ORSeq<T, I>::erase(typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator i)
  {
    ORSeq<T, I> res;
    if (i != l.end())
    {
      res.c.insertDot( get<1>( *i ) );
      l.erase(i);
    }
    return res;
  }

  template<typename T, typename I>
  DotContext<I>& ORSeq<T, I>::context()
  {
    return c;
  }

  template<typename T, typename I>
  ORSeq<T, I> ORSeq<T, I>::reset()
  {
    ORSeq<T, I> res;
    for ( auto const & t : l )
      res.c.insertDot( get<1>( t ) );

    l.clear();
    return res;
  }

  template<typename T, typename I>
  ORSeq<T, I> ORSeq<T, I>::insert( typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator i, const T& val )
  {
    ORSeq<T, I> res;
    if ( i == l.end() )
      res = pushBack(val);
    else
    {
      if ( i == l.begin() )
        res = pushFront(val);
      else
      {
        typename list<tuple<vector<bool>, pair<I, int>, T>>::iterator j = i;
        j--;
        vector<bool> bl, br, pos;
        bl = get<0>( *j );
        br = get<0>( *i );
        pos = among( bl, br );
        // get new dot
        auto dot = c.makeDot( id );
        auto tuple = make_tuple( pos, dot, val );
        l.insert( i, tuple );
        // delta
        res.c.insertDot( dot );
        res.l.push_front( tuple );
      }
    }

    return res;
  }

  // add 1st element
  template<typename T, typename I>
  ORSeq<T, I> ORSeq<T, I>::makeFirst( const T& val )
  {
    assert( l.empty() );

    ORSeq<T, I> res;
    vector<bool> bl, br, pos;
    bl.push_back( false );
    br.push_back( true );
    pos = among( bl, br );
    // get new dot
    pair<I, int> dot = c.makeDot( id );
    l.push_back(make_tuple(pos, dot, val));
    // delta
    res.c.insertDot( dot );
    res.l=l;
    return res;
  }

  template<typename T, typename I>
  ORSeq<T, I> ORSeq<T, I>::pushBack( const T& val )
  {
    ORSeq<T, I> res;
    if ( l.empty() )
      res = makeFirst( val );
    else
    {
      vector<bool> bl, br, pos;
      bl = get<0>( l.back() );
      br.push_back( true );
      pos = among( bl, br );
      // get new dot
      auto dot = c.makeDot( id );
      auto tuple = make_tuple( pos, dot, val );
      l.push_back( tuple );
      // delta
      res.c.insertDot( dot );
      res.l.push_front( tuple );
    }
    return res;
  }

  template<typename T, typename I>
  ORSeq<T, I> ORSeq<T, I>::pushFront( const T& val )
  {
    ORSeq<T, I> res;
    if ( l.empty() )
      res = makeFirst( val );
    else
    {
      vector<bool> bl, br, pos;
      br = get<0>( l.front() );
      bl.push_back( false );
      pos = among( bl, br );
      // get new dot
      auto dot = c.makeDot( id );
      auto tuple = make_tuple( pos, dot, val );
      l.push_front( tuple );
      // delta
      res.c.insertDot( dot );
      res.l.push_front( tuple );
    }
    return res;
  }

  template<typename T, typename I>
  void ORSeq<T, I>::join( const ORSeq<T, I>& o )
  {
    if ( this == &o ) return; // Join is idempotent, but just don't do it.
    auto it = l.begin(); auto ito = o.l.begin();
    pair<vector<bool>, I> e, eo;
    do 
    {
      if ( it != l.end() )
      {
        e.first = get<0>( *it );
        e.second = get<1>( *it ).first;
      }
      if ( ito != o.l.end() )
      {
        eo.first = get<0>( *ito );
        eo.second = get<1>( *ito ).first;
      }
      if ( it != l.end() && ( ito == o.l.end() || e < eo ) )
      {
        // cout << "ds one\n";
        // entry only at this
        if ( o.c.dotin( get<1>( *it ) ) ) // other knows dot, must delete here 
          l.erase(it++);
        else // keep it
          ++it;
      }
      else if ( ito != o.l.end() && ( it == l.end() || eo < e ) )
      {
        //cout << "ds two\n";
        // entry only at other
        if( !c.dotin( get<1>( *ito ) ) ) // If I dont know, import
        {
          l.insert( it, *ito ); // it keeps pointing to next
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
    c.join( o.c );
  }
#pragma endregion
}


