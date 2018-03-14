// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/IntrusiveList.h"
#include "Common/IntrusivePtr.h"
#include "Common/HeapAllocAllocator.h"
#include "Common/Serialization.h"

#define BIQUE_DEBUG_ASSERT( expr ) ASSERT_IFNOT( expr, COMMON_STRINGIFY( expr ) " is false" )

namespace Common
{
    typedef size_t sequence_t;
    typedef ptrdiff_t seq_diff_t;

    namespace detail
    {
        class BufferData
        {
        public:
            __declspec(property(get=get_IsShared)) bool IsShared;

            bool get_IsShared() { return isShared_; }

            void AddRef() const
            {
                if (isShared_)
                {
                    InterlockedIncrement(&refCount_);
                }
                else
                {
                    isShared_ = true;
                    refCount_++;
                }
            }

            void Release()
            {
                LONG result;
                if (isShared_)
                {
                    result = InterlockedDecrement(&refCount_);
                }
                else
                {
                    result = --refCount_;
                    coding_error_assert(result == 0);
                }

                coding_error_assert(result >= 0);

                if (result == 0)
                {
                    OnDestroy();
                }
            }

            bool ReleaseIfShared()
            {
                if (isShared_)
                {
                    auto original = InterlockedDecrement(&refCount_);
                    if (original == 0)
                    {
                        isShared_ = false;
                        refCount_ = 1;
                        return false;
                    }
                    else
                    {
                        coding_error_assert(original > 0);
                        return true;
                    }
                }
                else
                {
                    coding_error_assert(refCount_ == 1);
                    return false;
                }
            }

        protected:
            BufferData() : refCount_(1), isShared_(false) {}
            ~BufferData() {}

            virtual void OnDestroy() = 0;

        private:
            mutable volatile LONG refCount_;
            mutable volatile bool isShared_;
        };

        template <class T> class BufferListEntry : public BufferData
        {
            DENY_COPY( BufferListEntry );
            typedef BufferListEntry<T> this_type;

            ~BufferListEntry() {}
        public:
            intrusive::list_entry link_;

            static this_type * Create( sequence_t start, size_t num_elements )
            {
                // Workaround for PREfast bug; use static_cast<size_t> here
                size_t offset = std::max( sizeof( this_type ), static_cast<size_t>(__alignof( T )));

                size_t allocationSize;
                if (FAILED(SizeTMult(sizeof(T), num_elements, &allocationSize)))
                    throw std::bad_alloc();

                if (FAILED(SizeTAdd(offset, allocationSize, &allocationSize)))
                    throw std::bad_alloc();

                void* mem = HeapAlloc( GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, allocationSize);
                if ( mem == nullptr )
                    throw std::bad_alloc();

                T * data = reinterpret_cast<T*>(( byte* )mem + offset );

                return new( mem ) this_type( start, num_elements, data );
            }

            BufferListEntry( sequence_t start, size_t num_elements, T* data )
                : sequence_( start ), size_( num_elements ), data_( data )
            {}

            T & at( sequence_t pos ) { return data_[pos - sequence_]; }
            T const & at( sequence_t pos ) const { return data_[pos - sequence_]; }
            size_t available_at( sequence_t pos ) { return size_ -( pos - sequence_ ); }
            T * begin() { return data_; }
            T * end() { return data_ + size(); }

            size_t size() const { return size_; }
            sequence_t sequence() const { return sequence_; }
            void resequence( sequence_t new_sequence ) { sequence_ = new_sequence; }

            void WriteTo( TextWriter &w, FormatOptions const& ) const
            {
                w << '[' << sequence_ << ']';
            }

        protected:
            void OnDestroy()
            {
                this->~BufferListEntry();
                HeapFree( GetProcessHeap(), 0, this );
            }

        private:
            T * data_;
            sequence_t sequence_;
            size_t size_;
        };

        template <class T> class BufferList
        {
            typedef BufferList<T> this_type;
        public:
            typedef BufferListEntry<T> BufferRef;
            typedef intrusive::member_hook<BufferRef, &BufferRef::link_> hook;
            typedef intrusive::list<BufferRef, hook> storage;

            typedef typename storage::iterator iterator;
            typedef typename storage::const_iterator const_iterator;

            BufferList( size_t buffer_size ) 
                : capacity_( 0 )
                , DEFAULT_BUFFER_SIZE( buffer_size )
                , desired_capacity_( DEFAULT_BUFFER_SIZE * 2 ) 
            {}

            BufferList(BufferList && other)
                : capacity_( other.capacity_ )
                , DEFAULT_BUFFER_SIZE( other.DEFAULT_BUFFER_SIZE )
                , desired_capacity_(other.desired_capacity_) 
            {
                q_.swap(other.q_);
                other.capacity_ = 0;
            }

            ~BufferList()
            {
                while ( !q_.empty() )
                {
                    BufferRef * p = q_.pop_front();
                    p->Release();
                }
            }

            BufferList & operator = (BufferList && other)
            {
                if (this != &other)
                {
                    coding_error_assert(q_.empty());
                    q_.swap(other.q_);

                    capacity_ = other.capacity_;
                    DEFAULT_BUFFER_SIZE = other.DEFAULT_BUFFER_SIZE;
                    desired_capacity_ = other.desired_capacity_;    
                    other.capacity_ = 0;
                }

                return *this;
            }

            void allocate_and_push_back( sequence_t sequence, size_t num_elements )
            {
                if (sequence == 0 && num_elements == 0) 
                {
                    // avoid miscalculating on empty buffer lists
                    return;
                }

                ptrdiff_t count = num_elements;
                while ( count > 0 )
                {
                    size_t size = DEFAULT_BUFFER_SIZE;

                    BufferRef * p = BufferRef::Create( sequence, size );

                    q_.push_back( p );

                    count -= size;
                    sequence += size;
                }
                capacity_ = sequence - front().sequence();
            }

            BufferRef & back() { return q_.back(); }
            BufferRef & front() { return q_.front(); }
            bool empty() { return q_.empty(); }

            iterator begin() { return q_.begin(); }
            iterator end() { return q_.end(); }
            //iterator last() { return q_.last(); }

            const_iterator begin() const { return q_.begin(); }
            const_iterator end() const { return q_.end(); }
            //const_iterator last() const { return q_.last(); }

            void check_invariant()
            {
                if ( q_.empty() )
                {
                    ASSERT_IFNOT( capacity_ == 0, "Capacity != 0." );
                }
                else
                {
                    ASSERT_IFNOT( capacity_ > 0, "Capacity <= 0" );
                }
            }

            sequence_t pop_front()
            { 
                coding_error_assert(!q_.empty());

                auto buffer = q_.pop_front();
                auto bufferSize = buffer->size();
                auto frontSequence = buffer->sequence();

                // Blink points at the list head now.  In case it is still in use, make sure we
                // do not accidentally traverse backwards across invalid pointer.  Setting null
                // ensures we failfast rather than corrupting data.
                buffer->link_.Blink = nullptr;

                if (capacity_ > desired_capacity_)
                {
                    buffer->Release();
                    capacity_ -= bufferSize;
                }
                else if (buffer->ReleaseIfShared())
                {
                    capacity_ -= bufferSize;
                }
                else
                {
                    q_.push_back(buffer);
                    buffer->resequence(frontSequence + capacity_);
                }

                check_invariant();
                return frontSequence + bufferSize;
            }

            void WriteTo( TextWriter & w, FormatOptions const& ) const { w.Write( q_ ); }

            size_t capacity() const { return capacity_; }
            size_t desired_capacity() const { return desired_capacity_; }
            void desired_capacity( size_t desired ) { desired_capacity_ = desired; }

        private:
            friend void UnsafeClearForMove(BufferList && bl )
            {
                if( !bl.q_.empty() )
                {
                    PLIST_ENTRY thisHeadPtr = bl.front().link_.Blink;
                    PLIST_ENTRY frontPtr = thisHeadPtr->Flink;
                    PLIST_ENTRY backPtr = thisHeadPtr->Blink;

                    // unlink "front"
                    thisHeadPtr->Flink = thisHeadPtr;
                    frontPtr->Blink = nullptr;

                    // unlink "back"
                    thisHeadPtr->Blink = thisHeadPtr;
                    backPtr->Flink = nullptr;
                    
                    // It is not required to set frontPtr->Blink and backPtr->Flink to nullptr above, because the buffer is being moved
                    // to BiqueRange and we should never go out of range with BiqueRange. The only reason we are setting these pointers
                    // null is, if "going out of range" ever happens, null pointers will crash the application immediately. Otherwise,
                    // we will be at the mercy of random runtime behavior.

                    bl.capacity_ = 0;
                } 
            }

            mutable storage q_;
            size_t capacity_;         // end() - begin()
            size_t DEFAULT_BUFFER_SIZE;
            size_t desired_capacity_;
        };
    }



    struct fragmented_random_access_iterator_tag : public std::forward_iterator_tag {}; 

    template <class T, class BufferQueue = detail::BufferList<T>>
    class bique_base
    {
        DENY_COPY( bique_base );
        typedef typename BufferQueue::BufferRef BufferRef;
        typedef typename BufferQueue::iterator buffer_iterator;

    public:

        class const_iterator : public std::iterator<fragmented_random_access_iterator_tag, T>
        {
            mutable buffer_iterator buffer_;
            sequence_t pos_;
        public:

            void check_iterator_invariant()
            {
                // todo: check if there is other way to check invariant without calling "at_end()", so that we can add back the calls to
                // check_iterator_invariant() that got removed in change list 598148
                if ( !buffer_.at_end() )
                {
                    coding_error_assert( pos_ >= buffer_->sequence() );
                    coding_error_assert( pos_ < buffer_->sequence() + buffer_->size() );
                }
            }

            const_iterator( sequence_t pos, buffer_iterator buffer ) : buffer_( buffer ), pos_( pos ) 
            {
            }

            T const * fragment_begin() const { return &buffer()->at( pos_ ); }
            T const * fragment_end() const { return buffer()->end(); }
            size_t fragment_size() const { return buffer()->available_at( pos_ ); }
            sequence_t sequence() const { return pos_; }
            buffer_iterator const & buffer() const { return buffer_; }

            T const & operator * () const { return *fragment_begin(); }
            T const * operator & () const { return fragment_begin(); }
            T const * operator->() const { return( &**this ); }

            const_iterator & operator ++ () 
            { 
                ++pos_;

                if ( fragment_size() == 0 )
                {
                    ++buffer_;
                }
                return *this; 
            }

            bool at_end() const { return buffer_.at_end(); }

            const_iterator & operator --()
            {
                if ( buffer_.at_end() || fragment_begin() == buffer()->begin() )
                {
                    --buffer_;
                }
                --pos_;
                check_iterator_invariant();
                return *this;
            }

            friend seq_diff_t operator -( const_iterator const & a, const_iterator const & b ) 
            {
                return a.sequence() - b.sequence();
            }

            const_iterator & operator += ( size_t count )
            {
                for ( ;; )
                {
                    size_t available = fragment_size();

                    if ( count < available )
                    {
                        pos_ += count;
                        return *this;
                    }
                    pos_ += available;
                    ++buffer_;
                    count -= available;
                    if ( count == 0 )
                        return *this;
                }
            }

            const_iterator & operator -= ( size_t count )
            {
                if ( count == 0 )
                    return *this;

                if ( buffer_.at_end() || fragment_begin() == buffer()->begin() )
                {
                    --buffer_;
                }

                for ( ;; )
                {
                    size_t available = pos_ - buffer()->sequence();

                    if ( count <= available )
                    {
                        pos_ -= count;
                        return *this;
                    }
                    pos_ -= available;
                    --buffer_;

                    coding_error_assert( !buffer_.at_end() );

                    count -= available;
                }
            }

            const_iterator operator -( size_t b ) const
            {
                const_iterator result( *this );
                return result -= b;
            }

            const_iterator operator + ( size_t b ) const
            {
                const_iterator result( *this );
                return result += b;
            }

            void WriteTo( TextWriter& w, Common::FormatOptions const& ) const
            {
                w.Write( "{1}@{0}", sequence_, fragment_size() );
            }

            bool operator == ( const_iterator const & rhs ) const { return pos_ == rhs.pos_; }
            bool operator != ( const_iterator const & rhs ) const { return pos_ != rhs.pos_; }
            bool operator <( const_iterator const & rhs ) const { return pos_ < rhs.pos_; }
            bool operator <=( const_iterator const & rhs ) const { return pos_ <= rhs.pos_; }
            bool operator >( const_iterator const & rhs ) const { return pos_ > rhs.pos_; }
            bool operator >=( const_iterator const & rhs ) const { return pos_ >= rhs.pos_; }
        };

        class iterator : public const_iterator
        {
        public:
            iterator( sequence_t pos, buffer_iterator buffer ) : const_iterator( pos,buffer ) {}

            T * fragment_begin() const { return const_cast<T*>( static_cast<const_iterator const*>( this )->fragment_begin() ); }
            T * fragment_end() const { return const_cast<T*>( static_cast<const_iterator const*>( this )->fragment_end() ); }
            T & operator * () const { return *fragment_begin(); }
            T * operator & () const { return fragment_begin(); }
            T * operator->() const { return( &**this ); }
            buffer_iterator buffer() const { return const_cast<buffer_iterator&>( static_cast<const_iterator const*>( this )->buffer() ); }

            iterator & operator --() 
            { 
                --( * ( const_iterator * )this );
                return *this;
            }

            iterator & operator ++ () 
            { 
                ++ ( * ( const_iterator * )this );
                return *this;
            }

            iterator & operator += ( size_t count )
            {
               ( * ( const_iterator * )this ) += count;
                return *this;
            }

            iterator operator + ( size_t b ) const
            {
                iterator result( *this );
                return result += b;
            }

            iterator & operator -= ( size_t count )
            {
               ( * ( const_iterator * )this ) -= count;
                return *this;
            }

            iterator operator -( size_t b ) const
            {
                iterator result( *this );
                return result -= b;
            }
        };

        explicit bique_base( size_t bufSize ) : chain_( bufSize ), sequence_()  {}
        explicit bique_base(bique_base && other) : chain_(std::move(other.chain_)), sequence_(other.sequence_)  {}

        // truncate all the buffers prior to the one pointing by pos
        void truncate_before( const_iterator pos )
        {
            BIQUE_DEBUG_ASSERT( uninitialized_cbegin() <= pos && pos <= uninitialized_cend() );

            while (!chain_.empty() &&
                   (chain_.front().sequence() + chain_.front().size() <= pos.sequence()))
            {
                sequence_ = chain_.pop_front();
                coding_error_assert(chain_.empty() || (chain_.front().sequence() == sequence_));
            }
            coding_error_assert(chain_.empty() || (chain_.front().sequence() <= pos.sequence()));
        }

        void reserve_after( iterator pos, size_t count )
        {
            size_t available = uninitialized_end() - pos;
            if ( available > count )
                return;

            chain_.allocate_and_push_back( sequence() + capacity(), count - available );
        }

        void reserve( size_t capacity )
        {
            reserve_after( uninitialized_begin(), capacity );
        }

        void reserve( size_t capacity, size_t desired_capacity )
        {
            reserve( capacity );
            chain_.desired_capacity( desired_capacity );
        }

        void WriteTo( TextWriter &w, Common::FormatOptions const& ) const
        {
            w.Write( "start {0} size {1} desired {2} chain_ {3}", 
                sequence_, capacity(), desired_capacity(), chain_ );
        }

        size_t desired_capacity() const { return chain_.desired_capacity(); } 
        size_t capacity() const { return chain_.capacity(); }
        sequence_t sequence() const { return sequence_; }

        iterator uninitialized_begin() { return iterator( sequence(), chain_.begin() ); }
        iterator uninitialized_end() { return iterator( sequence() + capacity(), chain_.end() ); }
        const_iterator uninitialized_begin() const { return iterator( sequence(), chain_.begin() ); }
        const_iterator uninitialized_end() const { return iterator( sequence() + capacity(), chain_.end() ); }
        const_iterator uninitialized_cbegin() { return iterator( sequence(), chain_.begin() ); }
        const_iterator uninitialized_cend() { return iterator( sequence() + capacity(), chain_.end() ); }

    protected:
        sequence_t sequence_; // start sequence
        BufferQueue chain_;
    };

    template <class T, class BufferQueue = detail::BufferList<T>>
    class bique : public bique_base<T, BufferQueue>
    {
        DENY_COPY( bique );
        typedef bique_base<T, BufferQueue> base_type;

    public:
        bique( size_t bufSize = 3 ) 
            : bique_base<T, BufferQueue>( bufSize )
            , begin_( base_type::uninitialized_begin() ), end_( base_type::uninitialized_begin() )
        {}

        bique(bique && other)
            : bique_base<T, BufferQueue>(std::move(other))
            , begin_( other.begin_), end_(other.end_)
        {
            other.begin_ = other.uninitialized_begin();
            other.end_ = other.uninitialized_begin();
        }

        bique & operator = (bique && other)
        {
            if (this != &other)
            {
                base_type::chain_ = std::move(other.chain_);
                base_type::sequence_ = other.sequence_;
                begin_ = other.begin_;
                end_ = other.end_;

                other.begin_ = other.uninitialized_begin();
                other.end_ = other.uninitialized_begin();        
            }

            return *this;
        }

        void reserve_back( size_t size )
        {
            typename bique_base<T, BufferQueue>::iterator remember_end = this->uninitialized_end();
            if (this->uninitialized_begin() != remember_end)
            {
                --remember_end;
            }

            this->reserve_after( end_, size );

            coding_error_assert( begin_.sequence() >= base_type::sequence() );
            coding_error_assert( end_.sequence() >= base_type::sequence() );

            if ( begin_.at_end() ) 
            { 
                if ( remember_end.at_end() )
                {
                    remember_end = this->uninitialized_begin();
                }

                ptrdiff_t diff = begin_ - remember_end;

                if ( diff < 0 )
                {
                    begin_ = remember_end -( size_t )( -diff );
                }
                else
                {
                    begin_ = remember_end + ( size_t )diff;
                }
            }
            if ( end_.at_end() ) 
            {
                end_ = begin_ + ( end_.sequence() - begin_.sequence() );
            }
        }

        void truncate_before( typename bique_base<T, BufferQueue>::const_iterator pos )
        {
            BIQUE_DEBUG_ASSERT( cbegin() <= pos && pos <= cend() );

            base_type::truncate_before( pos );
            begin_ = static_cast<typename bique_base<T, BufferQueue>::iterator&>( pos );

            if ( begin_ == end_ )
            {
                begin_ = end_;
            }
            coding_error_assert( begin_.sequence() >= base_type::sequence() );
            coding_error_assert( end_.sequence() >= base_type::sequence() );
        }

        template <class t0>
        void emplace_back( t0 && a0 )
        {
            reserve_back( 1 );
            //end_.advance_if_at_end();
            allocator_.construct( end_.fragment_begin(), std::forward<t0>( a0 ));
            ++end_; //.increment_do_not_advance();
        }

        template <class t0, class t1>
        void emplace_back( t0 && a0, t1 && a1 )
        {
            reserve_back( 1 );
            //end_.advance_if_at_end();
            allocator_.construct( end_.fragment_begin(), std::forward<t0>( a0 ), std::forward<t1>( a1 ));
            ++end_; //.increment_do_not_advance();
        }

        template <class t0, class t1, class t2>
        void emplace_back( t0 && a0, t1 && a1, t2 && a2 )
        {
            reserve_back( 1 );
            //end_.advance_if_at_end();
            allocator_.construct( end_.fragment_begin(), 
                std::forward<t0>( a0 ), std::forward<t1>( a1 ), std::forward<t2>( a2 ));
            ++end_; //.increment_do_not_advance();
        }

        template <class t0, class t1, class t2, class t3>
        void emplace_back(t0 && a0, t1 && a1, t2 && a2, t3 && a3)
        {
            reserve_back(1);
            //end_.advance_if_at_end();
            allocator_.construct(end_.fragment_begin(),
                std::forward<t0>(a0), std::forward<t1>(a1), std::forward<t2>(a2), std::forward<t3>(a3));
            ++end_; //.increment_do_not_advance();
        }

        void push_back( T && value )
        {
            reserve_back( 1 );
            //end_.advance_if_at_end();
            allocator_.construct( end_.fragment_begin(), std::forward<T>( value ));
            ++end_; //.increment_do_not_advance();
        }
        /*
        void push_back( T const & value )
        {
            bool wasEmpty = ( sequence() + capacity() == 0 );

            reserve_after( end(), 1 );
            if ( wasEmpty )
            {
                begin_ = uninitialized_begin();
                end_ = uninitialized_begin();
            }

            end_.advance_if_at_end();
            allocator_.construct( end_.fragment_begin(), value );
            end_.increment_do_not_advance();
        }
        */

        T const & front() const { return *begin(); }
        T const & back() const { return *--end(); }

        T & front() { return *begin(); }
        T & back() { return *--end(); } 

        bool empty() const { return begin_ == end_; }
        size_t size() const { return end_ - begin_; }

        void pop_front()
        {
            allocator_.destroy( begin().fragment_begin() );
            ++begin_;
        }

        template <class InputIterator>
        void insert( typename bique_base<T, BufferQueue>::const_iterator position, InputIterator first, InputIterator last )
        {
            position;
            append( first, last - first );
        }

        void no_fill_advance_back( size_t size )
        {
            while ( size > 0 )
            {
//                end_.advance_if_at_end();

                size_t available = end_.fragment_size();
                if ( available > size )
                    available = size;

                size -= available;
                end_ += available;

//                end_.add_do_not_advance(available);
            }
        }

        template <class Iterator>
        void append( Iterator beg, size_t size )
        {
            reserve_back( size );

            while ( size > 0 )
            {
                //end_.advance_if_at_end();

                size_t available = end_.fragment_size();
                if ( available > size )
                    available = size;

                uninitialized_copy_n_checked( beg, available, &*end_.fragment_begin(), available);

                beg += available;
                size -= available;
                end_ += available;

                //end_.add_do_not_advance(available);
            }
        }

        template <class Iterator>
        void pop_front_n( size_t count, Iterator beg )
        {
            ASSERT_IF( count > size(), "count > size" );

            while ( count > 0 )
            {
                size_t available = std::min( count, begin_.fragment_size() );

                copy_n_checked( &*begin_, available, beg, available);

                beg += available;
                begin_ += available;
                count -= available;
            }
        }

        template <class Iterator>
        void append( Iterator beg, Iterator end )
        {
            if ( beg == end )
                return;

            bool wasEmpty = ( sequence() + capacity() == 0 );
            chain_.append( sequence() + capacity(), beg, end );
            if ( wasEmpty )
            {
                begin_ = iterator( 0, chain_.begin() );
            }
            end_ = iterator( capacity(), chain_.end() );
        }

        typename bique_base<T, BufferQueue>::iterator end() { return end_; }
        typename bique_base<T, BufferQueue>::iterator begin() { return begin_; }

        typename bique_base<T, BufferQueue>::const_iterator end() const { return end_; }
        typename bique_base<T, BufferQueue>::const_iterator begin() const { return begin_; }

        typename bique_base<T, BufferQueue>::const_iterator cend() const { return end_; }
        typename bique_base<T, BufferQueue>::const_iterator cbegin() const { return begin_; }

        void WriteTo( TextWriter & w, FormatOptions const & ) const
        {
            int itemCount;
            //w.WriteRange(begin(), end());
            w.WriteListBegin( itemCount );
            for(T const & v : *this )
            {
                w.WriteElementBegin( itemCount );
                w << v;
                w.WriteElementEnd( itemCount );
            }
            w.WriteListEnd( itemCount );
        }

    private:
        typename bique_base<T, BufferQueue>::iterator begin_, end_;
        HeapAlloc_allocator<T> allocator_;

        friend void UnsafeClearForMove(bique && b)
        {
            UnsafeClearForMove(std::move(b.chain_));
            b.begin_ = b.uninitialized_begin();
            b.end_ = b.uninitialized_begin();
        }
    };

    //
    // !!! Note: The life time of the "head" entry of the buffers in BiqueRange is not guaranteed by BiqueRange. So, please do NOT call
    // at_end() on the bique iterators from BiqueRange. Also, please do NOT call -- and -= operators either, since these two iterators
    // call at_end().
    //
    template <class T, class BufferQueue = detail::BufferList<T>>
    class BiqueRange
    {
    public:
        typedef typename bique_base<T,BufferQueue>::iterator iterator;
        typedef typename BufferQueue::iterator buffer_iterator;

        BiqueRange(iterator begin, iterator end, bool addRef)
            : begin_(begin), count_(end - begin), addRef_(addRef)
        {
            if (addRef_) { AddRefBuffers(); }
        }

        explicit BiqueRange(BiqueRange const & other)
            : begin_(other.begin_), count_(other.count_), addRef_(true)
        {
            AddRefBuffers();
        }

        explicit BiqueRange(BiqueRange && other)
            : begin_(other.begin_), count_(other.count_), addRef_(other.addRef_)
        {
            other.Clear();
        }

        // When moving buffers from bique, BiqueRange takes refcount ownership from bique. addRef_ needs to be set true, although we do
        // not increment buffer refcount here. Otherwise, ReleaseBuffers() will not be called in BiqueRange destructor as it should be.
        explicit BiqueRange(bique<T> && b)
            : begin_(b.begin()), count_(b.size()), addRef_(true)
        {
            UnsafeClearForMove(std::move(b));
        }

        ~BiqueRange()
        {
            if (addRef_) { ReleaseBuffers(); }
        }

        bool IsEmpty()
        {
            return count_ == 0;
        }

        BiqueRange & operator = (BiqueRange const & other)
        {
            if (this != &other)
            {
                other.AddRefBuffers();
                if (addRef_) { ReleaseBuffers(); }

                begin_ = other.begin_;
                count_ = other.count_;
                addRef_ = true;
            }
            return *this;
        }

        BiqueRange & operator = (BiqueRange && other)
        {
            if (this != &other)
            {
                if (addRef_) { ReleaseBuffers(); }

                begin_ = other.begin_;
                count_ = other.count_;
                addRef_ = other.addRef_;

                other.Clear();
            }
            return *this;
        }

        void swap(BiqueRange & other)
        {
            if (this != &other)
            {
                std::swap(begin_, other.begin_);
                std::swap(count_, other.count_);
                std::swap(addRef_, other.addRef_);
            }
        }

        __declspec(property(get=get_Begin)) iterator Begin;
        __declspec(property(get=get_End)) iterator End;

        iterator get_Begin() const { return begin_; }
        iterator get_End() const 
        {
            if (count_ == 0)
            {
                return begin_;
            }

            return begin_ + count_;
        }

    private:
        iterator begin_;
        size_t count_;
        bool addRef_;

        void AddRefBuffers()
        {
            int64 count = count_;
            iterator iter = begin_;

            while (count > 0)
            {
                iter.buffer()->AddRef();
                count -= iter.fragment_size();
                iter += iter.fragment_size();
            }
        }

        void ReleaseBuffers()
        {
            int64 count = count_;
            iterator iter = begin_;

            while (count > 0)
            {
                auto copy = iter.buffer();
                count -= iter.fragment_size();
                iter += iter.fragment_size();
                copy->Release();
            }
        }

        void Clear()
        {
            addRef_ = false;
            count_ = 0;
        }
    };

    template <class Container, class Iterator>
    void pop_front_n( Container & b, size_t n, Iterator output )
    {
        std::copy_n( b.begin(), n, output );
        b.erase( b.begin(), b.begin() + n );
    }

    template <class T, class Iterator>
    void pop_front_n( bique<T> & c, size_t n, Iterator output )
    {
        c.pop_front_n( n, output );
    }
}

namespace Common
{
    struct BiqueStream : public ByteStream<BiqueStream>
    {
        BiqueStream( Common::bique<byte> & b ) : q_( b ) {}

        void WriteBytes( void const * buf, size_t size ) 
        {
            //Log.WriteLine("writing {0} bytes", size); 
            byte const * begin = reinterpret_cast<byte const*>( buf );
            q_.insert( q_.end(), begin, begin + size );
        }
        void ReadBytes( void * buf, size_t size ) 
        {
            //Log.WriteLine("reading {0} bytes", size); 
            byte * begin = reinterpret_cast<byte*>( buf );

            q_.pop_front_n( size, begin );
        }
        bool empty() { return q_.empty(); }
    private:
        DENY_COPY_ASSIGNMENT( BiqueStream );

        Common::bique<byte> & q_;
    };

    struct BiqueRangeReadStream : public ByteStream<BiqueRangeReadStream>
    {
        typedef bique<byte> container;
        typedef range<container::const_iterator> range;

        BiqueRangeReadStream( container const & b ) : range_( make_range( b )) {}
        BiqueRangeReadStream( range const & r ) : range_( r ) {}

        void ReadBytes( void * buf, size_t count ) 
        {
            byte * beg = static_cast<byte*>( buf );

            while ( count > 0 )
            {
                size_t available = std::min( count, range_.first.fragment_size() );

                copy_n_checked( &*range_.first, available, beg, available);

                beg += available;
                range_.first += available;
                count -= available;
            }
        }
        bool empty() { return range_.end() == range_.begin(); }
    private:
        DENY_COPY_ASSIGNMENT( BiqueRangeReadStream );

        range range_;
    };
}
