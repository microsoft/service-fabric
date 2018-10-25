/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KXmlParser.h

    Description:
      Kernel Tempate Library (KTL) XML Parser base class defintions

    History:
      NOTE: This is a direct port of FWXML originally written by raymcc
      richhas          16-Jan-2012         Initial version.

--*/


#pragma once

class KXmlParser : public KObject<KXmlParser>, public KShared<KXmlParser>
{
    K_FORCE_SHARED(KXmlParser);

public:
    typedef enum  {
        XSDTYPE_string,                  // strVal
        XSDTYPE_token,                   // strVal
        XSDTYPE_byte,                    // lowest 8 bits of lVal
        XSDTYPE_unsignedByte,            // lowest 8 bits of lVal
        XSDTYPE_base64Binary,            // strVal
        XSDTYPE_hexBinary,               // strVal
        XSDTYPE_integer,                 // lVal
        XSDTYPE_int,                     // lVal
        XSDTYPE_unsignedInt,             // lVal
        XSDTYPE_long,                    // int64Val
        XSDTYPE_unsignedLong,            // int64Val
        XSDTYPE_short,                   // lowest 16 bits of lVal
        XSDTYPE_unsignedShort,           // lowest 16 bits of lVal
        XSDTYPE_double,                  // dblVal
        XSDTYPE_boolean,                 // boolVal
        XSDTYPE_time,                    // strVal
        XSDTYPE_dateTime,                // strVal
        XSDTYPE_duration,                // strVal
        XSDTYPE_date,                    // strVal
        XSDTYPE_anyURI,                  // strVal
        XSDTYPE_language,                // strVal
        XSDTYPE_CDATA                    // byte array
    }   XSDTYPE;

    //** Abstract interface implemented by parser extension
    class IHook
    {
    public:
	    virtual ~IHook() {}

        // Called when a new element is discovered - establishes a "current" element - loigcally a "push" operation
        virtual NTSTATUS OpenElement(
            __in_ecount(ElementNsLength) WCHAR* ElementNs, 
            __in ULONG  ElementNsLength, 
            __in_ecount(ElementNameLength) WCHAR* ElementName, 
            __in ULONG  ElementNameLength, 
            __in_z const WCHAR* StartElementSection) = 0;

        // Called for every attribute on a discovered element
        virtual NTSTATUS AddAttribute(
            __in BOOLEAN HeaderAttributes,    // Set to true if these are the <?xml header attributes
            __in_ecount(AttributeNsLength) WCHAR* AttributeNs,
            __in ULONG  AttributeNsLength,
            __in_ecount(AttributeNameLength) WCHAR* AttributeName,
            __in ULONG  AttributeNameLength,
            __in_ecount(ValueLength) WCHAR* Value,
            __in ULONG  ValueLength
            ) = 0;

        // Called when is it known that all attributes for the current element have been discovered
        virtual NTSTATUS CloseAllAttributes() = 0;

        virtual NTSTATUS AddContent(
            __in ULONG  ContentIndex,
            __in ULONG  ContentType,
            __in_ecount(ContentLength) WCHAR* ContentText,
            __in ULONG  ContentLength
            ) = 0;

        // Called when the current element's end is has been discovered
        virtual NTSTATUS CloseElement(__in ULONG ElementSectionLength) = 0;

        // Called when the Parse() has completed
        virtual NTSTATUS Done(__in BOOLEAN Error) = 0;

        // Called for the current element to discovery the whitespace preservation rule for that element
        virtual BOOLEAN ElementWhitespacePreserved() = 0;
    };
    
    //** Public interface
    static const ULONG      MaxNestedElements = 64;     // NOTE: Does not us recursion - KM friendly

    // Factory
    static NTSTATUS
    Create(__out KXmlParser::SPtr& Result, __in KAllocator& Allocator, __in ULONG AllocationTag);

    // Parse Src (for the length of SizeOfSrc) for an XML document; passing the discovered XML components to
    // the provides IHook instance.
    NTSTATUS 
    Parse(__in_ecount(SizeOfSrc) WCHAR* Src, __in ULONG SizeOfSrc, __in IHook& HookToUse);

    // Allow reuse of the instance.
    void 
    Reset();

    // Failed line and position properties
    ULONG
    GetFailedLine();

    ULONG
    GetFailedCharaterPosition();

private:
    NTSTATUS RecognizeIdent2(_Outref_result_buffer_(IdentLen) WCHAR*& Ident, _Out_ ULONG& IdentLen, _Outref_result_buffer_maybenull_(NsPrefixLen) WCHAR*& NsPrefix, _Out_ ULONG& NsPrefixLen);
    NTSTATUS RecognizeElement();
    NTSTATUS RecognizeXmlHeader();
    NTSTATUS RecognizeAttributes(__in BOOLEAN HeaderAttributes);
    NTSTATUS RecognizeQString(_Outref_result_buffer_(Len) WCHAR*& Target, _Out_ ULONG& Len);
    NTSTATUS RecognizeContent();
    NTSTATUS RecognizeCDATA(__out BOOLEAN& Result);
    NTSTATUS StripComments();

    inline BOOLEAN IsSpace(WCHAR Char)
    {
        return ((Char == 0x0009) ||     // HT  
                (Char == 0x000A) ||     // LF
                (Char == 0x000B) ||     // VT
                (Char == 0x000C) ||     // FF
                (Char == 0x000D) ||     // CR
                (Char == 0x0020));      // or space
    }

    inline LONG StrNCmp(__in_ecount(Size) const WCHAR* String1, __in_ecount(Size) const WCHAR* String2, __in ULONG Size)
    {
        #pragma warning(disable:4127)   // C4127: conditional expression is constant 
        while (TRUE)
        {
            USHORT         todo = (Size*2 > MAXUSHORT) ? MAXUSHORT : (USHORT)(Size*2);  // todo is the buffer chunk size to compare in bytes
            UNICODE_STRING string1;
            UNICODE_STRING string2;

            string2.MaximumLength = (string2.Length = (string1.MaximumLength = (string1.Length = todo)));
            string1.Buffer = (WCHAR*)String1;
            string2.Buffer = (WCHAR*)String2;

            LONG    result =  RtlCompareUnicodeString(&string1, &string2, FALSE);
            if (result != 0)
            {
                return result;
            }

            if ((Size -= todo/2) == 0)
            {
                return 0;
            }

            String1 += todo/2;
            String2 += todo/2;
        }
    }

    inline NTSTATUS 
    Peek(WCHAR& Result, LONG Offset = 0)
    {
        if (_CurrentPos + Offset >= _LastValid)
        {
            return K_STATUS_XML_END_OF_INPUT;
        }
        
        Result = _CurrentPos[Offset];
        return STATUS_SUCCESS;
    }

    inline BOOLEAN
    PeekMultiple(__out NTSTATUS& Status, __in_ecount(LengthToPeek) WCHAR const * const TestChars, ULONG LengthToPeek)
    {
        Status = STATUS_SUCCESS;

        for (ULONG ix = 0; ix < LengthToPeek; ix++)
        {
            WCHAR       c;
            Status = Peek(c, ix);
            if (!NT_SUCCESS(Status) || (TestChars[ix] != c))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    inline NTSTATUS 
    Next()
    {
        if ((_CurrentPos) >= _LastValid)
        {
            return K_STATUS_XML_END_OF_INPUT;
        }

        if (*_CurrentPos == L'\n')
        {
            _LineNumber++;
            _CharPos = 0;
        }

        _CharPos++;
        _CurrentPos++;

        return STATUS_SUCCESS;
    }

    inline NTSTATUS
    SkipNext(ULONG CharsToSkip)
    {
        NTSTATUS    status = STATUS_SUCCESS;

        while (CharsToSkip > 0)
        {
            status = Next();
            if (!NT_SUCCESS (status))
            {
                return status;
            }

            CharsToSkip--;
        }

        return STATUS_SUCCESS;
    }

    inline NTSTATUS 
    StripWs() 
    {
        #pragma warning(disable:4127)   // C4127: conditional expression is constant
        while(TRUE) 
        {
            WCHAR       c;
            NTSTATUS    status = Peek(c);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            if (!IsSpace(c))
            {
                return STATUS_SUCCESS;
            }

            status = Next();
            if (!NT_SUCCESS(status))
            {
                return status;
            }
        };
    }

    inline NTSTATUS 
    StripComment(BOOLEAN& Result)
    {
        NTSTATUS    status = STATUS_SUCCESS;
        WCHAR       c;

        Result = FALSE;

        if (!NT_SUCCESS((status = Peek(c))) || (c != L'<')) // Quick short-circuit test
        {
            return status;
        }

        static WCHAR    startComment[]  = L"<!--";
        static WCHAR    endComment[]    = L"-->";

        Result = PeekMultiple(status, &startComment[0], 4);
        if (Result)
        {
            // Have comment start
            status = SkipNext(4);           // Skip start-of-comment
            if (NT_SUCCESS(status))
            {
                // skip comment content - stopping at the end-of-comment
                while (!PeekMultiple(status, &endComment[0], 3))
                {
                    status = Next();
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                status = SkipNext(3);       // Skip end-of-comment
            }

            Result = (NT_SUCCESS(status));
        }

        return status;
    }

    /*
    From http://www.w3.org/TR/2004/REC-xml11-20040204:

    Start-tag
    [40]   STag    ::=    '<' Name (S Attribute)* S? '>'

    [4]    NameStartChar    ::=    ":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] |
                                    [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF] |
                                    [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
    [4a]   NameChar    ::=    NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
    [5]    Name    ::=    NameStartChar (NameChar)*
    */

    /*
        Currently LegalIdent_FirstChar doesn't deal with supplementary code points
        (Unicode code point between U+10000 and U+EFFFF)

        Also ":" is not allowed as the first char since we treat it as
        namespace prefix separator
    */
    inline BOOLEAN 
    LegalIdent_FirstChar(WCHAR Char)
    {
        if (Char >= L'a' && Char <= 'z') return TRUE;
        if (Char >= L'A' && Char <= 'Z') return TRUE;
        if (Char == L'_') return TRUE;
        if (Char >= 0xC0 && Char <= 0xD6) return TRUE;
        if (Char >= 0xD8 && Char <= 0xF6) return TRUE;
        if (Char >= 0xF8 && Char <= 0x2FF) return TRUE;
        if (Char >= 0x370 && Char <= 0x37D) return TRUE;
        if (Char >= 0x37F && Char <= 0x1FFF) return TRUE;
        if (Char == 0x200C || Char == 0x200D) return TRUE;
        if (Char >= 0x2070 && Char <= 0x218F) return TRUE;
        if (Char >= 0x2C00 && Char <= 0x2FEF) return TRUE;
        if (Char >= 0x3001 && Char <= 0xD7FF) return TRUE;
        if (Char >= 0xF900 && Char <= 0xFDCF) return TRUE;
        if (Char >= 0xFDF0 && Char <= 0xFFFD) return TRUE;
        return FALSE;
    }

    inline BOOLEAN 
    LegalIdent_Char(WCHAR Char)
    {
        if (LegalIdent_FirstChar(Char))  return TRUE;
        if (Char >= L'0' && Char <= L'9') return TRUE;
        if (Char == L'.' || Char == L'-' || Char == 0xB7) return TRUE;
        if (Char >= 0x300 && Char <= 0x36F) return TRUE;
        if (Char >= 0x203F && Char <= 0x2040) return TRUE;
        return FALSE;
    }

private:
    WCHAR const*    _Start;
    WCHAR const*    _CurrentPos;
    WCHAR const*    _LastValid;
    ULONG           _LineNumber;
    ULONG           _CharPos;
    IHook*          _Hook;

    static const ULONG  CDATA_START_LENGTH = 9;
    static const ULONG  CDATA_END_LENGTH = 3;
    static const WCHAR* CDATA_BEGIN_TAG;
    static const WCHAR* CDATA_END_TAG;
};

