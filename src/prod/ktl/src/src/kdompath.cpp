

/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kdompath.cpp

    Description:
      Kernel Tempate Library (KTL): KDomPath

      Path helper/parser for KDom-related objects.

    History:
      raymcc          26-Jul-2012         Initial version.

--*/


#include <ktl.h>

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

//
//  KDomPath::Parse
//
//  Parses a DOM path
//
NTSTATUS
KDomPath::Parse(
    __in  KAllocator& Alloc,
    __out KArray<PToken>& Output
    ) const
{
    NTSTATUS Res;
    if (!_RawPath)
    {
        return STATUS_NO_DATA_DETECTED;
    }

    // Clear the output array.
    //
    Output.Clear();

    //
    enum { eStart = 0, eStartEl = 1, eContinueEl = 2, eStartArray = 3, eEndArray = 4, eStartAttr = 5, eContinueAttr = 6 } eState;
    eState = eStart;

    KStringView Tracer = _RawPath;
    Tracer.MeasureThis();

    KStringView Token;
    KStringView FullPath;

    PToken CurrentPToken;

    // Basic DFA outline for parsing.
    //
    while (Tracer.Length())
    {
        switch (eState)
        {
            case eStart:
            {
                CurrentPToken.Clear();
                Token.SetAddress(LPWSTR(Tracer));
                WCHAR c = Tracer.PeekFirst();

                if (c == '@')
                {
                    eState = eStartAttr;
                    Token.Clear();
					Tracer.ConsumeChar();
                    Token.SetAddress(LPWSTR(Tracer));
                    continue;
                }

                eState = eStartEl;
                continue;
            }

            case eStartEl:
            {
                WCHAR c = Tracer.PeekFirst();
                if (IsAlpha(c) || c == '_')
                {
                    Token.IncLen();;
                    Tracer.ConsumeChar();
                    eState = eContinueEl;
                    continue;
                }
                return STATUS_OBJECT_PATH_INVALID;
            }


            case eContinueEl:
            {
                WCHAR c = Tracer.PeekFirst();

                if (IsAlpha(c) || IsDigit(c) || c == L'_' || c == L'.')
                {
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    eState = eContinueEl;
                    continue;
                }

                if (c == L':' && !CurrentPToken._NsPrefix)
                {
                   // We have hit a namespace prefix.
                   //
                   CurrentPToken._NsPrefix = KString::Create(Token, Alloc);
                   if (!CurrentPToken._NsPrefix)
                   {
                       return STATUS_INSUFFICIENT_RESOURCES;
                   }
                   Token.Clear();
                   Tracer.ConsumeChar();
                   Token.SetAddress(LPWSTR(Tracer));
                   eState =  eStartEl;
                   continue;
                }

                if (c == L'[')
                {
                   CurrentPToken._Ident = KString::Create(Token, Alloc);
                   if (!CurrentPToken._Ident)
                   {
                       return STATUS_INSUFFICIENT_RESOURCES;
                   }
				   CurrentPToken._IdentType = KDomPath::eArrayElement;
                   Token.Clear();
                   Tracer.ConsumeChar();
				   Token.SetAddress(LPWSTR(Tracer));
                   eState = eStartArray;
				   continue;
                }

                if (c == L'/')
                {
                   // If here, we are done with the current token.

                   CurrentPToken._Ident = KString::Create(Token, Alloc);
                   if (!CurrentPToken._Ident)
                   {
                       return STATUS_INSUFFICIENT_RESOURCES;
                   }
                   CurrentPToken._IdentType = KDomPath::eElement;
                   Res = Output.Append(CurrentPToken);
                   if (!NT_SUCCESS(Res))
                   {
                       return Res;
                   }
				   Tracer.ConsumeChar();
                   eState = eStart;
                   continue;
                }

                return STATUS_OBJECT_PATH_INVALID;
            }

            case eStartArray:
            {
                WCHAR c = Tracer.PeekFirst();
                if (c == ']')
                {
                    if (Token.Length() > 0)
                    {
                        if (!Token.ToLONG(CurrentPToken._LiteralIndex))
                        {
                            return STATUS_OBJECT_PATH_INVALID;
                        }
                    }

                    Res = Output.Append(CurrentPToken);
                    if (!NT_SUCCESS(Res))
                    {
                       return Res;
                    }
                    eState = eEndArray;
					Tracer.ConsumeChar();
                    continue;
                 }

                if (IsDigit(c))
                {
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    continue;
                }

                return STATUS_OBJECT_PATH_INVALID;
            }

            case eEndArray :
            {
                WCHAR c = Tracer.PeekFirst();
                if (c == '/')
                {
                    Tracer.ConsumeChar();
                    eState = eStart;
                    continue;
                }

                return STATUS_OBJECT_PATH_INVALID;
            }


            case eStartAttr:
            {
                WCHAR c = Tracer.PeekFirst();
                if (IsAlpha(c) || c == '_')
                {
                    Token.IncLen();
                    Tracer.ConsumeChar();
                    eState = eContinueAttr;
                    continue;
                }
                return STATUS_OBJECT_PATH_INVALID;
            }

            case eContinueAttr:
            {
                 WCHAR c = Tracer.PeekFirst();

                 if (IsAlpha(c) || IsDigit(c) || c == L'_' || c == L'.')
                 {
                     Token.IncLen();
                     Tracer.ConsumeChar();
                     eState = eContinueAttr ;
                     continue;
                 }

                 if (c == L':' && !CurrentPToken._NsPrefix)
                 {
                    // We have hit a namespace prefix.
                    //
                    CurrentPToken._NsPrefix = KString::Create(Token, Alloc);
                    if (!CurrentPToken._NsPrefix)
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    Token.Clear();
                    Tracer.ConsumeChar();
                    Token.SetAddress(LPWSTR(Tracer));
                    eState =  eStartAttr;
                    continue;
                 }

                 return STATUS_OBJECT_PATH_INVALID;
            }

            default:
                return STATUS_INTERNAL_ERROR;
        }
    }

    // Legal terminating DFA states
    //
    //
    if (eState == eContinueEl)
    {
         CurrentPToken._Ident = KString::Create(Token, Alloc);
         if (!CurrentPToken._Ident)
         {
             return STATUS_INSUFFICIENT_RESOURCES;
         }
         CurrentPToken._IdentType = KDomPath::eElement;
         Res = Output.Append(CurrentPToken);
         return Res;
    }
    else if (eState == eContinueAttr)
    {
         CurrentPToken._Ident = KString::Create(Token, Alloc);
         if (!CurrentPToken._Ident)
         {
             return STATUS_INSUFFICIENT_RESOURCES;
         }
         CurrentPToken._IdentType = KDomPath::eAttribute;
         Res = Output.Append(CurrentPToken);
         return Res;
    }
    else if (eState == eEndArray)
    {
        return STATUS_SUCCESS;
    }

    // Any other state is an error/
    //
    return STATUS_OBJECT_PATH_INVALID;
}

