
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kuri.cpp

    Description:
      Kernel Tempate Library (KTL): KUri

      URI wrapper and parser

    History:
      raymcc          7-Mar-2012         Initial version.

    Notes:

    The parser is a loose superset of RFC 3986.  The superset is to allow
    for custom macro expansion, Microsoft UNC values, XML URI Reference,
    and other slightly non-URI constructs.  The parser currently does not accept
    some types of legal URIs, such as those empty internal segments
    in the path portion.  The parser does not expand or convert escape characters,
    such as %20 vs. a space.  The string should be preprocessed/postprocessed
    if that is a concern.

--*/

#include <ktl.h>

// The first part of this file consists of the URI parser and its helpers.
// KUriView and derived classes are later in the file.

// IsAlpha
//
// Parser helper
//
inline BOOLEAN
IsAlpha(
    __in WCHAR c
    )
{
    if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') )
    {
        return TRUE;
    }
    return FALSE;
}

// IsDigit
//
// Parser helper
//
inline BOOLEAN
IsDigit(
    __in WCHAR c
    )
{
    if (c >= L'0' && c <= L'9')
    {
        return TRUE;
    }
    return FALSE;
}

// IsHexDigit
//
// Parser helper
//
inline BOOLEAN
IsHexDigit(
    __in WCHAR c
    )
{
    if ((c >= L'0' && c <= L'9') ||
        (c >= L'a' && c <= L'f') ||
        (c >= L'A' && c <= L'F'))
    {
        return TRUE;
    }

    return FALSE;
}


// IsInSet
//
// Parser helper
//
inline BOOLEAN
IsInSet(
    __in WCHAR c,
    __in LPCWSTR StrSet
    )
{
    WCHAR const * p = StrSet;
    while (*p)
    {
        if (*p++ == c)
        {
            return TRUE;
        }
    }
    return FALSE;
}

// IsSubdelim
//
// Parser helper
//
inline BOOLEAN
IsSubdelim(
    __in WCHAR c
    )
{
    if (IsInSet(c, L"!$&'()*+,;="))
    {
        return TRUE;
    }
    return FALSE;
}


// IsQueryStringDelim
//
// Parser helper
//
inline BOOLEAN
IsQueryStringDelim(
    __in WCHAR c
    )
{
    if (c == L'&' || c == L'|' || c == L';' || c == L'+')
    {
        return TRUE;
    }
    return FALSE;
}

// IsUnreserved
//
// Parser helper
//
inline BOOLEAN
IsUnreserved(
    __in WCHAR c
    )
{
    if (IsAlpha(c) || IsDigit(c) || IsInSet(c, L"-._~"))
    {
        return TRUE;
    }
    return FALSE;
}

// IsNonFirstSchemeChar
//
// Parser helper
//
inline BOOLEAN
IsNonFirstSchemeChar(WCHAR c)
{
    if (IsAlpha(c) || IsDigit(c) || IsInSet(c, L"+-."))
    {
        return TRUE;
    }
    return FALSE;
}

// IsInitialSchemeChar
//
// Parser helper
//
inline BOOLEAN
IsInitialSchemeChar(
    __in WCHAR c
    )
{
    return IsAlpha(c);
}

// IsPathChar
//
// Parser helper
//
inline BOOLEAN
IsPathChar(WCHAR c)
{
    if (IsUnreserved(c) || IsSubdelim(c) || IsInSet(c, L":@%{}*<>^"))
    {
        return TRUE;
    }
    return FALSE;
}

//  QueryStrParamCallback
//
//  This delegate is implemented by anybody wanting to parse the query string.  Returning FALSE from
//  the delegate will halt the parser with an error.  This allow the callback to veto the parsing
//  if an illegal construct (such as a repeated parameter) is discovered in one of the values, even though the overall string may be
//  syntactically correct.
//
typedef KDelegate <BOOLEAN(const KStringView& Param, const KStringView& Val, ULONG ParamNum)> QueryStrParamCallback;

// UriParserCallback
//
// This delegate is implemented by anybody using the parser.  If FALSE is returned, the parser will
// halt with an error. This allows the callback to veto a parser decision, which can occur with illegal
// authority constructs or query strings.
//
typedef KDelegate <BOOLEAN(ULONG UriElement, const KStringView& Val, ULONG SegIndex)> UriParserCallback;


// _Base_Authority_Parser
//
// Breaks apart the authority field in order to separate out the port.  Since the colon
// can occur within an IPv6 address, there is special processing so that the Authority
// and Port are separated.
//
// Currently, parsing out a 'user' field is not supported.  All information is assumed
// to be host-related.
//
// Currently, the parser does not insist on legal IPv4 or IPv6 addresses, but does
// partially enforce IpV6 square bracket usage and that the digits are hex digits or colons.
//
// Parameters:
//      RawAuthority            The raw authority string as determined by the base parser.
//
//      Host                    Receives the host portion of the authority, without the port.
//                              IPv6 addresses are stripped of the surrounding square brackets.
//
//      Port                    Receives the port, if any.
//
// Return value:
//  TRUE  if a legal authority string.
//  FALSE if illegal syntax is encountered.  A port can be omitted legally, so FALSE
//  indicates an actual error in the URI syntax.
//
static BOOLEAN
_Base_Authority_Parser(
    __in  const KStringView& RawAuthority,
    __out KStringView& Host,
    __out ULONG& PortVal
    )
{
    if (RawAuthority.IsEmpty())
    {
        return FALSE;
    }

    enum { eHost, eBracketed, ePossiblePort, ePort } eState;
    eState = eHost;

    KStringView Tracer = RawAuthority;
    Host.Zero();
    Host.SetAddress(PWSTR(Tracer));
    KStringView Port;

    while (Tracer.Length())
    {
        switch (eState)
        {
            case eHost:
                if (Tracer.PeekFirst() == L'[')
                {
                    eState = eBracketed;
                    Tracer.ConsumeChar();
                    Host.SetAddress(PWSTR(Tracer)); // Don't include bracket in address
                    continue;
                }
                if (Tracer.PeekFirst() == L':')
                {
                    eState = ePort;
                    Tracer.ConsumeChar();
                    Port.SetAddress(PWSTR(Tracer));
                    continue;
                }
                Host.IncLen();
                Tracer.ConsumeChar();
                continue;


            case eBracketed:
                if (IsHexDigit(Tracer.PeekFirst())  || Tracer.PeekFirst() == L':')
                {
                    Host.IncLen();
                    Tracer.ConsumeChar();
                    continue;
                }
                if (Tracer.PeekFirst() == L']')
                {
                    eState = ePossiblePort;
                    Tracer.ConsumeChar();
                    continue;
                }
                return FALSE;

            case ePossiblePort:
                if (Tracer.PeekFirst() == L':')
                {
                    Tracer.ConsumeChar();
                    Port.SetAddress(PWSTR(Tracer));
                    eState = ePort;
                    continue;
                }
                return FALSE;

            case ePort:
                if (IsDigit(Tracer.PeekFirst()))
                {
                    Port.IncLen();
                    Tracer.ConsumeChar();
                    continue;
                }
                return FALSE;

            default:
                return FALSE;
        }
    }

    if (eState == ePort && Port.Length() == 0)
    {
        return FALSE;
    }

    if (!Port.ToULONG(PortVal))
    {
        return FALSE;
    }

    return TRUE;
}



// _Base_QueryString_Parser
//
// This parses the query string if it conforms to one of the de facto standards
// for layout.
//
//
// Returns TRUE if the query string is standardized, FALSE if not.  The intermediate
// callbacks will occur in any case even if FALSE is returned, so the caller must
// accumulate callback data and also check the return value at the end.
//
static BOOLEAN
_Base_QueryString_Parser(
    __in const KStringView& Src,
    __in QueryStrParamCallback CBack
    )
{
    // For param name, look for alpha numeric _ up to the = symbol
    // For param value, look for anything not a delimiter |+;&
    //
    KStringView Tracer = Src;

    enum { eWaiting, eParam, eValue } eState;
    eState = eWaiting;
    KStringView ParamName;
    KStringView ParamValue;
    ULONG ParamNum = 0;

    while (Tracer.Length())
    {
        switch (eState)
        {
            case eWaiting:
                if (IsUnreserved(Tracer.PeekFirst()))
                {
                    ParamName.Zero();
                    ParamName.SetAddress(PWSTR(Tracer));
                    ParamName.IncLen();
                    Tracer.ConsumeChar();
                    eState = eParam;
                    continue;
                }
                return FALSE;

            case eParam:
                if (IsUnreserved(Tracer.PeekFirst()))
                {
                    ParamName.IncLen();
                    Tracer.ConsumeChar();
                    continue;
                }
                if (Tracer.PeekFirst() == L'=')
                {
                    Tracer.ConsumeChar();
                    eState = eValue;
                    ParamValue.Zero();
                    ParamValue.SetAddress(PWSTR(Tracer));
                    continue;
                }
                return FALSE;

            case eValue:
                if (IsQueryStringDelim(Tracer.PeekFirst()))
                {
                    CBack(ParamName, ParamValue, ParamNum++);
                    ParamName.Zero();
                    ParamValue.Zero();
                    Tracer.ConsumeChar();
                    eState = eWaiting;
                    continue;
                }
                else
                {
                    Tracer.ConsumeChar();
                    ParamValue.IncLen();
                    continue;
                }

            default:
                return FALSE;
        }
    }

    if (eState == eValue)
    {
        CBack(ParamName, ParamValue, ParamNum++);
        return TRUE;
    }

    return FALSE;
}



// _Base_UriParser
//
// This is a basic non-allocating parser which allows a loose superset of RFC 3986, i.e., it supports relative
// paths, and does not strictly validate that IP addresses are legal, witin range, or that port
// values are legimitate, etc.  It sacrifices strictness for speed.
//
// The parser invokes the callback as various significant portions of the URI are detected and isolated.
// Additional validation can occur in those callbacks.  The query string is isolated as a complete unit.
// A secondary _Base_QueryString parser for just that portion can be invoked as required to isolate the
// individual parameters of conformant query strings.
//
// The callback is invoked as the parser moves along.  Note that there may be an error in the URI
// later on after the callback has been invoked several times, so the caller should not make a final
// commit decision on using the URI elements until STATUS_SUCCESS is returned.
//
// Parameters:
//      Src         The candidate URI to be parsed.
//      CBack       The callback which is invoked as the parse progresses
//      ErrorAt     If an error occurs, this is the character position where the parser was confused.
//
// Return values:
//      STATUS_SUCCESS
//      STATUS_OBJECT_PATH_SYNTAX_BAD           The URI is not legal.
//      STATUS_SOURCE_ELEMENT_EMPTY             The URI
//      STATUS_INTERNAL_ERROR                   Indicates a bug in the state machine.
//

#define URI_PARSE_SCHEME           1
#define URI_PARSE_AUTHORITY        2
#define URI_PARSE_SEGMENT          3
#define URI_PARSE_QUERYSTRING      4
#define URI_PARSE_FRAGMENT         5
#define URI_PARSE_FULL_PATH        6

static NTSTATUS
_Base_UriParser(
    __in  const KStringView& Src,
    __in  UriParserCallback CBack,
    __out ULONG&       ErrorAt
    )
{
    ErrorAt = 0;

    if (Src.IsEmpty())
    {
        return STATUS_SOURCE_ELEMENT_EMPTY;
    }

    enum { eNone = 0, eAmbiguous = 1, ePostScheme = 2, eAuthority = 3, ePath = 4, eQueryString = 5, eFragment = 6, eError = 7 } eState;
    eState = eNone;

    KStringView Tracer = Src;
    KStringView Token;
    KStringView FullPath;

    ULONG SegIndex = 0;

    // Basic DFA outline for parsing.
    //
    while (Tracer.Length())
    {
        switch (eState)
        {
            case eNone:
                // Check for UNC
                //
                if (Tracer.PeekFirst() == L'/' && Tracer.PeekN(1) == L'/')
                {
                    Tracer.ConsumeChar();
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWCHAR(Tracer));
                    eState = eAuthority;
                    continue;
                }
                // Otherwise an initial slash is a rel-path with no scheme
                //
                if (Tracer.PeekFirst() == L'/')
                {
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWCHAR(Tracer));
                    eState = ePath;
                    continue;
                }
                // Here, we don't know yet what we are looking at.
                //
                if (IsInitialSchemeChar(Tracer.PeekFirst()))
                {
                    Token.SetAddress(PWCHAR(Tracer));
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    eState = eAmbiguous;    // May be a path or may be a scheme; don't know yet.
                    continue;
                }
                // If here, we are not in a scheme, but still may be the first
                // path element.
                //
                if (IsPathChar(Tracer.PeekFirst()))
                {
                    Token.SetAddress(PWCHAR(Tracer));
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    eState = ePath;
                    continue;
                }

                ErrorAt = ULONG(PWSTR(Tracer) - PWSTR(Src));
                return STATUS_OBJECT_PATH_SYNTAX_BAD;

            // This occurs because we can't tell an initial path from a scheme
            // until we either hit a non-scheme character or a colon.
            //
            case eAmbiguous:
                if (IsNonFirstSchemeChar(Tracer.PeekFirst()))
                {
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    continue;
                }
                if (Tracer.PeekFirst() == L':')
                {
                    // Everything up to now was a scheme, but we just
                    // found that out.
                    //
                    if (CBack)
                    {
                        if (!CBack(URI_PARSE_SCHEME, Token, 0))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    eState = ePostScheme;
                    continue;
                }
                if (IsPathChar(Tracer.PeekFirst()))
                {
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    eState = ePath;
                    continue;
                }
                if (IsInSet(Tracer.PeekFirst(), L"/#?"))
                {
                    // We have been looking at a segment all this time.
                    // Switch states, no consumption of the char.
                    if (CBack)
                    {
                        if (SegIndex == 0)
                        {
                            FullPath.SetAddress(PWSTR(Token));
                        }
                        FullPath.IncLen(Token.Length()+1);

                        if (!CBack(URI_PARSE_SEGMENT, Token, SegIndex++))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    eState = ePath;
                    continue;
                }
                // Otherwise, an error
                //
                ErrorAt = ULONG(PWSTR(Tracer) - PWSTR(Src));
                return STATUS_OBJECT_PATH_SYNTAX_BAD;

            // Here after we know we have recognized a scheme.
            // Then, we either are looking at an authority (two slashes)
            // or entry into an authority-less URI (single slash) which leads to ePath.
            //
            case ePostScheme:
                if (Tracer.PeekFirst() == L'/' && Tracer.PeekN(1) == L'/')
                {
                    eState = eAuthority;
                    // Move past the two slashes
                    Tracer.ConsumeChar();
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    continue;
                }
                if (Tracer.PeekFirst() == L'?')
                {
                    Tracer.ConsumeChar();
                    eState = eQueryString;
                    continue;
                }
                if (Tracer.PeekFirst() == L'#')
                {
                    Tracer.ConsumeChar();
                    eState = eFragment;
                    continue;
                }
                if (Tracer.PeekFirst() == L'/')
                {
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    eState = ePath;
                    continue;
                }
                if (IsPathChar(Tracer.PeekFirst()))
                {
                    Token.SetAddress(PWSTR(Tracer));
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    eState = ePath;
                    continue;
                }

                ErrorAt = ULONG(PWSTR(Tracer) - PWSTR(Src));
                return STATUS_OBJECT_PATH_SYNTAX_BAD;

            case eAuthority:
                if (Tracer.PeekFirst() == L'/')
                {
                    Tracer.ConsumeChar();
                    if (CBack)
                    {
                        if (!CBack(URI_PARSE_AUTHORITY, Token, 0))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Token.SetAddress(PWSTR(Tracer));
                    eState = ePath;
                    continue;
                }
                if (Tracer.PeekFirst() == L'?')
                {
                    if (CBack)
                    {
                        if (!CBack(URI_PARSE_AUTHORITY, Token, 0))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    eState = eQueryString;
                    continue;
                }
                if (Tracer.PeekFirst() == L'#')
                {
                    if (CBack)
                    {
                        if (!CBack(URI_PARSE_AUTHORITY, Token, 0))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    eState = eFragment;
                    continue;
                }

                // Take everything up to / as authority without additional parsing for now.
                //
                Token.IncLen();
                Tracer.ConsumeChar();
                continue;

            case ePath:
                if (Tracer.PeekFirst() == '/')
                {
                    if (CBack)
                    {
                        if (SegIndex == 0)
                        {
                            FullPath.SetAddress(PWSTR(Token));
                        }
                        FullPath.IncLen(Token.Length()+1);
                        if (!CBack(URI_PARSE_SEGMENT, Token, SegIndex++))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    continue;
                }
                if (IsPathChar(Tracer.PeekFirst()))
                {
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    continue;
                }
                if (Tracer.PeekFirst() == L'?')
                {
                    if (CBack)
                    {
                        if (SegIndex == 0)
                        {
                            FullPath.SetAddress(PWSTR(Token));
                        }
                        FullPath.IncLen(Token.Length());
                        if (!CBack(URI_PARSE_SEGMENT, Token, SegIndex++))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    eState = eQueryString;
                    continue;
                }
                if (Tracer.PeekFirst() == L'#')
                {
                    if (CBack)
                    {
                        if (SegIndex == 0)
                        {
                            FullPath.SetAddress(PWSTR(Token));
                        }
                        FullPath.IncLen(Token.Length());
                        if (!CBack(URI_PARSE_SEGMENT, Token, SegIndex++))
                        {
                            eState = eError;
                            continue;
                        }
                    }
                    Tracer.ConsumeChar();
                    Token.SetAddress(PWSTR(Tracer));
                    eState = eFragment;
                    continue;
                }

                ErrorAt = ULONG(PWSTR(Tracer) - PWSTR(Src));
                return STATUS_OBJECT_PATH_SYNTAX_BAD;

            case eQueryString:
                {
                    ULONG Pos;
                    if (Tracer.Find(L'#', Pos))
                    {
                        Token = Tracer.LeftString(Pos);
                        if (CBack)
                        {
                            if (!CBack(URI_PARSE_QUERYSTRING, Token, 0))
                            {
                                eState = eError;
                                continue;
                            }
                        }
                        eState = eFragment;
                        Tracer.ConsumeChars(Pos+1);
                        continue;
                    }
                    else
                    {
                        Token = Tracer.Remainder(0);
                        Tracer.SetLength(0);
                        continue;
                    }
                }
                break;

            case eFragment:
                // Accumulate all content until end of string.
                Token = Tracer;
                Tracer.Zero();
                continue;

            case eError:
            default:
                ErrorAt = ULONG(PWSTR(Tracer) - PWSTR(Src));
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
    }

    // Legal final states when no more input is left.
    //
    switch (eState)
    {
        case ePostScheme:
            if (CBack)
            {
                if (!CBack(URI_PARSE_SCHEME, Token, 0))
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
            break;

        case eAmbiguous:
        case ePath:
            if (CBack)
            {
                BOOLEAN invokeCallback = TRUE;

                if (SegIndex == 0)
                {
                   FullPath.SetAddress(PWSTR(Token));

                   //
                   // There is a special case where the callback should
                   // not be invoked. Consider the case where the URI
                   // has an empty path and the URI ends with a /.
                   // Examples include "http://fred/" or "//fred/". In
                   // these cases the path should be empty and have no
                   // segments.
                   //
                   invokeCallback = (Token.Length() != 0);
                }
                FullPath.IncLen(Token.Length()); // Used to be Token.Length()+1 ; check this

                if (invokeCallback)
                {
                    if (!CBack(URI_PARSE_SEGMENT, Token, SegIndex++))
                    {
                        return STATUS_OBJECT_PATH_SYNTAX_BAD;
                    }
                }
            }
            break;

        case eAuthority:
            if (CBack)
            {
                if (!CBack(URI_PARSE_AUTHORITY, Token, 0))
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
            break;

        case eFragment:
            if (CBack)
            {
                if (!CBack(URI_PARSE_FRAGMENT, Token, 0))
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
            break;

        case eQueryString:
            if (CBack)
            {
                if (!CBack(URI_PARSE_QUERYSTRING, Token, 0))
                {
                    return STATUS_OBJECT_PATH_SYNTAX_BAD;
                }
            }
            break;

        default:
            ErrorAt = ULONG(PWSTR(Tracer) - PWSTR(Src));
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    if (FullPath.Length())
    {
        if (!CBack(URI_PARSE_FULL_PATH, FullPath, 0))
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
    }

    return STATUS_SUCCESS;
}

//*****************************************************************************
//
// KUriView and derived classes


//  KUriView::Initialize
//
//  Implements the constructor/parser sequence.
//
VOID
KUriView::Initialize(
    __in const KStringView& Src
    )
{
    _PortVal = 0;
    _Raw = Src;
    _ErrorPos = 0;

    if (Src.IsEmpty())
    {
        _IsValid = FALSE;
        return;
    }

    // Now implement the parser sequence.
    //
    UriParserCallback Cb;
    Cb.Bind(this, &KUriView::ParserCallback);

    NTSTATUS Result = _Base_UriParser(KStringView(_Raw), Cb, _ErrorPos);
    if (!NT_SUCCESS(Result))
    {
        _IsValid = FALSE;
    }
    else
    {
        _IsValid = TRUE;
    }
}

//
// KUriView::Clear
//
// Clears all fields except _Raw
//
VOID
KUriView::Clear()
{
    _ErrorPos = 0;
    _IsValid = FALSE;
    _StandardQueryString = FALSE;
    _Scheme.Clear();
    _Authority.Clear();
    _Host.Clear();
    _PortVal = 0;
    _Path.Clear();
    _QueryString.Clear();
    _Fragment.Clear();
}


//
//  KUriView::Get
//
KStringView&
KUriView::Get(
    __in UriFieldType Field
)
{
    switch (Field)
    {
    case eScheme:
        return _Scheme;
    case eHost:
        return _Host;
    case eAuthority:
        return _Authority;
    case ePath:
        return _Path;
    case eQueryString:
        return _QueryString;
    case eFragment:
        return _Fragment;
    }
    return _Raw;
}


//
//  KUriView::Get const
//
const KStringView&
KUriView::Get(
    __in UriFieldType Field
    ) const
{
    switch (Field)
    {
        case eScheme:
            return _Scheme;
        case eHost:
            return _Host;
        case eAuthority:
            return _Authority;
        case ePath:
            return _Path;
        case eQueryString:
            return _QueryString;
        case eFragment:
            return _Fragment;
    }
    return _Raw;
}


//
//  KUriView::Has
//
//  Tests for presence of the various URI elements.
//
BOOLEAN
KUriView::Has(
   __in ULONG Fields
   ) const
{
    if (Fields & eScheme)
    {
        if (_Scheme.IsEmpty())
        {
            return FALSE;
        }
    }

    if (Fields & eHost)
    {
        if (_Host.IsEmpty())
        {
            return FALSE;
        }
    }

    if (Fields & eAuthority)
    {
        if (_Authority.IsEmpty())
        {
            return FALSE;
        }
    }

    if (Fields & ePath)
    {
        if (_Path.IsEmpty())
        {
            return FALSE;
        }
    }

    if (Fields & eQueryString)
    {
        if (_QueryString.IsEmpty())
        {
            return FALSE;
        }
    }

    if (Fields & eFragment)
    {
        if (_Fragment.IsEmpty())
        {
            return FALSE;
        }
    }

    if (Fields & ePort)
    {
        if (_PortVal == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
KUriView::HostIsWildcard() const
{
    if (_Host.Compare(KStringView(L"*")) == 0 ||
        _Host.Compare(KStringView(L"+")) == 0
        )
    {
        return TRUE;
    }
    return FALSE;
}


//
// KUriView::Compare
//
BOOLEAN
KUriView::Compare(
   __in ULONG FieldsToCompare,
   __in KUriView const & Comparand
   ) const
{
    if (FieldsToCompare & eScheme)
    {
        if (_Scheme.Compare(Comparand._Scheme) != 0)
        {
            return FALSE;
        }
    }

    if (FieldsToCompare & eHost)
    {
        if (!HostIsWildcard())
        {
            if (_Host.CompareNoCase(Comparand._Host) != 0)
            {
                return FALSE;
            }
        }
    }

    if (FieldsToCompare & eLiteralHost)
    {
        if (_Host.CompareNoCase(Comparand._Host) != 0)
        {
            return FALSE;
        }
    }

    if (FieldsToCompare & eAuthority)
    {
        if (_Authority.Compare(Comparand._Authority) != 0)
        {
            return FALSE;
        }
    }


    if (FieldsToCompare & ePath)
    {
        if (_Path.Compare(Comparand._Path) != 0)
        {
            return FALSE;
        }
    }

    if (FieldsToCompare & eQueryString)
    {
        if (_QueryString.Compare(Comparand._QueryString) != 0)
        {
            return FALSE;
        }
    }

    if (FieldsToCompare & eFragment)
    {
        if (_Fragment.Compare(Comparand._Fragment) != 0)
        {
            return FALSE;
        }
    }

    if (FieldsToCompare & ePort)
    {
        if (_PortVal != Comparand._PortVal)
        {
            return FALSE;
        }
    }

    if (FieldsToCompare & eRaw)
    {
        if (_Raw.Compare(Comparand._Raw) != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}



//
//  KUriView::QueryStrCallback
//
BOOLEAN
KUriView::QueryStrCallback(
    __in const KStringView& Param,
    __in const KStringView& Val,
    __in ULONG ParamNum
    )
{
    return QueryStringParamRecognized(ParamNum, Param, Val);
}


//
//  KUriView::ParserCallback
//
//
BOOLEAN
KUriView::ParserCallback(
    ULONG UriElement,
    const KStringView& Val,
    ULONG SegIndex
    )
{
    switch (UriElement)
    {
        case URI_PARSE_SEGMENT:
            return SegmentRecognized(SegIndex, Val);   // Virtual call

        case URI_PARSE_SCHEME:
            _Scheme = Val;
            break;

        case URI_PARSE_AUTHORITY:
            _Authority = Val;
            break;

        case URI_PARSE_QUERYSTRING:
            _QueryString = Val;
            break;

        case URI_PARSE_FRAGMENT:
            _Fragment = Val;
            break;

        case URI_PARSE_FULL_PATH:
            _Path = Val;
            break;
    }

    if (UriElement == URI_PARSE_AUTHORITY && Val.Length() > 0)
    {
        return _Base_Authority_Parser(Val, _Host, _PortVal);
    }

    if (UriElement == URI_PARSE_QUERYSTRING)
    {
        if (Val.Length() > 0)
        {
            QueryStrParamCallback Cb;
            Cb.Bind(this, &KUriView::QueryStrCallback);

            BOOLEAN Res = _Base_QueryString_Parser(Val, Cb);
            if (!Res)
            {
                _StandardQueryString = FALSE;
            }
            else
            {
                _StandardQueryString = TRUE;
            }
        }
        else
        {
            _StandardQueryString = FALSE;
        }
    }
    return TRUE;
}



BOOLEAN
KDynUri::IsPrefixFor(
    __in KDynUri& Candidate
    )
{
    if (_Scheme.Compare(Candidate._Scheme) != 0)
    {
        return FALSE;
    }

    if (_Authority.Compare(Candidate._Authority) != 0)
    {
        return FALSE;
    }

    for (ULONG i = 0; i < _Segments.Count(); i++)
    {
        if (i >= Candidate._Segments.Count())
        {
            return FALSE;
        }

        if (_Segments[i]->Compare(*Candidate._Segments[i]) != 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}


NTSTATUS
KDynUri::GetSuffix(
    __in  KDynUri& Candidate,
    __out KDynUri& Suffix
    )
{
    Suffix.Clear();

    if (!Candidate.IsValid())
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (! Compare(eScheme | eHost, Candidate))
    {
        return STATUS_UNSUCCESSFUL;
    }

    ULONG i = 0;

    for (i = 0; i < _Segments.Count(); i++)
    {
        if (i >= Candidate._Segments.Count())
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (_Segments[i]->Compare(*Candidate._Segments[i]) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }


    // For the remainder of the URI, build a string.

    // If here, there has been a full match so far.  All we have
    // to do is compute Diff.

    KDynString Tmp(_Allocator);
    BOOLEAN First = TRUE;

    for ( ; i < Candidate._Segments.Count(); i++)
    {
        if (!First)
        {
            if (!Tmp.Concat(KStringView(L"/")))
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        if (!Tmp.Concat(*Candidate._Segments[i]))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        First = FALSE;
    }

    if (Candidate._QueryString.Length())
    {
        if (!Tmp.Concat(KStringView(L"?")))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        if (!Tmp.Concat(Candidate._QueryString))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (Candidate._Fragment.Length())
    {
        if (!Tmp.Concat(KStringView(L"#")))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        Tmp.Concat(Candidate._Fragment);
    }

    if (Tmp.Length())
    {
        return Suffix.Set(Tmp);
    }
    else
    {
        // Return an empty URI
        //
        return STATUS_SUCCESS;
    }

}





//
//  KDynUri::QueryStringParamRecognized
//
BOOLEAN
KDynUri::QueryStringParamRecognized(
    __in ULONG ParamIndex,
    __in const KStringView& ParamName,
    __in const KStringView& ParamVal
    )
{
    UNREFERENCED_PARAMETER(ParamIndex);

    QueryStrParam::SPtr NewParam = _new(KTL_TAG_URI, _Allocator) QueryStrParam(_Allocator);
    if (!NewParam)
    {
        return FALSE;
    }

	HRESULT hr;
	ULONG result;
	hr = ULongAdd(ParamName.Length(), 1, &result);
	KInvariant(SUCCEEDED(hr));
    if (!NewParam->_Param.Resize(result))
    {
        return FALSE;
    }

	hr = ULongAdd(ParamVal.Length(), 1, &result);
	KInvariant(SUCCEEDED(hr));
    if (!NewParam->_Value.Resize(result))
    {
        return FALSE;
    }

    NewParam->_Param.CopyFrom(ParamName);
    NewParam->_Value.CopyFrom(ParamVal);

    NTSTATUS Res = _QueryStrParams.Append(NewParam);
    if (!NT_SUCCESS(Res))
    {
        return FALSE;
    }

    return TRUE;
}


KUri::KUri(
    __in KAllocator& Alloc
    )
    : KDynUri(Alloc)
{
}

KUri::KUri(
    __in const KStringView& Src,
    __in KAllocator& Alloc
    )
    : KDynUri(Src, Alloc)
{
}

KUri::KUri(
    __in KUriView& Src,
    __in KAllocator& Alloc
    )
    : KDynUri(Src, Alloc)
{
}

KUri::~KUri()
{
}

NTSTATUS
KUri::Create(
    __in  const KStringView& RawString,
    __in  KAllocator& Alloc,
    __out KUri::SPtr& Uri
    )
{
    Uri = _new(KTL_TAG_URI, Alloc) KUri(RawString, Alloc);
    if (!Uri)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
KUri::Create(
    __in  const KStringView& RawString,
    __in  KAllocator& Alloc,
    __out KUri::CSPtr& Uri
    )
{
    Uri = _new(KTL_TAG_URI, Alloc) KUri(RawString, Alloc);
    if (!Uri)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
KUri::Create(
    __in  KUriView& ExistingUri,
    __in  KAllocator& Alloc,
    __out KUri::SPtr& Uri
    )
{
    Uri = _new(KTL_TAG_URI, Alloc) KUri(ExistingUri, Alloc);
    if (!Uri)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
KUri::Create(
    __in  KUriView& ExistingUri,
    __in  KAllocator& Alloc,
    __out KUri::CSPtr& Uri
    )
{
    Uri = _new(KTL_TAG_URI, Alloc) KUri(ExistingUri, Alloc);
    if (!Uri)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
KUri::Create(
    __in  KAllocator& Alloc,
    __out KUri::SPtr& Uri
    )
{
    Uri = _new(KTL_TAG_URI, Alloc) KUri(Alloc);
    if (!Uri)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

// Clone
//
// Convenience function for simple clone of entire URI.
//
NTSTATUS
KUri::Clone(
    __out KUri::SPtr& NewCopy
    )
{
    return KUri::Create(*this, GetThisAllocator(), NewCopy);
}


NTSTATUS
KUri::Create(
    __in KUriView& PrefixUri,
    __in KUriView& RelativeUri,
    __in KAllocator& Alloc,
    __out KUri::SPtr& NewCompositeUri
    )
{
    if (!PrefixUri.IsValid() || !RelativeUri.IsValid())
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!RelativeUri.IsRelative())
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (PrefixUri.Has(KUriView::eQueryString | KUriView::eFragment))
    {
        return STATUS_INVALID_PARAMETER;
    }

    KDynString Buf(Alloc);

    if (!Buf.CopyFrom(KStringView((PWSTR)PrefixUri)))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!Buf.Concat(KStringView(L"/")))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!Buf.Concat(KStringView((PWSTR)RelativeUri)))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Res = Create(Buf, Alloc, NewCompositeUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return STATUS_SUCCESS;
}

