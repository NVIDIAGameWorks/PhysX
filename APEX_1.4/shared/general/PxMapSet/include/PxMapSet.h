/*
Copyright (C) 2009-2010 Electronic Arts, Inc.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1.  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
3.  Neither the name of Electronic Arts, Inc. ("EA") nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY ELECTRONIC ARTS AND ITS CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL ELECTRONIC ARTS OR ITS CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

///////////////////////////////////////////////////////////////////////////////
// Written by Paul Pedriana.
//
// Refactored to be compliant with the PhysX data types by John W. Ratcliff
// on March 23, 2011
//////////////////////////////////////////////////////////////////////////////



#ifndef PHYSX_MAP_SET_H
#define PHYSX_MAP_SET_H

#include "PxMapSetInternal.h"
#include "foundation/PxAllocatorCallback.h"


namespace physx
{

    /// EASTL_MAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_MAP_DEFAULT_NAME
        #define EASTL_MAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " map" // Unless the user overrides something, this is "EASTL map".
    #endif


    /// EASTL_MULTIMAP_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_MULTIMAP_DEFAULT_NAME
        #define EASTL_MULTIMAP_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " multimap" // Unless the user overrides something, this is "EASTL multimap".
    #endif


    /// EASTL_MAP_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_MAP_DEFAULT_ALLOCATOR
        #define EASTL_MAP_DEFAULT_ALLOCATOR allocator_type(EASTL_MAP_DEFAULT_NAME)
    #endif

    /// EASTL_MULTIMAP_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_MULTIMAP_DEFAULT_ALLOCATOR
        #define EASTL_MULTIMAP_DEFAULT_ALLOCATOR allocator_type(EASTL_MULTIMAP_DEFAULT_NAME)
    #endif



    /// map
    ///
    /// Implements a canonical map. 
    ///
    /// The large majority of the implementation of this class is found in the rbtree
    /// base class. We control the behaviour of rbtree via template parameters.
    ///
    /// Pool allocation
    /// If you want to make a custom memory pool for a map container, your pool 
    /// needs to contain items of type map::node_type. So if you have a memory
    /// pool that has a constructor that takes the size of pool items and the
    /// count of pool items, you would do this (assuming that MemoryPool implements
    /// the Allocator interface):
    ///     typedef map<Widget, int, less<Widget>, MemoryPool> WidgetMap;  // Delare your WidgetMap type.
    ///     MemoryPool myPool(sizeof(WidgetMap::node_type), 100);          // Make a pool of 100 Widget nodes.
    ///     WidgetMap myMap(&myPool);                                      // Create a map that uses the pool.
    ///
    template <typename Key, typename T, typename Compare = physx::less<Key>, typename Allocator = EASTLAllocatorType>
    class map
        : public rbtree<Key, physx::pair<const Key, T>, Compare, Allocator, physx::use_first<physx::pair<const Key, T> >, true, true>
    {
    public:
        typedef rbtree<Key, physx::pair<const Key, T>, Compare, Allocator,
                        physx::use_first<physx::pair<const Key, T> >, true, true>   base_type;
        typedef map<Key, T, Compare, Allocator>                                     this_type;
        typedef typename base_type::size_type                                       size_type;
        typedef typename base_type::key_type                                        key_type;
        typedef T                                                                   mapped_type;
        typedef typename base_type::value_type                                      value_type;
        typedef typename base_type::node_type                                       node_type;
        typedef typename base_type::iterator                                        iterator;
        typedef typename base_type::const_iterator                                  const_iterator;
        typedef typename base_type::allocator_type                                  allocator_type;
        typedef typename base_type::insert_return_type                              insert_return_type;
        typedef typename base_type::extract_key                                     extract_key;
        // Other types are inherited from the base class.

        using base_type::begin;
        using base_type::end;
        using base_type::find;
        using base_type::lower_bound;
        using base_type::upper_bound;
        using base_type::mCompare;

        #if !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x has a bug which we work around.
        using base_type::insert;
        using base_type::erase;
        #endif

    public:
        map(const allocator_type& allocator = EASTL_MAP_DEFAULT_ALLOCATOR);
        map(const Compare& compare, const allocator_type& allocator = EASTL_MAP_DEFAULT_ALLOCATOR);
        map(const this_type& x);

        template <typename Iterator>
        map(Iterator itBegin, Iterator itEnd); // allocator arg removed because VC7.1 fails on the default arg. To consider: Make a second version of this function without a default arg.

    public:
        /// This is an extension to the C++ standard. We insert a default-constructed 
        /// element with the given key. The reason for this is that we can avoid the 
        /// potentially expensive operation of creating and/or copying a mapped_type
        /// object on the stack.
        insert_return_type insert(const Key& key);

        #if defined(__GNUC__) && (__GNUC__ < 3) // If using old GCC (GCC 2.x has a bug which we work around)
            template <typename InputIterator>
            void               insert(InputIterator first, InputIterator last)     { return base_type::insert(first, last);     }
            insert_return_type insert(const value_type& value)                     { return base_type::insert(value);           }
            iterator           insert(iterator position, const value_type& value)  { return base_type::insert(position, value); }
            iterator           erase(iterator position)                            { return base_type::erase(position);         }
            iterator           erase(iterator first, iterator last)                { return base_type::erase(first, last);      }
        #endif

        size_type erase(const Key& key);
        size_type count(const Key& key) const;

        physx::pair<iterator, iterator>             equal_range(const Key& key);
        physx::pair<const_iterator, const_iterator> equal_range(const Key& key) const;

        T& operator[](const Key& key); // Of map, multimap, set, and multimap, only map has operator[].

    }; // map






    /// multimap
    ///
    /// Implements a canonical multimap.
    ///
    /// The large majority of the implementation of this class is found in the rbtree
    /// base class. We control the behaviour of rbtree via template parameters.
    ///
    /// Pool allocation
    /// If you want to make a custom memory pool for a multimap container, your pool 
    /// needs to contain items of type multimap::node_type. So if you have a memory
    /// pool that has a constructor that takes the size of pool items and the
    /// count of pool items, you would do this (assuming that MemoryPool implements
    /// the Allocator interface):
    ///     typedef multimap<Widget, int, less<Widget>, MemoryPool> WidgetMap;  // Delare your WidgetMap type.
    ///     MemoryPool myPool(sizeof(WidgetMap::node_type), 100);               // Make a pool of 100 Widget nodes.
    ///     WidgetMap myMap(&myPool);                                           // Create a map that uses the pool.
    ///
    template <typename Key, typename T, typename Compare = physx::less<Key>, typename Allocator = EASTLAllocatorType>
    class multimap
        : public rbtree<Key, physx::pair<const Key, T>, Compare, Allocator, physx::use_first<physx::pair<const Key, T> >, true, false>
    {
    public:
        typedef rbtree<Key, physx::pair<const Key, T>, Compare, Allocator, 
                        physx::use_first<physx::pair<const Key, T> >, true, false>  base_type;
        typedef multimap<Key, T, Compare, Allocator>                                this_type;
        typedef typename base_type::size_type                                       size_type;
        typedef typename base_type::key_type                                        key_type;
        typedef T                                                                   mapped_type;
        typedef typename base_type::value_type                                      value_type;
        typedef typename base_type::node_type                                       node_type;
        typedef typename base_type::iterator                                        iterator;
        typedef typename base_type::const_iterator                                  const_iterator;
        typedef typename base_type::allocator_type                                  allocator_type;
        typedef typename base_type::insert_return_type                              insert_return_type;
        typedef typename base_type::extract_key                                     extract_key;
        // Other types are inherited from the base class.

        using base_type::begin;
        using base_type::end;
        using base_type::find;
        using base_type::lower_bound;
        using base_type::upper_bound;
        using base_type::mCompare;

        #if !defined(__GNUC__) || (__GNUC__ >= 3) // GCC 2.x has a bug which we work around.
        using base_type::insert;
        using base_type::erase;
        #endif

    public:
        multimap(const allocator_type& allocator = EASTL_MULTIMAP_DEFAULT_ALLOCATOR);
        multimap(const Compare& compare, const allocator_type& allocator = EASTL_MULTIMAP_DEFAULT_ALLOCATOR);
        multimap(const this_type& x);

        template <typename Iterator>
        multimap(Iterator itBegin, Iterator itEnd); // allocator arg removed because VC7.1 fails on the default arg. To consider: Make a second version of this function without a default arg.

    public:
        /// This is an extension to the C++ standard. We insert a default-constructed 
        /// element with the given key. The reason for this is that we can avoid the 
        /// potentially expensive operation of creating and/or copying a mapped_type
        /// object on the stack.
        insert_return_type insert(const Key& key);

        #if defined(__GNUC__) && (__GNUC__ < 3) // If using old GCC (GCC 2.x has a bug which we work around)
            template <typename InputIterator>
            void               insert(InputIterator first, InputIterator last)     { return base_type::insert(first, last);     }
            insert_return_type insert(const value_type& value)                     { return base_type::insert(value);           }
            iterator           insert(iterator position, const value_type& value)  { return base_type::insert(position, value); }
            iterator           erase(iterator position)                            { return base_type::erase(position);         }
            iterator           erase(iterator first, iterator last)                { return base_type::erase(first, last);      }
        #endif

        size_type erase(const Key& key);
        size_type count(const Key& key) const;

        physx::pair<iterator, iterator>             equal_range(const Key& key);
        physx::pair<const_iterator, const_iterator> equal_range(const Key& key) const;

        /// equal_range_small
        /// This is a special version of equal_range which is optimized for the 
        /// case of there being few or no duplicated keys in the tree.
        physx::pair<iterator, iterator>             equal_range_small(const Key& key);
        physx::pair<const_iterator, const_iterator> equal_range_small(const Key& key) const;

    }; // multimap





    ///////////////////////////////////////////////////////////////////////
    // map
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, typename Compare, typename Allocator>
    inline map<Key, T, Compare, Allocator>::map(const allocator_type& allocator)
        : base_type(allocator) { }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline map<Key, T, Compare, Allocator>::map(const Compare& compare, const allocator_type& allocator)
        : base_type(compare, allocator) { }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline map<Key, T, Compare, Allocator>::map(const this_type& x)
        : base_type(x) { }


    template <typename Key, typename T, typename Compare, typename Allocator>
    template <typename Iterator>
    inline map<Key, T, Compare, Allocator>::map(Iterator itBegin, Iterator itEnd)
        : base_type(itBegin, itEnd, Compare(), EASTL_MAP_DEFAULT_ALLOCATOR) { }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline typename map<Key, T, Compare, Allocator>::insert_return_type
    map<Key, T, Compare, Allocator>::insert(const Key& key)
    {
        return base_type::DoInsertKey(key, true_type());
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline typename map<Key, T, Compare, Allocator>::size_type
    map<Key, T, Compare, Allocator>::erase(const Key& key)
    {
        const iterator it(find(key));

        if(it != end()) // If it exists...
        {
            base_type::erase(it);
            return 1;
        }
        return 0;
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline typename map<Key, T, Compare, Allocator>::size_type
    map<Key, T, Compare, Allocator>::count(const Key& key) const
    {
        const const_iterator it(find(key));
        return (it != end()) ? 1 : 0;
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline physx::pair<typename map<Key, T, Compare, Allocator>::iterator,
                       typename map<Key, T, Compare, Allocator>::iterator>
    map<Key, T, Compare, Allocator>::equal_range(const Key& key)
    {
        // The resulting range will either be empty or have one element,
        // so instead of doing two tree searches (one for lower_bound and 
        // one for upper_bound), we do just lower_bound and see if the 
        // result is a range of size zero or one.
        const iterator itLower(lower_bound(key));

        if((itLower == end()) || mCompare(key, itLower.mpNode->mValue.first)) // If at the end or if (key is < itLower)...
            return physx::pair<iterator, iterator>(itLower, itLower);

        iterator itUpper(itLower);
        return physx::pair<iterator, iterator>(itLower, ++itUpper);
    }
    

    template <typename Key, typename T, typename Compare, typename Allocator>
    inline physx::pair<typename map<Key, T, Compare, Allocator>::const_iterator, 
                       typename map<Key, T, Compare, Allocator>::const_iterator>
    map<Key, T, Compare, Allocator>::equal_range(const Key& key) const
    {
        // See equal_range above for comments.
        const const_iterator itLower(lower_bound(key));

        if((itLower == end()) || mCompare(key, itLower.mpNode->mValue.first)) // If at the end or if (key is < itLower)...
            return physx::pair<const_iterator, const_iterator>(itLower, itLower);

        const_iterator itUpper(itLower);
        return physx::pair<const_iterator, const_iterator>(itLower, ++itUpper);
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline T& map<Key, T, Compare, Allocator>::operator[](const Key& key)
    {
        iterator itLower(lower_bound(key)); // itLower->first is >= key.

        if((itLower == end()) || mCompare(key, (*itLower).first))
        {
            itLower = base_type::insert(itLower, value_type(key, T()));

            // To do: Convert this to use the more efficient:
            //    itLower = DoInsertKey(itLower, key, true_type());
            // when we gain confidence in that function.
        }

        return (*itLower).second;

        // Reference implementation of this function, which may not be as fast:
        //iterator it(base_type::insert(physx::pair<iterator, iterator>(key, T())).first);
        //return it->second;
    }






    ///////////////////////////////////////////////////////////////////////
    // multimap
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename T, typename Compare, typename Allocator>
    inline multimap<Key, T, Compare, Allocator>::multimap(const allocator_type& allocator)
        : base_type(allocator)
    {
        // Empty
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline multimap<Key, T, Compare, Allocator>::multimap(const Compare& compare, const allocator_type& allocator)
        : base_type(compare, allocator)
    {
        // Empty
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline multimap<Key, T, Compare, Allocator>::multimap(const this_type& x)
        : base_type(x)
    {
        // Empty
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    template <typename Iterator>
    inline multimap<Key, T, Compare, Allocator>::multimap(Iterator itBegin, Iterator itEnd)
        : base_type(itBegin, itEnd, Compare(), EASTL_MULTIMAP_DEFAULT_ALLOCATOR)
    {
        // Empty
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline typename multimap<Key, T, Compare, Allocator>::insert_return_type
    multimap<Key, T, Compare, Allocator>::insert(const Key& key)
    {
        return base_type::DoInsertKey(key, false_type());
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline typename multimap<Key, T, Compare, Allocator>::size_type
    multimap<Key, T, Compare, Allocator>::erase(const Key& key)
    {
        const physx::pair<iterator, iterator> range(equal_range(key));
        const size_type n = (size_type)physx::distance(range.first, range.second);
        base_type::erase(range.first, range.second);
        return n;
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline typename multimap<Key, T, Compare, Allocator>::size_type
    multimap<Key, T, Compare, Allocator>::count(const Key& key) const
    {
        const physx::pair<const_iterator, const_iterator> range(equal_range(key));
        return (size_type)physx::distance(range.first, range.second);
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline physx::pair<typename multimap<Key, T, Compare, Allocator>::iterator,
                       typename multimap<Key, T, Compare, Allocator>::iterator>
    multimap<Key, T, Compare, Allocator>::equal_range(const Key& key)
    {
        // There are multiple ways to implement equal_range. The implementation mentioned
        // in the C++ standard and which is used by most (all?) commercial STL implementations
        // is this:
        //    return physx::pair<iterator, iterator>(lower_bound(key), upper_bound(key));
        //
        // This does two tree searches -- one for the lower bound and one for the 
        // upper bound. This works well for the case whereby you have a large container
        // and there are lots of duplicated values. We provide an alternative version
        // of equal_range called equal_range_small for cases where the user is confident
        // that the number of duplicated items is only a few.

        return physx::pair<iterator, iterator>(lower_bound(key), upper_bound(key));
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline physx::pair<typename multimap<Key, T, Compare, Allocator>::const_iterator, 
                       typename multimap<Key, T, Compare, Allocator>::const_iterator>
    multimap<Key, T, Compare, Allocator>::equal_range(const Key& key) const
    {
        // See comments above in the non-const version of equal_range.
        return physx::pair<const_iterator, const_iterator>(lower_bound(key), upper_bound(key));
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline physx::pair<typename multimap<Key, T, Compare, Allocator>::iterator,
                       typename multimap<Key, T, Compare, Allocator>::iterator>
    multimap<Key, T, Compare, Allocator>::equal_range_small(const Key& key)
    {
        // We provide alternative version of equal_range here which works faster
        // for the case where there are at most small number of potential duplicated keys.
        const iterator itLower(lower_bound(key));
        iterator       itUpper(itLower);

        while((itUpper != end()) && !mCompare(key, itUpper.mpNode->mValue.first))
            ++itUpper;

        return physx::pair<iterator, iterator>(itLower, itUpper);
    }


    template <typename Key, typename T, typename Compare, typename Allocator>
    inline physx::pair<typename multimap<Key, T, Compare, Allocator>::const_iterator, 
                       typename multimap<Key, T, Compare, Allocator>::const_iterator>
    multimap<Key, T, Compare, Allocator>::equal_range_small(const Key& key) const
    {
        // We provide alternative version of equal_range here which works faster
        // for the case where there are at most small number of potential duplicated keys.
        const const_iterator itLower(lower_bound(key));
        const_iterator       itUpper(itLower);

        while((itUpper != end()) && !mCompare(key, itUpper.mpNode->mValue.first))
            ++itUpper;

        return physx::pair<const_iterator, const_iterator>(itLower, itUpper);
    }


    /// EASTL_SET_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_SET_DEFAULT_NAME
        #define EASTL_SET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " set" // Unless the user overrides something, this is "EASTL set".
    #endif


    /// EASTL_MULTISET_DEFAULT_NAME
    ///
    /// Defines a default container name in the absence of a user-provided name.
    ///
    #ifndef EASTL_MULTISET_DEFAULT_NAME
        #define EASTL_MULTISET_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " multiset" // Unless the user overrides something, this is "EASTL multiset".
    #endif


    /// EASTL_SET_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_SET_DEFAULT_ALLOCATOR
        #define EASTL_SET_DEFAULT_ALLOCATOR allocator_type(EASTL_SET_DEFAULT_NAME)
    #endif

    /// EASTL_MULTISET_DEFAULT_ALLOCATOR
    ///
    #ifndef EASTL_MULTISET_DEFAULT_ALLOCATOR
        #define EASTL_MULTISET_DEFAULT_ALLOCATOR allocator_type(EASTL_MULTISET_DEFAULT_NAME)
    #endif



    /// set
    ///
    /// Implements a canonical set. 
    ///
    /// The large majority of the implementation of this class is found in the rbtree
    /// base class. We control the behaviour of rbtree via template parameters.
    ///
    /// Note that the 'bMutableIterators' template parameter to rbtree is set to false.
    /// This means that set::iterator is const and the same as set::const_iterator.
    /// This is by design and it follows the C++ standard defect report recommendation.
    /// If the user wants to modify a container element, the user needs to either use
    /// mutable data members or use const_cast on the iterator's data member. Both of 
    /// these solutions are recommended by the C++ standard defect report.
    /// To consider: Expose the bMutableIterators template policy here at the set level
    /// so the user can have non-const set iterators via a template parameter.
    ///
    /// Pool allocation
    /// If you want to make a custom memory pool for a set container, your pool 
    /// needs to contain items of type set::node_type. So if you have a memory
    /// pool that has a constructor that takes the size of pool items and the
    /// count of pool items, you would do this (assuming that MemoryPool implements
    /// the Allocator interface):
    ///     typedef set<Widget, less<Widget>, MemoryPool> WidgetSet;    // Delare your WidgetSet type.
    ///     MemoryPool myPool(sizeof(WidgetSet::node_type), 100);       // Make a pool of 100 Widget nodes.
    ///     WidgetSet mySet(&myPool);                                   // Create a map that uses the pool.
    ///
    template <typename Key, typename Compare = physx::less<Key>, typename Allocator = EASTLAllocatorType>
    class set
        : public rbtree<Key, Key, Compare, Allocator, physx::use_self<Key>, false, true>
    {
    public:
        typedef rbtree<Key, Key, Compare, Allocator, physx::use_self<Key>, false, true> base_type;
        typedef set<Key, Compare, Allocator>                                            this_type;
        typedef typename base_type::size_type                                           size_type;
        typedef typename base_type::value_type                                          value_type;
        typedef typename base_type::iterator                                            iterator;
        typedef typename base_type::const_iterator                                      const_iterator;
        typedef typename base_type::reverse_iterator                                    reverse_iterator;
        typedef typename base_type::const_reverse_iterator                              const_reverse_iterator;
        typedef typename base_type::allocator_type                                      allocator_type;
        // Other types are inherited from the base class.

        using base_type::begin;
        using base_type::end;
        using base_type::find;
        using base_type::lower_bound;
        using base_type::upper_bound;
        using base_type::mCompare;

    public:
        set(const allocator_type& allocator = EASTL_SET_DEFAULT_ALLOCATOR);
        set(const Compare& compare, const allocator_type& allocator = EASTL_SET_DEFAULT_ALLOCATOR);
        set(const this_type& x);

        template <typename Iterator>
        set(Iterator itBegin, Iterator itEnd); // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

    public:
        size_type erase(const Key& k);
        iterator  erase(iterator position);
        iterator  erase(iterator first, iterator last);

        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);

        size_type count(const Key& k) const;

        physx::pair<iterator, iterator>             equal_range(const Key& k);
        physx::pair<const_iterator, const_iterator> equal_range(const Key& k) const;

    }; // set





    /// multiset
    ///
    /// Implements a canonical multiset.
    ///
    /// The large majority of the implementation of this class is found in the rbtree
    /// base class. We control the behaviour of rbtree via template parameters.
    ///
    /// See notes above in 'set' regarding multable iterators.
    ///
    /// Pool allocation
    /// If you want to make a custom memory pool for a multiset container, your pool 
    /// needs to contain items of type multiset::node_type. So if you have a memory
    /// pool that has a constructor that takes the size of pool items and the
    /// count of pool items, you would do this (assuming that MemoryPool implements
    /// the Allocator interface):
    ///     typedef multiset<Widget, less<Widget>, MemoryPool> WidgetSet;   // Delare your WidgetSet type.
    ///     MemoryPool myPool(sizeof(WidgetSet::node_type), 100);           // Make a pool of 100 Widget nodes.
    ///     WidgetSet mySet(&myPool);                                       // Create a map that uses the pool.
    ///
    template <typename Key, typename Compare = physx::less<Key>, typename Allocator = EASTLAllocatorType>
    class multiset
        : public rbtree<Key, Key, Compare, Allocator, physx::use_self<Key>, false, false>
    {
    public:
        typedef rbtree<Key, Key, Compare, Allocator, physx::use_self<Key>, false, false>    base_type;
        typedef multiset<Key, Compare, Allocator>                                           this_type;
        typedef typename base_type::size_type                                               size_type;
        typedef typename base_type::value_type                                              value_type;
        typedef typename base_type::iterator                                                iterator;
        typedef typename base_type::const_iterator                                          const_iterator;
        typedef typename base_type::reverse_iterator                                    reverse_iterator;
        typedef typename base_type::const_reverse_iterator                              const_reverse_iterator;
        typedef typename base_type::allocator_type                                          allocator_type;
        // Other types are inherited from the base class.

        using base_type::begin;
        using base_type::end;
        using base_type::find;
        using base_type::lower_bound;
        using base_type::upper_bound;
        using base_type::mCompare;

    public:
        multiset(const allocator_type& allocator = EASTL_MULTISET_DEFAULT_ALLOCATOR);
        multiset(const Compare& compare, const allocator_type& allocator = EASTL_MULTISET_DEFAULT_ALLOCATOR);
        multiset(const this_type& x);

        template <typename Iterator>
        multiset(Iterator itBegin, Iterator itEnd); // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

    public:
        size_type erase(const Key& k);
        iterator  erase(iterator position);
        iterator  erase(iterator first, iterator last);

        reverse_iterator erase(reverse_iterator position);
        reverse_iterator erase(reverse_iterator first, reverse_iterator last);

        size_type count(const Key& k) const;

        physx::pair<iterator, iterator>             equal_range(const Key& k);
        physx::pair<const_iterator, const_iterator> equal_range(const Key& k) const;

        /// equal_range_small
        /// This is a special version of equal_range which is optimized for the 
        /// case of there being few or no duplicated keys in the tree.
        physx::pair<iterator, iterator>             equal_range_small(const Key& k);
        physx::pair<const_iterator, const_iterator> equal_range_small(const Key& k) const;

    }; // multiset





    ///////////////////////////////////////////////////////////////////////
    // set
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename Compare, typename Allocator>
    inline set<Key, Compare, Allocator>::set(const allocator_type& allocator)
        : base_type(allocator)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    inline set<Key, Compare, Allocator>::set(const Compare& compare, const allocator_type& allocator)
        : base_type(compare, allocator)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    inline set<Key, Compare, Allocator>::set(const this_type& x)
        : base_type(x)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    template <typename Iterator>
    inline set<Key, Compare, Allocator>::set(Iterator itBegin, Iterator itEnd)
        : base_type(itBegin, itEnd, Compare(), EASTL_SET_DEFAULT_ALLOCATOR)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename set<Key, Compare, Allocator>::size_type
    set<Key, Compare, Allocator>::erase(const Key& k)
    {
        const iterator it(find(k));

        if(it != end()) // If it exists...
        {
            base_type::erase(it);
            return 1;
        }
        return 0;
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename set<Key, Compare, Allocator>::iterator
    set<Key, Compare, Allocator>::erase(iterator position)
    {
        // We need to provide this version because we override another version 
        // and C++ hiding rules would make the base version of this hidden.
        return base_type::erase(position);
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename set<Key, Compare, Allocator>::iterator
    set<Key, Compare, Allocator>::erase(iterator first, iterator last)
    {
        // We need to provide this version because we override another version 
        // and C++ hiding rules would make the base version of this hidden.
        return base_type::erase(first, last);
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename set<Key, Compare, Allocator>::size_type
    set<Key, Compare, Allocator>::count(const Key& k) const
    {
        const const_iterator it(find(k));
        return (it != end()) ? (size_type)1 : (size_type)0;
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename set<Key, Compare, Allocator>::reverse_iterator
    set<Key, Compare, Allocator>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename set<Key, Compare, Allocator>::reverse_iterator
    set<Key, Compare, Allocator>::erase(reverse_iterator first, reverse_iterator last)
    {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }


    template <typename Key, typename Compare, typename Allocator>
    inline physx::pair<typename set<Key, Compare, Allocator>::iterator,
                       typename set<Key, Compare, Allocator>::iterator>
    set<Key, Compare, Allocator>::equal_range(const Key& k)
    {
        // The resulting range will either be empty or have one element,
        // so instead of doing two tree searches (one for lower_bound and 
        // one for upper_bound), we do just lower_bound and see if the 
        // result is a range of size zero or one.
        const iterator itLower(lower_bound(k));

        if((itLower == end()) || mCompare(k, *itLower)) // If at the end or if (k is < itLower)...
            return physx::pair<iterator, iterator>(itLower, itLower);

        iterator itUpper(itLower);
        return physx::pair<iterator, iterator>(itLower, ++itUpper);
    }
    

    template <typename Key, typename Compare, typename Allocator>
    inline physx::pair<typename set<Key, Compare, Allocator>::const_iterator, 
                       typename set<Key, Compare, Allocator>::const_iterator>
    set<Key, Compare, Allocator>::equal_range(const Key& k) const
    {
        // See equal_range above for comments.
        const const_iterator itLower(lower_bound(k));

        if((itLower == end()) || mCompare(k, *itLower)) // If at the end or if (k is < itLower)...
            return physx::pair<const_iterator, const_iterator>(itLower, itLower);

        const_iterator itUpper(itLower);
        return physx::pair<const_iterator, const_iterator>(itLower, ++itUpper);
    }





    ///////////////////////////////////////////////////////////////////////
    // multiset
    ///////////////////////////////////////////////////////////////////////

    template <typename Key, typename Compare, typename Allocator>
    inline multiset<Key, Compare, Allocator>::multiset(const allocator_type& allocator)
        : base_type(allocator)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    inline multiset<Key, Compare, Allocator>::multiset(const Compare& compare, const allocator_type& allocator)
        : base_type(compare, allocator)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    inline multiset<Key, Compare, Allocator>::multiset(const this_type& x)
        : base_type(x)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    template <typename Iterator>
    inline multiset<Key, Compare, Allocator>::multiset(Iterator itBegin, Iterator itEnd)
        : base_type(itBegin, itEnd, Compare(), EASTL_MULTISET_DEFAULT_ALLOCATOR)
    {
        // Empty
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename multiset<Key, Compare, Allocator>::size_type
    multiset<Key, Compare, Allocator>::erase(const Key& k)
    {
        const physx::pair<iterator, iterator> range(equal_range(k));
        const size_type n = (size_type)physx::distance(range.first, range.second);
        base_type::erase(range.first, range.second);
        return n;
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename multiset<Key, Compare, Allocator>::iterator
    multiset<Key, Compare, Allocator>::erase(iterator position)
    {
        // We need to provide this version because we override another version 
        // and C++ hiding rules would make the base version of this hidden.
        return base_type::erase(position);
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename multiset<Key, Compare, Allocator>::iterator
    multiset<Key, Compare, Allocator>::erase(iterator first, iterator last)
    {
        // We need to provide this version because we override another version 
        // and C++ hiding rules would make the base version of this hidden.
        return base_type::erase(first, last);
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename multiset<Key, Compare, Allocator>::size_type
    multiset<Key, Compare, Allocator>::count(const Key& k) const
    {
        const physx::pair<const_iterator, const_iterator> range(equal_range(k));
        return (size_type)physx::distance(range.first, range.second);
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename multiset<Key, Compare, Allocator>::reverse_iterator
    multiset<Key, Compare, Allocator>::erase(reverse_iterator position)
    {
        return reverse_iterator(erase((++position).base()));
    }


    template <typename Key, typename Compare, typename Allocator>
    inline typename multiset<Key, Compare, Allocator>::reverse_iterator
    multiset<Key, Compare, Allocator>::erase(reverse_iterator first, reverse_iterator last)
    {
        // Version which erases in order from first to last.
        // difference_type i(first.base() - last.base());
        // while(i--)
        //     first = erase(first);
        // return first;

        // Version which erases in order from last to first, but is slightly more efficient:
        return reverse_iterator(erase((++last).base(), (++first).base()));
    }


    template <typename Key, typename Compare, typename Allocator>
    inline physx::pair<typename multiset<Key, Compare, Allocator>::iterator,
                       typename multiset<Key, Compare, Allocator>::iterator>
    multiset<Key, Compare, Allocator>::equal_range(const Key& k)
    {
        // There are multiple ways to implement equal_range. The implementation mentioned
        // in the C++ standard and which is used by most (all?) commercial STL implementations
        // is this:
        //    return physx::pair<iterator, iterator>(lower_bound(k), upper_bound(k));
        //
        // This does two tree searches -- one for the lower bound and one for the 
        // upper bound. This works well for the case whereby you have a large container
        // and there are lots of duplicated values. We provide an alternative version
        // of equal_range called equal_range_small for cases where the user is confident
        // that the number of duplicated items is only a few.

        return physx::pair<iterator, iterator>(lower_bound(k), upper_bound(k));
    }


    template <typename Key, typename Compare, typename Allocator>
    inline physx::pair<typename multiset<Key, Compare, Allocator>::const_iterator, 
                       typename multiset<Key, Compare, Allocator>::const_iterator>
    multiset<Key, Compare, Allocator>::equal_range(const Key& k) const
    {
        // See comments above in the non-const version of equal_range.
        return physx::pair<iterator, iterator>(lower_bound(k), upper_bound(k));
    }


    template <typename Key, typename Compare, typename Allocator>
    inline physx::pair<typename multiset<Key, Compare, Allocator>::iterator,
                       typename multiset<Key, Compare, Allocator>::iterator>
    multiset<Key, Compare, Allocator>::equal_range_small(const Key& k)
    {
        // We provide alternative version of equal_range here which works faster
        // for the case where there are at most small number of potential duplicated keys.
        const iterator itLower(lower_bound(k));
        iterator       itUpper(itLower);

        while((itUpper != end()) && !mCompare(k, itUpper.mpNode->mValue))
            ++itUpper;

        return physx::pair<iterator, iterator>(itLower, itUpper);
    }


    template <typename Key, typename Compare, typename Allocator>
    inline physx::pair<typename multiset<Key, Compare, Allocator>::const_iterator, 
                       typename multiset<Key, Compare, Allocator>::const_iterator>
    multiset<Key, Compare, Allocator>::equal_range_small(const Key& k) const
    {
        // We provide alternative version of equal_range here which works faster
        // for the case where there are at most small number of potential duplicated keys.
        const const_iterator itLower(lower_bound(k));
        const_iterator       itUpper(itLower);

        while((itUpper != end()) && !mCompare(k, *itUpper))
            ++itUpper;

        return physx::pair<const_iterator, const_iterator>(itLower, itUpper);
    }


	void setPxMapSetAllocatorCallback(physx::PxAllocatorCallback *allocator);


} // namespace physx


#endif // Header include guard
