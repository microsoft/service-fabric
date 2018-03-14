// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//#define CXL_NEED_NT_LIST 1
//#include "VsStuff.h"

#include <iterator>
//#include <ntrtl_x.h>

// wrapper around nt LIST_ENTRY based lists
//
// see IntrusiveList.Test.cpp for usage example
//
// allocation and deallocation is responsibility of the caller

namespace Common
{
    namespace intrusive
    {
        struct list_entry : public LIST_ENTRY 
        {
            list_entry(bool head = false): head_(head) { Blink = Flink = nullptr; }
            bool inserted() const { return Blink != nullptr; }
            void clear() { Blink = nullptr; }

            bool is_head() const { return head_; }
        private:
            bool head_;
        };

        namespace detail
        {
            template <class T> struct base_adapter
            {
                static T const * get(LIST_ENTRY const *p) { return static_cast<T const *>( p ); }
                static T* get(LIST_ENTRY *p) { return static_cast<T*>( p ); }
                static LIST_ENTRY & hook(T *p) { return *p; }
                static LIST_ENTRY const & hook(T const *p) { return *p; }
            };
        }

        template <class T, list_entry T::*Hook> struct member_hook
        {
            static list_entry & hook(T *p) { return p->*Hook; }
            static list_entry const & hook(T const *p) { return p->*Hook; }

            static T * get(LIST_ENTRY *p)
            {
                return reinterpret_cast<T*> ( 
                    reinterpret_cast<byte*>(p) - reinterpret_cast<byte*>( &hook((T*)0) ) );
            }
            static T const * get(LIST_ENTRY const *p)
            {
                return reinterpret_cast<T const *> ( 
                    reinterpret_cast<byte const *>(p) - reinterpret_cast<byte const*>( &hook((T const*)0) ) );
            }
        };

        template <class T, class A = detail::base_adapter<T> > class list 
        {
            DENY_COPY(list); // intrusive list cannot be copied
        public:
            struct const_iterator : public std::iterator<std::bidirectional_iterator_tag, T>
            {
                const_iterator (LIST_ENTRY const * value) : pos_(A::get(value)) {}
                const_iterator & operator ++ () { pos_ = A::get(A::hook(pos_).Flink); return *this; }
                const_iterator & operator -- () { pos_ = A::get(A::hook(pos_).Blink); return *this; }
                T const & operator * () const { return *pos_; }
                T const * operator -> () const { return pos_; }
                bool operator == (const_iterator const & rhs) const { return pos_ == rhs.pos_; }
                bool operator != (const_iterator const & rhs) const { return pos_ != rhs.pos_; }

                bool at_end() const { return A::hook(pos_).is_head(); }

            protected:
                T const * pos_;
            };
            struct iterator : public const_iterator
            {
                iterator (LIST_ENTRY * value) : const_iterator(value) {}
                iterator & operator ++ () { this->pos_ = A::get(A::hook(this->pos_).Flink); return *this; }
                iterator & operator -- () { this->pos_ = A::get(A::hook(this->pos_).Blink); return *this; }
                T & operator * () const { return *const_cast<T*>(this->pos_); }
                T * operator -> () const { return &**this; }
                bool operator == (iterator const & rhs) const { return this->pos_ == rhs.pos_; }
                bool operator != (iterator const & rhs) const { return this->pos_ != rhs.pos_; }
            };

            list () : head_(true) { InitializeListHead(&head_); }
            bool empty () { return IsListEmpty(&head_) != 0; }
            bool empty () const { return IsListEmpty(&head_) != 0; }

            iterator begin() { return next(); }
            iterator end() { return iterator(&head_); }

            // for each will always bind to const versions
            //const_iterator begin() const { return next(); }
            //const_iterator end() const { return const_iterator(&head_); }

            const_iterator cbegin() const { return next(); }
            const_iterator cend() const { return const_iterator(&head_); }

            T & front() { return *begin(); }
            T const & front() const { return *begin(); }

            T & back() { return *prev(); }
            T const & back() const { return *prev(); }

            static bool erase(T* value) 
            { 
                bool wasEmpty = RemoveEntryList(&A::hook(value)) != 0; 
                static_cast<list_entry&>(A::hook(value)).clear();
                return wasEmpty;
            } 

            // giving an element T will return a pointer to the next element
            // or nullptr if T was the last element
            static T* get_next(T* value)
            {
                LIST_ENTRY* next_entry = A::hook(value).Flink;

                if (static_cast<list_entry*>(next_entry)->is_head())
                    return nullptr;

                return A::get(next_entry);
            }

            T* pop_back () 
            { 
                PLIST_ENTRY result = RemoveTailList(&head_);
                return result == &head_ ? nullptr : A::get(result); 
            }

            T* pop_front () 
            { 
                PLIST_ENTRY result = RemoveHeadList(&head_);
                return result == &head_ ? nullptr : A::get(result); 
            }

            void push_back(T* value) { InsertTailList(&head_, &A::hook(value)); }
            void push_front(T* value) { InsertHeadList(&head_, &A::hook(value)); }

            void WriteTo(TextWriter & w, FormatOptions const&) const
            {
                w.Write(TextWritableCollection<list<T, A>>(*this));
            }

            void swap(list & other)
            {
                if (empty())
                {
                    if (!other.empty())
                    {
                        InitializeFrom(other);
                    }
                }
                else
                {
                    if (other.empty())
                    {
                        other.InitializeFrom(*this);
                    }
                    else
                    {
                        SwapNonEmpty(other);
                    }
                } 
            }

        private:
            const_iterator next() const { return head_.Flink; }
            const_iterator prev() const { return head_.Blink; }

            iterator next() { return head_.Flink; }
            iterator prev() { return head_.Blink; }

            void SwapNonEmpty(list & other)
            { 
                using std::swap;

                // swap Blink of "front" entry
                PLIST_ENTRY thisFront = head_.Flink;
                PLIST_ENTRY otherFront = other.head_.Flink;
                swap(thisFront->Blink, otherFront->Blink);

                // swap Flink of "back" entry
                PLIST_ENTRY thisBack = head_.Blink;
                PLIST_ENTRY otherBack = other.head_.Blink;
                swap(thisBack->Flink, otherBack->Flink);

                swap(head_, other.head_);
            }
            
            void InitializeFrom(list & other)
            {                
                PLIST_ENTRY otherFront = other.head_.Flink;
                otherFront->Blink = &head_;

                PLIST_ENTRY otherBack = other.head_.Blink;
                otherBack->Flink = &head_;
              
                head_ = other.head_;
                InitializeListHead(&(other.head_));
            } 

        private:
            list_entry head_;
        };

        template <class T, class A> 
        void swap(list<T,A> & a, list<T,A> & b) { a.swap(b); }
    }
}
