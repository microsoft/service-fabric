

/*++

    (c) 2010 by Microsoft Corp. All Rights Reserved.

    kstringview.cpp

    Description:
      Kernel Tempate Library (KTL): KStringView prototype imp

      String manipulation & conversion utilities - implemented in a type

    History:
      raymcc          27-Feb-2012         Initial version.

--*/

// Type and constant type-specfic source parameter defs
#if defined(K$AnsiStringTarget)
#define K$KStringView KStringViewA
#define K$KStringPool KStringPoolA
#define K$KLocalString KLocalStringA
#define K$KDynString KDynStringA
#define K$KString KStringA
#define K$KBufferString KBufferStringA
#define K$KSharedBufferString KSharedBufferStringA
#define K$CHAR CHAR
#define K$CHARSIZE 1
#define K$LPCSTR LPCSTR
#define K$PCHAR PCHAR
#define K$STRING(s) s

#else
#define K$KStringView KStringView
#define K$KStringPool KStringPool
#define K$KLocalString KLocalString
#define K$KDynString KDynString
#define K$KString KString
#define K$KBufferString KBufferString
#define K$KSharedBufferString KSharedBufferString
#define K$CHAR WCHAR
#define K$CHARSIZE 2
#define K$LPCSTR LPCWSTR
#define K$PCHAR PWCHAR
#define K$STRING(s) L##s
#endif

const ULONG K$KStringView::MaxIso8601DateTime;
const ULONG K$KStringView::MaxIso8601Duration;
const ULONG K$KStringView::Max64BitNumString;
const ULONG K$KStringView::Max32BitNumString;
const ULONG K$KStringView::MaxBooleanString;
const ULONG K$KStringView::MaxGuidString;
const ULONG K$KStringView::GuidLengthInChars;
const ULONG K$KStringView::MaxStandardType; // Largest of the above for properly sized temp work buffers used for conversion
const ULONG K$KStringView::MaxLength;

//  KStringView::ToGUID
//
//
BOOLEAN
K$KStringView::ToGUID(
    __out GUID& Dest
    ) const
{
    if (IsNull() || _LenInChars != GuidLengthInChars)
    {
        return FALSE;
    }
    K$KStringView Tmp(*this);
    RtlZeroMemory(&Dest, sizeof(Dest));


    // Verify a
    //
    if (Tmp.PeekFirst() != '{')
    {
        RtlZeroMemory(&Dest, sizeof(Dest));
        return FALSE;
    }
    Tmp.ConsumeChar();

    // Next eight characters are hex digits
    //
    for (ULONG i = 0; i < 8; i++)
    {
        ULONG Val = 0;
        if (!HexDigitToULONG(Tmp.PeekFirst(), Val))
        {
             RtlZeroMemory(&Dest, sizeof(Dest));
             return FALSE;
        }
        Dest.Data1 <<= 4;
        Dest.Data1 |= Val;
        Tmp.ConsumeChar();
    }

    // Verify a -
    //
    if (Tmp.PeekFirst() != '-')
    {
        RtlZeroMemory(&Dest, sizeof(Dest));
        return FALSE;
    }
    Tmp.ConsumeChar();

    // Next four characters are hex digits
    //
    for (ULONG i = 0; i < 4; i++)
    {
        ULONG Val = 0;
        if (!HexDigitToULONG(Tmp.PeekFirst(), Val))
        {
             RtlZeroMemory(&Dest, sizeof(Dest));
             return FALSE;
        }
        Dest.Data2 <<= 4;
        Dest.Data2 |= Val;
        Tmp.ConsumeChar();
    }

    // Verify a -
    //
    if (Tmp.PeekFirst() != '-')
    {
        RtlZeroMemory(&Dest, sizeof(Dest));
        return FALSE;
    }
    Tmp.ConsumeChar();

    // Next four characters are hex digits
    //
    for (ULONG i = 0; i < 4; i++)
    {
        ULONG Val = 0;
        if (!HexDigitToULONG(Tmp.PeekFirst(), Val))
        {
             RtlZeroMemory(&Dest, sizeof(Dest));
             return FALSE;
        }
        Dest.Data3 <<= 4;
        Dest.Data3 |= Val;
        Tmp.ConsumeChar();
    }

    // Verify a -
    //
    if (Tmp.PeekFirst() != '-')
    {
        RtlZeroMemory(&Dest, sizeof(Dest));
        return FALSE;
    }
    Tmp.ConsumeChar();

    // The final 16 digits plus one hyphen at position 4.
    //
    ULONG i2 = 0;
    ULONG NybbleCount = 0;
    for (ULONG i = 0; i < 17; i++)
    {
        ULONG Val = 0;
        if (i == 4)
        {
            if (Tmp.PeekFirst() != '-')
            {
                RtlZeroMemory(&Dest, sizeof(Dest));
                return FALSE;
            }
            Tmp.ConsumeChar();
            continue;
        }
        if (!HexDigitToULONG(Tmp.PeekFirst(), Val))
        {
             RtlZeroMemory(&Dest, sizeof(Dest));
             return FALSE;
        }
        Dest.Data4[i2] <<= 4;
        Dest.Data4[i2] |= Val;
        NybbleCount++;
        if (NybbleCount == 2)
        {
            i2++;
            NybbleCount = 0;
        }
        Tmp.ConsumeChar();
    }

    // Verify a final curly
    //
    if (Tmp.PeekFirst() != '}')
    {
        RtlZeroMemory(&Dest, sizeof(Dest));
        return FALSE;
    }
    Tmp.ConsumeChar();

    if (Tmp.Length() != 0)
    {
        return FALSE;
    }
    return TRUE;
}

//
// KStringView::UnencodeEscapeChars
//
// Removes URI-style escapes and replaces
// them with their normal equivalent characters.
//
VOID
K$KStringView::UnencodeEscapeChars()
{
    if (IsEmpty())
    {
        return;
    }

    ULONG  NewLength = _LenInChars;
    K$PCHAR Src = _Buffer;
    K$PCHAR Dest = _Buffer;
    K$PCHAR Last = _Buffer + _LenInChars;

    for (;;)
    {
        if (Src == Last)
        {
            break;
        }

        if (*Src != '%')
        {
            *Dest++ = *Src++;
            continue;
        }

        // If here, we have a possible escape sequence.
        // We err on the side of safety and ignore escapes
        // which don't conform.
        //
        if (Last - Src < 3)
        {
            break;
        }
        K$CHAR digit1 = Src[1];
        K$CHAR digit2 = Src[2];
        ULONG v1, v2;
        if (!HexDigitToULONG(digit1, v1) || !HexDigitToULONG(digit2, v2))
        {
            break;
        }
        v1 *= 16;
        v2 += v1;
        K$CHAR NewChar = (K$CHAR) v2;
        *Dest++ = NewChar;
        Src += 3;
        NewLength -= 2;
    }

    _LenInChars = NewLength;
}

//  KStringView::FromGUID
//
//
BOOLEAN
K$KStringView::FromGUID(
    __in const GUID& Src,
    __in BOOLEAN DoAppend
    )
{
    if (!DoAppend)
    {
        Clear();
    }

    if ((_BufLenInChars - _LenInChars) < K$KStringView::GuidLengthInChars)
    {
        return FALSE;
    }

    ULONG Accumulator = Src.Data1;
    UCHAR Char;
    AppendChar('{');
    for (ULONG i = 0; i < 8; i++)
    {
        (Char = (((Accumulator & 0xF0000000) >> 28) + 0x30)) > '9' ? Char += 0x27: 0;
        AppendChar(Char);
        Accumulator <<= 4;
    }
    AppendChar('-');
    Accumulator = Src.Data2;
    for (ULONG i = 0; i < 4; i++)
    {
        (Char = (((Accumulator & 0xF000) >> 12) + 0x30)) > '9' ? Char += 0x27: 0;
        AppendChar(Char);
        Accumulator <<= 4;
    }
    AppendChar('-');
    Accumulator = Src.Data3;
    for (ULONG i = 0; i < 4; i++)
    {
        (Char = (((Accumulator & 0xF000) >> 12) + 0x30)) > '9' ? Char += 0x27: 0;
        AppendChar(Char);
        Accumulator <<= 4;
    }
    AppendChar('-');
    UCHAR Acc;
    for (ULONG i = 0; i < 8; i++)
    {
        Acc = Src.Data4[i];
        (Char = (Acc >> 4) + 0x30) > '9' ? Char += 0x27 : 0;
        AppendChar(Char);
        (Char = (Acc & 0xF) + 0x30) > '9' ? Char += 0x27 : 0;
        AppendChar(Char);
        if (i == 1)
        {
            AppendChar('-');
        }
    }
    AppendChar('}');
    return TRUE;
}


//
//  KStringView::ToULONGLONG
//
BOOLEAN
K$KStringView::ToULONGLONG(
    __out ULONGLONG& Destination
    ) const
{
    Destination = 0;
    ULONGLONG Prev = 0;
    BOOLEAN Hex = FALSE;

    if (_LenInChars == 0)
    {
        return TRUE;
    }

    K$KStringView Tmp(*this);
    Tmp.StripWs(TRUE, TRUE);

    if (Tmp.LeftString(2).CompareNoCase(K$KStringView(K$STRING("0x"))) == 0)
    {
        Hex = TRUE;
        Tmp.ResetOrigin(2);
    }
    while (Tmp.Length())
    {
        K$CHAR c = Tmp.PeekFirst();
        if (Hex)
        {
            Destination <<= 4;
            ULONG Val = 0;
            if (!HexDigitToULONG(c, Val))
            {
                Destination = 0;
                return FALSE;
            }
            Destination |= Val;
        }
        else
        {
            if (c >= '0' && c <= '9')
            {
                Destination = Destination * 10 + (c - 0x30);
            }
            else  // Not a number after all
            {
                Destination = 0;
                return FALSE;
            }
        }

        if (Destination  < Prev) // Overflow test
        {
            Destination = 0;
            return FALSE;
        }
        Prev = Destination;

        Tmp.ConsumeChar();
    }

    return TRUE;
}


//
//  KStringView::FromLONGLONG
//
BOOLEAN
K$KStringView::FromLONGLONG(
    __in LONGLONG Src,
    __in BOOLEAN DoAppend
    )
{
    K$KLocalString<Max64BitNumString> temp;
    K$KStringView&      dest = (DoAppend) ? temp : *this;

    if (!DoAppend)
    {
        _LenInChars = 0;
    }

    ULONG       savedLength = dest._LenInChars;

    BOOLEAN Neg = FALSE;
    if (Src < 0)
    {
        Src = -Src;
        Neg = TRUE;
    }

    do
    {
        K$CHAR Digit = K$CHAR(Src % 10 + 0x30);
        if (!dest.AppendChar(Digit))
        {
            dest._LenInChars = savedLength;
            return FALSE;
        }
        Src /= 10;
    }
    while (Src);

    dest.Reverse();
    if (Neg)
    {
        if (!dest.PrependChar('-'))
        {
            dest._LenInChars = savedLength;
            return FALSE;
        }
    }

    if (DoAppend)
    {
        if ((this->_BufLenInChars - this->_LenInChars) < temp.Length())
        {
            return FALSE;
        }

        KInvariant(this->Concat(temp));
    }

    return TRUE;
}


//
//  KStringView::ToLONGLONG
//
BOOLEAN
K$KStringView::ToLONGLONG(
    __out LONGLONG& Destination
    ) const
{
    Destination = 0;
    if (IsNull())
    {
        Destination = 0;
        return TRUE;
    }

    K$KStringView Tmp(*this);
    Tmp.StripWs(TRUE, TRUE);

    BOOLEAN Negate = Tmp.PeekFirst() == '-' ? Tmp.ConsumeChar(), TRUE : FALSE;
    LONGLONG Prev = 0;
    while (Tmp.Length())
    {
        K$CHAR c = Tmp.PeekFirst ();
        if (c >= '0' && c <= '9')
        {
            Destination = Destination * 10 + (c - 0x30);
            if (Destination  < Prev) // Overflow test
            {
                Destination = 0;
                return FALSE;
            }
            Prev = Destination;
        }
        else
        {
            Destination = 0;
            return FALSE;
        }
        Tmp.ConsumeChar();
    }
    if (Negate)
    {
        Destination = -Destination;
    }
    return TRUE;
}


//
//  KStringView::FromULONGLONG
//
BOOLEAN
K$KStringView::FromULONGLONG(
    __in ULONGLONG Src,
    __in BOOLEAN EncodeAsHex,
    __in BOOLEAN DoAppend
    )
{
    K$KLocalString<Max64BitNumString> temp;
    K$KStringView&      dest = (DoAppend) ? temp : *this;

    if (!DoAppend)
    {
        _LenInChars = 0;
    }

    ULONG       savedLength = dest._LenInChars;

    do
    {
        K$CHAR c;
        if (EncodeAsHex)
        {
            c = Src & 0xF;
            c += c > 9 ? 0x37 : 0x30;
            Src >>= 4;
        }
        else
        {
            c = K$CHAR(Src % 10 + 0x30);
            Src /= 10;
        }
        if (!dest.AppendChar(c))
        {
            dest._LenInChars = savedLength;
            return FALSE;
        }
    }
    while (Src);

    dest.Reverse();
    if (EncodeAsHex)
    {
        BOOLEAN Res = dest.PrependChar('x');
        if (!Res)
        {
            dest._LenInChars = savedLength;
            return FALSE;
        }
        Res = dest.PrependChar('0');
        if (!Res)
        {
            dest._LenInChars = savedLength;
            return FALSE;
        }
    }

    if (DoAppend)
    {
        if ((this->_BufLenInChars - this->_LenInChars) < temp.Length())
        {
            return FALSE;
        }

        KInvariant(this->Concat(temp));
    }

    return TRUE;
}

//
//  KStringView::CompareNoCase
//
LONG
K$KStringView::CompareNoCase(
    __in K$KStringView const & Comparand
    ) const
{
    ULONG Temp;
    
    return(CompareNoCase(Comparand, Temp));
}

//
//  KStringView::CompareNoCase
//
LONG
K$KStringView::CompareNoCase(
    __in K$KStringView const & Comparand,
    __out ULONG& Index
    ) const
{
    if (Comparand.IsNull())
    {
        if (IsNull())
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if (IsNull())
        {
            return -1;
        }
    }

    ULONG Max = _LenInChars > Comparand._LenInChars ? Comparand._LenInChars : _LenInChars;
    for (Index = 0; Index < Max; Index++)
    {
        K$CHAR c1 = CharToUpper(_Buffer[Index]);
        K$CHAR c2 = CharToUpper(Comparand._Buffer[Index]);
        LONG Diff = c1 - c2;
        if (Diff > 0)
        {
            return 1;
        }
        else if (Diff < 0)
        {
            return -1;
        }
    }

    if (_LenInChars > Comparand._LenInChars)
    {
        return 1;
    }
    else if (Comparand._LenInChars > _LenInChars)
    {
        return -1;
    }
    return 0;
}

#if !defined(K$AnsiStringTarget)
//
//  KStringView::CompareNoCaseAnsi
//
LONG
KStringView::CompareNoCaseAnsi(
    __in_ecount_opt(CharsToCompare) CHAR const * TestBuffer,
    __in ULONG CharsToCompare
    ) const
{
    if (IsNull())
    {
        if (TestBuffer == nullptr)
        {
            return 0;
        }
        else return -1;
    }
    else
    {
        if (!TestBuffer)
        {
            return 1;
        }
    }

    // If here, we have some stuff to compare.
    //
    for (ULONG ix = 0; ix < CharsToCompare; ix++)
    {
        WCHAR c1 = CharToUpper(_Buffer[ix]);
        WCHAR c2 = CharToUpper(TestBuffer[ix]);
        LONG Diff = c1 - c2;
        if (Diff > 0)
        {
            return 1;
        }
        else if (Diff < 0)
        {
            return -1;
        }
    }

    return 0;
}
#endif




//
//  KStringView::MatchUntil
//
BOOLEAN
K$KStringView::MatchUntil(
    __in  K$KStringView const & TerminatorSet,
    __out K$KStringView & ResultString
    ) const
{
    ResultString.Zero();
    K$KStringView Tmp(*this);
    Tmp.SetLength(0);

    if (TerminatorSet.IsNull() || IsNull())
    {
        return FALSE;
    }

    for (ULONG Ix = 0; Ix < _LenInChars; Ix++)
    {
        K$CHAR c = _Buffer[Ix];
        ULONG Pos = 0;
        BOOLEAN Match = TerminatorSet.Find(c, Pos);
        if (!Match)
        {
            Tmp.SetLength(Ix+1);
        }
        else
        {
            ResultString = Tmp;
            return TRUE;
        }
    }

    if (Tmp.Length() > 0)
    {
        ResultString = Tmp;
        return TRUE;
    }

    return FALSE;
}


//
//  KStringView::MatchWhile
//
BOOLEAN
K$KStringView::MatchWhile(
    __in  K$KStringView const & AcceptorSet,
    __out K$KStringView& ResultString
    ) const
{
    ResultString.Zero();
    K$KStringView Tmp(*this);
    Tmp.SetLength(0);

    if (AcceptorSet.IsNull() || IsNull())
    {
        return FALSE;
    }

    for (ULONG Ix = 0; Ix < _LenInChars; Ix++)
    {
        K$CHAR c = _Buffer[Ix];
        ULONG Pos = 0;
        BOOLEAN Match = AcceptorSet.Find(c, Pos);
        if (Match)
        {
            Tmp.SetLength(Ix+1);
        }
        else
        {
            ResultString = Tmp;
            return TRUE;
        }
    }

    if (Tmp.Length() > 0)
    {
        ResultString = Tmp;
        return TRUE;
    }

    return FALSE;
}






//
//  FromSystemTimeToIso8601
//
BOOLEAN
K$KStringView::FromSystemTimeToIso8601(
    __in LONGLONG Time,
    __in BOOLEAN DoAppend
)
{
    if (IsNull() || ((DoAppend ? (_BufLenInChars - _LenInChars) : _BufLenInChars) < K$KStringView::MaxIso8601DateTime))
    {
        // No Room
        return FALSE;
    }

    if (!DoAppend)
    {
        Clear();
    }

    K$KLocalString<Max32BitNumString> Tmp;  // Scratch area

#if KTL_USER_MODE
    // Convert to UTC
    //
    FILETIME* ft = (FILETIME *) &Time;
    SYSTEMTIME st;
    BOOL Res = FileTimeToSystemTime(ft, &st);
    if (!Res)
    {
        return FALSE;
    }

    // The following conversions and concatenations
    // cannot fail.
    //
    Tmp.FromULONG(st.wYear);
    Concat(Tmp);
    AppendChar('-');

    Tmp.FromULONG(st.wMonth);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar('-');

    Tmp.FromULONG(st.wDay);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar('T');

    Tmp.FromULONG(st.wHour);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar(L':');

    Tmp.FromULONG(st.wMinute);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar(':');

    Tmp.FromULONG(st.wSecond);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar('Z');

    return TRUE;

#else
    // Kernel
    //
    TIME_FIELDS st;
    RtlTimeToTimeFields(
        PLARGE_INTEGER(&Time),
        &st
        );

    Tmp.FromULONG(st.Year);
    Concat(Tmp);
    AppendChar('-');

    Tmp.FromULONG(st.Month);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar('-');

    Tmp.FromULONG(st.Day);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar('T');

    Tmp.FromULONG(st.Hour);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar(':');

    Tmp.FromULONG(st.Minute);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar(':');

    Tmp.FromULONG(st.Second);
    Tmp.Length() == 2 ? 0 : Tmp.PrependChar('0');
    Concat(Tmp);
    AppendChar('Z');

    return TRUE;

#endif
}

//
//  KStringView::ToDurationFromTime
//
BOOLEAN
K$KStringView::ToDurationFromTime(
    __in LONGLONG Timespan
    )
{
    ULONGLONG Hours = 0;
    ULONGLONG Minutes = 0;
    ULONGLONG Seconds = 0;
    ULONGLONG FractionalSeconds = 0;

    Seconds = Timespan / 10000000;
    FractionalSeconds = Timespan % 10000000;

    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    Hours = Minutes / 60;
    Minutes = Minutes % 60;

    K$KLocalString<Max32BitNumString> Tmp;

    if (!Concat(K$STRING("PT")))
    {
        return FALSE;
    }

    // Special case a duration of 0
    //
    if (Timespan == 0)
    {
        if (!AppendChar('0'))
        {
            return FALSE;
        }
        if (!AppendChar('S'))
        {
            return FALSE;
        }
        SetNullTerminator();
        return TRUE;
    }

    if (Hours)
    {
        BOOLEAN Res = Tmp.FromULONGLONG(Hours);
        if (!Res)
        {
            return FALSE;
        }
        if (!Concat(Tmp))
        {
            return FALSE;
        }
        if (!AppendChar('H'))
        {
            return FALSE;
        }
    }

    if (Minutes)
    {
        BOOLEAN Res = Tmp.FromULONGLONG(Minutes);
        if (!Res)
        {
            return FALSE;
        }
        if (!Concat(Tmp))
        {
            return FALSE;
        }
        if (!AppendChar('M'))
        {
            return FALSE;
        }
    }

    // Seconds
    //
    // The main part of seconds may be zero
    //
    BOOLEAN Res = Tmp.FromULONGLONG(Seconds);
    if (!Res)
    {
        return FALSE;
    }
    if (!Concat(Tmp))
    {
        return FALSE;
    }
    if (FractionalSeconds)
    {
        if (!AppendChar('.'))
        {
            return FALSE;
        }
        if (!Tmp.FromULONGLONG(FractionalSeconds))
        {
            return FALSE;
        }

        while (Tmp.Length() < 7)
        {
            if (!Tmp.PrependChar('0'))
            {
                return FALSE;
            }
        }
        if (!Concat(Tmp))
        {
            return FALSE;
        }
    }

    if (!AppendChar('S'))
    {
        return FALSE;
    }

    SetNullTerminator();
    return TRUE;
}


//
//  KStringView::FromDurationToTime
//
// PT00H00M00.0000000S
//
BOOLEAN
K$KStringView::ToTimeFromDuration(
    __out LONGLONG& Timespan
    ) const
{
    BOOLEAN Res;
    K$KStringView Field(*this);

    // Verify the ISO 8601 markers for durations
    //
    if (Field.PeekFirst() != 'P')
    {
        return FALSE;
    }
    Field.ConsumeChar();
    if (Field.PeekFirst() != 'T')
    {
        return FALSE;
    }
    Field.ConsumeChar();

    Timespan = 0;
    enum { eHours = 1, eMinutes = 2, eSeconds = 4, eSubseconds = 8 };
    ULONG Allowed = eHours | eMinutes | eSeconds | eSubseconds;

    for (;;)
    {
        K$KStringView Match;
        Res = Field.MatchWhile(K$KStringView(K$STRING("0123456789")), Match);
        Field.ConsumeChars(Match.Length());
        WCHAR Suffix = Field.PeekFirst();
        Field.ConsumeChar();

        if (Suffix == 'H' && (Allowed & eHours))
        {
            LONGLONG nHours = 0;
            if (!Match.ToLONGLONG(nHours))
            {
                return FALSE;
            }
            Timespan += LONGLONG(36000000000)*nHours;
            Allowed = eMinutes | eSeconds | eSubseconds;
            continue;
        }
        else if (Suffix == 'M' && (Allowed & eMinutes))
        {
            LONGLONG nMinutes = 0;
            if (!Match.ToLONGLONG(nMinutes))
            {
                return FALSE;
            }
            Timespan += (600000000L)*nMinutes;
            Allowed = eSeconds;
            continue;
        }
        else if (Suffix == 'S' && (Allowed & eSeconds))
        {
            LONGLONG nSeconds = 0;
            if (!Match.ToLONGLONG(nSeconds))
            {
                return FALSE;
            }
            Timespan += (10000000)*nSeconds;
            break;
        }
        else if (Suffix == '.' && (Allowed & eSeconds))
        {
            LONGLONG nSeconds = 0;
            if (!Match.ToLONGLONG(nSeconds))
            {
                return FALSE;
            }
            Timespan += (10000000L)*nSeconds;
            Allowed = eSubseconds;
        }
        else if (Suffix == 'S' && (Allowed &eSubseconds))
        {
            LONGLONG Factor = 1000000L;
            while (Match.PeekFirst())
            {
                ULONG DigitVal = Match.PeekFirst() - 0x30;
                Timespan += (Factor * DigitVal);
                Factor /= 10;
                Match.ConsumeChar();
            }
            break;
        }
        else
        {
            // error
            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
K$KStringView::Search(
    __in K$KStringView const & Target,
    __out ULONG& Pos,
    __in ULONG  StartPos
    ) const
{
    if (IsNull() || Target.IsNull() || IsEmpty() || Target.IsEmpty())
    {
        return FALSE;
    }

    for (ULONG i = StartPos; i < _LenInChars; i++)
    {
        ULONG MatchCount = 0;
        for (ULONG i2 = 0; i2 < Target._LenInChars && i2 + i < _LenInChars; i2++)
        {
            if (Target._Buffer[i2] == _Buffer[i + i2])
            {
                MatchCount++;
            }
            else
            {
                break;
            }
        }
        if (MatchCount == Target._LenInChars)
        {
            Pos = i;
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
K$KStringView::RSearch(
    __in K$KStringView const& Target,
    __in ULONG& Pos,
    __in ULONG  LimitPos
    ) const
{
    if (IsNull() || Target.IsNull() || IsEmpty() || Target.IsEmpty())
    {
        return FALSE;
    }

    LimitPos = __min(LimitPos, (_LenInChars-1));

    for (LONG i =  LimitPos; i >= 0; i--)
    {
        ULONG MatchCount = 0;
        for (LONG i2 = (LONG)(Target._LenInChars-1); i2 >= 0; i2--)
        {
            if (Target._Buffer[i2] == _Buffer[i + (i2 - (Target._LenInChars-1))])
            {
                MatchCount++;
            }
            else
            {
                break;
            }
        }
        if (MatchCount == Target._LenInChars)
        {
            Pos = i - (MatchCount - 1);
            return TRUE;
        }
    }

    return FALSE;
}


BOOLEAN
K$KStringView::Replace(
    __in  K$KStringView const & Target,
    __in  K$KStringView const & Replacement,
    __out ULONG& ReplacementCount,
    __in  BOOLEAN ReplaceAll
    )
{
    ReplacementCount = 0;
    ULONG StartPos = 0;

    if (IsEmpty() || Target.Length() == 0)
    {
        return FALSE;
    }

    for (;;)
    {
        ULONG Locus;
        BOOLEAN Res = Search(Target, Locus, StartPos);
        if (!Res)
        {
            // We're done.  The target wasn't present.
            //
            return TRUE;
        }

        // If here, there is an occurrence. First determine if it will fit.
        //
        ULONG NetChange = Replacement.Length() - Target.Length();
        if (_LenInChars + NetChange > _BufLenInChars)
        {
            return FALSE;   // Won't fit.
        }

        // If here, we can do the replacement.
        //
        Res = Remove(Locus, Target.Length());
        if (!Res)
        {
            return FALSE;
        }
        if (Replacement.Length())
        {
            Res = Insert(Locus, Replacement);
            if (!Res)
            {
                return FALSE;
            }
        }
        if (ReplaceAll == FALSE)
        {
            break;
        }

        ReplacementCount++;
        StartPos = (Locus + Replacement.Length());
    }

    return TRUE;
}


//
//  KString constructor
//
K$KString::K$KString()
   : K$KDynString(GetThisAllocator())
{
    _UseShortBuffer = FALSE;
}

K$KString::~K$KString()
{
}


NTSTATUS
K$KString::Create(
    __out K$KString::SPtr& String,
    __in KAllocator& Alloc,
    __in ULONG BufferLengthInChars
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    K$KString::SPtr Tmp;

    String = nullptr;

    Tmp = _new(KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$KString;

    if (! Tmp)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, sizeof(K$KString), NULL);
        return status;
    }

    if (BufferLengthInChars > 0)
    {
        Tmp->_Buffer = _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$CHAR[BufferLengthInChars];

        if (! Tmp->_Buffer)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceOutOfMemory(0, status, NULL, BufferLengthInChars, NULL);
            return status;
        }
    }

    Tmp->_LenInChars = 0;
    Tmp->_BufLenInChars = BufferLengthInChars;

    String = Ktl::Move(Tmp);
    return STATUS_SUCCESS;
}

NTSTATUS
K$KString::Create(
    __out K$KString::SPtr& String,
    __in KAllocator& Alloc,
    __in K$KStringView const & ToCopy,
    __in BOOLEAN IncludeNullTerminator
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    K$KString::SPtr Tmp;

    String = nullptr;

    ULONG BufferLength = ToCopy.Length();
    if (IncludeNullTerminator)
    {
        HRESULT hr;
        hr = ULongAdd(BufferLength, 1, &BufferLength);
        KInvariant(SUCCEEDED(hr));
    }

    status = K$KString::Create(Tmp, Alloc, BufferLength);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    if (BufferLength > 0)
    {
        if (! Tmp->CopyFrom(ToCopy, IncludeNullTerminator))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    String = Ktl::Move(Tmp);
    return STATUS_SUCCESS;
}

NTSTATUS
K$KString::Create(
    __out K$KString::SPtr& String,
    __in KAllocator& Alloc,
    __in K$LPCSTR ToCopy,
    __in BOOLEAN IncludeNullTerminator
    )
{
    return K$KString::Create(String, Alloc, K$KStringView(ToCopy), IncludeNullTerminator);
}

//  DEPRECATED
K$KString::SPtr
K$KString::Create(
    __in KAllocator& Alloc
    )
{
    K$KString::SPtr Tmp = _new(KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$KString;
    return Tmp;
}

//  DEPRECATED
K$KString::SPtr
K$KString::Create(
    __in KAllocator& Alloc,
    __in ULONG BufferSizeInChars
    )
{
    K$KString::SPtr Tmp = _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$KString;
    Tmp->_Buffer =  _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$CHAR[BufferSizeInChars];
    if (!Tmp->_Buffer)
    {
        // Tmp will out of scope and self-delete
        return nullptr;
    }
    Tmp->_LenInChars = 0;
    Tmp->_BufLenInChars = BufferSizeInChars;

    return Tmp;

}

//  DEPRECATED
K$KString::SPtr
K$KString::Clone(
    __in BOOLEAN IncludeNullTerminator
    ) const
{
    if (IsNull())
    {
        return nullptr;
    }

    K$KString::SPtr Tmp = _new(KTL_TAG_BUFFERED_STRINGVIEW, GetThisAllocator()) K$KString;
    if (!Tmp)
    {
        return Tmp;
    }

    ULONG Adjustment = IncludeNullTerminator ? 1 : 0;
	
	HRESULT hr;
	ULONG result;
	hr = ULongAdd(_LenInChars, Adjustment, &result);
	KInvariant(SUCCEEDED(hr));
	Tmp->_Buffer =  _new (KTL_TAG_BUFFERED_STRINGVIEW, GetThisAllocator()) K$CHAR[result];
	
    if (!Tmp->_Buffer)
    {
        // Tmp will out of scope and self-delete
        return nullptr;
    }
    Tmp->_BufLenInChars = _LenInChars + Adjustment;
    Tmp->CopyFrom(*this, IncludeNullTerminator); // Cannot fail
    return Tmp;
}

//  DEPRECATED
K$KString::SPtr
K$KString::Create(
    __in K$LPCSTR Str,
    __in KAllocator& Alloc,
    __in BOOLEAN IncludeNullTerminator
    )
{
    if (!Str)
    {
        return nullptr;
    }

    ULONG Len = K$KStringView(Str).Length();
    if (Len == 0)
    {
        return nullptr;
    }

    K$KString::SPtr Tmp = _new(KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$KString;
	
	HRESULT hr;
	ULONG result;
	hr = ULongAdd(Len, 1, &result);
	KInvariant(SUCCEEDED(hr));
	Tmp->_Buffer =  _new(KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$CHAR[result];
	
	if (!Tmp->_Buffer)
    {
        return nullptr;
    }
    ULONG Adjustment = IncludeNullTerminator ? 1 : 0;
    Tmp->_BufLenInChars = Len+Adjustment;
    Tmp->CopyFrom(K$KStringView(Str));
    if (IncludeNullTerminator)
    {
        Tmp->SetNullTerminator();
    }
    return Tmp;
}

//  DEPRECATED
K$KString::SPtr
K$KString::Create(
    __in const K$KStringView& Src,
    __in KAllocator& Alloc,
    __in BOOLEAN IncludeNullTerminator
    )
{
    if (Src.IsNull())
    {
        return nullptr;
    }

    ULONG Len = Src.Length();
    if (IncludeNullTerminator)
    {
		HRESULT hr;
		hr = ULongAdd(Len, 1, &Len);
		KInvariant(SUCCEEDED(hr));
    }

    K$KString::SPtr Tmp = _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$KString;
    Tmp->_Buffer =  _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) K$CHAR[Len];
    if (!Tmp->_Buffer)
    {
        return nullptr;
    }

    Tmp->_BufLenInChars = Len;
    Tmp->CopyFrom(Src);
    if (IncludeNullTerminator)
    {
        Tmp->SetNullTerminator();
    }

    return Tmp;
}



#if !defined(K$AnsiStringTarget)

NTSTATUS
KString::Create(
    __out KString::SPtr& String,
    __in KAllocator& Alloc,
    __in UNICODE_STRING const & ToCopy,
    __in BOOLEAN IncludeNullTerminator
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KString::SPtr Tmp;

    String = nullptr;

    ULONG BufferLengthInChars = ToCopy.Length / 2;
    if (IncludeNullTerminator)
    {
        HRESULT hr;
        hr = ULongAdd(BufferLengthInChars, 1, &BufferLengthInChars);
        KInvariant(SUCCEEDED(hr));
    }

    status = KString::Create(Tmp, Alloc, BufferLengthInChars);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    if (BufferLengthInChars > 0)
    {
        KStringView toCopy(static_cast<PWCHAR>(ToCopy.Buffer), ToCopy.Length / 2, ToCopy.Length / 2);
        if (! Tmp->CopyFrom(toCopy, IncludeNullTerminator))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    String = Ktl::Move(Tmp);
    return STATUS_SUCCESS;
}

//  DEPRECATED
KString::SPtr
KString::Create(
    __in UNICODE_STRING& Src,
    __in KAllocator& Alloc,
    __in BOOLEAN IncludeNullTerminator
    )
{
   if (Src.Length == 0 || Src.Buffer == 0)
   {
       return nullptr;
   }

   ULONG Len = Src.Length / 2;

   // See if null terminator already present

   PWCHAR NullProbe = PWCHAR(Src.Buffer) + (Src.Length/2);
   if (*NullProbe && IncludeNullTerminator)
   {
		HRESULT hr;
		hr = ULongAdd(Len, 1, &Len);
		KInvariant(SUCCEEDED(hr));
   }
   KString::SPtr Tmp = _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) KString;
   Tmp->_Buffer =  _new (KTL_TAG_BUFFERED_STRINGVIEW, Alloc) WCHAR[Len];
   if (!Tmp->_Buffer)
   {
        return nullptr;
   }
   Tmp->_BufLenInChars = Len;
   Tmp->CopyFrom(KStringView(PWCHAR(Src.Buffer), Len));
   if (IncludeNullTerminator)
   {
       Tmp->SetNullTerminator();
   }

   return Tmp;
}
#endif



NTSTATUS
K$KBufferString::MapBackingBuffer(
    __in KBuffer& Buffer,
    __in ULONG StringLengthInChars,
    __in ULONG ByteOffset,
    __in ULONG MaxStringLengthInChars
    )
{
    ULONG kBufferSize = Buffer.QuerySize();
    if (kBufferSize < ByteOffset)  //  Offset starts beyond end of buffer
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    ULONG remainingBufferLengthInChars = (kBufferSize - ByteOffset) / K$CHARSIZE;
    ULONG bufferLengthInChars = __min(remainingBufferLengthInChars, MaxStringLengthInChars);
    if (bufferLengthInChars < StringLengthInChars)  //  String size is larger than allowed buffer region
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    PVOID stringPtr = static_cast<PCHAR>(Buffer.GetBuffer()) + ByteOffset;

    _KBuffer = &Buffer;
    _Buffer = static_cast<K$PCHAR>(stringPtr);
    _BufLenInChars = bufferLengthInChars;
    _LenInChars = StringLengthInChars;

    return STATUS_SUCCESS;
}

VOID
K$KBufferString::UnMapBackingBuffer()
{
    Zero();
    _KBuffer = nullptr;
}

KBuffer::SPtr
K$KBufferString::GetBackingBuffer() const
{
    return _KBuffer;
}

K$KBufferString::K$KBufferString()
{
    Zero();
    _KBuffer = nullptr;
}

K$KBufferString::~K$KBufferString()
{
}

FAILABLE
K$KBufferString::K$KBufferString(
    __in KAllocator& Allocator,
    __in K$KStringView const & ToCopy
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KBuffer::SPtr buffer;

    status = KBuffer::Create(
        ToCopy.LengthInBytes(),
        buffer,
        Allocator
        );

    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        SetConstructorStatus(status);
        return;
    }

    KMemCpySafe(buffer->GetBuffer(), buffer->QuerySize(), (PVOID) ToCopy, ToCopy.LengthInBytes());
    status = MapBackingBuffer(*buffer, ToCopy.Length());

    SetConstructorStatus(status);
}

K$KSharedBufferString::K$KSharedBufferString()
{
}

K$KSharedBufferString::~K$KSharedBufferString()
{
}

NTSTATUS
K$KSharedBufferString::Create(
    __out K$KSharedBufferString::SPtr& SharedString,
    __in KAllocator& Alloc,
    __in ULONG AllocationTag
    )
{
    K$KSharedBufferString::SPtr sharedString;

    SharedString = nullptr;

    sharedString = _new(AllocationTag, Alloc) K$KSharedBufferString();
    if (! sharedString)
    {
        NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, 0, 0);
        return status;
    }

    SharedString = Ktl::Move(sharedString);
    return STATUS_SUCCESS;
}

FAILABLE
K$KSharedBufferString::K$KSharedBufferString(
    __in KAllocator& Allocator,
    __in K$KStringView const & ToCopy
    )
    :
        K$KBufferString(Allocator, ToCopy)
{
}

NTSTATUS
K$KSharedBufferString::Create(
    __out K$KSharedBufferString::SPtr& SharedString,
    __in K$KStringView const & ToCopy,
    __in KAllocator& Alloc,
    __in ULONG AllocationTag
    )
{
    K$KSharedBufferString::SPtr sharedString;

    SharedString = nullptr;

    sharedString = _new(AllocationTag, Alloc) K$KSharedBufferString(Alloc, ToCopy);
    if (! sharedString)
    {
        NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, 0, 0);
        return status;
    }
    if (! NT_SUCCESS(sharedString->Status()))
    {
        return sharedString->Status();
    }

    SharedString = Ktl::Move(sharedString);
    return STATUS_SUCCESS;
}


#undef K$KStringView 
#undef K$KStringPool 
#undef K$KLocalString 
#undef K$KDynString 
#undef K$KString 
#undef K$KBufferString
#undef K$KSharedBufferString
#undef K$CHAR 
#undef K$CHARSIZE 
#undef K$LPCSTR 
#undef K$PCHAR 
#undef K$STRING
