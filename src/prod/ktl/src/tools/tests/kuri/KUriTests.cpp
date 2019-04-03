/*++

Copyright (c) Microsoft Corporation

Module Name:

    KUriTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KUriTests.h.
    2. Add an entry to the gs_KuTestCases table in KUriTestCases.cpp.
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
#include "KUriTests.h"
#include <CommandLineParser.h>


#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* g_Allocator = nullptr;

#if KTL_USER_MODE

extern volatile LONGLONG gs_AllocsRemaining;

#endif

LPCWSTR LegalUris[] =
{
    L"foo#",

    // Mainline simple cases

    L"foo-2://",
    L"foo:",
    L"foo+1:",
    L"foo.a.b://",

    L"foo://b",
    L"foo://b/c"
    L"foo://bb/cc"

    // Schemeless

    L"foo",
    L"foo/a",
    L"foo#",
    L"foo?#",
    L"foo?a#b",
    L"124abc/foo"

    L"//foo",
    L"//foo/a",

    // Complex authority

    L"foo://b.a",
    L"foo://b.a:80",
    L"foo://[fe80::202:b3ff:fe1e:8329]",      // ipv6 inset
    L"foo://[fe80::202:b3ff:fe1e:8329]:80",
    L"foo://121.32.355.12:80",

    // Fragment weirdness

    L"foo:#",
    L"foo://#",
    L"foo://b#",
    L"foo://b/c#",
    L"foo://bb/cc#"

    L"foo:#f",
    L"foo://#f",
    L"foo://b#f",
    L"foo://b/c#f",
    L"foo://bb/cc#f",

    // Query string

    L"foo://b/cc?",
    L"foo://b/cc?a",
    L"foo://b/cc?ab",
    L"foo://b/cc?abc=def",

    // Delimiters

    L"foo://b/cc?abc=def&xyz=123",
    L"foo://b/cc?abc=def|xyz=123",
    L"foo://b/cc?abc=def,xyz=123",
    L"foo://b/cc?abc=def;xyz=123",
    L"foo://b/cc?abc=def+xyz=123",

    // Authorityless

    L"foo:b",
    L"foo:b/c",
    L"foo:/bb/cc",
    L"foo:./bb/cc",
    L"foo:../bb/cc",

    // Weird
    L"foo://",
    L"http://fred/"
    L"//fred/"

};

LPCWSTR IllegalUris[] =
{
    L"foo~x://a",
    L"1foo://a",
    L"foo://[abc]",
    L"foo:/[abc]"
};


VOID PrintStr(KStringView& v)
{
    for (ULONG i = 0; i < v.Length(); i++)
    {
        KTestPrintf("%C", v[i]);
    }
}

VOID DumpUri(
    __in KDynUri& Uri
    )
{
    KTestPrintf("\n---Uri Parse Result:---\n");
    KTestPrintf("IsValid    = %S\n", Uri.IsValid() ? L"TRUE" : L"FALSE");
    if (!Uri.IsValid())
    {
        KTestPrintf("!!! Error at position %d\n", Uri.GetErrorPosition());
    }
    KTestPrintf("StdQueryStr= %S\n", Uri.HasStandardQueryString() ? L"TRUE" : L"FALSE");

    KTestPrintf("Raw         = ");
    PrintStr(Uri.Get(KUriView::eRaw));
    KTestPrintf("\n");

    KTestPrintf("Scheme      = ");
    PrintStr(Uri.Get(KUriView::eScheme));
    KTestPrintf("\n");

    KTestPrintf("Path        = ");
    PrintStr(Uri.Get(KUriView::ePath));
    KTestPrintf("\n");

    if (Uri.Has(KUri::ePath))
    {
        for (ULONG i = 0; i < Uri.GetSegmentCount(); i++)
        {
            KStringView s;
            Uri.GetSegment(i, s);
            KTestPrintf("    Segment [%d] = ", i);
            PrintStr(s);
            KTestPrintf("\n");
        }
    }

    KTestPrintf("Authority   = ");
    PrintStr(Uri.Get(KUriView::eAuthority));
    KTestPrintf("\n");

    KTestPrintf("Host        = ");
    PrintStr(Uri.Get(KUriView::eHost));
    KTestPrintf("\n");

    KTestPrintf("Port        = %u", Uri.GetPort());
    KTestPrintf("\n");

    KTestPrintf("QueryString = ");
    PrintStr(Uri.Get(KUriView::eQueryString));
    KTestPrintf("\n");

    KTestPrintf("Fragment    = ");
    PrintStr(Uri.Get(KUriView::eFragment));
    KTestPrintf("\n");

    if (Uri.Has(KUriView::eQueryString) && Uri.HasStandardQueryString())
    {
        for (ULONG i = 0; i < Uri.GetQueryStringParamCount(); i++)
        {
            KStringView p, v;
            Uri.GetQueryStringParam(i, p, v);

            KTestPrintf("Query string [%d]  : ", i);
            PrintStr(p);
            KTestPrintf(" = ");
            PrintStr(v);
            KTestPrintf("\n");
        }
    }


    KTestPrintf("----------------------------------\n");
}


NTSTATUS ValidationSuite()
{
    const ULONG TestCount = sizeof(LegalUris)/sizeof(wchar_t*);

    for (ULONG i = 0; i < TestCount; i++)
    {
        KStringView x(LegalUris[i]);
        KUriView vv(x);
        if (!vv.IsValid())
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS BasicKUriViewTest()
{
    KUriView v1(L"http://foo:82/a/b?x=y#ff");
    if (v1.IsEmpty() || !v1.IsValid())
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView sc = v1.Get(KUriView::eScheme);
    if (sc.Compare(KStringView(L"http")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView host = v1.Get(KUriView::eHost);
    if (host.Compare(KStringView(L"foo")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ULONG Port = v1.GetPort();
    if (Port != 82)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView auth = v1.Get(KUriView::eAuthority);
    if (auth.Compare(KStringView(L"foo:82")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView path = v1.Get(KUriView::ePath);
    if (path.Compare(KStringView(L"a/b")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView q = v1.Get(KUriView::eQueryString);
    if (q.Compare(KStringView(L"x=y")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView fr = v1.Get(KUriView::eFragment);
    if (fr.Compare(KStringView(L"ff")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS BasicKUriViewTest_Const()
{
    const KUriView v1(L"http://foo:82/a/b?x=y#ff");
    if (v1.IsEmpty() || !v1.IsValid())
    {
        return STATUS_UNSUCCESSFUL;
    }

    const KStringView sc = v1.Get(KUriView::eScheme);
    if (sc.Compare(KStringView(L"http")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    const KStringView host = v1.Get(KUriView::eHost);
    if (host.Compare(KStringView(L"foo")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ULONG Port = v1.GetPort();
    if (Port != 82)
    {
        return STATUS_UNSUCCESSFUL;
    }

    const KStringView auth = v1.Get(KUriView::eAuthority);
    if (auth.Compare(KStringView(L"foo:82")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    const KStringView path = v1.Get(KUriView::ePath);
    if (path.Compare(KStringView(L"a/b")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    const KStringView q = v1.Get(KUriView::eQueryString);
    if (q.Compare(KStringView(L"x=y")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    const KStringView fr = v1.Get(KUriView::eFragment);
    if (fr.Compare(KStringView(L"ff")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

VOID
ComparisonOperatorTests()
{
    KUriView uri1(L"fabric:/a");
    KUriView uri2(L"fabric:/a");
    KUriView uri3(L"fabric:/b");

    KInvariant(uri1 == uri2);
    KInvariant((uri1 == uri3) == FALSE);

    KInvariant(uri1 != uri3);
    KInvariant((uri1 != uri2) == FALSE);
}


NTSTATUS KUriViewTest2()
{
    KUriView v1(L"http://foo:82/a/b");
    if (v1.IsEmpty() || !v1.IsValid())
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView sc = v1.Get(KUriView::eScheme);
    if (sc.Compare(KStringView(L"http")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView host = v1.Get(KUriView::eHost);
    if (host.Compare(KStringView(L"foo")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ULONG Port = v1.GetPort();
    if (Port != 82)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView auth = v1.Get(KUriView::eAuthority);
    if (auth.Compare(KStringView(L"foo:82")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView path = v1.Get(KUriView::ePath);
    if (path.Compare(KStringView(L"a/b")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (v1.Has(KUriView::eFragment))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (v1.Has(KUriView::eQueryString))
    {
        return STATUS_UNSUCCESSFUL;
    }


    KUriView v2(L"http://foo:82/a/b");

    if (!v2.Compare(KUriView::eScheme, v1))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!v2.Compare(KUriView::ePath, v1))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KUriView v3(L"http://foo:82/c/b");
    if (v3.Compare(KUriView::ePath, v1))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (v3.Compare(v1) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (v3.Compare(KUriView(L"http://foo:82/c/b")) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS KDynUriViewTest()
{
    KDynUri v2(KStringView(L"http://foo:82/a/bc?a1=b2|c3=d4"), *g_Allocator);
    KDynUri v1(*g_Allocator);
    v1.Set(v2);

    if (v1.IsEmpty() || !v1.IsValid())
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (v1.GetSegmentCount() != 2)
    {
        return STATUS_UNSUCCESSFUL;
    }


    KStringView seg;
    v1.GetSegment(0, seg);
    if (seg.Compare(KStringView(L"a")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v1.GetSegment(1, seg);
    if (seg.Compare(KStringView(L"bc")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    ULONG QCount = v1.GetQueryStringParamCount();
    if (QCount != 2)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KStringView p, v;

    v1.GetQueryStringParam(0, p, v);
    if (p.Compare(KStringView(L"a1")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (v.Compare(KStringView(L"b2")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    v1.GetQueryStringParam(1, p, v);
    if (p.Compare(KStringView(L"c3")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (v.Compare(KStringView(L"d4")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
GetRelativeUri()
{
    KUri::SPtr RootUri;
    NTSTATUS Res;

    // Create two URIs for the sample

    Res = KUri::Create(KStringView(L"rvdtcp://foo:5003/rvd"), *g_Allocator, RootUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KUri::SPtr FullUri;
    Res = KUri::Create(KStringView(L"rvdtcp://foo:5003/rvd/xx/yy/zz"), *g_Allocator, FullUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }


    // Now assuming we have the two URIs, get string view of the root

    KStringView RootView = RootUri->Get(KUriView::eRaw);

    KDynString Buf(*g_Allocator);
    if (!Buf.CopyFrom(FullUri->Get(KUriView::eRaw)))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ULONG Pos = 0;
    if (!Buf.Search(RootView, Pos, 0) || Pos != 0)  // Check that the URIs have the same prefix
    {
        return STATUS_INTERNAL_ERROR;   // URIs don't have the same prefix
    }

    Buf.Remove(0, RootView.Length() + 1);    // Already a KStringView so you can just use this
                                             // The +1 gets read of the leading slash
    Buf.SetNullTerminator();            // May or may not be necessary depending on what you do with it

    // Do whatever with it if you want a KWString

    KWString ResultString(*g_Allocator);

    ResultString = Buf;
    return STATUS_SUCCESS;
}



NTSTATUS
KUriTest()
{
    KUri::SPtr PUri;
    NTSTATUS Res;

    Res = KUri::Create(KStringView(L"http://foo:82/a/bc?a1=b2|c3=d4"), *g_Allocator, PUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    if (!PUri->IsValid())
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (PUri->GetSegmentCount() != 2)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KUri::SPtr Copy;

    Res = PUri->Clone(Copy);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    if (PUri->Compare(*Copy) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KUri::SPtr Uri2;
    Res = KUri::Create(KStringView(L"http://foo:81/a/bc?a1=b2|c3=d4"), *g_Allocator, Uri2);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Only differs by port
    //
    if (PUri->Compare(*Uri2) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (PUri->Compare(KUriView::eScheme | KUriView::eHost | KUriView::ePath | KUriView::eQueryString, *Uri2) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Comparison operator test
    //
    if (*PUri != *Copy)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (*PUri <= *Uri2)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (*Uri2 >= *PUri)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
PrefixTests()
{
    KDynUri Base(KStringView(L"foo://a/b"), *g_Allocator);

    KDynUri Candidate1(KStringView(L"foo://a/b?x=y"), *g_Allocator);
    KDynUri Candidate2(KStringView(L"foo://a/b"), *g_Allocator);
    KDynUri Candidate3(KStringView(L"foo://a/b/c"), *g_Allocator);
    KDynUri Candidate4(KStringView(L"foo://a/b/c#af"), *g_Allocator);
    KDynUri Candidate5(KStringView(L"foo://a/b#af"), *g_Allocator);

    KDynUri Candidate6(KStringView(L"foo://a/c"), *g_Allocator);
    KDynUri Candidate7(KStringView(L"foo://a:9/b/c"), *g_Allocator);
    KDynUri Candidate8(KStringView(L"fox://a/b/c"), *g_Allocator);


    if (Base.IsPrefixFor(Candidate1) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Base.IsPrefixFor(Candidate2) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Base.IsPrefixFor(Candidate3) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Base.IsPrefixFor(Candidate4) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Base.IsPrefixFor(Candidate5) != TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // These should not match

    if (Base.IsPrefixFor(Candidate6) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Base.IsPrefixFor(Candidate7) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Base.IsPrefixFor(Candidate8) == TRUE)
    {
        return STATUS_UNSUCCESSFUL;

    }

    // Test case from driver

    KDynUri RvdTestBase(KStringView(L"rvd://343659C03-01:5005"), *g_Allocator);
    KDynUri Other(KStringView(L"rvd://343659C03-01:5005/csi/{c43c8c05-0648-3d30-8d70-2b075e3092f0}"), *g_Allocator);

    if (RvdTestBase.IsPrefixFor(Other) == FALSE)
    {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
ComposeTests()
{
    KDynUri Base(KStringView(L"foo://a/b"), *g_Allocator);
    KUriView Suffix(L"x/y");

    KUri::SPtr NewUri;
    NTSTATUS Res = KUri::Create(Base, Suffix, *g_Allocator, NewUri);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }
    KStringView Result = *NewUri;

    if (Result.Compare(KStringView(L"foo://a/b/x/y")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
}

VOID
DumpOutput(
    __in KArray<KDomPath::PToken>& Output
    )
{
    KTestPrintf("\n-------------------------\n");
    for (ULONG i = 0; i < Output.Count(); i++)
    {
        KTestPrintf("Item [%d] ", i);

        switch (Output[i]._IdentType)
        {
            case KDomPath::eNone:
                KTestPrintf(" : <no type; internal error>\n");
                break;

            case KDomPath::eElement:
                KTestPrintf(" : element\n");
                break;

            case KDomPath::eAttribute:
                KTestPrintf(" : attribute\n");
                break;

            case KDomPath::eArrayElement:
                {
                    KTestPrintf(" : array ");
                    if (Output[i]._LiteralIndex == -1)
                    {
                        KTestPrintf(" ; bounds unspecified\n");
                    }
                    else
                    {
                        KTestPrintf(" ; literal index (%d)\n", Output[i]._LiteralIndex);
                    }
                }
                break;

        }

        KTestPrintf("   Ident = '%S'\n", LPWSTR(*Output[i]._Ident));
        if (Output[i]._NsPrefix)
        {
            KTestPrintf("   Ns    = %S\n", LPWSTR(*Output[i]._NsPrefix));
        }
        else
        {
            KTestPrintf("   Ns    = (empty)\n");
        }
    }
}


NTSTATUS KDomPathTests()
{
    LPCWSTR GoodPaths[] = {
        L"xns:ab",
        L"a/@xns:ccc",
        L"a/b/c[1]/d[2]/@x",
        L"ab[2]/@x",
        L"aa/bb[]",
        L"ab",
        L"a/b",
        L"a/b.c/c_d",
        L"a/b/@x",
        L"a/b/c[1]/@x",
        L"a/ns1:b/c[1]/ns2:d[2]/@ns3:x"
        };

    ULONG GoodPathsCount = sizeof(GoodPaths)/sizeof(LPCWSTR);

    LPCWSTR BadPaths[] = {
        L"",
        L"/a",
        L"a/b/",
        L"//",
        L"a/b/@@x",
        L"a/b/c[0]/@x@",
        L"a/b/c]/@x",
        L"a/[b]/c[1]/d[2]/@x"
        L"a/ns1::b/"
        };

    ULONG BadPathsCount = sizeof(BadPaths)/sizeof(LPCWSTR);


    for (ULONG i = 0; i < GoodPathsCount; i++)
    {
        KDomPath p(GoodPaths[i]);
        KArray<KDomPath::PToken> ParseOutput(*g_Allocator);

        NTSTATUS Res = p.Parse(*g_Allocator, ParseOutput);

        if (!NT_SUCCESS(Res))
        {
            return Res;
        }

        DumpOutput(ParseOutput);
    }

    for (ULONG i = 0; i < BadPathsCount; i++)
    {
        KDomPath p(BadPaths[i]);
        KArray<KDomPath::PToken> ParseOutput(*g_Allocator);

        NTSTATUS Res = p.Parse(*g_Allocator, ParseOutput);

        if (NT_SUCCESS(Res))
        {
            return STATUS_INTERNAL_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
SuffixTests()
{
    NTSTATUS Status;
    KDynUri Tmp1(*g_Allocator);

    Status = Tmp1.Set(KStringView(L"foo://basic/uri"));

    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KUri::SPtr Main;

    Status = KUri::Create(Tmp1, *g_Allocator, Main);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Tmp1.Clear();
    Status = Tmp1.Set(KStringView(L"foo://basic/uri/longer/than/original"));
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KDynUri Diff(*g_Allocator);
    Status = Main->GetSuffix(Tmp1, Diff);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Diff.Compare(KUriView(L"longer/than/original")))
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Test query string & fragment


    Tmp1.Clear();
    Status = Tmp1.Set(KStringView(L"foo://basic/uri/longer/than/original?q1=v1&q2=v2#frag"));
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Status = Main->GetSuffix(Tmp1, Diff);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Diff.Compare(KUriView(L"longer/than/original?q1=v1&q2=v2#frag")))
    {
        return STATUS_UNSUCCESSFUL;
    }


    // This test failed originally using Winfab URLs
    //
    KDynUri LongUri(*g_Allocator);

    Status = LongUri.Set(KStringView(L"http://RAYMCCMOBILE8:80/pav/svc/fabric:/rvd/testnodeservice:129905745079993957"));
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Status = Tmp1.Set(KStringView(L"http://RAYMCCMOBILE8:80/pav/svc/fabric:/rvd/testnodeservice:129905745079993957"));
    Diff.Clear();
    Status = LongUri.GetSuffix(Tmp1, Diff);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Root URI
    //
    KDynUri RootUri(*g_Allocator);

    Status = RootUri.Set(KStringView(L"http://fred/"));
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Status = Tmp1.Set(KStringView(L"http://fred/nodes/mynode/blah"));
    Diff.Clear();
    Status = RootUri.GetSuffix(Tmp1, Diff);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    KDynUri RootUri2(*g_Allocator);

    Status = RootUri2.Set(KStringView(L"//fred/"));
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Status = Tmp1.Set(KStringView(L"//fred/nodes/mynode/blah"));
    Diff.Clear();
    Status = RootUri2.GetSuffix(Tmp1, Diff);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}

NTSTATUS
WildcardTests()
{
    KUriView Auth(L"foo://*/blah/blah2");
    KUriView Test(L"foo://auth/blah/blah2");
    KUriView Test2(L"foo://xx/blah/blah2");

    if (Auth.Compare(Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Auth.Compare(KUriView::eLiteralHost, Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Auth.Compare(KUriView::eHost, Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Auth.Compare(KUriView::eHost, Test2))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (!Auth.Compare(KUriView::eScheme | KUriView::ePath, Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
TestSequence()
{
    KTestPrintf("Starting tests...\n");

    NTSTATUS Status;

    Status = WildcardTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    Status = SuffixTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    Status = KDomPathTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    Status = ComposeTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    Status = PrefixTests();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    Status = GetRelativeUri();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ValidationSuite();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = BasicKUriViewTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = BasicKUriViewTest_Const();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ComparisonOperatorTests();

    Status = KUriViewTest2();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KDynUriViewTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = KUriTest();
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    KTestPrintf("Completed. Success.\n");

    return STATUS_SUCCESS;
}


VOID TestCommandLineUri(
    __in LPCWSTR Test
    )
{
    KDynUri v(*g_Allocator);
    v.Set(KStringView(Test));
    DumpUri(v);
}


NTSTATUS
KUriTest(
    int argc, WCHAR* args[]
    )
{
#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KUriTest: STARTED\n");

    NTSTATUS Result;
    Result = KtlSystem::Initialize();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KtlSystem::Initializer() failure\n");
        return Result;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });
    g_Allocator = &KtlSystem::GlobalNonPagedAllocator();

    ULONGLONG StartingAllocs = KAllocatorSupport::gs_AllocsRemaining;

    EventRegisterMicrosoft_Windows_KTL();

    if (argc == 0)
    {
        Result = TestSequence();
        if (! NT_SUCCESS(Result))
        {
            KTestPrintf("Test failed %x\n", Result);
        }
    }
    else
    {
        TestCommandLineUri(args[0]);
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

    KTestPrintf("KUriTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return Result;
}



#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
wmain(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    if (argc > 0)
    {
        argc--;
        args++;
    }

    return KUriTest(argc, args);
}
#endif



