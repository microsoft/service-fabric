/*++

Copyright (c) Microsoft Corporation

Module Name:

    KStringViewTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KStringViewTests.h.
    2. Add an entry to the gs_KuTestCases table in KStringViewTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/

// Type parameter defs
#if defined(K$AnsiStringTarget)
#define K$KStringView KStringViewA
#define K$KStringViewTokenizer KStringViewTokenizerA
#define K$KStringPool KStringPoolA
#define K$KLocalString KLocalStringA
#define K$KDynString KDynStringA
#define K$KString KStringA
#define K$KBufferString KBufferStringA
#define K$KSharedBufferString KSharedBufferStringA
#define K$CHAR CHAR
#define K$CHARSIZE 1
#define K$STRLEN strlen
#define K$LPCSTR LPCSTR
#define K$PCHAR PCHAR
#define K$STRING(s) s
#define K$TestFunction(n) n##A

#else
#define K$KStringView KStringView
#define K$KStringViewTokenizer KStringViewTokenizer
#define K$KStringPool KStringPool
#define K$KLocalString KLocalString
#define K$KDynString KDynString
#define K$KString KString
#define K$KBufferString KBufferString
#define K$KSharedBufferString KSharedBufferString
#define K$CHAR WCHAR
#define K$CHARSIZE 2
#define K$STRLEN wcslen
#define K$LPCSTR LPCWSTR
#define K$PCHAR PWCHAR
#define K$STRING(s) L##s
#define K$TestFunction(n) n
#endif



NTSTATUS
K$TestFunction(ConstructorsTest)()
{
    K$CHAR Buf[256];
    K$CHAR Buf2[1024];

    // Basic constructors

    K$KStringView v1;              // default constructor
    K$KStringView v3(Buf, 256);    // Explicit empty buffer
    K$KStringView v2(K$STRING("test"));     // Constant
    K$KStringView v4(Buf2, sizeof(Buf2));
    K$KStringView v5(v2);

    v1 = v2;
    v3.Concat(v2);
    v4.CopyFrom(v2);
    v4.Concat(K$KStringView(K$STRING("abc")));
    v3.CopyFrom(v4);
    v3.AppendChar('x');

    // LONG Len =
    v3.Length();
    v1 = v4;

    return STATUS_SUCCESS;
}


// Test for Find() and MatchUntil()
//
NTSTATUS
K$TestFunction(FindAndMatchTest)()
{
    K$KStringView Src(K$STRING("ABCD;F"));
    BOOLEAN Res;
    ULONG Pos;

    Res = Src.Find('C', Pos);
    if (!Res || Pos != 2)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Src.Find('A', Pos);
    if (!Res || Pos != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Src.Find('F', Pos);
    if (!Res || Pos != 5)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Src.Find('Z', Pos);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Now check MatchUntil

    K$KStringView Output;
    Res = Src.MatchUntil(K$KStringView(K$STRING("0;")), Output);
    if (!Res || Output.Length() != 4)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Src.MatchUntil(K$KStringView(K$STRING("-")), Output);
    if (!Res || Output.Length() != 6)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Test for null terminator set
    //
    Res = Src.MatchUntil(K$KStringView(), Output);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Test for zero length terminator set
    //
    Res = Src.MatchUntil(K$KStringView(K$STRING("")), Output);
    if (!Res || Output.Length() != 6)
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}


NTSTATUS
K$TestFunction(SubstringAndCompares)()
{
    K$KStringView v(K$STRING("ABCDEF"));

    K$KStringView sub = v.SubString(2, 3); // Should return CDE

    if (sub.Compare(K$KStringView(K$STRING("CDE"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    sub = v.SubString(0, 0);
    sub = v.SubString(0, 2);
    sub = v.SubString(0, 3);
    sub = v.SubString(0, 4);
    sub = v.SubString(0, 5);
    sub = v.SubString(0, 6);
    sub = v.SubString(0, 7);

    sub = v.SubString(3, 2);
    sub = v.SubString(3, 4);
    sub = v.SubString(3, 5);

    sub = v.SubString(6, 1);
    sub = v.SubString(6, 2);
    sub = v.SubString(7, 0);
    sub = v.SubString(7, 1);


    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(LeftRightSub)()
{
    K$KStringView v(K$STRING("ABCD"));
    K$KStringView left = v.LeftString(2);

    if (left.Compare(K$KStringView(K$STRING("AB"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    left = v.LeftString(4);
    if (left.Compare(K$KStringView(K$STRING("ABCD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    left = v.LeftString(5);
    if (left.IsNull() == FALSE || left.Length() != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    left = v.LeftString(0);
    if (left.IsNull() == FALSE || left.Length() != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView right = v.RightString(2);
    if (right.Compare(K$KStringView(K$STRING("CD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    right = v.RightString(4);
    if (right.Compare(K$KStringView(K$STRING("ABCD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    right = v.RightString(5);
    if (right.IsNull() == FALSE || right.Length() != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    right = v.RightString(0);
    if (right.IsNull() == FALSE || right.Length() != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    // Substring tests
    //
    K$KStringView sub;
    sub = v.SubString(0, 2);
    if (sub.Compare(K$KStringView(K$STRING("AB"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    sub = v.SubString(1, 2);
    if (sub.Compare(K$KStringView(K$STRING("BC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    sub = v.SubString(2, 2);
    if (sub.Compare(K$KStringView(K$STRING("CD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    sub = v.SubString(3, 2);
    if (sub.IsNull() == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    sub = v.SubString(0, 4);
    if (sub.Compare(K$KStringView(K$STRING("ABCD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    sub = v.SubString(4, 0);
    if (sub.IsNull() == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }


    // Remainder

    K$KStringView rem;

    rem = v.Remainder(0);
    if (rem.Compare(K$KStringView(K$STRING("ABCD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    rem = v.Remainder(1);
    if (rem.Compare(K$KStringView(K$STRING("BCD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    rem = v.Remainder(2);
    if (rem.Compare(K$KStringView(K$STRING("CD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    rem = v.Remainder(3);
    if (rem.Compare(K$KStringView(K$STRING("D"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    rem = v.Remainder(4);
    if (!rem.IsEmpty())
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}


NTSTATUS
K$TestFunction(UnencodeEscapeTest)()
{
    K$KStringView Src(K$STRING("abc%5Cdef"));
    K$KLocalString<128> Dest;

    Dest.CopyFrom(Src);
    Dest.UnencodeEscapeChars();
    Dest.SetNullTerminator();

    if (Dest.Compare(K$KStringView(K$STRING("abc\\def"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView Src2(K$STRING("abc%5C"));
    Dest.CopyFrom(Src2);
    Dest.UnencodeEscapeChars();
    Dest.SetNullTerminator();

    if (Dest.Compare(K$KStringView(K$STRING("abc\\"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView Src3(K$STRING("abc%C"));
    Dest.CopyFrom(Src3);
    Dest.UnencodeEscapeChars();
    Dest.SetNullTerminator();

    if (Dest.Compare(K$KStringView(K$STRING("abc%C"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView Src4(K$STRING("abc%"));
    Dest.CopyFrom(Src4);
    Dest.UnencodeEscapeChars();
    Dest.SetNullTerminator();

    if (Dest.Compare(K$KStringView(K$STRING("abc%"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}


NTSTATUS
K$TestFunction(InsertRemoveConcat)()
{
    BOOLEAN Res;
    K$CHAR Buf[10];
    K$KStringView main(Buf, 10);

    Res = main.Concat(K$KStringView(K$STRING("ABC")));
    if (!Res || main.Compare(K$KStringView(K$STRING("ABC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Concat(K$KStringView(K$STRING("XYZ2234")));
    if (!Res || main.Compare(K$KStringView(K$STRING("ABCXYZ2234"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Now there is no room left

    ULONG Free = main.FreeSpace();
    if (Free != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Concat(K$KStringView(K$STRING("A")));   // Should fail; no more room
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.AppendChar('x');            // Should fail; no more room
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.PrependChar('x');            // Should fail; no more room
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }


    // Now delete two characters
    Res = main.Remove(0, 2);
    if (main.Compare(K$KStringView(K$STRING("CXYZ2234"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.AppendChar('z');            // Should fail; no more room
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.PrependChar('a');            // Should fail; no more room
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (main.Compare(K$KStringView(K$STRING("aCXYZ2234z"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Now try to delete various parts of the string
    Res = main.Remove(0, 1);
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Remove(main.Length()-1, 1);
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.TruncateLast();
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }


    if (main.Compare(K$KStringView(K$STRING("CXYZ223"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    Res = main.Remove(2, 4);
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (main.Compare(K$KStringView(K$STRING("CX3"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    // insert tests
    //
    Res = main.Insert(0, K$KStringView(K$STRING("AB")));
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (main.Compare(K$KStringView(K$STRING("ABCX3"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Insert(main.Length(), K$KStringView(K$STRING("XYZ")));
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (main.Compare(K$KStringView(K$STRING("ABCX3XYZ"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Insert(3, K$KStringView(K$STRING("fg")));
    if (!Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (main.Compare(K$KStringView(K$STRING("ABCfgX3XYZ"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    Res = main.Insert(0, K$KStringView(K$STRING("")));
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView dummy;
    Res = main.Insert(0, dummy);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}




NTSTATUS
K$TestFunction(Conversions)()
{
    K$CHAR Buf[128];
    K$KStringView v(Buf, 128);

    {
        ULONG Vals[] = { 0, 1, 2, 0x1000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFE, 0xFFFFFFFF };
        LONG LVals[] = { 0, 1, 2, 123123, -1241294129, LONG(0x7FFFFFFF), LONG(0xFFFFFFFE), LONG(0xFFFFFFFF) };

        ULONG Count = sizeof(Vals) / sizeof(ULONG);
        ULONG UStart, URes;
        LONG  LStart, LRes;


        // Verify extreme ranges on 32 bit
        for (ULONG ix = 0; ix < Count; ix++)
        {
            UStart = Vals[ix];
            v.FromULONG(UStart);
            if (!v.ToULONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }

            // Append test
            v.Clear();
            v.Concat(K$STRING("Test Preamble "));
            int  sizeOfPreamble = v.Length();

            v.FromULONG(UStart, FALSE, TRUE);
            if (!v.RightString(v.Length() - sizeOfPreamble).ToULONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Hex form of test
        for (ULONG ix = 0; ix < Count; ix++)
        {
            UStart = Vals[ix];
            v.FromULONG(UStart, TRUE);
            if (!v.ToULONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }

            // Append test
            v.Clear();
            v.Concat(K$STRING("Test Preamble "));
            int  sizeOfPreamble = v.Length();

            v.FromULONG(UStart, TRUE, TRUE);
            if (!v.RightString(v.Length() - sizeOfPreamble).ToULONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        Count = sizeof(LVals) / sizeof(LONG);

        for (ULONG ix = 0; ix < Count; ix++)
        {
            LStart = LVals[ix];
            v.FromLONG(LStart);
            if (!v.ToLONG(LRes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (LRes != LStart)
            {
                return STATUS_UNSUCCESSFUL;
            }

            // Append test
            v.Clear();
            v.Concat(K$STRING("Test Preamble "));
            int  sizeOfPreamble = v.Length();

            v.FromLONG(LStart, TRUE);
            if (!v.RightString(v.Length() - sizeOfPreamble).ToLONG(LRes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (LRes != LStart)
            {
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    //* Now prove ULONGLONG and LONGLONG conversion
    {
        ULONGLONG Vals[] = { 0, 1, 2, 0x1000, 0x7FFFFFFFFFFFFFFF, 0x8000000000000000, 0xFFFFFFFFFFFFFFFE, 0xFFFFFFFFFFFFFFFF };
        LONGLONG LVals[] = { 0, 1, 2, 123123, -12412941299214, LONGLONG(0x7FFFFFFFFFFFFFFF), LONGLONG(0x7FFFFFFFFFFFFFFE), LONGLONG(0xFFFFFFFFFFFFFFFF) };

        ULONG Count = sizeof(Vals) / sizeof(ULONGLONG);
        ULONGLONG UStart, URes;
        LONGLONG  LStart, LRes;


        // Verify extreme ranges on 64 bit
        for (ULONG ix = 0; ix < Count; ix++)
        {
            UStart = Vals[ix];
            v.FromULONGLONG(UStart);
            if (!v.ToULONGLONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }

            // Append test
            v.Clear();
            v.Concat(K$STRING("Test Preamble "));
            int  sizeOfPreamble = v.Length();

            v.FromULONGLONG(UStart, FALSE, TRUE);
            if (!v.RightString(v.Length() - sizeOfPreamble).ToULONGLONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        // Hex format version of test
        for (ULONG ix = 0; ix < Count; ix++)
        {
            LStart = Vals[ix];
            v.FromULONGLONG(UStart, TRUE);
            if (!v.ToULONGLONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }

            // Append test
            v.Clear();
            v.Concat(K$STRING("Test Preamble "));
            int  sizeOfPreamble = v.Length();

            v.FromULONGLONG(UStart, TRUE, TRUE);
            if (!v.RightString(v.Length() - sizeOfPreamble).ToULONGLONG(URes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (URes != UStart)
            {
                return STATUS_UNSUCCESSFUL;
            }
        }

        Count = sizeof(LVals) / sizeof(LONGLONG);

        for (ULONG ix = 0; ix < Count; ix++)
        {
            LStart = LVals[ix];
            v.FromLONGLONG(LStart);
            if (!v.ToLONGLONG(LRes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (LRes != LStart)
            {
                return STATUS_UNSUCCESSFUL;
            }

            // Append test
            v.Clear();
            v.Concat(K$STRING("Test Preamble "));
            int  sizeOfPreamble = v.Length();

            v.FromLONGLONG(LStart, TRUE);
            if (!v.RightString(v.Length() - sizeOfPreamble).ToLONGLONG(LRes))
            {
                return STATUS_UNSUCCESSFUL;
            }
            if (LRes != LStart)
            {
                return STATUS_UNSUCCESSFUL;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
K$TestFunction(Conversions2)()
{
    K$CHAR Buf[132];
    K$KStringView buf(Buf, 132);

    ULONG Count = 0;

    for (LONG i = 1; Count < 0x10000; i -= 11379)
    {
        buf.FromLONG(i);
        LONG i2 = 0;
        buf.ToLONG(i2);

        if (i2 != i)
        {
            return STATUS_UNSUCCESSFUL;
        }

        buf.FromULONG(ULONG(i), TRUE);
        ULONG i3 = 0;
        buf.ToULONG(i3);
        if (i3 != ULONG(i))
        {
            return STATUS_UNSUCCESSFUL;
        }

        buf.FromULONG(ULONG(i), FALSE);
        ULONG i4 = 0;
        buf.ToULONG(i4);
        if (i4 != ULONG(i))
        {
            return STATUS_UNSUCCESSFUL;
        }

        Count++;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
K$TestFunction(SearchTests)()
{
    K$KStringView main(K$STRING("ABCDEFG"));

    ULONG Pos;
    BOOLEAN Res = main.Search(K$KStringView(K$STRING("DEF")), Pos);
    if (!Res || Pos != 3)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Search(K$KStringView(K$STRING("ABCDEFG")), Pos);
    if (!Res || Pos != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Search(K$KStringView(K$STRING("ABCDEFGX")), Pos);// BUGBUG
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }


    Res = main.Search(K$KStringView(K$STRING("AB")), Pos);
    if (!Res || Pos != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Search(K$KStringView(K$STRING("FG")), Pos);
    if (!Res || Pos != 5)
    {
        return STATUS_UNSUCCESSFUL;
    }


    Res = main.Search(K$KStringView(K$STRING("G")), Pos);
    if (!Res || Pos != 6)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Search(K$KStringView(K$STRING("DEX")), Pos);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Search(K$KStringView(K$STRING("")), Pos);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = main.Search(K$KStringView(), Pos);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView Empty1(K$STRING(""));
    K$KStringView Empty2;

    Res = Empty1.Search(K$KStringView(K$STRING("BA")), Pos);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Empty2.Search(K$KStringView(K$STRING("BA")), Pos);
    if (Res)
    {
        return STATUS_UNSUCCESSFUL;
    }



    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(MutatingOps)()
{
    // Delete substring

    // Concat

    // Insert

    //
    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(GuidConvert)()
{
   K$CHAR Tmp[128];
   K$KStringView v(Tmp, 128);

   {
       v.FromGUID(TestGUID);

       GUID New;
       v.ToGUID(New);

       if (New != TestGUID)
       {
           return STATUS_UNSUCCESSFUL;
       }
   }

   //* Append test
   {
       v.Clear();
       v.Concat(K$STRING("Test Preamble: "));

       ULONG    sizeOfPreamble = v.Length();

       v.FromGUID(TestGUID, TRUE);

       GUID New;
       v.RightString(v.Length() - sizeOfPreamble).ToGUID(New);

       if (New != TestGUID)
       {
           return STATUS_UNSUCCESSFUL;
       }
   }

   return STATUS_SUCCESS;
}


NTSTATUS
K$TestFunction(Tokenizer)()
{
    K$KStringViewTokenizer tokenizer;
    BOOLEAN ret;
    K$KStringView token;

    //
    //  Make sure it fails without calling Start()
    //
    {
        ret = tokenizer.NextToken(token);

        if (ret)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    //  Simple test case
    //
    {
        K$KStringView str(K$STRING("Test,String"));
        tokenizer.Initialize(str, K$STRING(','));

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("Test")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("String")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    //  Test the empty string
    //
    {
        K$KStringView str;
        tokenizer.Initialize(str);

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    //  Test just a delimiter
    //
    {
        K$KStringView str(K$STRING(","));
        tokenizer.Initialize(str, K$STRING(','));

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //
    //  Test a longer and more "complex" string
    //
    {
        K$KStringView str(K$STRING(",Tes,tString,,223,;394~,,"));
        tokenizer.Initialize(str); // default comma as delim

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("Tes")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("tString")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("223")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING(";394~")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        //
        //  Test with the same string and another delim
        //
        tokenizer.Initialize(str, 't');

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING(",Tes,")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("S")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (!ret || token.Compare(K$STRING("ring,,223,;394~,,")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }

        ret = tokenizer.NextToken(token);
        if (ret || token.Compare(K$STRING("")) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(KLocalStringTests)()
{
    K$KLocalString<128> Str1;

    Str1.CopyFrom(K$KStringView(K$STRING("Blah")));
    Str1.Concat(K$KStringView(K$STRING("More")));
    Str1.SetNullTerminator();       // There is room in this case

    if (Str1.Compare(K$KStringView(K$STRING("BlahMore"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

#if !defined(K$AnsiStringTarget)
NTSTATUS
KWStringInterop()
{
    K$KStringView v(K$STRING("ABCD"));
    K$KStringView v2(PWCHAR(v), 3, 3);

    KWString tmp1(*g_Allocator);
    tmp1 = v;

    if (tmp1.Compare(KWString(*g_Allocator, K$STRING("ABCD"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    tmp1 = v2;
    if (tmp1.Compare(KWString(*g_Allocator, K$STRING("ABC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView v3;
    tmp1 = v3;

    if (tmp1.Compare(KWString(*g_Allocator, K$STRING(""))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
K$TestFunction(KDynStringTests)()
{
    K$KDynString v(*g_Allocator, 256);

    v.Concat(K$KStringView(K$STRING("Test")));
    v.Concat(K$KStringView(K$STRING("String")));


    K$KStringView v2;
    v2 = v;

    if (v2.Compare(K$KStringView(K$STRING("TestString"))) != 0)
    {
       return STATUS_UNSUCCESSFUL;
    }

    v.Resize(v.BufferSizeInChars() * 2);
    v2 = v;

    if (v2.Compare(K$KStringView(K$STRING("TestString"))) != 0)
    {
       return STATUS_UNSUCCESSFUL;
    }

    if (v.Compare(K$KStringView(K$STRING("TestString"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KDynString v3(*g_Allocator);

    if (!v3.Resize(v.Length() + 1))
    {
        return STATUS_UNSUCCESSFUL;
    }

    v3.CopyFrom(v);
    v3.SetNullTerminator();

    if (v3.Compare(K$KStringView(K$STRING("TestString"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v3.Resize(1024);

    if (v3.Compare(K$KStringView(K$STRING("TestString"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //* Prove short internal buffer behavior
    {
        K$KDynString v1(*g_Allocator, 3);
        K$KStringView view;

        auto IsUsingShortBuffer = [](K$KStringView const & str) -> BOOLEAN
        {
            void const * start = &str;
            void const * end = (char*)(&str + 1) -1;

            return ((((K$PCHAR)str) >= start) && (((K$PCHAR)str) <= end));
        };

        if (!v1)
            return STATUS_UNSUCCESSFUL;

        if (!IsUsingShortBuffer(v1))
            return STATUS_UNSUCCESSFUL;

#if K$CHARSIZE==2
        if (!v1.CopyFrom(K$STRING("1")))
            return STATUS_UNSUCCESSFUL;

        if (!IsUsingShortBuffer(v1))
            return STATUS_UNSUCCESSFUL;

        view = v1;
        if (view.Compare(v1) != 0)
            return STATUS_UNSUCCESSFUL;

        if (!v1.CopyFrom(K$STRING("123")))
            return STATUS_UNSUCCESSFUL;

        if (!IsUsingShortBuffer(v1))
            return STATUS_UNSUCCESSFUL;

        view = v1;
        if (view.Compare(v1) != 0)
            return STATUS_UNSUCCESSFUL;

        if (!v1.CopyFrom(K$STRING("1234")))
            return STATUS_UNSUCCESSFUL;

        if (IsUsingShortBuffer(v1))
            return STATUS_UNSUCCESSFUL;

        view = v1;
        if (view.Compare(v1) != 0)
            return STATUS_UNSUCCESSFUL;
#else
        if (!v1.CopyFrom(K$STRING("1234567")))
            return STATUS_UNSUCCESSFUL;

        if (!IsUsingShortBuffer(v1))
            return STATUS_UNSUCCESSFUL;

        view = v1;
        if (view.Compare(v1) != 0)
            return STATUS_UNSUCCESSFUL;

        if (!v1.CopyFrom(K$STRING("12345678")))
            return STATUS_UNSUCCESSFUL;

        if (IsUsingShortBuffer(v1))
            return STATUS_UNSUCCESSFUL;

        view = v1;
        if (view.Compare(v1) != 0)
            return STATUS_UNSUCCESSFUL;
#endif
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(KStringTests)()
{
    K$KString::SPtr Tmp;
    Tmp = K$KString::Create(*g_Allocator);

    Tmp->Resize(128);

    Tmp->CopyFrom(K$KStringView(K$STRING("Blax")));
    Tmp->SetNullTerminator();

    K$KString::SPtr Tmp2 = K$KString::Create(K$STRING("EvenMoreBlah"), *g_Allocator);

    K$KString& x = *Tmp2;
    K$KString& x2 = x;

    Tmp->Concat(x2);

    Tmp2 = Tmp;

    if (Tmp2->Compare(K$KStringView(K$STRING("BlaxEvenMoreBlah"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Verify the resize retains the string.
    //
    Tmp2->Resize(1024);
    if (Tmp2->Compare(K$KStringView(K$STRING("BlaxEvenMoreBlah"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    //  Test the various Create(...) methods
    //
    {
        K$LPCSTR constStr = K$STRING("Some test string");
        K$KStringView compString(constStr);
        K$KString::SPtr str;
        NTSTATUS status;

        status = K$KString::Create(str, *g_Allocator);
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc) failed.");
            return status;
        }
        if (! str->IsEmpty())
        {
            KTestPrintf("IsEmpty() returned false for empty string.");
            return STATUS_UNSUCCESSFUL;
        }
        if (static_cast<K$KStringView&>(*str).CopyFrom(compString) || str->Compare(compString) == 0)
        {
            KTestPrintf("KStringView non-resize copy succeeded for string with insufficient buffer.");
            return STATUS_UNSUCCESSFUL;
        }
        if (! str->CopyFrom(compString) || str->Compare(compString) != 0)
        {
            KTestPrintf("KString resize-copy failed.");
        }

        status = K$KString::Create(str, *g_Allocator, compString.Length());
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc, BufferLengthInChars) failed.");
            return status;
        }
        if (! str->IsEmpty())
        {
            KTestPrintf("IsEmpty() returned false for empty string.");
            return STATUS_UNSUCCESSFUL;
        }
        if (! static_cast<K$KStringView&>(*str).CopyFrom(compString) || str->Compare(compString) != 0)
        {
            KTestPrintf("KStringView non-resize CopyFrom failed for string with sufficient buffer.");
            return STATUS_UNSUCCESSFUL;
        }

        status = K$KString::Create(str, *g_Allocator, compString, TRUE);
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc, ToCopy, IncludeNullTerminator) failed for KStringView.");
            return status;
        }
        if (str->Compare(compString) != 0)
        {
            KTestPrintf("Copy Create failed with KStringView.");
            return STATUS_UNSUCCESSFUL;
        }
        if (! str->IsNullTerminated())
        {
            KTestPrintf("Copy create did not set null terminator correctly.");
            return STATUS_UNSUCCESSFUL;
        }

        status = K$KString::Create(str, *g_Allocator, compString, FALSE);
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc, ToCopy, IncludeNullTerminator) failed for KStringView.");
            return status;
        }
        if (str->Compare(compString) != 0)
        {
            KTestPrintf("Copy Create failed with KStringView.");
            return STATUS_UNSUCCESSFUL;
        }

        status = K$KString::Create(str, *g_Allocator, constStr, TRUE);
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc, ToCopy, IncludeNullTerminator) failed for constant string.");
            return status;
        }
        if (str->Compare(compString) != 0)
        {
            KTestPrintf("Copy Create failed with constant string.");
            return STATUS_UNSUCCESSFUL;
        }
        if (! str->IsNullTerminated())
        {
            KTestPrintf("Copy create did not set null terminator correctly.");
            return STATUS_UNSUCCESSFUL;
        }

        status = K$KString::Create(str, *g_Allocator, constStr, FALSE);
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc, ToCopy, IncludeNullTerminator) failed for constant string.");
            return status;
        }
        if (str->Compare(compString) != 0)
        {
            KTestPrintf("Copy Create failed with constant string.");
            return STATUS_UNSUCCESSFUL;
        }

#if !defined(K$AnsiStringTarget)
        LPCWSTR constWideString = L"Another test string";
        UNICODE_STRING unicodeString;
        unicodeString.Length = static_cast<USHORT>(wcslen(constWideString) * sizeof(WCHAR));
        unicodeString.MaximumLength = unicodeString.Length;
        unicodeString.Buffer = const_cast<PWCHAR>(constWideString);

        status = KString::Create(str, *g_Allocator, unicodeString, TRUE);
        if (! NT_SUCCESS(status) || !str)
        {
            KTestPrintf("Create(String, Alloc, ToCopy, IncludeNullTerminator) failed for unicode string.");
            return status;
        }
        if (str->Compare(KStringView(constWideString)) != 0)
        {
            KTestPrintf("Copy Create failed with unicode string.");
            return STATUS_UNSUCCESSFUL;
        }
        if (! str->IsNullTerminated())
        {
            KTestPrintf("Copy create did not set null terminator correctly.");
            return STATUS_UNSUCCESSFUL;
        }

        status = KString::Create(str, *g_Allocator, unicodeString, FALSE);
        if (! NT_SUCCESS(status) || !str)
        {
            return status;
        }
        if (str->Compare(KStringView(constWideString)) != 0)
        {
            KTestPrintf("Copy Create failed with unicode string.");
            return STATUS_UNSUCCESSFUL;
        }
# endif
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(KBufferStringTests)()
{
    NTSTATUS status;
    BOOLEAN res;
    K$KBufferString firstString;
    K$KBufferString secondString;
    K$KSharedBufferString::SPtr sharedString;

    const K$CHAR* const buffer = K$STRING("...First.stringSecond.string..");

    //  Create the backing kBuffer, with buffer as data
    HRESULT hr;
    size_t result;
    hr = SIZETMult(K$STRLEN(buffer), sizeof(K$CHAR), &result);
    KInvariant(SUCCEEDED(hr));
    KBuffer::SPtr kBuffer;
    status = KBuffer::CreateOrCopy(
        kBuffer,
        buffer,
        static_cast<ULONG>(result),
        *g_Allocator
        );
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to allocate KBuffer::SPtr: %x", status);
        return status;
    }

    //  Create and test a KSharedBufferString
    status = K$KSharedBufferString::Create(
        sharedString,
        *g_Allocator,
        KTL_TAG_TEST
        );
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to allocate KSharedBufferString::SPtr: %x", status);
        return status;
    }

    status = sharedString->MapBackingBuffer(
        *kBuffer,
        static_cast<ULONG>(K$STRLEN(buffer)),
        0 // Entire string
        );
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to map sharedString: %x", status);
        return status;
    }

    if (sharedString->GetBackingBuffer() != kBuffer)
    {
        KTestPrintf("sharedString->GetBuffer() not equal to mapped KBuffer");
        return STATUS_UNSUCCESSFUL;
    }

    if (sharedString->Compare(buffer) != 0)
    {
        KTestPrintf("sharedString does not show equality to the entire string under Compare after mapping");
        return STATUS_UNSUCCESSFUL;
    }

    //  Test unmapping
    sharedString->UnMapBackingBuffer();
    if (sharedString->GetBackingBuffer() != nullptr)
    {
        KTestPrintf("sharedString->GetBackingBffer() not nullptr after call to UnMapBackingBuffer");
        return STATUS_UNSUCCESSFUL;
    }

    if (sharedString->Compare(K$KStringView(K$STRING(""))) != 0)
    {
        KTestPrintf("sharedString does not show equality to the empty string under Compare after call to UnMapBackingBuffer");
        return STATUS_UNSUCCESSFUL;
    }

    //  Test sharedString copy constructor
    sharedString = nullptr;
    status = K$KSharedBufferString::Create(
        sharedString,
        buffer,
        *g_Allocator,
        KTL_TAG_TEST
        );

    if (sharedString->Compare(buffer) != 0)
    {
        KTestPrintf("sharedString does not show equality to the entire string under Compare after copy constructor");
        return STATUS_UNSUCCESSFUL;
    }



    //  Map the test strings
    status = firstString.MapBackingBuffer(
        *kBuffer,
        static_cast<ULONG>(K$STRLEN(K$STRING("First.string"))),         // Length in chars
        static_cast<ULONG>(K$STRLEN(K$STRING("...")) * sizeof(K$CHAR)),  // Offset
        ULONG_MAX  // Can be expanded
        );
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to map firstString: %x", status);
        return status;
    }

    if (firstString.Compare(K$KStringView(K$STRING("First.string"))) != 0)
    {
        KTestPrintf("firstString does not show equality to mapped region under Compare");
        return STATUS_UNSUCCESSFUL;
    }

    status = secondString.MapBackingBuffer(
        *kBuffer,
        static_cast<ULONG>(K$STRLEN(K$STRING("Second.string"))),
        static_cast<ULONG>((firstString.Length() + 3) * sizeof(K$CHAR)),
        static_cast<ULONG>(K$STRLEN(K$STRING("Second.string"))) // do not allow expansion
        );
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to map secondString: %x", status);
        return status;
    }

    if (secondString.Compare(K$KStringView(K$STRING("Second.string"))) != 0)
    {
        KTestPrintf("secondString does not show equality to mapped region under Compare");
        return STATUS_UNSUCCESSFUL;
    }


    //  Test valid append
    res = firstString.AppendChar(K$STRING('X'));
    if (! res)
    {
        KTestPrintf("AppendChar failed for expandable string");
        return STATUS_UNSUCCESSFUL;
    }

    if (firstString.Compare(K$KStringView(K$STRING("First.stringX"))) != 0
        || secondString.Compare(K$KStringView(K$STRING("Xecond.string"))) != 0)
    {
        KTestPrintf("AppendChar showed incorrect behavior");
        return STATUS_UNSUCCESSFUL;
    }


    //  Test invalid append
    res = secondString.AppendChar(K$STRING('~'));
    if (res)
    {
        KTestPrintf("AppendChar succeeded for non-expandable string");
        return STATUS_UNSUCCESSFUL;
    }


    //  Test too-small max length
    status = secondString.MapBackingBuffer(
        *kBuffer,
        2,      //  length
        0,      //  offset
        1       //  max length < string length
        );
    if (NT_SUCCESS(status))
    {
        KTestPrintf("MapBackingBuffer succeeded with too-small max length: %x", status);
        return STATUS_UNSUCCESSFUL;
    }


    //  Test offset beyond end of kbuffer
    status = secondString.MapBackingBuffer(
        *kBuffer,
        1,
        kBuffer->QuerySize() + 1,
        2);
    if (NT_SUCCESS(status))
    {
        KTestPrintf("MapBackingBuffer succeeded with offset larger than kbuffer length: %x", status);
        return STATUS_UNSUCCESSFUL;
    }


    //  Test the copy constructor
    K$KStringView toCopy = K$KStringView(K$STRING("Copy me"));
    K$KBufferString fromCopy(
        KtlSystem::GlobalNonPagedAllocator(),
        toCopy
        );

    status = fromCopy.Status();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Copy constructor for KBufferString failed: %x", status);
        return status;
    }

    if (toCopy.Compare(fromCopy) != 0)
    {
        KTestPrintf("KBufferString does not show equality to copied KStringView under Compare");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(DateConversions)()
{
    LONGLONG Now = 0x01cd0c461bbd3253;

    K$KLocalString<128> Str;

    {
        if (!Str.FromSystemTimeToIso8601(Now))
        {
            return STATUS_UNSUCCESSFUL;
        }

        Str.SetNullTerminator();

        if (Str.Compare(K$KStringView(K$STRING("2012-03-27T18:19:11Z"))) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    //* Append test
    {
        Str.Clear();
        Str.Concat(K$STRING("Current System Time: "));

        ULONG       sizeOfPreamble = Str.Length();

        if (!Str.FromSystemTimeToIso8601(Now, TRUE))
        {
            return STATUS_UNSUCCESSFUL;
        }

        Str.SetNullTerminator();

        if (Str.RightString(Str.Length() - sizeOfPreamble).Compare(K$KStringView(K$STRING("2012-03-27T18:19:11Z"))) != 0)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }
    return STATUS_SUCCESS;
}

#if !defined(K$AnsiStringTarget)
NTSTATUS
AnsiConversion()
{
    // Convert from ANSI and check.
    //
    LPSTR Tmp = "This is a regular string";

    K$KLocalString<128> Uni;
    Uni.CopyFromAnsi(Tmp, 24);
    Uni.SetNullTerminator();

    if (Uni.Compare(K$KStringView(K$STRING("This is a regular string"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Uni.EnsureBOM();

    if (!Uni.HasBOM())
    {
       return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;

}

NTSTATUS
UnicodeStringTest()
{
    UNICODE_STRING Str1;

    RtlInitUnicodeString(&Str1, K$STRING("TEST"));

    K$KStringView V(Str1);

    if (V.IsNullTerminated() == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
#endif

NTSTATUS
K$TestFunction(KStringPoolTests)()
{
    K$KStringPool Pool(*g_Allocator, 1024);
    if (!Pool.FreeSpace())
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KStringView v1 = Pool(128);
    if (v1.IsNull())
    {
        return STATUS_UNSUCCESSFUL;
    }
    K$KStringView v2 = Pool(64);
    if (v2.IsNull())
    {
        return STATUS_UNSUCCESSFUL;
    }


    v1.Concat(K$KStringView(K$STRING("ABC")));
    v1.Concat(K$KStringView(K$STRING("DEF")));

    v2.CopyFrom(v1);

    if (v2.Compare(K$KStringView(K$STRING("ABCDEF"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(SearchAndReplace)()
{
    // Base test

    K$KLocalString<256> Buf;
    Buf.CopyFrom(K$KStringView(K$STRING("The cat was a good cat.")));

    ULONG Count;
    BOOLEAN Res = Buf.Replace(K$KStringView(K$STRING("cat")), K$KStringView(K$STRING("Dog")), Count);

    if (!Res || Buf.Compare(K$KStringView(K$STRING("The Dog was a good Dog."))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Buf.Replace(K$KStringView(K$STRING("Dog")), K$KStringView(K$STRING("Doggy")), Count);
    if (!Res || Buf.Compare(K$KStringView(K$STRING("The Doggy was a good Doggy."))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Buf.Replace(K$KStringView(K$STRING("Doggy")), K$KStringView(K$STRING("X")), Count);
    if (!Res || Buf.Compare(K$KStringView(K$STRING("The X was a good X."))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Now replace with nothing.
    Res = Buf.Replace(K$KStringView(K$STRING("X")), K$KStringView(), Count);
    if (!Res || Buf.Compare(K$KStringView(K$STRING("The  was a good ."))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Buf.SetNullTerminator();

    Buf.Clear();
    Buf.Concat(' ');
    Res = Buf.Replace(K$KStringView(K$STRING(" ")), K$KStringView(), Count);
    if (!Res || Buf.Length() != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Verify a partial replacement.

    K$KLocalString<11> Test;
    Test.CopyFrom(K$KStringView(K$STRING("abcdefghaj")));

    Res = Test.Replace(K$KStringView(K$STRING("a")), K$KStringView(K$STRING("X")), Count);
    Res = Test.Replace(K$KStringView(K$STRING("X")), K$KStringView(K$STRING("zz")), Count);


    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(PrependTests)()
{
    K$KStringView v1(K$STRING("DEFG"));
    K$KStringView empty;
    K$KLocalString<128> v2;

    v2.CopyFrom(K$KStringView(K$STRING("ABC")));
    v2.Prepend(v1);

    if (v2.Compare(K$KStringView(K$STRING("DEFGABC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v2.Prepend(empty);
    if (v2.Compare(K$KStringView(K$STRING("DEFGABC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    K$KDynString d1(*g_Allocator);
    d1.Prepend(v2);
    if (d1.Compare(K$KStringView(K$STRING("DEFGABC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    d1.Prepend(K$KStringView(K$STRING("XYZ")));
    if (d1.Compare(K$KStringView(K$STRING("XYZDEFGABC"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    d1.Concat(K$KStringView(K$STRING("123")));
    if (d1.Compare(K$KStringView(K$STRING("XYZDEFGABC123"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    d1.AppendChar('g');
    if (d1.Compare(K$KStringView(K$STRING("XYZDEFGABC123g"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    d1.PrependChar('i');
    if (d1.Compare(K$KStringView(K$STRING("iXYZDEFGABC123g"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    d1.Concat(K$KStringView(K$STRING("TheEnd")));
    if (d1.Compare(K$KStringView(K$STRING("iXYZDEFGABC123gTheEnd"))) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(DynReplace)()
{
    K$KDynString Str(*g_Allocator);

    if (!Str.Concat(K$KStringView(K$STRING("abcdbefgba"))))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG ReplacementCount = 0;
    if (!Str.Replace(K$KStringView(K$STRING("a")), K$KStringView(K$STRING("xx")), ReplacementCount, TRUE))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ReplacementCount != 2)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Str.Replace(K$KStringView(K$STRING("b")), K$KStringView(K$STRING("ZZ")), ReplacementCount, TRUE))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (ReplacementCount != 3)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(StripWhitespace)()
{
    K$KStringView const test(K$STRING("\r\n\t test; ;\n\r \n"));

    K$KStringView copy;

    copy = test;
    copy.StripWs(TRUE, FALSE);
    if (copy.Compare(K$STRING("test; ;\n\r \n")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    copy = test;
    copy.StripWs(FALSE, TRUE);
    if (copy.Compare(K$STRING("\r\n\t test; ;")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    copy = test;
    copy.StripWs(TRUE, TRUE);
    if (copy.Compare(K$STRING("test; ;")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    copy = test;
    copy.StripWs(FALSE, FALSE);
    if (copy.Compare(test) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
K$TestFunction(MeasureThisTests)()
{
    // Prove MeasureThis() on a const str works right; MeasureThis() is called internally in this case
    {
        K$KStringView                 testStr(K$STRING("123456789012345678901"));
        KFatal(testStr.Length() == 21);
        KFatal(testStr.IsNullTerminated());
        KFatal(testStr.BufferSizeInChars() == 22);
    }

    // Prove MeasureThis() on a str works with an over specificed
    // buffer length and when zero terminated
    {
        K$KStringView                 testStr(K$STRING("123456789012345678901"), 25, 0);
        KFatal(testStr.MeasureThis());
        KFatal(testStr.Length() == 21);
        KFatal(testStr.IsNullTerminated());
        KFatal(testStr.BufferSizeInChars() == 25);
    }

    // Prove MeasureThis() on a str works when not zero terminated
    {
        K$KStringView                 testStr(K$STRING("123456789012345678901"), 15, 0);
        KFatal(!testStr.MeasureThis());
        KFatal(testStr.Length() == 15);
        KFatal(!testStr.IsNullTerminated());
        KFatal(testStr.BufferSizeInChars() == 15);
    }

    return STATUS_SUCCESS;
}

void
K$TestFunction(CompareEdgeTests)()
{
    // Prove basic Compare < and > works
    {
        K$KStringView                 testStr1(K$STRING("123456789012345678901"));
        K$KStringView                 testStr2(K$STRING("123456789012345678900"));      // less than
        K$KStringView                 testStr3(K$STRING("123456789012345678902"));      // greater than

        KInvariant(testStr1.Compare(testStr2) == 1);
        KInvariant(testStr1.Compare(testStr3) == -1);

        KStringView x(L"test_key_00003");
        KStringView y(L"test_key_00001");
        KInvariant(x.Compare(y) == 1);
        KInvariant(y.Compare(x) == -1);
    }

    // Prove Compare < and > works for subset equal strings
    {
        K$KStringView                 testStr1(K$STRING("123456789012345678901"));
        K$KStringView                 testStr2(K$STRING("12345678901234567890"));       // shorter
        K$KStringView                 testStr3(K$STRING("1234567890123456789012"));     // longer

        KInvariant(testStr1.Compare(testStr2) == 1);
        KInvariant(testStr1.Compare(testStr3) == -1);
    }
}

#undef K$KStringView
#undef K$KStringViewTokenizer
#undef K$KStringPool
#undef K$KLocalString
#undef K$KDynString
#undef K$KString
#undef K$KBufferString
#undef K$KSharedBufferString
#undef K$CHAR
#undef K$CHARSIZE
#undef K$STRLEN
#undef K$LPCSTR
#undef K$PCHAR
#undef K$STRING
#undef K$TestFunction
