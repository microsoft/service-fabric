// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Utility.h"
#include "Common/StringUtility.h"
#include <list>     ///TODO: move to Common/Types.h
#include <type_traits>

namespace Common
{
    template < class T > struct is_blittable : public std::integral_constant< bool,
        (std::is_trivial< T >::value || 
            (std::is_trivially_copy_constructible<T>::value && 
            std::is_trivially_copy_assignable<T>::value && std::is_trivially_destructible<T>::value))
        && !( std::is_pointer< T >::value || std::is_member_pointer< T >::value ) > {};

    struct StreamBase {};

    template < class T > struct is_serializing_stream : public std::integral_constant< bool, 
        std::is_convertible< T&,Common::StreamBase& >::value > {};

#pragma region( Byte Stream )
    template < class Derived >
    struct ByteStream : public StreamBase
    {
        struct WriteFunctor
        {
            WriteFunctor( ByteStream & w ) : w_( w ) {}

            template < class T >
            void operator()( T const& value ) const
            {
                w_ << value;
            }

        private:
            DENY_COPY_ASSIGNMENT( WriteFunctor );

            ByteStream & w_;
        };

        struct ReadFunctor
        {
            ReadFunctor( ByteStream & w ) : w_( w ) {}

            template < class T >
            void operator()( T const& value ) const
            {
                w_ >> const_cast< T& >( value );
            }

        private:
            DENY_COPY_ASSIGNMENT( ReadFunctor );

            ByteStream & w_;
        };

        void WriteLength( size_t size )
        {
            if ( size < 255 )
                WriteBytes( &size, 1 );
            else
            {
                byte ch = 255;
                WriteBytes( &ch, 1 );
                WriteBytes( &size, 4 );
            }
        }
        size_t ReadLength()
        {
            byte ch;
            ReadBytes( &ch, 1 );
            if ( ch < 255 )
                return ch;

            size_t result = 0;
            ReadBytes( &result, 4 );
            return result;
        }
        void WriteBytes( void const * buf, size_t size )
        {
            static_cast< Derived* >( this )->WriteBytes( buf, size );
        }

        void ReadBytes( void * buf, size_t size ) 
        {
            static_cast< Derived* >( this )->ReadBytes( buf, size );
        }
    };
#pragma endregion

#pragma region( Byte Serializer )
    template < class T > struct ByteSerializer
    {
        template < class ByteStream >
        static void WriteTo( ByteStream & w, T const& value ) 
        {
            static_assert( is_blittable< T >::value, "define COMMON_FIELDS or ByteSerializer for this type" );
            w.WriteBytes( &value, sizeof( value ));
        }
        template < class ByteStream >
        static void ReadFrom( ByteStream & r, T & value ) 
        {
            static_assert( is_blittable< T >::value, "define COMMON_FIELDS or ByteSerializer for this type" );
            r.ReadBytes( &value, sizeof( value ));
        }
    };

    template < class T > struct ByteSerializer< T* >
    {
        template < class ByteStream >
        static void WriteTo( ByteStream & w, T* const& value ) 
        {
            static_assert( false, "pointers are not serializable" );
        }
        template < class ByteStream >
        static void ReadFrom( ByteStream & r, T* & value ) 
        {
            static_assert( false, "pointers are not serializable" );
        }
    };

    template <class E, class Tr, class Al> struct ByteSerializer<std::basic_string<E,Tr,Al>>
    {
        typedef std::basic_string<E,Tr,Al> value_type;

        template <class ByteStream>
        static void WriteTo(ByteStream & w, value_type const & value) 
        {
            w.WriteLength(value.size());
            w.WriteBytes(&value[0], value.size() * sizeof(E));
        }
        template <class ByteStream>
        static void ReadFrom(ByteStream & r, value_type & value) 
        {
            size_t size = r.ReadLength();
            value.resize(size);
            r.ReadBytes(&value[0], size * sizeof(E));
        }
    };

    template < class a, class b > struct ByteSerializer< std::pair< a,b >>
    {
        template < class ByteStream >
        static void WriteTo( ByteStream& w, std::pair< a,b > const& value )
        {
            w << value.first << value.second;
        }
        template < class ByteStream >
        static void ReadFrom( ByteStream& r, std::pair< a,b > & value )
        {
            r >> value.first >> value.second;
        }
    };

    template < class T > struct ByteSerializer< std::vector< T >>
    {
        typedef std::vector< T > value_type;

        template < class ByteStream >
        static void WriteTo( ByteStream& w, value_type const& value )
        {
            w.WriteLength( value.size() );
            for_each( value.begin(), value.end(), typename ByteStream::WriteFunctor( w ));
        }
        template < class ByteStream >
        static void ReadFrom( ByteStream& r, value_type & value )
        {
            size_t size = r.ReadLength();
            value.resize( size );

            for_each( value.begin(), value.end(), typename ByteStream::ReadFunctor( r ));
        }
    };

    template < class T > struct ByteSerializer< std::list< T >>
    {
        typedef std::list< T > value_type;

        template < class ByteStream >
        static void WriteTo( ByteStream& w, value_type const& value )
        {
            w.WriteLength( value.size() );
            for_each( value.begin(), value.end(), typename ByteStream::WriteFunctor( w ));
        }
        template < class ByteStream >
        static void ReadFrom( ByteStream& r, value_type & value )
        {
            size_t size = r.ReadLength();
            value.resize( size );

            for_each( value.begin(), value.end(), typename ByteStream::ReadFunctor( r ));
        }
    };
    
    // Vector of bytes can just be blitted.
    template < > struct ByteSerializer< std::vector< byte >>
    {
        typedef std::vector< byte > value_type;

        template < class ByteStream >
        static void WriteTo( ByteStream& w, value_type const& value )
        {
            w.WriteLength( value.size() );
            w.WriteBytes( &value[0], value.size() );
        }
        template < class ByteStream >
        static void ReadFrom( ByteStream& r, value_type & value )
        {
            size_t size = r.ReadLength();
            value.resize( size );
            r.ReadBytes( &value[0], size );
        }
    };

    template <class K, class T> struct ByteSerializer<std::map<K,T>>
    {
        typedef std::map<K,T> value_type;

        template <class ByteStream>
        static void WriteTo(ByteStream& w, value_type const& value)
        {
            w.WriteLength(value.size());
            for (value_type::value_type const & entry : value)
            {
                w << entry.first << entry.second;
            }
        }
        template <class ByteStream>
        static void ReadFrom(ByteStream& r, value_type & value)
        {
            value.clear();
            size_t size = r.ReadLength();
            value_type::iterator insert_hint = value.begin();

            while (size-- > 0)
            {
                K key;
                T val;
                r >> key >> val;
                insert_hint = value.insert(insert_hint, value_type::value_type(std::move(key), std::move(val)));
            }
        }
    };

#pragma endregion

    template < class T, class ByteStream > 
    typename std::enable_if< is_serializing_stream< ByteStream >::value, ByteStream & >::type 
        operator << ( ByteStream & w, T const& value ) 
    {
        ByteSerializer< T >::WriteTo( w, value );
        return w;
    }

    template < class T, class ByteStream >
    typename std::enable_if< is_serializing_stream< ByteStream >::value, ByteStream & >::type 
        operator >>( ByteStream & r, T & value ) 
    {
        ByteSerializer< T >::ReadFrom( r, value );
        return r;
    }

    struct ByteVectorWriteStream : public ByteStream< ByteVectorWriteStream >
    {
        ByteVectorWriteStream( std::vector< byte > & b ) : v_( b ) {}

        void WriteBytes( void const * buf, size_t size ) 
        {
            byte const * begin = reinterpret_cast< byte const* >( buf );
            v_.insert( v_.end(), begin, begin + size );
        }
    private:
        DENY_COPY_ASSIGNMENT( ByteVectorWriteStream );

        std::vector< byte > & v_;
    };

    struct ByteVectorReadStream : public ByteStream< ByteVectorReadStream >
    {
        typedef std::vector< byte > container;
        typedef range< container::const_iterator > range;

        ByteVectorReadStream( container const& b ) : range_( make_range( b )) {}
        ByteVectorReadStream( range const& r ) : range_( r ) {}

        void ReadBytes( void * buf, size_t count ) 
        {
            byte * beg = static_cast< byte* >( buf );

            size_t available = std::min( count, range_.size() );

            copy_n_checked( range_.begin(), available, beg, available);

            range_.first += available;
        }

        bool empty() { return range_.end() == range_.begin(); }
    private:
        DENY_COPY_ASSIGNMENT( ByteVectorReadStream );

        range range_;
    };

}

