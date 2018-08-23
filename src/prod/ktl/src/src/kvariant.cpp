
/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    KVariant

    Description:
      Kernel Tempate Library (KTL): KVariant, _KMemRef

      Describes a general-purpose late-bound container similar to the VARIANT
      in COM.

    History:
      raymcc          7-Mar-2011         Initial version.
      raymcc          5-Jum-2012         Updates for KString, KUri, ISO 8601, DOM, KBuffer, etc.

--*/


#include <ktl.h>


// KVariant::operator=
//
// Assignment operator.
//
// This always succeeds, as it either does a bitwise copy of the union
// fields or else logically copies the SPtr.
//
// *
KVariant& KVariant::operator=(
    __in const KVariant& Src
    )
{
    Cleanup();
    _Type = Src._Type;

    // If a shared pointer type, just copy the pointer.
    //
    if (IsSPtr())
    {
        GetSPtr() = Src.GetSPtr();
    }
    else // bitwise copy types
    {
        _Value = Src._Value;
    }
    return *this;
}




//
//  KVariant::Cleanup
//
//*
VOID
KVariant::Cleanup()
{
    if (IsSPtr())
    {
        GetSPtr() = 0;
    }

    RtlZeroMemory(&_Value, sizeof(_Value));
    _Type = Type_EMPTY;
}






//
//  KVariant::CloneTo
//
BOOLEAN
KVariant::CloneTo(
    __out KVariant& Dest,
    __in  KAllocator& Allocator
    )
{
    Dest.Clear();
    NTSTATUS Res;

    if (!IsSPtr())
    {
        // All the bitwise copy types.
        //
        Dest._Type = _Type;
        Dest._Value = _Value;
        return TRUE;
    }

    // If here, we have a deep copy to perform via
    // one of the SPtr types.
    //

    // Type_KString deep copy
    //
    if (_Type == Type_KString_SPtr)
    {
        KString::SPtr ThisStr = reinterpret_cast<KString::SPtr&>(GetSPtr());
        KString::SPtr Tmp = ThisStr->Clone(TRUE);
        if (!Tmp)
        {
            Dest.Clear();
            Dest._Type = Type_ERROR;
            Dest._Value._Status = STATUS_INSUFFICIENT_RESOURCES;
            return FALSE;
        }
        Dest.GetSPtr() = reinterpret_cast<KSharedUntyped::SPtr&>(Tmp);
        Dest._Type = Type_KString_SPtr;
        return TRUE;
    }

    // Type_KBuffer deep copy
    //
    else if (_Type == Type_KBuffer_SPtr)
    {
        KBuffer::SPtr SrcBuf = reinterpret_cast<KBuffer::SPtr&>(GetSPtr());
        if (!SrcBuf)
        {
            // We have cloned a null buffer
            Dest._Type = Type_KBuffer_SPtr;
            return TRUE;
        }
        KBuffer::SPtr DestBuf;
        Res = KBuffer::CreateOrCopy(DestBuf, SrcBuf.RawPtr(), Allocator);
        if (!NT_SUCCESS(Res))
        {
            Dest.Clear();
            Dest._Type = Type_ERROR;
            Dest._Value._Status = Res;
            return FALSE;
        }
        Dest.GetSPtr() = reinterpret_cast<KSharedUntyped::SPtr&>(DestBuf);
        Dest._Type = Type_KBuffer_SPtr;
        return TRUE;
    }

    // Type_KUri deep copy
    //
    else if (_Type == Type_KUri_SPtr)
    {
        KUri::SPtr Src = reinterpret_cast<KUri::SPtr&>(GetSPtr());
        KUri::SPtr DestUri;
        Res = Src->Clone(Allocator, DestUri);
        if (!NT_SUCCESS(Res))
        {
            Dest.Clear();
            Dest._Type = Type_ERROR;
            Dest._Value._Status = Res;
            return FALSE;
        }
        Dest._Type = Type_KUri_SPtr;
        Dest.GetSPtr() = reinterpret_cast<KSharedUntyped::SPtr&>(DestUri);
        return TRUE;
    }

    //  Type_KIMutableDomNode
    //
    else if (_Type == Type_KIMutableDomNode_SPtr)
    {
        KIMutableDomNode::SPtr Src = reinterpret_cast<KIMutableDomNode::SPtr&>(GetSPtr());
        KIMutableDomNode::SPtr Copy;

        Res = KDom::CloneAs(Src, Copy);
        if (!NT_SUCCESS(Res))
        {
            Dest.Clear();
            Dest._Type = Type_ERROR;
            Dest._Value._Status = Res;
            return FALSE;
        }
        Dest._Type = Type_KIMutableDomNode_SPtr;
        Dest.GetSPtr() = reinterpret_cast<KSharedUntyped::SPtr&>(Copy);
        return TRUE;
    }

    //
    KFatal(FALSE);
    return FALSE;
}




//
//  KVariant::ToStream
//
//  Notes:
//      Currently we don't serialize KIMutableDomNode, KBuffer, or KMemRef
//
// ! Serialize KBuffer, KIMutableDomNode
//
NTSTATUS
KVariant::ToStream(
    __in KIOutputStream& Stream
    )
{
    switch (_Type)
    {
    case Type_LONG:
        return Stream.Put(_Value._LONG_Val);

    case Type_ULONG:
        return Stream.Put(_Value._ULONG_Val);

    case Type_LONGLONG:
        return Stream.Put(_Value._LONGLONG_Val);

    case Type_ULONGLONG:
        return Stream.Put(_Value._ULONGLONG_Val);

    case Type_GUID:
        return Stream.Put(_Value._GUID_Val);

    case Type_KDateTime:
        return Stream.Put(KDateTime(_Value._ULONGLONG_Val));

    case Type_KDuration:
        return Stream.Put(KDuration(_Value._ULONGLONG_Val));

    case Type_KString_SPtr:
        return Stream.Put(reinterpret_cast<KString::SPtr&>(GetSPtr()));

    case Type_KUri_SPtr:
        return Stream.Put(reinterpret_cast<KUri::SPtr&>(GetSPtr()));

    case Type_BOOLEAN:
        return Stream.Put(_Value._BOOLEAN_Val);
    }

    return STATUS_NOT_IMPLEMENTED;
}


//
//  KVariant::_Construct
//
//  All the various Create overloads come to here for execution.
//  This is not intended for direct invocation by users.
//
//  This assumes a 'constructed' KVariant and will wipe its previous contents.
//
//  If this fails, it will set the variant to Type_ERROR.
//
NTSTATUS
KVariant::_Construct(
    __in  KStringView Src,
    __in  KVariantType DesiredType,
    __in  KAllocator& Allocator,
    __out KVariant& Target
    )
{
    NTSTATUS Res;
    Target.Clear();
    Target._Type = Type_ERROR;  // The default unless cleared during execution

    if (Src.IsNull())
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (DesiredType)
    {
        ////

        case Type_KString_SPtr:
        {
            KString::SPtr Tmp = KString::Create(Src, Allocator, TRUE);
            if (!Tmp)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            Target._Type = Type_KString_SPtr;
            Target.GetSPtr() = reinterpret_cast<const KSharedUntyped::SPtr&>(Tmp);
            return STATUS_SUCCESS;
        }

        ////

        case Type_KUri_SPtr:
        {
            KUri::SPtr Tmp;
            Res = KUri::Create(Src, Allocator, Tmp);
            if (!NT_SUCCESS(Res))
            {
                return Res;
            }
            Target._Type = Type_KUri_SPtr;
            Target.GetSPtr() =  reinterpret_cast<KSharedUntyped::SPtr&>(Tmp);
            return STATUS_SUCCESS;
        }

        ////

        case Type_KDateTime:
        {
            KInvariant(FALSE);
            return STATUS_NOT_SUPPORTED;
        }

        ////

        case Type_KDuration:
        {
            KDuration Duration;
            if (!Duration.FromString(Src))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            Target._Value._LONGLONG_Val = Duration;
            Target._Type = Type_KDuration;
            return STATUS_SUCCESS;
        }

        ////

        case Type_ULONGLONG:
        {
            if (!Src.ToULONGLONG(Target._Value._ULONGLONG_Val))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            Target._Type = Type_LONGLONG;
            return STATUS_SUCCESS;
        }

        ////

        case Type_LONGLONG:
        {
            if (!Src.ToLONGLONG(Target._Value._LONGLONG_Val))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            Target._Type = Type_LONGLONG;
            return STATUS_SUCCESS;
        }

        ////

        case Type_ULONG:
        {
            if (!Src.ToULONG(Target._Value._ULONG_Val))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            Target._Type = Type_ULONG;
            return STATUS_SUCCESS;
        }

        ////

        case Type_LONG:
        {
            if (!Src.ToLONG(Target._Value._LONG_Val))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            Target._Type = Type_LONG;
            return STATUS_SUCCESS;
        }

        ////

        case Type_GUID:
        {
            if (!Src.ToGUID(Target._Value._GUID_Val))
            {
                return STATUS_OBJECT_TYPE_MISMATCH;
            }
            Target._Type = Type_GUID;
            return STATUS_SUCCESS;
        }

        ////
        case Type_KIMutableDomNode_SPtr:
        {
            KIMutableDomNode::SPtr Tmp;
            Res = KDom::FromString(Src, Allocator, Tmp);
            if (!NT_SUCCESS(Res))
            {
                return Res;
            }
            Target._Type = Type_KIMutableDomNode_SPtr;
            Target.GetSPtr() =  reinterpret_cast<KSharedUntyped::SPtr&>(Tmp);
            return STATUS_SUCCESS;
        }

        ////

        case Type_KBuffer_SPtr:
        {
            KBuffer::SPtr Tmp;
            HRESULT hr;
            ULONG Size;
            hr = ULongAdd(Src.Length(), 1, &Size);
            KInvariant(SUCCEEDED(hr));
            hr = ULongMult(Size, 2, &Size);
            KInvariant(SUCCEEDED(hr));

            Res = KBuffer::Create(Size, Tmp, Allocator);
            if (!NT_SUCCESS(Res))
            {
                return Res;
            }

            KStringView Overlay(PWCHAR(Tmp->GetBuffer()), Size);
            Overlay.SetNullTerminator();
            Target._Type = Type_KIMutableDomNode_SPtr;
            Target.GetSPtr() =  reinterpret_cast<KSharedUntyped::SPtr&>(Tmp);
            return STATUS_SUCCESS;
        }

        case Type_BOOLEAN:
        {
            if (Src.CompareNoCase(KStringView(L"TRUE")) == 0)
            {
                Target = BOOLEAN(TRUE);
                return STATUS_SUCCESS;
            }
            else if (Src.CompareNoCase(KStringView(L"FALSE")) == 0)
            {
                Target = BOOLEAN(FALSE);
                return STATUS_SUCCESS;
            }
            else if (Src.Compare(KStringView(L"1")) == 0)
            {
                Target = BOOLEAN(TRUE);
                return STATUS_SUCCESS;
            }
            else if (Src.Compare(KStringView(L"0")) == 0)
            {
                Target = BOOLEAN(FALSE);
                return STATUS_SUCCESS;
            }

            Target.Clear();
            Target._Type = Type_ERROR;
            return STATUS_INVALID_PARAMETER;
        }
    }

    KFatal(FALSE);
    return STATUS_INVALID_PARAMETER;
}



//
//  KVariant::ToString
//
BOOLEAN
KVariant::ToString(
    __in  KAllocator& Allocator,
    __out KString::SPtr& Stringized
    )
{
    NTSTATUS Res;

    switch (_Type)
    {
        case Type_ERROR:
        {
            KString::SPtr Tmp = KString::Create(L"!ERROR!", Allocator, TRUE);
            if (!Tmp)
            {
                _Value._Status = STATUS_INSUFFICIENT_RESOURCES;
                return FALSE;
            }
            Tmp->SetNullTerminator(); // Send back an empty string
            Stringized = Tmp;
            return TRUE;
        }

        case Type_EMPTY:
        {
            KString::SPtr Tmp = KString::Create(Allocator, 1);
            if (!Tmp)
            {
                return FALSE;
            }
            Tmp->SetNullTerminator(); // Send back an empty string
            Stringized = Tmp;
            return TRUE;
        }

        ////

        case Type_KString_SPtr:
        {
            KString::SPtr This = reinterpret_cast<const KString::SPtr&>(GetSPtr());
            Stringized = This->Clone();
            if (!Stringized)
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_KUri_SPtr:
        {
            KUri::SPtr This = reinterpret_cast<const KUri::SPtr&>(GetSPtr());
            KString::SPtr Tmp = KString::Create((KStringView&)(*This), Allocator);
            if (!Tmp)
            {
                return FALSE;
            }
            Stringized = Tmp;
            return TRUE;
        }

        ////

        case Type_KDateTime:
        {
            KDateTime Date(_Value._LONGLONG_Val);
            Res = Date.ToString(Allocator, Stringized);
            if (!NT_SUCCESS(Res))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_KDuration:
        {
            KDuration Duration(_Value._LONGLONG_Val);
            Res = Duration.ToString(Allocator, Stringized);
            if (!NT_SUCCESS(Res))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_BOOLEAN:
        {
            KString::SPtr Tmp = KString::Create(Allocator, KStringView::MaxBooleanString);
            if (!Tmp)
            {
                return FALSE;
            }
            if (_Value._BOOLEAN_Val == TRUE)
            {
                Tmp->CopyFrom(KStringView(L"TRUE"));
            }
            else
            {
                Tmp->CopyFrom(KStringView(L"FALSE"));
            }
            Tmp->SetNullTerminator();
            Stringized = Tmp;
            return TRUE;
        }

        ////

        case Type_ULONGLONG:
        {
            Stringized = KString::Create(Allocator, KStringView::Max64BitNumString);
            if (!Stringized)
            {
                return FALSE;
            }
            if (!Stringized->FromULONGLONG(_Value._ULONGLONG_Val))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_LONGLONG:
        {
            Stringized = KString::Create(Allocator, KStringView::Max64BitNumString);
            if (!Stringized)
            {
                return FALSE;
            }
            if (!Stringized->FromLONGLONG(_Value._LONGLONG_Val))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_ULONG:
        {
            Stringized = KString::Create(Allocator, KStringView::Max32BitNumString);
            if (!Stringized)
            {
                return FALSE;
            }
            if (!Stringized->FromULONG(_Value._ULONG_Val))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_LONG:
        {
            Stringized = KString::Create(Allocator, KStringView::Max32BitNumString);
            if (!Stringized)
            {
                return FALSE;
            }
            if (!Stringized->FromLONG(_Value._LONG_Val))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_GUID:
        {
            Stringized = KString::Create(Allocator, KStringView::MaxGuidString);
            if (!Stringized)
            {
                return FALSE;
            }
            if (!Stringized->FromGUID(_Value._GUID_Val))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////

        case Type_KIMutableDomNode_SPtr:
        {
            KIMutableDomNode::SPtr Tmp = reinterpret_cast<const KIMutableDomNode::SPtr&>(GetSPtr());;
            Res = KDom::ToString(Tmp, Allocator, Stringized);
            if (!NT_SUCCESS(Res))
            {
                return FALSE;
            }
            return TRUE;
        }

        ////
        /// Add Base64 text if we need this.
        //
        case Type_KBuffer_SPtr:
        {
            return FALSE;
        }

        ////
        case Type_KMemRef:
        {
            // This would serialize the buffer to base64
            return FALSE;
        }
    }

    return FALSE;
}


// KVariant Convert
//
// Not all combinations currently supported.  We will add more on an as-needed basis.
//


BOOLEAN
KVariant::Convert(
    __out LONGLONG& Value
    )
{
    if (_Type == Type_KString_SPtr)
    {
        KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
        return Tmp->ToLONGLONG(Value);
    }

    if (_Type == KVariant::Type_ULONGLONG)
    {
        Value = (LONGLONG) _Value._ULONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_ULONG)
    {
        Value = (LONGLONG) _Value._ULONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_LONG)
    {
        Value = (LONGLONG) _Value._LONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDateTime)
    {
        Value = _Value._LONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDuration)
    {
        Value = _Value._LONGLONG_Val;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
KVariant::Convert(
    __out ULONGLONG& Value
    )
{
    if (_Type == Type_KString_SPtr)
    {
        KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
        return Tmp->ToULONGLONG(Value);
    }

    if (_Type == KVariant::Type_LONGLONG)
    {
        Value = (ULONGLONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_ULONG)
    {
        Value = (ULONGLONG) _Value._ULONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_LONG)
    {
        Value = (ULONGLONG) _Value._LONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDateTime)
    {
        Value = (ULONGLONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDuration)
    {
        Value = (ULONGLONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
KVariant::Convert(
    __out LONG& Value
    )
{
    if (_Type == Type_KString_SPtr)
    {
        KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
        return Tmp->ToLONG(Value);
    }

    if (_Type == KVariant::Type_ULONGLONG)
    {
        Value = (LONG) _Value._ULONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_ULONG)
    {
        Value = (LONG) _Value._ULONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_LONGLONG)
    {
        Value = (LONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDateTime)
    {
        Value = (LONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDuration)
    {
        Value = (LONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
KVariant::Convert(
    __out ULONG& Value
    )
{
    if (_Type == Type_KString_SPtr)
    {
        KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
        return Tmp->ToULONG(Value);
    }

    if (_Type == KVariant::Type_ULONGLONG)
    {
        Value = (ULONG) _Value._ULONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_LONGLONG)
    {
        Value = (ULONG) _Value._ULONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_LONG)
    {
        Value = (ULONG) _Value._LONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDateTime)
    {
        Value = (ULONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    if (_Type == KVariant::Type_KDuration)
    {
        Value = (ULONG) _Value._LONGLONG_Val;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
KVariant::Convert(
    __out GUID& Value
    )
{
    if (_Type == KVariant::Type_KString_SPtr)
    {
        KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
        return Tmp->ToGUID(Value);
    }

    return FALSE;
}

BOOLEAN
KVariant::Convert(
    __out KDateTime& Value
    )
{
    if (_Type != Type_KString_SPtr)
    {
        KInvariant(FALSE);
    }

    if (_Type == KVariant::Type_LONGLONG)
    {
        Value = KDateTime(_Value._LONGLONG_Val);
        return TRUE;
    }

    if (_Type == KVariant::Type_ULONGLONG)
    {
        Value = KDateTime((LONGLONG) _Value._ULONGLONG_Val);
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
KVariant::Convert(
    __out KDuration& Value
    )
{
    if (_Type == Type_KString_SPtr)
    {
        KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
        return Value.FromString(*Tmp);
    }

    if (_Type == KVariant::Type_LONGLONG)
    {
        Value = KDuration(_Value._LONGLONG_Val);
        return TRUE;
    }

    if (_Type == KVariant::Type_ULONGLONG)
    {
        Value = KDuration((LONGLONG) _Value._ULONGLONG_Val);
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
KVariant::Convert(
    __in  KAllocator& Allocator,
    __out KUri::SPtr& Uri
    )
{
    if (_Type != Type_KString_SPtr)
    {
        return FALSE;
    }

    KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());
    NTSTATUS Res = KUri::Create(*Tmp, Allocator, Uri);
    if (!NT_SUCCESS(Res))
    {
        return FALSE;
    }

    return TRUE;
}



NTSTATUS
KVariant::Convert(
    __in  KAllocator& Allocator,
    __out KBuffer::SPtr& Buffer,
    __in  BOOLEAN IncludeNull
    )
{
    if (_Type != Type_KString_SPtr)
    {
        return FALSE;
    }

    KString::SPtr Tmp = reinterpret_cast<const KString::SPtr&>(GetSPtr());

    HRESULT hr;
    ULONG Size;
    hr = ULongMult(Tmp->Length(), 2, &Size);
    KInvariant(SUCCEEDED(hr));

    if (IncludeNull)
    {
        // OLDCODE: Size += 2;   // For null terminator
        hr = ULongAdd(Size, 2, &Size);
        KInvariant(SUCCEEDED(hr));
    }

    NTSTATUS Res = KBuffer::Create(Size, Buffer, Allocator);
    if (!NT_SUCCESS(Res))
    {
        return FALSE;
    }

    KStringView Overlay(PWCHAR(Buffer->GetBuffer()), Size/2);
    Overlay.CopyFrom(*Tmp);

    if (IncludeNull)
    {
        Overlay.SetNullTerminator();
    }

    return TRUE;
}

