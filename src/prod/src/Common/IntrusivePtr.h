// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // interface based on http://www.boost.org/doc/libs/1_38_0/libs/smart_ptr/intrusive_ptr.html
    // with some tweaks.
    //
    // Unlike boost intrusive_ptr Common::intrusive_ptr does not support implicit T* to intrusive<T> conversion
    // expected use is to use make_intrusive<T> for initial pointer creation and
    // if there is need to convert T* to intrusive_ptr, use two argument ctor and indicate whether
    // ref count needs to be implemented.
    //    explicit intrusive_ptr( T * p, bool add_ref): ptr_( p )
    //
    // if you don't want to maintain your own ref count, you can inherit from basic_ref_counted
    //
    // make_intrusive is specialize to 2 parameters, add more as needed
    //
    // The intrusive_ptr class template stores a pointer to an object with an embedded reference count. 
    // Every new intrusive_ptr instance increments the reference count by using an unqualified call to the
    // function intrusive_ptr_add_ref, passing it the pointer as an argument. 
    // Similarly, when an intrusive_ptr is destroyed, it calls intrusive_ptr_release; 
    // this function is responsible for destroying the object when its reference count drops to zero. 
    // The user is expected to provide suitable definitions of these two functions. 
    // intrusive_ptr_add_ref and intrusive_ptr_release should be defined in the namespace that corresponds to their parameter;

    template<class T> class intrusive_ptr
    {
        typedef intrusive_ptr<T> this_type;
    public:
        typedef T* pointer;
        typedef T element_type;

        intrusive_ptr(): ptr_(0) {}

        intrusive_ptr(std::nullptr_t): ptr_(0) {}

        explicit intrusive_ptr( T * p, bool add_ref): ptr_( p )
        {
            if( ptr_ && add_ref ) intrusive_ptr_add_ref( ptr_ );
        }

        template<class U>
        intrusive_ptr( intrusive_ptr<U> const & rhs, typename std::enable_if<std::is_convertible<U, T>::value,void *>::type * = 0)
            : ptr_( rhs.get() )
        {
            if( ptr_ ) intrusive_ptr_add_ref( ptr_ );
        }

        intrusive_ptr(intrusive_ptr const & rhs): ptr_( rhs.ptr_ )
        {
            if( ptr_ ) intrusive_ptr_add_ref( ptr_ );
        }

        ~intrusive_ptr()
        {
            if( ptr_ ) intrusive_ptr_release( ptr_ );
        }

        template<class U> intrusive_ptr & operator=(intrusive_ptr<U> const & rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr(intrusive_ptr && rhs): ptr_( rhs.ptr_ )
        {
            rhs.ptr_ = 0;
        }

        intrusive_ptr & operator=(intrusive_ptr && rhs)
        {
            this_type(std::move(rhs)).swap(*this);
            return *this;
        }

        intrusive_ptr & operator=(intrusive_ptr const & rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        intrusive_ptr & operator=(T * rhs)
        {
            this_type(rhs).swap(*this);
            return *this;
        }

        void reset_no_release() { ptr_ = 0; }

        void reset() { this_type().swap( *this );  }

        void reset( T * rhs )  { this_type( rhs ).swap( *this );  }

        T * get() const  { return ptr_; }

        T & operator*() const { return *ptr_; }

        T * operator->() const { return ptr_; }

        explicit operator bool() const { return (ptr_ != nullptr);  }

        void swap(intrusive_ptr<T> & rhs)
        {
            T * tmp = ptr_;
            ptr_ = rhs.ptr_;
            rhs.ptr_ = tmp;
        }

    private:
        T * ptr_;
    };
    template<class T> void swap(intrusive_ptr<T> & lhs, intrusive_ptr<T> & rhs)
    {
        lhs.swap(rhs);
    }

    template<class T, class U> inline bool operator==(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
    {
        return a.get() == b.get();
    }

    template<class T, class U> inline bool operator!=(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
    {
        return a.get() != b.get();
    }
    template<class T, class U> inline bool operator<(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
    {
        return a.get() < b.get();
    }
    template<class T, class U> inline bool operator<=(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
    {
        return a.get() <= b.get();
    }
    template<class T, class U> inline bool operator>(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
    {
        return a.get() < b.get();
    }
    template<class T, class U> inline bool operator>=(intrusive_ptr<T> const & a, intrusive_ptr<U> const & b)
    {
        return a.get() <= b.get();
    }

    template<class T, class U> intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const & p)
    {
        return static_cast<T*>(p.get());
    }

    template<class T, class U> intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const & p)
    {
        return const_cast<T*>(p.get());
    }

    template<class T, class U> intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const & p)
    {
        return dynamic_cast<T*>(p.get());
    }

    class basic_ref_counted
    {
        basic_ref_counted & operator = (basic_ref_counted const&);
        basic_ref_counted (basic_ref_counted const&);
    public:
        basic_ref_counted () : ref_count_(1), unique_(true) {}

        virtual ~basic_ref_counted () 
        {
            //FN_ENTER();
        }

        void NonInterlockedAddRef(size_t increment) const
        {
            intrusive_ptr_add_ref_non_interlocked(this, increment);
        }

        friend void intrusive_ptr_add_ref_non_interlocked (basic_ref_counted const * p, size_t increment)
        {
            if (increment == 0)
                return;

            debug_assert(p);
            debug_assert(p->unique_);
            debug_assert(increment > 0);

            p->unique_ = false;

            p->ref_count_ += (LONG)increment;
        }

        friend void intrusive_ptr_add_ref( basic_ref_counted const * p )
        {
            debug_assert(p);
            p->unique_ = false;
            LONG result = InterlockedIncrement( &p->ref_count_ );
            result; // debug_assert expression is not present in free build
            debug_assert(result > 1);
        }

        friend void intrusive_ptr_release( basic_ref_counted const * p )
        {
            debug_assert(p);

            if (p->unique_)
            {
                --p->ref_count_;
                debug_assert(p->ref_count_ == 0);
                delete p;
            }
            else
            {
                LONG result = InterlockedDecrement( &p->ref_count_ );
                debug_assert(result >= 0);
                if (result == 0)
                    delete p;
            }
        }
    private:
        mutable volatile bool unique_;
        mutable LONG ref_count_;
    };
    typedef basic_ref_counted intrusive_ptr_base;

    template <class T> 
    Common::intrusive_ptr<T> make_intrusive() 
    { 
        return Common::intrusive_ptr<T>(new T, false); 
    }

    template <class T, class t0> 
    Common::intrusive_ptr<T> make_intrusive(t0 && a0) { 
        return Common::intrusive_ptr<T>(new T(std::forward<t0>(a0)), false);
    }

    template <class T, class t0, class t1> 
    Common::intrusive_ptr<T> make_intrusive(t0 && a0, t1 && a1) { 
        return Common::intrusive_ptr<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1)), false);
    }

    template <class T, class t0, class t1, class t2> 
    Common::intrusive_ptr<T> make_intrusive(t0 && a0, t1 && a1, t2 && a2) { 
        return Common::intrusive_ptr<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2)), false);
    }

    template <class T, class t0, class t1, class t2, class t3> 
    Common::intrusive_ptr<T> make_intrusive(t0 && a0, t1 && a1, t2 && a2, t3 && a3) { 
        return Common::intrusive_ptr<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3)), false);
    }

    template <class T, class t0, class t1, class t2, class t3, class t4> 
    Common::intrusive_ptr<T> make_intrusive(t0 && a0, t1 && a1, t2 && a2, t3 && a3, t4 && a4) { 
        return Common::intrusive_ptr<T>(new T(std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3), std::forward<t4>(a4)), false);
    }

}
