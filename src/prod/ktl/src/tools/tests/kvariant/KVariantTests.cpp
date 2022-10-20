/*++

Copyright (c) Microsoft Corporation

Module Name:

    KVariantTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KVariantTests.h.
    2. Add an entry to the gs_KuTestCases table in KVariantTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>


#include "KVariantTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
#include <ktlevents.um.h>
 extern volatile LONGLONG gs_AllocsRemaining;
#else
#include <ktlevents.km.h>
#endif

KAllocator* g_Allocator = nullptr;

NTSTATUS
ExecuteMetadataSubtests(KAllocator& Alloc);


// {13AE2A2F-D87B-4796-8694-621C3A908D30}
static const GUID Test_GUID_1 =
{ 0x13ae2a2f, 0xd87b, 0x4796, { 0x86, 0x94, 0x62, 0x1c, 0x3a, 0x90, 0x8d, 0x30 } };

// {13AE2A2F-D87B-4796-8694-621C3A908D32}
static const GUID Test_GUID_2 =
{ 0x13ae2a2f, 0xd87b, 0x4796, { 0x86, 0x94, 0x62, 0x1c, 0x3a, 0x90, 0x8d, 0x32 } };


// BasicNumericTest
//
// Test construction of all types except SPtr-based
//
NTSTATUS BasicNumericTests()
{
    KTestPrintf("Basic numeric tests\n");

    KVariant vEmpty;
    KVariant v1(ULONG(100));
    KVariant v2(LONG(200));
    KVariant v3(LONGLONG(300));
    KVariant v4(ULONGLONG(400));

    KVariant v5 = vEmpty;
    if (!v5.IsEmpty())
    {
        return STATUS_UNSUCCESSFUL;
    }

    v5 = v1;
    if (v5.Type() != KVariant::Type_ULONG || ULONG(v5) != 100)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v5 = v2;
    if (v5.Type() != KVariant::Type_LONG || LONG(v5) != 200)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v5 = v3;
    if (v5.Type() != KVariant::Type_LONGLONG || LONGLONG(v5) != 300)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v5 = v4;
    if (v5.Type() != KVariant::Type_ULONGLONG || ULONGLONG(v5) != 400)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KTestPrintf("BasicNumericTests() passed\n");

    v5 = (GUID) Test_GUID_1;

    if (GUID(v5) == Test_GUID_2)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (GUID(v5) != Test_GUID_1)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v5.Clear();
    if (!v5.IsEmpty())
    {
        return STATUS_UNSUCCESSFUL;
    }


    KVariant b1(BOOLEAN(TRUE));
    b1 = BOOLEAN(FALSE);

    BOOLEAN x = TRUE;
    KVariant b2(x);

    b2 = b1;

    if (b2.Type() != KVariant::Type_BOOLEAN || BOOLEAN(b2) != FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant test;

    b2.CloneTo(test, *g_Allocator);

    KVariant b3(test);
    b2 = b3;

    if (b2.Type() != KVariant::Type_BOOLEAN || BOOLEAN(b2) != FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

//
//  BasicCopyConstructTests
//
//  Test all bitwise copy constructors
//
NTSTATUS
BasicCopyConstructTests()
{
    KVariant vEmpty;
    KVariant v1(ULONG(100));
    KVariant v2(LONG(200));
    KVariant v3(LONGLONG(300));
    KVariant v4(ULONGLONG(400));
    KVariant v5(Test_GUID_1);

    KVariant cv0(vEmpty);
    KVariant cv1(v1);
    KVariant cv2(v2);
    KVariant cv3(v3);
    KVariant cv4(v4);
    KVariant cv5(v5);

    if (cv0.Type() != KVariant::Type_EMPTY)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant ce2;
    ce2 = cv1;
    ce2 = cv0;
    if (ce2.Type() != KVariant::Type_EMPTY)
    {
        return STATUS_UNSUCCESSFUL;
    }


    if (cv1.Type() != KVariant::Type_ULONG || ULONG(cv1) != 100)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (cv2.Type() != KVariant::Type_LONG || LONG(cv2) != 200)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (cv3.Type() != KVariant::Type_LONGLONG || LONGLONG(cv3) != 300)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (cv4.Type() != KVariant::Type_ULONGLONG || ULONGLONG(cv4) != 400)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (cv5.Type() != KVariant::Type_GUID || GUID(cv5) != Test_GUID_1)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KMemRef mref;
    mref._Address = &cv5;

    mref._Param = 99;
    mref._Size = 98;
    KVariant v10(mref);
    KVariant v11;
    v11 = v10;
    KVariant v9(v11);
    v10 = v9;
    v9.Clear();
    v10 = v11;

    if (v10.Type() != KVariant::Type_KMemRef)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (KMemRef(v10)._Address != &cv5 || KMemRef(v10)._Param != 99 || KMemRef(v10)._Size != 98)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////////////////


NTSTATUS
DateTimeTests()
{
    NTSTATUS Res;

    KDateTime dtStart(KDateTime::Now());
    KNt::Sleep(500);
    KDateTime dtStop(KDateTime::Now());

    KDuration Elapsed = dtStop - dtStart;

    // LONGLONG Milliseconds =
        Elapsed.Milliseconds();

    KVariant v1(dtStart);

    if (v1.Type() != KVariant::Type_KDateTime)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant v2;
    v2 = Elapsed;
    KVariant v3(v2);
    v1 = v3;

    if (v2.Type() != KVariant::Type_KDuration)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (KDuration(v2) != KDuration(Elapsed))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KString::SPtr Tmp;
    Res = v2.ToString(*g_Allocator, Tmp);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

#ifdef PLATFORM_UNIX
    KTestPrintf("Duration string is %s\n", Utf16To8(PWSTR(*Tmp)).c_str());
#else
    KTestPrintf("Duration string is %S\n", PWSTR(*Tmp));
#endif

    if (v1.Type() != KVariant::Type_KDuration)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v1 = dtStart;

    if (KDateTime(v1) != KDateTime(dtStart))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant dt(Elapsed);

    for (ULONG i = 0;i < 100; i++)
    {
        KDuration More;
        More.SetSeconds(45);
        dt = (KDuration(dt) += More);

        KString::SPtr Tmp1;
        Res = dt.ToString(*g_Allocator, Tmp1);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }

#ifdef PLATFORM_UNIX
        KTestPrintf("Duration string is %s\n", Utf16To8(PWSTR(*Tmp)).c_str());
#else
        KTestPrintf("Duration string is %S\n", PWSTR(*Tmp1));
#endif


        KVariant vReverse = KVariant::Create(
            *Tmp1,
            KVariant::Type_KDuration,
            *g_Allocator
            );

        if (!vReverse)
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (KDuration(vReverse) != KDuration(dt))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }



    KTestPrintf("DateTime tests passed\n");
    return STATUS_SUCCESS;
}

NTSTATUS
BasicStringTests()
{
   // Allocate a KString::SPtr
   KString::SPtr Str = KString::Create(L"ABCD", *g_Allocator);


   KVariant v1;
   KVariant v2(Str);

   v1 = v2;
   v2.Clear();

   // Assign
   KString::SPtr Str2 = v1;
   v1.Clear();

   v2 = Str2;

   if (KString::SPtr(v2)->Compare(KStringView(L"ABCD")) != 0)
   {
       return STATUS_UNSUCCESSFUL;
   }

   // Reassign

   KString::SPtr Str3 = KString::Create(L"XXXX", *g_Allocator);
   KString::SPtr Str4 = KString::Create(L"YYYY", *g_Allocator);

   v2 = Str3;
   v1 = Str4;

   v1 = v2;
   v2 = Str;
   v2 = v1;

   if (KString::SPtr(v2)->Compare(KStringView(L"XXXX")) != 0)
   {
       return STATUS_UNSUCCESSFUL;
   }

   // Wipe out the string and make it a number

   v1 = (LONG) 33;

   if (LONG(v1) != LONG(33))
   {
        return STATUS_UNSUCCESSFUL;
   }

   KTestPrintf("Basic string tests passed\n");

   // Clone tests

   KVariant s1(Str);

   KVariant s2;
   NTSTATUS Res = s1.CloneTo(s2, *g_Allocator);

   if (!NT_SUCCESS(Res))
   {
      return Res;
   }

   if (KString::SPtr(s2)->Compare(*KString::SPtr(s1)) != 0)
   {
      return STATUS_UNSUCCESSFUL;
   }

   KString::SPtr(s2)->ReplaceChar(0, L'X');
   KString::SPtr(s2)->ReplaceChar(3, L'Z');

   if (KString::SPtr(s2)->Compare(*KString::SPtr(s1)) == 0)
   {
      return STATUS_UNSUCCESSFUL;
   }

   KVariant s3;
   s3 = s2;            // assign
   KVariant s4(s3);    // copy cons truct

   if (KString::SPtr(s4)->Compare(KStringView(L"XBCZ")) != 0)
   {
      return STATUS_UNSUCCESSFUL;
   }

   if (KString::SPtr(s1)->Compare(KStringView(L"ABCD")) != 0)
   {
      return STATUS_UNSUCCESSFUL;
   }

   const UNICODE_STRING& Ustr = (UNICODE_STRING) s1;
   KWString x(KtlSystem::GlobalNonPagedAllocator(), Ustr);

   if (x.Compare(KWString(KtlSystem::GlobalNonPagedAllocator(), L"ABCD")) != 0)
   {
      return STATUS_UNSUCCESSFUL;
   }

   // KVariant string creation ambiguity

   KVariant t1 = KVariant::Create(L"abc", *g_Allocator);

   KVariant t2 = KVariant::Create(KStringView(L"abcd"), *g_Allocator);

   KStringView xx = t2;
   if (!xx.IsNullTerminated())
   {
       return STATUS_UNSUCCESSFUL;
   }
   ((KStringView&)t1).ReplaceChar(0, L'x');

   if (((KStringView&)t1).Compare(KStringView(L"xbc")) != 0)
   {
       return STATUS_UNSUCCESSFUL;
   }

   return STATUS_SUCCESS;
}


NTSTATUS
ConvertTests()
{
    {
     KVariant v1_a((LONG) 10);

     LONGLONG v1_b;
     if (!v1_a.Convert(v1_b) || v1_b != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }

     ULONGLONG v1_c;
     if (!v1_a.Convert(v1_c) || v1_c != 10)
     {
         return STATUS_UNSUCCESSFUL;
     }

     ULONG v1_d;
     if (!v1_a.Convert(v1_d) || v1_d != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }
    }

    {
     KVariant v1_a((LONGLONG) 10);

     LONG v1_b;
     if (!v1_a.Convert(v1_b) || v1_b != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }

     ULONGLONG v1_c;
     if (!v1_a.Convert(v1_c) || v1_c != 10)
     {
         return STATUS_UNSUCCESSFUL;
     }

     ULONG v1_d;
     if (!v1_a.Convert(v1_d) || v1_d != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }
    }

    {
     KVariant v1_a((ULONG) 10);

     LONG v1_b;
     if (!v1_a.Convert(v1_b) || v1_b != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }

     ULONGLONG v1_c;
     if (!v1_a.Convert(v1_c) || v1_c != 10)
     {
         return STATUS_UNSUCCESSFUL;
     }

     LONGLONG v1_d;
     if (!v1_a.Convert(v1_d) || v1_d != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }
    }


    {
     KVariant v1_a((ULONGLONG) 10);

     LONG v1_b;
     if (!v1_a.Convert(v1_b) || v1_b != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }

     LONGLONG v1_c;
     if (!v1_a.Convert(v1_c) || v1_c != 10)
     {
         return STATUS_UNSUCCESSFUL;
     }

     ULONG v1_d;
     if (!v1_a.Convert(v1_d) || v1_d != 10)
     {
        return STATUS_UNSUCCESSFUL;
     }
    }

    KVariant v2((LONGLONG) 123);
    KString::SPtr v2str;
    if (!v2.ToString(*g_Allocator, v2str))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (v2str->Compare(KStringView(L"123")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    //

    KVariant str = KVariant::Create(L"123", *g_Allocator);

    LONG n1;
    ULONG n2;
    LONGLONG n3;
    ULONGLONG n4;
    str.Convert(n1);
    str.Convert(n2);
    str.Convert(n3);
    str.Convert(n4);

    if (n1 != 123 || n2 != 123 || n3 != 123 || n4 != 123)
    {
        return STATUS_UNSUCCESSFUL;
    }

    str = KVariant::Create(L"{13AE2A2F-D87B-4796-8694-621C3A908D30}", *g_Allocator);

    GUID g;
    if (!str.Convert(g))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (g != Test_GUID_1)
    {
        return STATUS_UNSUCCESSFUL;
    }

    str = KVariant::Create(L"http://www.google.com", *g_Allocator);

    KUri::SPtr Tmp;
    if (!str.Convert(*g_Allocator, Tmp))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Tmp->IsValid())
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Tmp->Get(KUriView::eHost).Compare(KStringView(L"www.google.com")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KBuffer::SPtr Buf;
    if (!str.Convert(*g_Allocator, Buf))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView ViewOfBuf((PWCHAR)Buf->GetBuffer(), Buf->QuerySize()/2, (Buf->QuerySize()/2)-1);

    if (ViewOfBuf.Compare(KStringView(L"http://www.google.com")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
KBufferTests()
{
    KBuffer::SPtr Tmp;
    NTSTATUS Res = KBuffer::Create(1024, Tmp, *g_Allocator);

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    RtlFillMemory(Tmp->GetBuffer(), 1024, L'Z');


    KVariant t(Tmp);

    KVariant x;
    x = t;
    KVariant z(x);
    t.Clear();
    x.Clear();

    KBuffer::SPtr Buf = (KBuffer::SPtr) z;


    return STATUS_SUCCESS;

}

NTSTATUS
UriTests()
{
    KUriView v(L"http://google.com");
    KUri::SPtr Tmp;

    NTSTATUS Res = KUri::Create(v, *g_Allocator, Tmp);

    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KVariant u1;
    u1 = Tmp;
    KVariant u2(u1);
    u1.Clear();

    KUri::SPtr TheUri = (KUri::SPtr) u2;

#ifdef PLATFORM_UNIX
    KTestPrintf("URI = %s\n", Utf16To8(LPCWSTR(*TheUri)).c_str());
#else
    KTestPrintf("URI = %S\n", LPCWSTR(*TheUri));
#endif

    KUriView Test = u2;

#ifdef PLATFORM_UNIX
    KTestPrintf("URI = %s\n", Utf16To8(LPCWSTR(u2)).c_str());
#else
    KTestPrintf("URI = %S\n", LPCWSTR(u2));
#endif

    KVariant x;
    u2.CloneTo(x, *g_Allocator);
    u2.Clear();

#ifdef PLATFORM_UNIX
    KTestPrintf("URI = %s\n", Utf16To8(LPCWSTR(x)).c_str());
#else
    KTestPrintf("URI = %S\n", LPCWSTR(x));
#endif

    KVariant vv = KVariant::Create(KStringView(L"http://www.bing.com"), KVariant::Type_KUri_SPtr, *g_Allocator);
    if (!vv)
    {
        return STATUS_UNSUCCESSFUL;
    }

#ifdef PLATFORM_UNIX
    KTestPrintf("URI = %s\n", Utf16To8(LPCWSTR(vv)).c_str());
#else
    KTestPrintf("URI = %S\n", LPCWSTR(vv));
#endif

    return STATUS_SUCCESS;
}



NTSTATUS
ToStringTests()
{
    KVariant v1;
    KString::SPtr Str;

    NTSTATUS Res;

    Res = v1.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->Compare(KStringView(L"")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant v2(LONG(12));
    Res = v2.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Str->Compare(KStringView(L"12")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant v3(LONGLONG(-13));
    Res = v3.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->Compare(KStringView(L"-13")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant v4(ULONG(14));
    Res = v4.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->Compare(KStringView(L"14")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    KVariant v5(ULONGLONG(1500));
    Res = v5.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->Compare(KStringView(L"1500")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant v6(Test_GUID_1);
    Res = v6.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->CompareNoCase(KStringView(L"{13AE2A2F-D87B-4796-8694-621C3A908D30}")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KVariant v7(BOOLEAN(TRUE));
    Res = v7.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->CompareNoCase(KStringView(L"true")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    v7 = BOOLEAN(FALSE);
    Res = v7.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (Str->CompareNoCase(KStringView(L"False")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KDateTime t(KDateTime::Now());
    KVariant vt(t);
    Res = vt.ToString(*g_Allocator, Str);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
#ifdef PLATFORM_UNIX
    KTestPrintf("The date is %s\n", Utf16To8(PWSTR(*Str)).c_str());
#else
    KTestPrintf("The date is %S\n", PWSTR(*Str));
#endif

    KString::SPtr Str2;
    Res = vt.ToString(*g_Allocator, Str2);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }
#ifdef PLATFORM_UNIX
    KTestPrintf("The recreated date is %s\n", Utf16To8(PWSTR(*Str2)).c_str());
#else
    KTestPrintf("The recreated date is %S\n", PWSTR(*Str2));
#endif


    return STATUS_SUCCESS;
}


// Basic string tests

NTSTATUS Test2(KAllocator& Alloc)
{
    KVariant v1 = KVariant::Create(KStringView(L"A string"), KVariant::Type_KString_SPtr, Alloc);
    PWSTR Test = L"A string";
    KVariant v2;
    KVariant v3(v1);

    PWSTR pTest = PWSTR(v1);
    if (wcscmp(Test, pTest) != 0)
    {
        KTestPrintf("FAILURE: Test2() (a) Basic comparison failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    v2 = v1;

    if (wcscmp(Test, v2) != 0)
    {
        KTestPrintf("FAILURE: Test2() (b) Basic comparison failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (wcscmp(Test, v3) != 0)
    {
        KTestPrintf("FAILURE: Test2() (c) Basic comparison failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    UNICODE_STRING pu = v3;

    if (wcscmp(pu.Buffer, Test) != 0)
    {
        KTestPrintf("FAILURE: Test3() (d) Basic comparison failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    v3 = ULONG(33);

    if (ULONG(v3) != 33)
    {
        KTestPrintf("FAILURE: Test4() (e) Conversion to new type failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    v3 = v2;

    if (wcscmp(Test, v3) != 0)
    {
        KTestPrintf("FAILURE: Test2() (f) Conversion back to string failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    KTestPrintf("Test2() Succeeded\n");
    return STATUS_SUCCESS;

}


NTSTATUS ExecuteSubtests(KAllocator& Alloc)
{
    NTSTATUS Result;

    Result = Test2(Alloc);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KTestPrintf("KVariant tests completed\n");

    Result = ExecuteMetadataSubtests(Alloc);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KTestPrintf("All tests completed\n");

    return STATUS_SUCCESS;
}


//***************************************************************************
//
// MetadataTest1
//
// Tests getting/setting metadata from the KDatagramMetadata container.
//
NTSTATUS MetadataTest1(KAllocator& Alloc)
{
    KDatagramMetadata::SPtr Md;
    NTSTATUS status = KDatagramMetadata::Create(Alloc, Md);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Md->Set(0, KVariant(ULONG(1000)));

    KVariant v2 = KVariant::Create(KStringView(L"MyString"), KVariant::Type_KString_SPtr, Alloc);

    Md->Set(1, v2);

    // ULONG Count =
        Md->Count();

    // Read the values back
    KVariant k1, k2;
    ULONG Id1, Id2;

    Md->GetAt(0, Id1, k1);
    Md->GetAt(1, Id2, k2);

    if (ULONG(k1) != 1000)
    {
        KTestPrintf("MetadataTest1() failure ; item corrupted");
        return STATUS_UNSUCCESSFUL;
    }

    LPCWSTR Tmp = PWSTR(k2);

    if (wcscmp(Tmp, L"MyString") != 0)
    {
        KTestPrintf("MetadataTest1() failure ; item corrupted");
        return STATUS_UNSUCCESSFUL;
    }

    Md->Clear();

    if (Md->Count() != 0)
    {
        KTestPrintf("MetadataTest1() failure to clear the container");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS MetadataTest2(KAllocator& Alloc)
{
    KDatagramMetadata::SPtr Md;

    NTSTATUS status = KDatagramMetadata::Create(Alloc, Md);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    KVariant v1 = KVariant::Create(KStringView(L"String100"), KVariant::Type_KString_SPtr, Alloc);

    KVariant v2 = KVariant::Create(KStringView(L"String200"), KVariant::Type_KString_SPtr, Alloc);

    KVariant v3 = KVariant::Create(KStringView(L"String300"), KVariant::Type_KString_SPtr, Alloc);

    KVariant v4 = KVariant::Create(KStringView(L"String400"), KVariant::Type_KString_SPtr, Alloc);

    KVariant v5 = KVariant::Create(KStringView(L"String500"), KVariant::Type_KString_SPtr, Alloc);

    Md->Set(100, v1);
    Md->Set(200, v2);
    Md->Set(300, v3);
    Md->Set(400, v4);
    Md->Set(500, v5);

    // Now remove the middle one

    Md->RemoveById(300);

    // Now the item at index [2] should be 400

    KVariant Check;

    NTSTATUS Res = Md->Get(300, Check);

    if (Res != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        KTestPrintf("MetadataTest2() : Found an item that should have been deleted");
        return STATUS_UNSUCCESSFUL;
    }


    // Swap some items around.

    Md->Get(500, Check);
    Check = KString::Create(L"501", *g_Allocator);
    Res = Md->Set(500, Check);   // Update

    // Check count
    ULONG Count = Md->Count();

    if (Count != 4)
    {
        KTestPrintf("MetadataTest2() : Container has wrong number of items");
        return STATUS_UNSUCCESSFUL;
    }

    KTestPrintf("Looping through %I32d items\n", Count);
    //
    BOOLEAN bRes = TRUE;
    ULONG Index = 0;

    for (;;)
    {
        KVariant Item;
        ULONG    Id;
        bRes = Md->GetAt(Index, Id, Item);
        if (!bRes)
        {
            break;
        }

        switch (Item.Type())
        {
            case KVariant::Type_EMPTY:          KTestPrintf("[%u]  Type_EMPTY\n", Index); break;
            case KVariant::Type_LONG:           KTestPrintf("[%u]  LONG=%I32d\n", Index, LONG(Item)); break;
            case KVariant::Type_ULONG:          KTestPrintf("[%u]  ULONG=%I32u\n", Index, ULONG(Item)); break;
            case KVariant::Type_ULONGLONG:      KTestPrintf("[%u]  ULONGLONG=%I64u\n", Index,  ULONGLONG(Item)); break;
#ifdef PLATFORM_UNIX
            case KVariant::Type_KString_SPtr:   KTestPrintf("[%u]  String=%s\n", Index,  Utf16To8(PWCHAR(Item)).c_str()); break;
#else
            case KVariant::Type_KString_SPtr:   KTestPrintf("[%u]  String=%S\n", Index,  PWCHAR(Item)); break;
#endif
            default:
                KTestPrintf("[%u] <error>\n", Index);
        }

        Index++;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
ExecuteMetadataSubtests(KAllocator& Alloc)
{
    NTSTATUS Result = MetadataTest1(Alloc);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("MetadataTest1 failure");
        return Result;
    }

    Result = MetadataTest2(Alloc);
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("MetadataTest2 failure");
        return Result;
    }

    KTestPrintf("Metadata Tests completed. No Errors");
    return STATUS_SUCCESS;
}


NTSTATUS
TestSequence()
{
    NTSTATUS Res;
    Res = BasicNumericTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = BasicCopyConstructTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = BasicStringTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = ToStringTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    Res = DateTimeTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    Res = KBufferTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    Res = UriTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = ConvertTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = ExecuteMetadataSubtests(*g_Allocator);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return Res;
}


NTSTATUS
KVariantTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif

    KTestPrintf("KVariantTest: STARTED\n");

    NTSTATUS Result;
    Result = KtlSystem::Initialize();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    EventRegisterMicrosoft_Windows_KTL();

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    Result = TestSequence();

    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("Test failure\n");
    }

    ULONGLONG Leaks = KAllocatorSupport::gs_AllocsRemaining - StartingAllocs;
    if (Leaks)
    {
        KTestPrintf("Leaks = %u\n", Leaks);
        Result = STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KVariantTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return Result;
}


#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    std::vector<WCHAR*> args_vec(argc);
    WCHAR** args = (WCHAR**)args_vec.data();
    std::vector<std::wstring> wargs(argc);
    for (int iter = 0; iter < argc; iter++)
    {
        wargs[iter] = Utf8To16(cargs[iter]);
        args[iter] = (WCHAR*)(wargs[iter].data());
    }
#endif
    KVariantTest(argc, args);
return 0;
}
#endif



