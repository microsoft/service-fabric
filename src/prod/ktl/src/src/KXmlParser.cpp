/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KXmlParser.cpp

    Description:
      Kernel Tempate Library (KTL) XML Parser base class implementation

    History:
      NOTE: This is a direct port of FWXML originally written by raymcc
      richhas          16-Jan-2012         Initial version.

--*/

#include <ktl.h>
#include <ktrace.h>

NTSTATUS
KXmlParser::Create(__out KXmlParser::SPtr& Result, __in KAllocator& Allocator, __in ULONG AllocationTag)
{
    Result = _new(AllocationTag, Allocator) KXmlParser();
    if (Result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return STATUS_SUCCESS;
}

KXmlParser::KXmlParser()
{
}

KXmlParser::~KXmlParser()
{
}

NTSTATUS 
KXmlParser::Parse(__in_ecount(Size) WCHAR* Src, __in ULONG Size, __in IHook& HookToUse)
{
    Reset();
    _Hook = &HookToUse;
    _Start = Src;
    _CurrentPos = _Start;
    _LastValid = _Start + Size;

    NTSTATUS    status = RecognizeXmlHeader();       // Won't fail if not present
    if (NT_SUCCESS(status))
    {
        status = RecognizeElement();
        if (NT_SUCCESS(status))
        {
            status = _Hook->Done(FALSE);
        }
    }

    if (!NT_SUCCESS(status))
    {
        KFatal(NT_SUCCESS(_Hook->Done(TRUE)));
    }

    return status;
}

void KXmlParser::Reset()
{
    _Start = 0;
    _CurrentPos = 0;
    _LastValid = 0;
    _LineNumber = 1;
    _CharPos = 1;
}

ULONG
KXmlParser::GetFailedLine()
{
    return _LineNumber;
}

ULONG
KXmlParser::GetFailedCharaterPosition()
{
    return _CharPos;
}

NTSTATUS 
KXmlParser::RecognizeIdent2(_Outref_result_buffer_(IdentLen) WCHAR*& Ident, _Out_ ULONG& IdentLen, _Outref_result_buffer_maybenull_(NsPrefixLen) WCHAR*& NsPrefix, _Out_ ULONG& NsPrefixLen)
{
    NTSTATUS    status = STATUS_SUCCESS;
    WCHAR       c;
    BOOLEAN     nsfound = FALSE;

    Ident = 0;
    NsPrefix = 0;
    IdentLen = 0;
    NsPrefixLen = 0;

    //
    // Well, it *looks* like we might be in business. But
    // we can trust nobody to give us good XML, so we'll just
    // check that first character and see if it's true W3C.
    //
    status = Peek(c);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (!LegalIdent_FirstChar(c))
    {
        return K_STATUS_XML_INVALID_NAME;
    }

    Ident = (WCHAR*)_CurrentPos;
    IdentLen++;

    status = Next();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    //  Well, let's try to go for the whole name.
    //
    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while(TRUE)
    {
        status = Peek(c);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (c == L':')
        {
            if (nsfound)
            {
                return K_STATUS_XML_SYNTAX_ERROR;   // Two namespaces? hardly.
            }

            // If here, we have been recognizing a namespace prefix instead
            // of an element without knowing it.  Such deception deserves a brain transplant.
            // We'll do an identity switch.
            nsfound = TRUE;
            NsPrefix = Ident;
            NsPrefixLen = IdentLen;
            IdentLen = 0;

            status = Next();
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            Ident = (WCHAR*) _CurrentPos;
            continue;
        }

        else if (!LegalIdent_Char(c))
        {
            break;
        }
        else
        {
            IdentLen++;
        }

        status = Next();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return (nsfound && (IdentLen == 0)) ? K_STATUS_XML_SYNTAX_ERROR : STATUS_SUCCESS;
}

NTSTATUS 
                
KXmlParser::RecognizeQString(_Outref_result_buffer_(Len) WCHAR*& Target, _Out_ ULONG& Len)
{
    NTSTATUS    status = STATUS_SUCCESS;
    WCHAR       chStart;

    Target = 0;
    Len = 0;

    status = Peek(chStart);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((chStart != L'"') && (chStart != L'\''))
    {
        return K_STATUS_XML_INVALID_ATTRIBUTE;
    }

    status = Next();     // Skip open quote
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Target = (WCHAR*)_CurrentPos;
    
    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        WCHAR       c;

        status = Peek(c);       // Next char
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = Next();        // Move forward
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (c == chStart)       // If close quote, we're done
        {
            break;
        }

        Len++;
    }

    return STATUS_SUCCESS;
}


//
//  Recognizes all the attributes for a given element
//
NTSTATUS 
KXmlParser::RecognizeAttributes(__in BOOLEAN HeaderAttributes)
{
    NTSTATUS        status = STATUS_SUCCESS;

    WCHAR*          name = 0;
    WCHAR*          nsPrefix = 0;
    WCHAR*          textValue = 0;
    ULONG           nameLength;
    ULONG           nsPrefixLength;
    ULONG           textValueLength;
    WCHAR           c;

    status = StripWs();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    while (NT_SUCCESS(status = Peek(c)) && (c != L'>') && (c != L'?') && (c != L'/'))
    {
        status = RecognizeIdent2(name, nameLength, nsPrefix, nsPrefixLength);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = StripWs();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        WCHAR c1;
        status = Peek(c1);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (c1 != L'=')
        {
            return K_STATUS_XML_INVALID_ATTRIBUTE;
        }

        status = Next();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = StripWs();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = RecognizeQString(textValue, textValueLength);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = StripWs();
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        status = _Hook->AddAttribute(HeaderAttributes, nsPrefix, nsPrefixLength, name, nameLength, textValue, textValueLength);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = _Hook->CloseAllAttributes();
    return status;
}

//
//  Recognizes an element.  A precondition is that any comments have been stripped
//  and that the current position is < followed by a legal ident char.  This should
//  be ensured by the prior caller.  This code strips trailing comments or comments
//  within the content so that each new recursive call meets this precondition.
//
NTSTATUS 
KXmlParser::RecognizeElement()
{
    //* Local stack frame for this function. This is used to implement a look aside stack
    //  for this function to avoid potential stack overflow issues when running in the 
    //  kernel.

    class StackFrame
    {
    public:
        NTSTATUS        status;
        WCHAR*          eltok;
        WCHAR*          elnstok;
        WCHAR*          endeltok;
        WCHAR*          endelnstok;

        ULONG           eltokLength;
        ULONG           elnstokLength;
        ULONG           endeltokLength;
        ULONG           endelnstokLength;
        BOOLEAN         namespaceSeen;

        WCHAR const*    startElement;
        WCHAR const*    endElement;
        WCHAR           c;
        WCHAR           c1;

    public:
        StackFrame(WCHAR const* StartElement)
        {
            status = STATUS_SUCCESS;
            namespaceSeen = FALSE;
            startElement = StartElement;
            endElement = nullptr;
        }

    private:
        StackFrame() {}
    };

    // Allocate local stack
	HRESULT hr;
	ULONG size;
	hr = ULongMult(sizeof(StackFrame), MaxNestedElements, &size);
	KInvariant(SUCCEEDED(hr));
	StackFrame *const   stack = (StackFrame *)_new(KTL_TAG_XMLPARSER, GetThisAllocator()) UCHAR[size];
                        KFinally([stack]() {    _delete(stack); });

    if (stack == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    StackFrame*         csf = stack + MaxNestedElements;        // current stack frame pointer
    NTSTATUS            result = STATUS_SUCCESS;

    // Local redirect macros
    #define $return(s)  result = s; goto PopStack;
    #define status csf->status
    #define eltok csf->eltok
    #define elnstok csf->elnstok
    #define endeltok csf->endeltok
    #define endelnstok csf->endelnstok
    #define eltokLength csf->eltokLength
    #define elnstokLength csf->elnstokLength
    #define endeltokLength csf->endeltokLength
    #define endelnstokLength csf->endelnstokLength
    #define namespaceSeen csf->namespaceSeen
    #define startElement csf->startElement
    #define endElement csf->endElement
    #define c csf->c
    #define c1 csf->c1

    // Trap recursive calls
    #define RecognizeElement() goto PushStack; ReturnFromNestedCall:


PushStack:
    // Create new stack frame
    csf -= 1;

    KAssert(csf > stack);
    if (csf == stack)
    {
        return K_STATUS_XML_PARSER_LIMIT;           // Stack overflow
    }
    #define return BUGGGG

    new(csf) StackFrame(_CurrentPos);
    {
        //
        // This is where the evil starts.  We must be positioned
        // on the '<' and a legal ident by the previous code.
        // Relying on friends in previous code is risky but we'll do it in this case.
        //
        status = Peek(c);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        if (c != L'<')
        {
            $return(K_STATUS_XML_SYNTAX_ERROR);
        }

        status = Next();
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        status = RecognizeIdent2(eltok, eltokLength, elnstok, elnstokLength);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        status = _Hook->OpenElement(elnstok, elnstokLength, eltok, eltokLength, startElement);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        status = RecognizeAttributes(FALSE);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        //
        // Check for wimpy element with no content.
        //
        if (PeekMultiple(status, L"/>", 2) && NT_SUCCESS(status))
        {
            status = SkipNext(2);       // strip "/>"
            if (!NT_SUCCESS(status))
            {
                $return(status);
            }

            endElement = _CurrentPos;
            status = StripComments();
            if (!NT_SUCCESS(status))
            {
                $return(status);
            }

            ULONGLONG       eleSize = endElement - startElement;
            KFatal(eleSize <= MAXULONG);

            status = _Hook->CloseElement((ULONG)eleSize);
            if (!NT_SUCCESS(status))
            {
                $return(status);
            }

            $return(STATUS_SUCCESS);
        }
        else if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        //
        // If here, we have a standard element with content.
        //
        status = Peek(c);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }
        if (c != L'>')
        {
            $return(K_STATUS_XML_SYNTAX_ERROR);
        }

        status = Next();
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        // Content for this element can be interlaced with child elements.
        #pragma warning(disable:4127)   // C4127: conditional expression is constant
        while(TRUE)
        {
            status = Peek(c);
            if (!NT_SUCCESS(status))
            {
                $return(status);
            }

            status = Peek(c1, 1);
            if (!NT_SUCCESS(status))
            {
                $return(status);
            }

            if (c == L'<' && LegalIdent_FirstChar(c1))
            {
                // Hmm. We are now at the '<' symbol. Any comments in the content will have
                // been removed, so we are either about to close the element, or recurse into
                // a child element.

                RecognizeElement();
                status = result;
                if (!NT_SUCCESS(status))
                {
                    $return(status);
                }
            }
            else if (PeekMultiple(status, L"</", 2) && NT_SUCCESS(status))
            {
                //we are at the end of this element
                break;
            }
            else
            {
                // Recognize  content for this element
                status = RecognizeContent();
                if (!NT_SUCCESS(status))
                {
                    $return(status);
                }
            }
        }
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }
    
        //we are at the end of this element
        status = SkipNext(2);       // Strip "</"
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        status = RecognizeIdent2(endeltok, endeltokLength, endelnstok, endelnstokLength);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        if ((endeltokLength != eltokLength) ||
            (StrNCmp(endeltok, eltok, __min(endeltokLength, eltokLength)) != 0) ||
            (endelnstokLength != elnstokLength) ||
            (StrNCmp(endelnstok, elnstok, __min(endelnstokLength, elnstokLength)) != 0))
        {
            $return(K_STATUS_XML_MISMATCHED_ELEMENT);
        }

        status = Peek(c);
        if (c != L'>')
        {
            $return(K_STATUS_XML_SYNTAX_ERROR);
        }

        status = Next();
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        endElement = _CurrentPos;

        // Strip trailing ws and comments.... may hit end-of-file in the process!  That's OK!
        status = StripComments();
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        ULONGLONG   eleSize = endElement - startElement;
        KFatal(eleSize <= MAXULONG);
        status = _Hook->CloseElement((ULONG)eleSize);
        if (!NT_SUCCESS(status))
        {
            $return(status);
        }

        $return(STATUS_SUCCESS);
    }

PopStack:
    #undef $return
    #undef return
    #undef status
    #undef eltok
    #undef elnstok
    #undef endeltok
    #undef endelnstok
    #undef eltokLength
    #undef elnstokLength
    #undef endeltokLength
    #undef endelnstokLength
    #undef namespaceSeen
    #undef startElement
    #undef endElement
    #undef c
    #undef c1
    #undef RecognizeElement

    if (csf == &stack[MaxNestedElements - 1])
    {
        // At TOS - time to return
        return result;
    }

    csf++;
    KFatal(csf < &stack[MaxNestedElements]);
    goto ReturnFromNestedCall;
}

NTSTATUS 
KXmlParser::StripComments()
{
    NTSTATUS status = StripWs();

    if (!NT_SUCCESS(status) && (status != K_STATUS_XML_END_OF_INPUT))
    {
        return status;
    }

    BOOLEAN         commentStripped;
    while (NT_SUCCESS(status = StripComment(commentStripped)) && commentStripped)
    {
        status = StripWs();
        if (!NT_SUCCESS(status))
        {
            if (status != K_STATUS_XML_END_OF_INPUT)
            {
                return(status);
            }

            break;
        }
    }
    if (!NT_SUCCESS(status) && (status != K_STATUS_XML_END_OF_INPUT))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS 
KXmlParser::RecognizeXmlHeader()
{
    NTSTATUS        status = STATUS_SUCCESS;
    WCHAR           c;

    StripWs();

    // Assume < is the current position
    if (!NT_SUCCESS((status = Peek(c, 1))) || (c != L'?'))
    {
        return status;
    }

    status = SkipNext(2);           // Strip "<?"
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    WCHAR*      xmlIdent = NULL;
    ULONG       xmlIdentLen = 0;
    WCHAR*      ns = NULL;
    ULONG       nsLen = 0;

    status = RecognizeIdent2(xmlIdent, xmlIdentLen, ns, nsLen);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if ((xmlIdentLen != 3) || (StrNCmp(xmlIdent, L"xml", 3) != 0))
    {
        return K_STATUS_XML_HEADER_ERROR;
    }

    status = RecognizeAttributes(TRUE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = Peek(c);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (c != L'?')
    {
        return K_STATUS_XML_HEADER_ERROR;
    }

    status = Next();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = Peek(c);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (c != L'>')
    {
        return K_STATUS_XML_HEADER_ERROR;
    }

    status = Next();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = StripWs();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    BOOLEAN     commentStripped;
    while (NT_SUCCESS(status = StripComment(commentStripped)) && commentStripped)
    {
        status = StripWs();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return status;
}

const WCHAR* KXmlParser::CDATA_BEGIN_TAG = L"<![CDATA[";
const WCHAR* KXmlParser::CDATA_END_TAG = L"]]>";

NTSTATUS 
KXmlParser::RecognizeCDATA(__out BOOLEAN& Result)
{
    Result = FALSE;

    if (_CurrentPos + 8 >= _LastValid)
    {
        return STATUS_SUCCESS;
    }

    if (_CurrentPos[0] == L'<' &&
        _CurrentPos[1] == L'!' &&
        _CurrentPos[2] == L'[' &&
        _CurrentPos[3] == L'C' &&
        _CurrentPos[4] == L'D' &&
        _CurrentPos[5] == L'A' &&
        _CurrentPos[6] == L'T' &&
        _CurrentPos[7] == L'A' &&
        _CurrentPos[8] == L'[')
    {
        //now we are in valid CDATA section skip <!CDATA[
        if (_CurrentPos + 9 >= _LastValid)
        {
            return K_STATUS_XML_END_OF_INPUT;
        }

        _CurrentPos += 9;
        _CharPos += 9;

        #pragma warning(disable:4127)   // C4127: conditional expression is constant
        while (TRUE)
        {
            if (_CurrentPos+3 >= _LastValid)
            {
                return K_STATUS_XML_END_OF_INPUT;
            }
            if (( _CurrentPos[2] == L']')||( _CurrentPos[2] == L'>'))
            {
                if ( (_CurrentPos[0] == L']') &&
                     (_CurrentPos[1] == L']') &&
                     (_CurrentPos[2] == L'>'))
                {
                    _CurrentPos += 3;
                    _CharPos += 3;
                    Result = TRUE;
                    return STATUS_SUCCESS;
                }
                else
                {
                    _CurrentPos += 1;
                    _CharPos += 1;
                }
            }
            else
            {
                _CurrentPos += 3;
                _CharPos += 3;
            }
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS 
KXmlParser::RecognizeContent()
{
    NTSTATUS    status = STATUS_SUCCESS;
    WCHAR       c;
    ULONGLONG   len = 0;
    WCHAR*      beginPos = (WCHAR*)_CurrentPos;
    BOOLEAN     preserveWhitespace = _Hook->ElementWhitespacePreserved();

    #pragma warning(disable:4127)   // C4127: conditional expression is constant
    while(TRUE)
    {
        status = Peek(c);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (c == L'<')
        {
             //recognize content from pBeginPos to m_pCurrentPos
            len = _CurrentPos - beginPos;
            if (len > 0)
            {
                if (!preserveWhitespace)
                {
                    // Trim any whitespace from front and end of the value the the hook
                    // says the current element does NOT preseve it
                    while ((len > 0) && (IsSpace(*_CurrentPos)))
                    {
                        // Trim trailing whitespace
                        _CurrentPos--;
                        len--;
                    }

                    while ((len > 0) && (IsSpace(*beginPos)))
                    {
                        // Trim leading whitespace
                        beginPos++;
                        len--;
                    }

                    if (len == 0)
                    {
                        KAssert(beginPos == _CurrentPos);
                        continue;
                    }
                }

                if (len > 0)
                {
                    KFatal(len <= MAXULONG);
                    status = _Hook->AddContent(1, XSDTYPE_string, beginPos, (ULONG)len);
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }
                }
            }
            beginPos = (WCHAR*)_CurrentPos;

            BOOLEAN         haveCDATA;
            if (NT_SUCCESS(status = RecognizeCDATA(haveCDATA)) && haveCDATA)
            {
                //
                // Adding CDATA as part of the content
                //
                len = _CurrentPos - beginPos - CDATA_START_LENGTH - CDATA_END_LENGTH;

                KFatal(len <= MAXULONG);
                status = _Hook->AddContent(1, XSDTYPE_CDATA, beginPos + CDATA_START_LENGTH, (ULONG)len);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                beginPos = (WCHAR*)_CurrentPos;
                continue;
            }
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            BOOLEAN         strippedComment;
            if (NT_SUCCESS(status = StripComment(strippedComment)) && strippedComment)
            {
                //
                // Add comment as part of the content
                //
                beginPos = (WCHAR*)_CurrentPos;
                continue;
            }
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            WCHAR       c1;
            status = Peek(c);
            if (!NT_SUCCESS(status))
            {
                return status;
            }
            status = Peek(c1, 1);
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            if (c == L'<' && LegalIdent_FirstChar(c1))
            {
                //valid start of the child element break
               break;
            }
            else if ((c == L'<' && c1 == L'/'))
            {
                //valid end of the current element break
                break;
            }

            //
            //if < does not imply cdata, comment, element or end of element then its
            //syntax error

            return K_STATUS_XML_SYNTAX_ERROR;
        }

        status = Next();
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}


