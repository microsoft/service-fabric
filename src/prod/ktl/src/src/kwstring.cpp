/*++

Module Name:

    kwstring.cpp

Abstract:

    This file contains the user mode and kernel mode implementations of a KWString object.

Author:

    Norbert P. Kusters (norbertk) 11-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

Revision History:

--*/

#include <ktl.h>

static WCHAR gs_Null = 0;


KWString::~KWString(
    )
{
    Cleanup();
}

//
//
//
KWString::KWString(
    __in KAllocator& Allocator
    )
    :   _Allocator(&Allocator)
{
    Zero();
}

KWString::KWString(
    __in const KWString& Source
    )
    :   _Allocator(Source._Allocator)
{
    Zero();
    if (NT_SUCCESS(Source.Status())) {
        SetConstructorStatus(InitializeFromUnicodeString(Source._String));
    } else {
        SetConstructorStatus(Source.Status());
    }
}

KWString::KWString(__in const KWString&& Source)
    :   _Allocator(nullptr)
{
    Zero();
#if !defined(PLATFORM_UNIX)
    #pragma warning(disable:4213)   // C4213: nonstandard extension used : cast on l-value
    const_cast<KAllocator*>(_Allocator) = Source._Allocator;    // Copy Allocator
#else
    _Allocator = Source._Allocator;
#endif

    SetConstructorStatus(Source.Status());                      // Move Status
    ((KWString&)Source).SetConstructorStatus(STATUS_SUCCESS);

    _String = Source._String;                                   // Move string
    ((KWString&)Source).Zero();
}

KWString::KWString(
    __in KAllocator& Allocator,
    __in const UNICODE_STRING& Source
    )
    :   _Allocator(&Allocator)
{
    Zero();
    SetConstructorStatus(InitializeFromUnicodeString(Source));
}

KWString::KWString(
    __in KAllocator& Allocator,
    __in_z const WCHAR* Source
    )
    :   _Allocator(&Allocator)
{
    UNICODE_STRING string;
    Zero();
    RtlInitUnicodeString(&string, Source);
    SetConstructorStatus(InitializeFromUnicodeString(string));
}

KWString::KWString(
    __in KAllocator& Allocator,
    __in const GUID& Guid
    )
    :   _Allocator(&Allocator)
{
    Zero();
    SetConstructorStatus(InitializeFromGuid(Guid));
}

KWString::KWString(
    __in KAllocator& Allocator,
    __in ULONGLONG Value,
    __in ULONG Base
    )
    :   _Allocator(&Allocator)
{
    Zero();
    SetConstructorStatus(InitializeFromULONGLONG(Value, Base));
}

KWString&
KWString::operator=(
    __in const KWString& Source
    )
{
    if (NT_SUCCESS(Source.Status())) {
        SetConstructorStatus(InitializeFromUnicodeString(Source._String));
    } else {
        SetConstructorStatus(Source.Status());
    }
    return *this;
}


KWString&
KWString::operator=(
    __in const KWString&& Source
    )
{
    Cleanup();      // Deallocate this.<state>

#if !defined(PLATFORM_UNIX)
    #pragma warning(disable:4213)   // C4213: nonstandard extension used : cast on l-value
    const_cast<KAllocator*>(_Allocator) = Source._Allocator;    // Copy Allocator
#else
    _Allocator = Source._Allocator;    // Copy Allocator
#endif

    SetConstructorStatus(Source.Status());                      // Move Status
    ((KWString&)Source).SetConstructorStatus(STATUS_SUCCESS);

    _String = Source._String;                                   // Move string
    ((KWString&)Source).Zero();

    return *this;
}

KWString&
KWString::operator=(
    __in const UNICODE_STRING& Source
    )
{
    SetConstructorStatus(InitializeFromUnicodeString(Source));
    return *this;
}


KWString&
KWString::operator=(
    __in const KStringView &Src
    )
{
    UNICODE_STRING Tmp;

    KFatal(Src.Length() * 2 < 0x10000);
    KFatal(Src.BufferSizeInChars() * 2 < 0x10000);

    Tmp.Buffer = PWCHAR(Src);
    Tmp.Length = (USHORT) Src.Length() * 2;
    Tmp.MaximumLength = (USHORT) Src.BufferSizeInChars() * 2;

    SetConstructorStatus(InitializeFromUnicodeString(Tmp));
    return *this;
}

KWString&
KWString::operator=(
    __in_z const WCHAR* Source
    )
{
    UNICODE_STRING string;
    RtlInitUnicodeString(&string, Source);
    SetConstructorStatus(InitializeFromUnicodeString(string));
    return *this;
}


KWString&
KWString::operator=(
    __in const GUID& Guid
    )
{
    SetConstructorStatus(InitializeFromGuid(Guid));
    return *this;
}


KWString&
KWString::operator+=(
    __in const KWString& Addend
    )
{
    if (NT_SUCCESS(Status())) {
        if (NT_SUCCESS(Addend.Status())) {
            *this += Addend._String;
        } else {
            SetConstructorStatus(Addend.Status());
        }
    }
    return *this;
}


KWString&
KWString::operator +=(
    __in const WCHAR c
    )
{
    if (_String.Length + 2 < _String.MaximumLength)
    {
        _String.Buffer[_String.Length/2] = c;
        _String.Buffer[(_String.Length/2)+1] = 0;    // Null terminate it (!)
        _String.Length += 2;
    }
    else
    {
        // We have to expand the current string in this
        // case as there is not enough room for one more wchar.

        WCHAR Buf[2];
        Buf[0] = c;
        Buf[1] = 0;
        *this += Buf;
    }
    return *this;
}

NTSTATUS
KWString::Reserve(
    __in USHORT MaxBytes
    )
{
    Cleanup();
    _String.Buffer = PWSTR(_newArray<UCHAR>(KTL_TAG_WSTRING, *_Allocator, MaxBytes));
    if (!_String.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _String.Length = 0;
    _String.MaximumLength = MaxBytes;
    return STATUS_SUCCESS;
}


KWString&
KWString::operator+=(
    __in const UNICODE_STRING& Addend
    )

/*++

Routine Description:

    This routine appends the given string to this string.

Arguments:

    Addend  - Supplies the string to append to this one.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING newString;

    if (!NT_SUCCESS(Status())) {
        return *this;
    }

    if (!Addend.Length) {
        return *this;
    }

    newString.Length = 0;
    HRESULT hr;
    hr = UShortAdd(_String.Length, Addend.Length, &newString.MaximumLength);
    KInvariant(SUCCEEDED(hr));
    hr = UShortAdd(newString.MaximumLength, sizeof(WCHAR), &newString.MaximumLength);
    KInvariant(SUCCEEDED(hr));

    newString.Buffer = _newArray<WCHAR>(KTL_TAG_WSTRING, *_Allocator, newString.MaximumLength/sizeof(WCHAR));

    if (!newString.Buffer) {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return *this;
    }

    RtlCopyUnicodeString(&newString, &_String);

    Cleanup();

    _String = newString;

    RtlAppendUnicodeStringToString(&_String, &Addend);

    return *this;
}

KWString&
KWString::operator+=(
    __in_z const WCHAR* Addend
    )
{
    UNICODE_STRING string;
    RtlInitUnicodeString(&string, Addend);
    return (*this += string);
}

KWString&
KWString::operator+=(
    __in const GUID& Guid
    )
{
    KWString string(*_Allocator, Guid);
    return (*this += string);
}

KWString::operator UNICODE_STRING*(
    )
{
    return &_String;
}

KWString::operator UNICODE_STRING&(
    )
{
    return _String;
}

KWString::operator WCHAR*(
    ) const
{
    return _String.Buffer;
}

LONG
KWString::CompareTo(
    __in const KWString& Comparand,
    __in BOOLEAN CaseInsensitive
    ) const
{
    return RtlCompareUnicodeString(&_String, &Comparand._String, CaseInsensitive);
}

LONG
KWString::CompareTo(
    __in const UNICODE_STRING& Comparand,
    __in BOOLEAN CaseInsensitive
    ) const
{
    return RtlCompareUnicodeString(&_String, &Comparand, CaseInsensitive);
}

LONG
KWString::CompareTo(
    __in_z const WCHAR* Comparand,
    __in BOOLEAN CaseInsensitive
    ) const
{
    UNICODE_STRING string;
    RtlInitUnicodeString(&string, Comparand);
    return RtlCompareUnicodeString(&_String, &string, CaseInsensitive);
}

NTSTATUS
KWString::ExtractGuidSuffix(
    __out GUID& Guid
    )

/*++

Routine Description:

    This routine will extract a GUID from the end of this string.  If the format does not match then an error is returned.

Arguments:

    Guid    - Returns the guid found at the end of the given string.

Return Value:

    NTSTATUS

--*/

{
    const int GuidLength = 38;
    UNICODE_STRING suffixString;
    NTSTATUS status;

    suffixString.Length = GuidLength*sizeof(WCHAR);
    if (suffixString.Length > _String.Length) {
        suffixString.Length = _String.Length;
    }
    suffixString.Buffer = (WCHAR*) (((UCHAR*) _String.Buffer) + (_String.Length - suffixString.Length));
    suffixString.MaximumLength = suffixString.Length + sizeof(WCHAR);

    status = RtlGUIDFromString(&suffixString, &Guid);

    return status;
}

VOID
KWString::Zero(
    )
{
    _String.Buffer = &gs_Null;
    _String.Length = 0;
    _String.MaximumLength = sizeof(WCHAR);
}

VOID
KWString::Cleanup(
    )
{
    if (_String.Buffer && _String.Buffer != &gs_Null) {
        _deleteArray(_String.Buffer);
    }
    Zero();
}

NTSTATUS
KWString::InitializeFromUnicodeString(
    __in const UNICODE_STRING& Source
    )

/*++

Routine Description:

    This routine initializes this string to be equal to the given string.

Arguments:

    Source  - Supplies the source string.

Return Value:

    NTSTATUS

--*/

{
    Cleanup();

    HRESULT hr;
    hr = UShortAdd(Source.Length, sizeof(WCHAR), &_String.MaximumLength);
    KInvariant(SUCCEEDED(hr));
    _String.Buffer = _newArray<WCHAR>(KTL_TAG_WSTRING, *_Allocator, _String.MaximumLength/sizeof(WCHAR));

    if (!_String.Buffer) {
        Cleanup();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&_String, &Source);

    return STATUS_SUCCESS;
}

NTSTATUS
KWString::InitializeFromGuid(
    __in const GUID& Guid
    )

/*++

Routine Description:

    This routine initializes this string to be the character version of the given GUID.

Arguments:

    Guid    - Supplies the GUID.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status;
    UNICODE_STRING guidString;

    status = RtlStringFromGUID(Guid, &guidString);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = InitializeFromUnicodeString(guidString);

    RtlFreeUnicodeString(&guidString);

    return status;
}

NTSTATUS
KWString::InitializeFromULONGLONG(
    __in ULONGLONG Value,
    __in ULONG Base
    )

/*++

Routine Description:

    This routine initializes this string to be the character version of the given ULONGLONG value.

Arguments:

    Value   - Supplies the value.

    Base    - Specifies the base to use when converting Value to a string. The possible values are:
                16: Hexadecimal; 8 : Octal; 2 : Binary; 0 or 10 : Decimal

Return Value:

    NTSTATUS

--*/

{
    _String.MaximumLength = 65 * sizeof(WCHAR); // for max 64-bit binary string.
    _String.Length = 0;

    _String.Buffer = _newArray<WCHAR>(KTL_TAG_WSTRING, *_Allocator, _String.MaximumLength/sizeof(WCHAR));
    if (!_String.Buffer) {
        Cleanup();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = RtlInt64ToUnicodeString(Value, Base, &_String);
    if (!NT_SUCCESS(status))
    {
        Cleanup();
        return status;
    }

    return STATUS_SUCCESS;
}
