

/*++

    (c) 2010-2012 by Microsoft Corp. All Rights Reserved.

    KVariant

    Description:
      Kernel Tempate Library (KTL): KVariant, _KMemRef

      Describes a general-purpose late-bound container similar to the VARIANT
      in COM.

    History:
      raymcc          7-Mar-2011         Initial version.
      raymcc          5-Jun-2012         Structural rewrite/ & updates for new types

--*/

#pragma once


class KIMutableDomNode;

// KVariant
//
// This is a general-purpose late-bound container of some other type, similar to the COM VARIANT.
//
// Copies of a KVariant via Copy construction and assignment are by value and thus
// use *shallow-copy* semantics for the SPtr-based types. (KBuffer, KString, KUri, etc.)
//
// Any operation that can fail returns NTSTATUS.   Operations with no status return value don't fail.
// This means that copy construction and assigment between KVariant instances always succeed and don't need to be checked.
//
// The static Create() methods with Allocators can fail and return a KVariant of Type_ERROR.  This can be checked
// with the Status() function, Type() or by operator !.
//
// In cases where the data source is variable length and allocation can file, there is an explicit Construct() function
// acting as a de facto constructor.
//
// To make deep copies, use the CloneTo()/CloneFrom() methods.
//
//
class KVariant
{
private:
    // This is used to store all SPtr types (by removing the type)
    //
    struct KSharedUntyped : public KSharedBase
    {
        typedef KSharedPtr<KSharedUntyped> SPtr;
        KSharedUntyped() {}
        virtual void OnDelete(){}
    };


public:
    enum KVariantType
    {
        // These are copied by value when the KVariant is copy constructed/assigned
        //
        Type_ERROR   = 0,
        Type_EMPTY   = 1,
        Type_BOOLEAN = 2,
        Type_LONG    = 3,
        Type_ULONG   = 4,
        Type_LONGLONG = 5,
        Type_ULONGLONG = 6,
        Type_GUID = 7,
        Type_KMemRef = 8,
        Type_KDateTime = 9,      // DateTime encoded as a LONGLONG
        Type_KDuration = 10,     // Duration encoded as LONGLONG

        // These are copied by reference when copy constructed/assigned. They
        // are copied by value on when CloneTo()/CloneFrom() is used.
        //
        Type_SPtr                  = 0x80000000,
        Type_KBuffer_SPtr          = 0x80000002,
        Type_KString_SPtr          = 0x80000003,
        Type_KUri_SPtr             = 0x80000004,
        Type_KIMutableDomNode_SPtr = 0x80000005
    };

    // Constructor
    //
    KVariant()
    {
        Initialize();
    }


    // Destructor
    //
   ~KVariant()
    {
        Cleanup();
    }

    // Copy constructor.
    //
    // Implemented via operator=
    //
    KVariant(
        __in const KVariant& Src
        )
    {
        Initialize();
        *this = Src;
    }

    //  Assignment operator
    //
    //  Because of shallow copy semantics, this cannot fail.
    //
    KVariant& operator= (
        __in const KVariant& Src
        );


    // Type
    //
    // Returns the type currently in the variant.
    //
    KVariantType Type() const
    {
        return _Type;
    }

    // Clear
    //
    // Empties the KVariant.
    //
    void Clear()
    {
        Cleanup();
    }

    BOOLEAN
    IsValid()
    {
        return _Type == Type_ERROR ? FALSE : TRUE;
    }

    BOOLEAN operator !()
    {
        return _Type == Type_ERROR ? TRUE: FALSE;
    }

    NTSTATUS
    Status()
    {
        if (_Type == Type_ERROR)
        {
            return _Value._Status;
        }
        return STATUS_SUCCESS;
    }


    // IsEmpty
    //
    // Returns TRUE if the variant has nothing in it.
    //
    BOOLEAN
    IsEmpty()
    {
        if (_Type == Type_EMPTY)
        {
            return TRUE;
        }
        return FALSE;
    }

    // IsNull
    //
    // If the underlying type is an SPtr-based value, this return TRUE
    // if the SPtr is null, FALSE if it is pointing to something.
    // Also returns TRUE if the variant is of Type_EMPTY
    //
    BOOLEAN
    IsNull()
    {
        if ((_Type & Type_SPtr) && !GetSPtr())
        {
            return TRUE;
        }
        return IsEmpty();
    }

    // IsSPtr
    //
    //
    BOOLEAN
    IsSPtr()
    {
        if (_Type & Type_SPtr)
        {
            return TRUE;
        }
        return FALSE;
    }

    // Create
    //
    // This is a series of static overloads which take an Allocator return KVariant.  Because
    // they can fail, the resulting KVariant may be of Type_ERROR and that can be checked by
    // operator!,  IsValid() or Status().
    //
    // This particular pattern is useful so that most code can place this call in an parameter
    // position within another call:
    //
    //      MyCall(KVariant::Create(L"Foo", Allocator));
    //
    // ...and the receiver can check that the KVariant is valid and/or of the correct type.
    // Note that because a failed KVariant won't have the expected type, just checking for the
    // desired type is sufficient on the receiving end.
    //

    // Returns a KVariant of Type_KString_SPtr, or Type_ERROR on failure
    //

    static KVariant
    Create(
        __in const LPCWSTR Str,
        __in KAllocator& Allocator
        )
    {
        KVariant Tmp;
        NTSTATUS Res = _Construct(KStringView(Str), Type_KString_SPtr, Allocator, Tmp);
        if (!NT_SUCCESS(Res))
        {
            Tmp._Value._Status = Res;
        }
        return Tmp;
    }

    // overload
    //
    static KVariant
    Create(
        __in const KStringView Str,
        __in KAllocator& Allocator
        )
    {
        KVariant Tmp;
        NTSTATUS Res = _Construct(Str, Type_KString_SPtr, Allocator, Tmp);
        if (!NT_SUCCESS(Res))
        {
            Tmp._Value._Status = Res;
        }
        return Tmp;
    }

    // overload
    // Returns a KVariant of Type_KString_SPtr, or Type_ERROR on failure
    //
    static KVariant
    Create(
        __in UNICODE_STRING& Src,
        __in KAllocator& Allocator
        )
    {
        KVariant Tmp;
        NTSTATUS Res = _Construct(KStringView(Src), Type_KString_SPtr, Allocator, Tmp);
        if (!NT_SUCCESS(Res))
        {
            Tmp._Value._Status = Res;
        }        
        return Tmp;
    }


    // Create (overload)
    //
    // Returns a KVariant of the required type if possible, or Type_ERROR on failure.
    //
    // To distinguish between failure to allocate and a type mismatch, the caller
    // can pass in an optional NTSTATUS address as the last parameter.
    //
    // Parameters:
    //      Src           The source string
    //      DesiredType   The desirned KVariant type.
    //      Allocator     The allocator to use in cases where allocation
    //                    is required.
    //
    // On failure, the resulting KVariant is of Type_ERROR.
    //
    // In practice, because the string may be syntactically incompatile
    // with the desired type, the caller may need to get more detail
    // as to the failure in order to distinguish between a type mismatch
    // and a failure to allocate.
    //
    static KVariant
    Create(
        __in  KStringView Src,
        __in  KVariantType DesiredType,
        __in  KAllocator& Alloc
        )
    {
        KVariant Tmp;
        NTSTATUS Res = _Construct(Src, DesiredType, Alloc, Tmp);
        if (!NT_SUCCESS(Res))
        {
            Tmp._Value._Status = Res;
        }
        return Tmp;
    }


    // ToString
    //
    // Converts the current variant to its string representation. If the KVariant is already
    // a string, this will make a COPY of the content.
    //
    // This always succeeds except when out of memory, as all types have a string
    // representation.
    //
    // Parameters:
    //      Allocator     The Allocator to use
    //      Stringized    The string representation
    //
    // Return value:
    //      TRUE on success
    //      FALSE if allocation failure, as that is the only failure case.
    //
    BOOLEAN
    ToString(
        __in  KAllocator& Allocator,
        __out KString::SPtr& Stringized
        );




    // CloneTo
    //
    // Makes a deep copy of the *this KVariant to the Destination object.  Any reference
    // types are completely copied such that the internal buffers are not shared
    // by the new KVariant.
    //
    // Parameters:
    //      Alloc           The allocator to use
    //      Dest            The object which will receive the cloned value of *this.
    //
    // Return values:
    //      TRUE on success
    //      FALSE on failure to allocate, as that is the only failure case
    //
    // The Variant will be Type_EMPTY if the allocation fails.
    //
    BOOLEAN
    CloneTo(
        __out KVariant& Dest,
        __in  KAllocator& Alloc
        );

    // CloneFrom
    //
    // Inverted version which clones *this from a source object.
    //
    BOOLEAN
    CloneFrom(
        __in KVariant& Src,
        __in KAllocator& Alloc
        )
    {
        return Src.CloneTo(*this, Alloc);
    }


    ///////////////////////////////////////////////////////////////////////////
    //
    // Convert
    //
    // (Not all possible coercions are currently supported; we will add
    //  these as we need them. The most useful ones are supported).
    //
    // These attempt to Converts the value to the required type.  This can be used
    // to Convert between types where it makes sense.
    //
    // The result may have a loss of precision, sign loss, or
    // other behaviors.  This works roughly the same as coercion
    // in scripting languages, such as Javascript.
    //
    // If the underlying KVariant is a string, this will do a
    // conversion to the required type, if possible.
    //
    // To Convert to a string, use ToString().
    //
    // As an example, this will Convert a LONGLONG to a ULONG, losing
    // the higher bits.
    //
    // Returns FALSE if the coercion could not be applied. For example,
    // coercing from a Uri to a GUID makes no sense.
    //
    //
    BOOLEAN
    Convert(
        __out LONGLONG& Value
        );

    BOOLEAN
    Convert(
        __out ULONGLONG& Value
        );

    BOOLEAN
    Convert(
        __out LONG& Value
        );

    BOOLEAN
    Convert(
        __out ULONG& Value
        );

    BOOLEAN
    Convert(
        __out GUID& Value
        );

    BOOLEAN
    Convert(
        __out KDateTime& Value
        );

    BOOLEAN
    Convert(
        __out KDuration& Value
        );

    BOOLEAN
    Convert(
        __in  KAllocator& Allocator,
        __out KUri::SPtr& Value
        );

    NTSTATUS
    Convert(
        __in  KAllocator& Allocator,
        __out KBuffer::SPtr& Buffer,
        __in  BOOLEAN IncludeNull = TRUE
        );

    ///////////////////////////////////////////////////////////////////////////
    //
    // Type-specific constructors.
    //
    // All are implemented by the operator= implementation.
    //

#define K_VARIANT_CONSTRUCTOR(TYPENAME) \
    KVariant(   \
        __in const TYPENAME Val \
        )                       \
                                \
    {                           \
        Initialize();           \
        *this=(Val);            \
    }

    K_VARIANT_CONSTRUCTOR(BOOLEAN);
    K_VARIANT_CONSTRUCTOR(LONG);
    K_VARIANT_CONSTRUCTOR(ULONG);
    K_VARIANT_CONSTRUCTOR(LONGLONG);
    K_VARIANT_CONSTRUCTOR(ULONGLONG);
    K_VARIANT_CONSTRUCTOR(GUID);
    K_VARIANT_CONSTRUCTOR(KDateTime);
    K_VARIANT_CONSTRUCTOR(KDuration);
    K_VARIANT_CONSTRUCTOR(KMemRef);

    K_VARIANT_CONSTRUCTOR(KString::SPtr);
    K_VARIANT_CONSTRUCTOR(KUri::SPtr);
    K_VARIANT_CONSTRUCTOR(KBuffer::SPtr);


    // This breaks the pattern is because of the mutual use of this
    // class and KVariant
    //
    KVariant(
        __in const KSharedPtr<KIMutableDomNode> DomPtr
        )
    {
        Initialize();
        *this = DomPtr;
    }

    // UNICODE STRING not supportd as a constructor; call Construct()

    ///////////////////////////////////////////////////////////////////////////////
    //
    // Assignment operations
    //
#define K_VARIANT_SAFE_ASSIGNMENT(TYPENAME, VARIANT_TYPE) \
    KVariant& operator=( \
        __in const TYPENAME Src \
        )                         \
    {                             \
        Cleanup();                \
        _Type = VARIANT_TYPE;        \
        _Value._##TYPENAME##_Val = Src;      \
        return *this;             \
    }                             \


    K_VARIANT_SAFE_ASSIGNMENT(BOOLEAN, Type_BOOLEAN);
    K_VARIANT_SAFE_ASSIGNMENT(LONG, Type_LONG);
    K_VARIANT_SAFE_ASSIGNMENT(ULONG, Type_ULONG);
    K_VARIANT_SAFE_ASSIGNMENT(LONGLONG, Type_LONGLONG);
    K_VARIANT_SAFE_ASSIGNMENT(ULONGLONG, Type_ULONGLONG);
    K_VARIANT_SAFE_ASSIGNMENT(GUID, Type_GUID);
    K_VARIANT_SAFE_ASSIGNMENT(KMemRef, Type_KMemRef);


#define K_VARIANT_SAFE_SPTR_ASSIGNMENT(TYPENAME, VARIANT_TYPE) \
    KVariant& \
    operator=(__in const TYPENAME::SPtr& Src) \
    {                                                  \
        Cleanup();                                     \
        _Type = VARIANT_TYPE;                          \
        GetSPtr() = reinterpret_cast<const KSharedUntyped::SPtr&>(Src); \
        return *this;                                             \
    }

    K_VARIANT_SAFE_SPTR_ASSIGNMENT(KString, Type_KString_SPtr);
    K_VARIANT_SAFE_SPTR_ASSIGNMENT(KBuffer, Type_KBuffer_SPtr);
    K_VARIANT_SAFE_SPTR_ASSIGNMENT(KUri, Type_KUri_SPtr);

    KVariant&
    operator=(__in const KSharedPtr<KIMutableDomNode> Src)
    {
        Cleanup();
        _Type = Type_KIMutableDomNode_SPtr;
        GetSPtr() = reinterpret_cast<const KSharedUntyped::SPtr&>(Src);
        return *this;
    }

    KVariant& operator=(KDateTime DateTime)
    {
        Cleanup();
        _Type = Type_KDateTime;
        _Value._LONGLONG_Val = LONGLONG(DateTime);
        return *this;
    }

    KVariant& operator=(KDuration DateTime)
    {
        Cleanup();
        _Type = Type_KDuration;
        _Value._LONGLONG_Val = LONGLONG(DateTime);
        return *this;
    }


    ///////////////////////////////////////////////////////////////////////////////////
    //
    // Casting
    //
    // Casts go to KFatal if the underlying type is not correct.
    //
#define K_VARIANT_RAW_CAST(TYPENAME, KVARIANT_TYPE) \
    operator TYPENAME() const                    \
    {                                       \
        KFatal(_Type == KVARIANT_TYPE);    \
        return _Value._##TYPENAME##_Val;    \
    }                                       \


    K_VARIANT_RAW_CAST(BOOLEAN, Type_BOOLEAN);
    K_VARIANT_RAW_CAST(LONG, Type_LONG);
    K_VARIANT_RAW_CAST(ULONG, Type_ULONG);
#if !defined(PLATFORM_UNIX)
    K_VARIANT_RAW_CAST(LONGLONG, Type_LONGLONG);
#else
    operator LONGLONG() const
    {
        KFatal(_Type == Type_KDateTime
            || _Type == Type_KDuration
            || _Type == Type_LONGLONG);
        return _Value._LONGLONG_Val;
    }
#endif
    K_VARIANT_RAW_CAST(ULONGLONG, Type_ULONGLONG);
    K_VARIANT_RAW_CAST(GUID, Type_GUID);

    operator KMemRef()
    {
        KFatal(_Type == Type_KMemRef);
        return KMemRef(_Value._KMemRef_Val);
    }

#if !defined(PLATFORM_UNIX)
    operator KDateTime() const
    {
        KFatal(_Type == Type_KDateTime);
        return KDateTime(_Value._LONGLONG_Val);
    }

    operator KDuration() const
    {
        KFatal(_Type == Type_KDuration);
        return KDuration(_Value._LONGLONG_Val);
    }
#endif

    operator PWSTR() const
    {
        if (_Type == Type_KString_SPtr)
        {
            KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
            if (!Tmp->IsNullTerminated())
            {
                KFatal(FALSE);
            }
            return PWSTR(*Tmp);
        }
        else if (_Type == Type_KUri_SPtr)
        {
#ifdef PLATFORM_UNIX
            KStringView Tmp = (KStringView)((LPCWSTR)(*reinterpret_cast<const KUri::SPtr&>(GetSPtr())));
#else
            KStringView Tmp = (KStringView) *reinterpret_cast<const KUri::SPtr&>(GetSPtr());
#endif
            if (!Tmp.IsNullTerminated())
            {
                KFatal(FALSE);
            }
            return PWSTR(Tmp);
        }
        KFatal(FALSE);
        return nullptr;
    }

    operator LPCWSTR() const
    {
        return operator PWSTR();
    }


    operator KUriView() const
    {
        KFatal(_Type == Type_KUri_SPtr);
        return *reinterpret_cast<const KUri::SPtr&>(GetSPtr());
    }

    operator KStringView&() const
    {
        if (_Type == Type_KString_SPtr)
        {
            return (*reinterpret_cast<const KString::SPtr&>(GetSPtr()));
        }
        else if (_Type == Type_KUri_SPtr)
        {
            return (*reinterpret_cast<const KUri::SPtr&>(GetSPtr()));
        }
        KFatal(FALSE);
        static KStringView* Tmp = 0;  // No path to here; just keep the compiler believing we return something
        return *Tmp;
    }

    operator KString::SPtr() const
    {
        KFatal(_Type == Type_KString_SPtr);
        return reinterpret_cast<const KString::SPtr&>(GetSPtr());
    }

    operator KUri::SPtr() const
    {
        KFatal(_Type == Type_KUri_SPtr);
        return reinterpret_cast<const KUri::SPtr&>(GetSPtr());
    }

    operator KBuffer::SPtr() const
    {
        KFatal(_Type == Type_KBuffer_SPtr);
        return reinterpret_cast<const KBuffer::SPtr&>(GetSPtr());
    }

    operator KSharedPtr<KIMutableDomNode>() const
    {
        KFatal(_Type == Type_KIMutableDomNode_SPtr);
        return reinterpret_cast<const KSharedPtr<KIMutableDomNode>&>(GetSPtr());
    }

    operator UNICODE_STRING() const
    {
        KFatal(_Type == Type_KString_SPtr);
        UNICODE_STRING Str;
        RtlZeroMemory(&Str, sizeof(UNICODE_STRING));
        KString::SPtr Tmp = reinterpret_cast<const KSharedPtr<KString>&>(GetSPtr());

        KFatal((Tmp->Length() * 2) < 0x10000); // KFata if this string is too large for UNICODE_STRING

        Str.Buffer = PWCHAR(*Tmp);
        Str.Length = USHORT(Tmp->Length() * 2);
        Str.MaximumLength = USHORT(Tmp->BufferSizeInChars() * 2);
        return Str;
    }

    operator GUID*()
    {
        KFatal(_Type == Type_GUID);
        return &_Value._GUID_Val;
    }

    // For Dom serialization
    //
    NTSTATUS
    ToStream(
        __in KIOutputStream& Stream
        );

private:
    // Begin -- We intentionally don't support these
    operator PUNICODE_STRING();
    KVariant(PUNICODE_STRING);
    KVariant(UNICODE_STRING&);
    KVariant(KStringView&);
    KVariant(LPCWSTR);
    // End

    KSharedUntyped::SPtr&
    GetSPtr() const
    {
        return *(KSharedUntyped::SPtr*) &_Value._SPtr_Val;
    }

    // The discriminant for the union.
    // Uses
    KVariantType _Type;

    // Union of common types needed.  Note that unions are incompatible
    // with C++ types which have constructors, since there is no way to know
    // which constructor/destructor to call.
    //
    union
    {
        NTSTATUS       _Status;         // Used only for Type_ERROR
        BOOLEAN        _BOOLEAN_Val;
        LONG           _LONG_Val;
        ULONG          _ULONG_Val;
        ULONGLONG      _ULONGLONG_Val;  // KDuration and KDateTime are also bundled here
        LONGLONG       _LONGLONG_Val;
        GUID           _GUID_Val;
        _KMemRef       _KMemRef_Val;
        UCHAR          _SPtr_Val[sizeof(KSharedUntyped::SPtr)];
    }   _Value;

    void Initialize()
    {
        RtlZeroMemory(&_Value, sizeof(_Value));
        _Type = Type_EMPTY;
    }

    // All the "Create" overloads come to here for execution.
    //
    static NTSTATUS
    _Construct(
        __in  const KStringView Src,
        __in  KVariantType DesiredType,
        __in  KAllocator& Alloc,
        __out KVariant& Target
        );

    // Cleanup
    //
    VOID
    Cleanup();
};





