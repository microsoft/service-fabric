
/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kwstring

    Description:
      Kernel Tempate Library (KTL): KWString

      Describes a general purpose wide-string class

    History:
      raymcc          16-Aug-2010         Initial version.

--*/
#pragma once

class KWString : public KObject<KWString> {
    KAllocator_Required();         // Enable nested KArrays

    public:

        ~KWString(
            );

        NOFAIL
        KWString(
            __in KAllocator& Allocator
            );

        FAILABLE
        KWString(
            __in const KWString& Source
            );

        FAILABLE
        KWString(
            __in const KWString&& Source
            );

        FAILABLE
        KWString(
            __in KAllocator& Allocator,
            __in const UNICODE_STRING& Source
            );

        FAILABLE
        KWString(
            __in KAllocator& Allocator,
            __in_z const WCHAR* Source
            );

        FAILABLE
        KWString(
            __in KAllocator& Allocator,
            __in const GUID& Guid
            );

        FAILABLE
        KWString(
            __in KAllocator& Allocator,
            __in ULONGLONG Value,
            __in ULONG Base
            );

        FAILABLE
        KWString&
        operator=(
            __in const KWString& Source
            );

        NOFAIL
        KWString&
        operator=(
            __in const KWString&& Source
            );

        FAILABLE
        KWString&
        operator=(
            __in const UNICODE_STRING& Source
            );


        FAILABLE
        KWString&
        operator=(
            __in const PUNICODE_STRING Source
            )
        {
            *this = *Source;
            return *this;
        }

        FAILABLE
        KWString&
        operator=(
            __in_z const WCHAR* Source
            );


        KWString&
        operator=(
            __in const KStringView& View
            );

        FAILABLE
        KWString&
        operator=(
            __in const GUID& Guid
            );

        FAILABLE
        KWString&
        operator+=(
            __in const KWString& Addend
            );

        FAILABLE
        KWString&
        operator+=(
            __in const UNICODE_STRING& Addend
            );

        FAILABLE
        KWString&
        operator +=(
            __in WCHAR c
            );

        FAILABLE
        KWString&
        operator+=(
            __in_z const WCHAR* Addend
            );

        FAILABLE
        KWString&
        operator+=(
            __in const GUID& Guid
            );

        operator UNICODE_STRING*(
            );

        operator UNICODE_STRING&(
            );

        operator WCHAR*(
            ) const;

        LONG
        CompareTo(
            __in const KWString& Comparand,
            __in BOOLEAN CaseInsensitive = FALSE
            ) const;

        LONG
        CompareTo(
            __in const UNICODE_STRING& Comparand,
            __in BOOLEAN CaseInsensitive = FALSE
            ) const;

        LONG
        CompareTo(
            __in_z const WCHAR* Comparand,
            __in BOOLEAN CaseInsensitive = FALSE
            ) const;


        // Compare
        //
        // This is a case-sensitive comparison that can rut at IRQL <= DPC.
        //
        __drv_maxIRQL(DISPATCH_LEVEL)
        LONG
        Compare(
            __in const UNICODE_STRING& Comparand
            ) const
        {
            PWSTR Other = Comparand.Buffer;
            USHORT OtherLength = Comparand.Length / 2;
            PWSTR This =  _String.Buffer;
            USHORT ThisLength = _String.Length / 2;

            // Check for either of these still being null.
            //
            if (!Other)
            {
                if (This == nullptr)
                {
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
            else if (!This)
            {
                return -1;
            }

            // If here, there are real strings to compare.  One may be a prefix
            // of the other, so a simple memcmp doesn't work.
            //
            for ( ; ThisLength && OtherLength; ThisLength--, OtherLength--)
            {
                LONG Res = *This++ - *Other++;
                if (Res)
                {
                    return Res;
                }
            }

            if (ThisLength)
            {
                return 1;
            }
            else if (OtherLength)
            {
                return -1;
            }
            else
            {
                return 0;
            }
        }

        // Compare
        //
        // This is a case-sensitive comparison that can rut at IRQL <= DPC.
        // Implemented as a pass-through to the other overload.
        //
        __drv_maxIRQL(DISPATCH_LEVEL)
        LONG
        Compare(
            __in const KWString& Comparand
            ) const
        {
            return Compare(Comparand._String);
        }

        BOOLEAN
        operator == (const KWString& Other) const
        {
            return Compare(Other) == 0 ? TRUE : FALSE;
        }

        BOOLEAN
        operator != (KWString& Other) const
        {
            return Compare(Other) == 0 ? FALSE : TRUE;
        }

        NTSTATUS
        ExtractGuidSuffix(
            __out GUID& Guid
            );

        // Serialization/Deserialization Helpers

        USHORT
        Length() const
        {
            return _String.Length;
        }

        PVOID
        Buffer() const
        {
            return _String.Buffer;
        }

        VOID
        Acquire(
            __in UNICODE_STRING& Src
            )
        {
            Cleanup();
            _String = Src;
        }

        // Reserves a buffer of arbitrary size.
        // Only operator+= with WCHAR works properly with this
        // at this point.
        //
        // Used for building strings without reallocation.
        //
        NTSTATUS
        Reserve(
            __in USHORT BufferSizeInBytes
            );

        // Retreive the KAllocator used by a KWString instance
        KAllocator&
        GetAllocator()
        {
            return *_Allocator;
        }

        // Null out an instance of a KWString
        VOID
        Reset()
        {
            Cleanup();
        }

    private:

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        InitializeFromUnicodeString(
            __in const UNICODE_STRING& Source
            );

        NTSTATUS
        InitializeFromGuid(
            __in const GUID& Guid
            );

        NTSTATUS
        InitializeFromULONGLONG(
            __in ULONGLONG Value,
            __in ULONG Base
            );

        UNICODE_STRING      _String;
#if !defined(PLATFORM_UNIX)
        KAllocator* const   _Allocator;
#else
        KAllocator*   _Allocator;
#endif
};

