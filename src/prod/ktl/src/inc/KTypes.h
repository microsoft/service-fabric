/*++

Module Name:

    KTypes.h

Abstract:

    This file defines types to promote writing strongly typed code.

Author:

    Peng Li (pengli)    17-Dec-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#pragma once

//
// Abstract base class to support strongly typed types.
//
// A derived type can work with its base type as if it is a base type most of the time.
// But instances of two derived types are not comparable, assignable or copy constructable with each other.
//
// For example, we can define two new "ULONG" types as the following:
//
// class UlongType1 : public KStrongType<UlongType1, ULONG>
// {
//      K_STRONG_TYPE(UlongType1, ULONG);
//  public:
//      UlongType1() {}
//      ~UlongType1() {}
// };
//
// class UlongType2 : public KStrongType<UlongType2, ULONG>
// {
//      K_STRONG_TYPE(UlongType2, ULONG);
//  public:
//      UlongType2() {}
//      ~UlongType2() {}
// };
//
// Both UlongType1 and UlongType2 are essentially ULONGs, but they are incompatible types.
//
// KStrongType<> can also use a non-scalar type, such as KGuid as the base_type.
//

template <typename derived_type, typename base_type>
class KStrongType abstract
{
public:
    KStrongType()
    {
    }

    KStrongType(derived_type const& Right)
    {
        _Value = Right._Value;
    }

    KStrongType<derived_type, base_type>& operator=(derived_type const& Right)
    {
        if (this != &Right)
        {
            _Value = Right._Value;
        }
        return *this;
    }

    KStrongType(base_type Value)
    {
        _Value = Value;
    }

    base_type& operator=(base_type Value)
    {
        Set(Value);
        return _Value;
    }

    base_type& GetReference()
    {
        return _Value;
    }

    base_type Get() const
    {
        return _Value;
    }

    VOID Set(base_type Value)
    {
        _Value = Value;
    }

    //
    // Commonly used comparison operators.
    //
    // If the derived type wishes to override or hide any of these operators,
    // follow this example:
    //
    // class UlongType1 : public KStrongType<UlongType1, ULONG>
    // {
    //      K_STRONG_TYPE(UlongType1, ULONG);
    //  public:
    //      UlongType1() {}
    //      ~UlongType1() {}
    //
    //      // Override operator< on the base class
    //      BOOLEAN operator<(const UlongType1& Right) const
    //      {
    //          ...
    //      }
    //
    //  private:
    //      // Trap operator< on the base class
    //      using KStrongType<UlongType1, ULONG>::operator<;
    // };
    //

    BOOLEAN operator==(const derived_type& Right) const
    {
        return _Value == Right._Value ? TRUE : FALSE;
    }

    BOOLEAN operator!=(const derived_type& Right) const
    {
        return !(*this == Right);
    }

    BOOLEAN operator<(const derived_type& Right) const
    {
        return _Value < Right._Value ? TRUE : FALSE;
    }

    BOOLEAN operator<=(const derived_type& Right) const
    {
        return _Value <= Right._Value ? TRUE : FALSE;
    }

    BOOLEAN operator>(const derived_type& Right) const
    {
        return _Value > Right._Value ? TRUE : FALSE;
    }

    BOOLEAN operator>=(const derived_type& Right) const
    {
        return _Value >= Right._Value ? TRUE : FALSE;
    }

protected:
    base_type _Value;
};

//
// An optional macro to be used with a KStrongType<> derived type.
//

#define K_STRONG_TYPE(derived_type, base_type) \
        static_assert(sizeof(base_type) == sizeof(KStrongType<derived_type, base_type>), "KStrongType has incorrect size"); \
    public: \
        derived_type(base_type x) : KStrongType<derived_type, base_type>(x) {} \
    private:


