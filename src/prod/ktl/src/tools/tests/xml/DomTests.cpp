/*++

Copyright (c) Microsoft Corporation

Module Name:

    XmlTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in XmlTests.h.
    2. Add an entry to the gs_KuTestCases table in XmlTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "XmlTests.h"
#include <CommandLineParser.h>
#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


//** Test version of KIOutputStream
class OutputStream : public KIOutputStream
{
private:
    KBuffer::SPtr   _Buffer;
    ULONGLONG       _NextOffset;


    void
    Append(WCHAR* Format, ...)
    {

        va_list         args;
        va_start(args, Format);

        static const ULONG  maxXmlLineSize = 1024;
        static WCHAR        dest[maxXmlLineSize];
        NTSTATUS            status;

        #if KTL_USER_MODE
        status = StringCchVPrintfW(STRSAFE_LPWSTR(dest), maxXmlLineSize, Format, args);
        #else
        status = RtlStringCchVPrintfW(dest, maxXmlLineSize, Format, args);
        #endif

        KFatal(NT_SUCCESS(status));

        size_t      cb;

        #if KTL_USER_MODE
        status = StringCbLengthW(STRSAFE_LPWSTR(dest), maxXmlLineSize, &cb);
        #else
        status = RtlStringCchLengthW(dest, maxXmlLineSize, &cb);
        cb *= sizeof(WCHAR);
        #endif

        KFatal(NT_SUCCESS(status));

        if ((_NextOffset + cb + sizeof(WCHAR)) > _Buffer->QuerySize())
        {
            status = _Buffer->SetSize(_Buffer->QuerySize() * 2, TRUE);
            KFatal(NT_SUCCESS(status));
        }

        KMemCpySafe(
            (UCHAR*)(_Buffer->GetBuffer()) + _NextOffset, 
            _Buffer->QuerySize() - _NextOffset, 
            &dest[0], cb + sizeof(WCHAR));

        _NextOffset += cb;
    }

public:
    OutputStream()
    {
        KFatal(NT_SUCCESS(KBuffer::Create(1024, _Buffer, KtlSystem::GlobalNonPagedAllocator())));
        *((WCHAR*)(_Buffer->GetBuffer())) = KTextFile::UnicodeBom;
        _NextOffset = sizeof(WCHAR);
    }

    KBuffer::SPtr&
    GetBuffer()
    {
        return _Buffer;
    }

    NTSTATUS Put(__in WCHAR Value) override
    {
        Append(L"%c", Value);
        KTestPrintf("%C", Value);
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in LONG Value)  override
    {
        Append(L"%i", Value);
        KTestPrintf("%i", Value);
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in ULONG Value) override
    {
        Append(L"%u", Value);
        KTestPrintf("%u", Value);
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in LONGLONG Value) override
    {
        Append(L"%I64i", Value);
        KTestPrintf("%I64i", Value);
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in ULONGLONG Value) override
    {
        Append(L"%I64u", Value);
        KTestPrintf("%I64u", Value);
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in GUID& Value) override
    {
        WCHAR       guidStr[39];
        GuidToUChars(Value, &guidStr[0]);
#if !defined(PLATFORM_UNIX)
        Append(L"%s", &guidStr[0]);
        KTestPrintf("%S", &guidStr[0]);
#else
        Append(L"%s", Utf16To8(&guidStr[0]).c_str());
        KTestPrintf("%s", Utf16To8(&guidStr[0]).c_str());
#endif
        return STATUS_SUCCESS;
    }


    NTSTATUS Put(__in_z const WCHAR* Value) override
    {
#if !defined(PLATFORM_UNIX)
        Append(L"%s", Value);
        KTestPrintf("%S", Value);
#else
        std::string ValueA = Utf16To8(Value);
        Append(L"%s", ValueA.c_str());
        KTestPrintf("%s", ValueA.c_str());
#endif
        return STATUS_SUCCESS;
    }



    NTSTATUS Put(__in KUri::SPtr Value) override
    {
#if !defined(PLATFORM_UNIX)
        KTestPrintf("%S", LPWSTR(*Value));
#else
        KTestPrintf("%s", Utf16To8(LPWSTR(*Value)).c_str());
#endif
        KStringView Tmp = *Value;
        if (!Tmp.IsNullTerminated())
        {
            KFatal(FALSE);
        }

#if !defined(PLATFORM_UNIX)
        Append(L"%s", LPWSTR(Tmp));
#else
        Append(L"%s", Utf16To8(LPWSTR(Tmp)).c_str());
#endif
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in KString::SPtr Value) override
    {
        if (!Value->IsNullTerminated())
        {
            KFatal(FALSE);
        }

#if !defined(PLATFORM_UNIX)
        KTestPrintf("%S", LPWSTR(*Value));
        Append(L"%s", LPCWSTR(*Value));
#else
        KTestPrintf("%s", Utf16To8(LPWSTR(*Value)).c_str());
        Append(L"%s", Utf16To8(LPCWSTR(*Value)).c_str());
#endif

        return STATUS_SUCCESS;
   }

    NTSTATUS Put(__in KDateTime Value) override
    {
        KString::SPtr Tmp;
        Value.ToString(KtlSystem::GlobalNonPagedAllocator(), Tmp);
#if !defined(PLATFORM_UNIX)
        KTestPrintf("%S", LPWSTR(*Tmp));
        Append(L"%s", LPWSTR(*Tmp));
#else
        KTestPrintf("%s", Utf16To8(LPWSTR(*Tmp)).c_str());
        Append(L"%s", Utf16To8(LPWSTR(*Tmp)).c_str());
#endif

        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in KDuration Value) override
    {
        KString::SPtr Tmp;
        Value.ToString(KtlSystem::GlobalNonPagedAllocator(), Tmp);
#if !defined(PLATFORM_UNIX)
        Append(L"%s", LPWSTR(*Tmp));
        KTestPrintf("%S", LPWSTR(*Tmp));
#else
        Append(L"%s", Utf16To8(LPWSTR(*Tmp)).c_str());
        KTestPrintf("%s", Utf16To8(LPWSTR(*Tmp)).c_str());
#endif
        return STATUS_SUCCESS;
    }

    NTSTATUS Put(__in BOOLEAN Value) override
    {
        if (Value == TRUE)
        {
            Append(L"TRUE");
            KTestPrintf("TRUE");
        }
        else
        {
            Append(L"FALSE");
            KTestPrintf("FALSE");
        }
        return STATUS_SUCCESS;
    }


private:
    void
    GuidToUChars(GUID& Guid, WCHAR* Result)
    {
        struct HexPair
        {
            static VOID
            ToAscii(UCHAR Value, WCHAR* OutputPtr)
            {
                static WCHAR        printableChars[] = L"0123456789ABCDEF";

                OutputPtr[0] = printableChars[((Value & 0xF0) >> 4)];
                OutputPtr[1] = printableChars[(Value & 0x0F)];
            }
        };

        Result[0] = '{';

        HexPair::ToAscii((UCHAR)(Guid.Data1 >> 24), &Result[1+0]);
        HexPair::ToAscii((UCHAR)(Guid.Data1 >> 16), &Result[1+2]);
        HexPair::ToAscii((UCHAR)(Guid.Data1 >> 8), &Result[1+4]);
        HexPair::ToAscii((UCHAR)(Guid.Data1 >> 0), &Result[1+6]);
        Result[1+8] = '-';

        HexPair::ToAscii((UCHAR)(Guid.Data2 >> 8), &Result[10+0]);
        HexPair::ToAscii((UCHAR)(Guid.Data2 >> 0), &Result[10+2]);
        Result[10+4] = '-';

        HexPair::ToAscii((UCHAR)(Guid.Data3 >> 8), &Result[15+0]);
        HexPair::ToAscii((UCHAR)(Guid.Data3 >> 0), &Result[15+2]);
        Result[15+4] = '-';

        HexPair::ToAscii(Guid.Data4[0], &Result[20+0]);
        HexPair::ToAscii(Guid.Data4[1], &Result[20+2]);
        Result[20+4] = '-';

        HexPair::ToAscii(Guid.Data4[2], &Result[25+0]);
        HexPair::ToAscii(Guid.Data4[3], &Result[25+2]);
        HexPair::ToAscii(Guid.Data4[4], &Result[25+4]);
        HexPair::ToAscii(Guid.Data4[5], &Result[25+6]);
        HexPair::ToAscii(Guid.Data4[6], &Result[25+8]);
        HexPair::ToAscii(Guid.Data4[7], &Result[25+10]);

        Result[25+12] = '}';
        Result[25+13] = 0;
    }
};

BOOLEAN
IsEqual(const KIDomNode::QName& Name0, const KIDomNode::QName& Name1)
{
    if ((Name0.Namespace == Name1.Namespace) && (Name0.Name == Name1.Name))
    {
        return TRUE;
    }

    return ((wcscmp(Name0.Namespace, Name1.Namespace) == 0) && (wcscmp(Name0.Name, Name1.Name) == 0));
}

BOOLEAN
IsEqual(KVariant& V0, KVariant& V1)
{
    if (V0.Type() != V1.Type())
    {
        return FALSE;
    }

    switch (V0.Type())
    {
        case KVariant::KVariantType::Type_GUID:
        {
            GUID        v0 = Ktl::Move(V0);
            GUID        v1 = Ktl::Move(V1);
            return memcmp(&v0, &v1, sizeof(GUID)) == 0;
        }

        case KVariant::KVariantType::Type_LONG:
            return (LONG)V0 == (LONG)V1;

        case KVariant::KVariantType::Type_ULONG:
            return (ULONG)V0 == (ULONG)V1;

        case KVariant::KVariantType::Type_ULONGLONG:
            return (ULONGLONG)V0 == (ULONGLONG)V1;

        case KVariant::KVariantType::Type_LONGLONG:
            return (LONGLONG)V0 == (LONGLONG)V1;

        case KVariant::KVariantType::Type_KDateTime:
            return (KDateTime) V0 == (KDateTime) V1;

        case KVariant::KVariantType::Type_KDuration:
            return (KDuration) V0 == (KDuration) V1;

        case KVariant::KVariantType::Type_BOOLEAN:
            return (BOOLEAN) V0 == (BOOLEAN) V1;

        case KVariant::KVariantType::Type_KString_SPtr:
            return *((KString::SPtr) V0) == *((KString::SPtr) V1);

        default:
            return V0.Type() == V1.Type();
    }
}

NTSTATUS
CommonValidateDomNode(KIDomNode::SPtr& Node, const KIDomNode::QName& Name, KVariant& Value)
{
    if (!IsEqual(Name, Node->GetName()))
    {
        return STATUS_OBJECT_NAME_INVALID;
    }
    Value = Node->GetValue();
    return STATUS_SUCCESS;
}

template<typename T>
NTSTATUS
ValidateDomNode(KIDomNode::SPtr& Node, const KIDomNode::QName& Name, T ValidValue)
{
    KVariant            value;
    NTSTATUS            status = CommonValidateDomNode(Node, Name, value);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return ((T)value == ValidValue) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
ValidateDomNode(KIDomNode::SPtr& Node, const KIDomNode::QName& Name, WCHAR* ValidValue)
{
    KVariant            value;
    NTSTATUS            status = CommonValidateDomNode(Node, Name, value);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return (wcscmp(ValidValue, (WCHAR*)value) == 0) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
Compare(KIDomNode::SPtr Dom0, KIDomNode::SPtr Dom1)
{
    NTSTATUS        status;

    if (!IsEqual(Dom0->GetName(), Dom1->GetName()))
    {
        return STATUS_OBJECT_NAME_INVALID;
    }

    if (!IsEqual(Dom0->GetValue(), Dom1->GetValue()))
    {
        return STATUS_DATA_ERROR;
    }

    KArray<KIDomNode::QName>    dom0AttrNames(KtlSystem::GlobalNonPagedAllocator());
    status = Dom0->GetAllAttributeNames(dom0AttrNames);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    {
        KArray<KIDomNode::QName>    dom1AttrNames(KtlSystem::GlobalNonPagedAllocator());
        status = Dom1->GetAllAttributeNames(dom1AttrNames);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (dom0AttrNames.Count() != dom1AttrNames.Count())
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    for (ULONG ix = 0; ix < dom0AttrNames.Count(); ix++)
    {
        KVariant        dom0Value;
        status = Dom0->GetAttribute(dom0AttrNames[ix], dom0Value);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        KVariant        dom1Value;
        status = Dom1->GetAttribute(dom0AttrNames[ix], dom1Value);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if (!IsEqual(dom0Value, dom1Value))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    KArray<KIDomNode::SPtr>     dom0Children(KtlSystem::GlobalNonPagedAllocator());
    status = Dom0->GetAllChildren(dom0Children);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KArray<KIDomNode::SPtr>     dom1Children(KtlSystem::GlobalNonPagedAllocator());
    status = Dom1->GetAllChildren(dom1Children);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (dom0Children.Count() != dom1Children.Count())
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG ix = 0; ix < dom0Children.Count(); ix++)
    {
        status = Compare(dom0Children[ix], dom1Children[ix]);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DefaultedTypesTest()
{

    KAllocator&                 allocator = KtlSystem::GlobalNonPagedAllocator();
    NTSTATUS                    status;
    KIMutableDomNode::SPtr      domRoot;

    KStringView Src(L"<Root  cc=\"xxx\"><aaa>100</aaa><bbb>200</bbb><ddd/><eee type=\"ktl:STRING\"/><fff type=\"ktl:STRING\"></fff></Root>");

    KIMutableDomNode::SPtr Test;
    status = KDom::FromString(Src, allocator, Test);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Unable to convert to Dom from string on line %u\n", __LINE__);
        return status;
    }

    KVariant aVal;
    status = Test->GetChildValue(KIDomNode::QName(L"aaa"), aVal);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    if (((KStringView&)aVal).Compare(KStringView(L"100")) != 0)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    KVariant bVal;
    status = Test->GetChildValue(KIDomNode::QName(L"bbb"), bVal);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    if (((KStringView&)bVal).Compare(KStringView(L"200")) != 0)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }


    KVariant cVal;
    status = Test->GetAttribute(KIDomNode::QName(L"cc"), cVal);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    if (((KStringView&)cVal).Compare(KStringView(L"xxx")) != 0)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    KVariant dVal;
    status = Test->GetChildValue(KIDomNode::QName(L"ddd"), dVal);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    if (dVal.Type() != KVariant::Type_KString_SPtr)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (KString::SPtr(dVal)->Length() != 0)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    KVariant eVal;
    status = Test->GetChildValue(KIDomNode::QName(L"eee"), eVal);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    if (eVal.Type() != KVariant::Type_KString_SPtr)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (KString::SPtr(eVal)->Length() != 0)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    KVariant fVal;
    status = Test->GetChildValue(KIDomNode::QName(L"fff"), fVal);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return status;
    }

    if (fVal.Type() != KVariant::Type_KString_SPtr)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (KString::SPtr(fVal)->Length() != 0)
    {
        KTestPrintf("DefaultedTypesTest failure on line %u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}

NTSTATUS
DomPathTests()
{
    KAllocator&                 allocator = KtlSystem::GlobalNonPagedAllocator();
    NTSTATUS                    status;
    KIMutableDomNode::SPtr      domRoot;

    KStringView Src(L"<Root TestAttr=\"xxx\"><ns2:aaa ns:foo=\"jjj\">100</ns2:aaa><bbb>200</bbb><bbb>201</bbb><bbb>202</bbb><bbb><xxx>1000</xxx><xxx>2000</xxx><xxx>3000</xxx></bbb><ccc/><ddd><eee>abc</eee><fff z=\"123\">afaaf</fff></ddd><bbb>final</bbb></Root>");

    KIMutableDomNode::SPtr Test;
    status = KDom::FromString(Src, allocator, Test);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Unable to convert to Dom from string on line %u\n", __LINE__);
        return status;
    }


    KVariant v;
    status = Test->GetValue(KDomPath(L"Root/ddd/eee"), v);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (v.Type() != KVariant::Type_KString_SPtr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (((KStringView&)v).Compare(KStringView(L"abc")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = Test->GetValue(KDomPath(L"Root/@TestAttr"), v);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (v.Type() != KVariant::Type_KString_SPtr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (((KStringView&)v).Compare(KStringView(L"xxx")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = Test->GetValue(KDomPath(L"Root/ns2:aaa/@ns:foo"), v);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (v.Type() != KVariant::Type_KString_SPtr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (((KStringView&)v).Compare(KStringView(L"jjj")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    status = Test->GetValue(KDomPath(L"Root/ddd/fff/@z"), v);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (v.Type() != KVariant::Type_KString_SPtr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (((KStringView&)v).Compare(KStringView(L"123")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Check the symbolic array access
    //
    status = Test->GetValue(KDomPath(L"Root/bbb[]", 1), v);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (v.Type() != KVariant::Type_KString_SPtr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (static_cast<KStringView&>(v).Compare(KStringView(L"201")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = Test->GetValue(KDomPath(L"Root/bbb[]", 2), v);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    if (v.Type() != KVariant::Type_KString_SPtr)
    {
        return STATUS_UNSUCCESSFUL;
    }
    if (((KStringView&)v).Compare(KStringView(L"202")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Check symbolic array access in two dimensions
    //
    status = Test->GetValue(KDomPath(L"Root/bbb[]/xxx[]", 3, 1), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"2000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Check for literal array access
    //
    status = Test->GetValue(KDomPath(L"Root/bbb[]/xxx[0]", 3), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"1000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = Test->GetValue(KDomPath(L"Root/bbb[3]/xxx[1]"), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"2000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    // Check for out of bounds array access
    //
    status = Test->GetValue(KDomPath(L"Root/bbb[]", 5), v);
    if (NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }

    // Test the counts
    ULONG Count = 0;
    status = Test->GetCount(KDomPath(L"Root/bbb[3]/xxx"), Count);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (Count != 3)
    {
        return STATUS_INTERNAL_ERROR;
    }

    Count = 0;
    status = Test->GetCount(KDomPath(L"Root/bbb"), Count);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (Count != 5)
    {
        return STATUS_INTERNAL_ERROR;
    }

    Count = 0;
    status = Test->GetCount(KDomPath(L"Root/bbb[3]/xxx[100]"), Count);
    if (NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (Count != 0)
    {
        return STATUS_INTERNAL_ERROR;
    }

    Count = 0;
    status = Test->GetCount(KDomPath(L"Root/bbb[3]/xxx[0]"), Count);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (Count != 1)
    {
        return STATUS_INTERNAL_ERROR;
    }


    Count = 0;
    status = Test->GetCount(KDomPath(L"Root/@TestAttr"), Count);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (Count != 1)
    {
        return STATUS_INTERNAL_ERROR;
    }

    // Now get a subnode and test subaccess to it.

    KIDomNode::SPtr BNode;
    status = Test->GetNode(KDomPath(L"Root/bbb[3]"), BNode);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }

    status = BNode->GetValue(KDomPath(L"bbb/xxx[1]"), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"2000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    KIDomNode::SPtr XNode;
    status = Test->GetNode(KDomPath(L"Root/bbb[3]/xxx[1]"), XNode);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }

    status = XNode->GetValue(KDomPath(L"xxx"), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"2000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Get multiple nodes

    KArray<KIDomNode::SPtr> ResultSet(allocator);
    Test->GetNodes(KDomPath(L"Root/bbb[3]/xxx"), ResultSet);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (ResultSet.Count() != 3)
    {
        return STATUS_INTERNAL_ERROR;
    }

    status = ResultSet[0]->GetValue(KDomPath(L"xxx"), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"1000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = ResultSet[1]->GetValue(KDomPath(L"xxx"), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"2000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    status = ResultSet[2]->GetValue(KDomPath(L"xxx"), v);
    if (!NT_SUCCESS(status))
    {
        return STATUS_INTERNAL_ERROR;
    }
    if (((KStringView&)v).Compare(KStringView(L"3000")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    KString::SPtr Str;
    status = ResultSet[2]->GetPath(Str);
    if (!NT_SUCCESS(status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Str->Compare(KStringView(L"Root/bbb[3]/xxx[2]")) != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}




NTSTATUS
ExtendedTypesTest()
{
    // Create a DOM object.

    KAllocator&                 allocator = KtlSystem::GlobalNonPagedAllocator();
    NTSTATUS                    status;
    KIMutableDomNode::SPtr      domRoot;

    //* Prove empty dom can be created

    status = KDom::CreateEmpty(domRoot, allocator, KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: CreateEmpty failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    status = domRoot->SetName(KIMutableDomNode::QName(L"RootElement"));
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetName failed @%u with 0x%08X\n", __LINE__, status);
        return STATUS_UNSUCCESSFUL;
    }

    /// Set up a simple ULONG

    KIMutableDomNode::SPtr  UlongChild;
    status = domRoot->AddChild(KIDomNode::QName(L"UlongVal"), UlongChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    status = UlongChild->SetValue(KVariant(ULONG(123)));
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    // Set up an "empty" element which should come back as a string of zero length.
    //
    KIMutableDomNode::SPtr EmptyChild;
    status = domRoot->AddChild(KIDomNode::QName(L"EmptyVal"), EmptyChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    status = EmptyChild->SetValue(KVariant());
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }


    // BOOLEAN

    KIMutableDomNode::SPtr  boolChild;
    status = domRoot->AddChild(KIDomNode::QName(L"BoolVal"), boolChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    status = boolChild->SetValue(KVariant(BOOLEAN(TRUE)));
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }


    // Type EMPTY
    KIMutableDomNode::SPtr nullChild;
    status = domRoot->AddChild(KIDomNode::QName(L"nullVal"), nullChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    status = nullChild->SetValue(KVariant());
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    KVariant vOtherNull = nullChild->GetValue();
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: GetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    if (vOtherNull.Type() != KVariant::Type_EMPTY)
    {
        KTestPrintf("ExtendedTypesTest: GetValue failed @%u with 0x%08X\n", __LINE__, status);
        return STATUS_UNSUCCESSFUL;
    }

    // KString

    KIMutableDomNode::SPtr  strChild;
    status = domRoot->AddChild(KIDomNode::QName(L"StringVal"), strChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    KString::SPtr Tmp = KString::Create(L"MYSTRING", allocator);
    if (!Tmp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    status = strChild->SetValue(KVariant(Tmp));
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    /// Duration

    KIMutableDomNode::SPtr  durationChild;
    status = domRoot->AddChild(KIDomNode::QName(L"DurationVal"), durationChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    KDuration Dur;
    Dur.SetSeconds(90);

    status = durationChild->SetValue(KVariant(Dur));
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    // Uri

    KIMutableDomNode::SPtr  UriChild;
    status = domRoot->AddChild(KIDomNode::QName(L"UriVal"), UriChild);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    KUri::SPtr Uri;
    status = KUri::Create(KStringView(L"http://www.google.com"), allocator, Uri);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Uri Create failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    status = UriChild->SetValue(KVariant(Uri));
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    // Clone it

    KIMutableDomNode::SPtr  cloned;
    KIDomNode::SPtr Root = domRoot.RawPtr();

    status = KDom::CloneAs(Root, cloned);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    KString::SPtr Text;
    status = KDom::ToString(Root, allocator, Text);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Unable to convert Dom to string on line %u\n", __LINE__);
        return status;
    }

     LPWSTR Str1 = (*Text);
#if !defined(PLATFORM_UNIX)
     KTestPrintf("First XML is\n%S\n", Str1);
#else
    KTestPrintf("First XML is\n%s\n", Utf16To8(Str1).c_str());
#endif

    // Now force a destruct of the original dom.

    Root = 0;
	KFatal(domRoot.Reset());


    KIMutableDomNode::SPtr Phoenix;
    status = KDom::FromString(*Text, allocator, Phoenix);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Unable to convert to Dom from string on line %u\n", __LINE__);
        return status;
    }

    // Convert the new dom to a string too

    KString::SPtr Text2;
    status = KDom::ToString(Phoenix, allocator, Text2);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Unable to convert Dom to string on line %u\n", __LINE__);
        return status;
    }

    // LPWSTR Str2 = (*Text2);
    // KTestPrintf("Second XML is\n%S\n", Str2);



    // All we have left is the clone.  Make sure the values are legit.

    KVariant BoolRetrieved;
    status = Phoenix->GetChildValue(KIDomNode::QName(L"BoolVal"), BoolRetrieved);
    if (!NT_SUCCESS(status) || BOOLEAN(BoolRetrieved) != TRUE)
    {
        KTestPrintf("ExtendedTypesTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    KVariant DurRetrieved;
    status = Phoenix->GetChildValue(KIDomNode::QName(L"DurationVal"), DurRetrieved);
    if (!NT_SUCCESS(status) || KDuration(DurRetrieved) != KDuration(Dur))
    {
        KTestPrintf("ExtendedTypesTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    // Retrieve the URI
    KVariant UriRetrieved;
    status = Phoenix->GetChildValue(KIDomNode::QName(L"UriVal"), UriRetrieved);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
        return status;
    }

    if (((KStringView&)UriRetrieved).Compare(KStringView(L"http://www.google.com")) != 0)
    {
        KTestPrintf("ExtendedTypesTest: URI was not correctly retrieved @%u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (!((KStringView&)UriRetrieved).IsNullTerminated())
    {
        KTestPrintf("ExtendedTypesTest: URI was not null terminated @%u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    KVariant StrRetrieved;
    status = Phoenix->GetChildValue(KIDomNode::QName(L"StringVal"), StrRetrieved);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
        return STATUS_UNSUCCESSFUL;
    }

    KString::SPtr StrV = (KString::SPtr) StrRetrieved;
    if (StrV->Compare(KStringView(L"MYSTRING")) != 0)
    {
        KTestPrintf("ExtendedTypesTest: GetChildValue failed @%u; string was changed\n", __LINE__);
        return status;
    }

    if (!StrV->IsNullTerminated())
    {
        KTestPrintf("ExtendedTypesTest: Missing null terminator @%u\n", __LINE__);
        return status;
    }

    // Get the "Empty" value

    KVariant EmptyValue;
    status = Phoenix->GetChildValue(KIDomNode::QName(L"EmptyVal"), EmptyValue);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Empty value was not correctly retrieved @%u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (EmptyValue.Type() != KVariant::Type_KString_SPtr)
    {
        KTestPrintf("ExtendedTypesTest: Empty value was not correctly retrieved @%u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    // Verify nullChild came back as string
    KVariant nullRetrieval;

    status = Phoenix->GetChildValue(KIDomNode::QName(L"nullVal"), nullRetrieval);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("ExtendedTypesTest: Empty value was not correctly retrieved @%u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    if (nullRetrieval.Type() != KVariant::Type_KString_SPtr)
    {
        KTestPrintf("ExtendedTypesTest: Empty value was not correctly retrieved @%u\n", __LINE__);
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}


//**
NTSTATUS
CreateEmptyDomTest()
{
    {
        KAllocator&                 allocator = KtlSystem::GlobalNonPagedAllocator();
        NTSTATUS                    status;
        KIMutableDomNode::SPtr      domRoot;

        //* Prove empty dom can be created
        status = KDom::CreateEmpty(domRoot, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CreateEmpty failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //* Prove IsMutable and ToMutableForm
        if (!domRoot->IsMutable())
        {
            KTestPrintf("CreateEmptyDomTest: IsMutatable failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (domRoot->ToMutableForm().RawPtr() != domRoot.RawPtr())
        {
            KTestPrintf("CreateEmptyDomTest: ToMutatableForm failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove setting the name on the root fails
        status = domRoot->SetName(KIMutableDomNode::QName(L"RootElement"));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetName failed @%u with 0x%08X\n", __LINE__, status);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove attrs can be added
        status = domRoot->SetAttribute(KIMutableDomNode::QName(L"version"), KVariant::Create(L"1.0", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->SetAttribute(KIMutableDomNode::QName(L"encoding"), KVariant::Create(L"UTF-8", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //* Prove attr names can be queried and are correct
        KArray<KIDomNode::QName>    attrs(allocator);
        status = domRoot->GetAllAttributeNames(attrs);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAllAttributeNames failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (attrs.Count() != 2)
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        BOOLEAN     sawVersion = FALSE;
        BOOLEAN     sawEncoding = FALSE;

        for (ULONG ix = 0; ix < attrs.Count(); ix++)
        {
            if (wcscmp(attrs[ix].Namespace, L"") != 0)
            {
                KTestPrintf("CreateEmptyDomTest: invalid namespace value @%u\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            sawEncoding = sawEncoding || (wcscmp(attrs[ix].Name, L"encoding") == 0);
            sawVersion = sawVersion || (wcscmp(attrs[ix].Name, L"version") == 0);
        }

        if (!sawVersion || !sawEncoding)
        {
            KTestPrintf("CreateEmptyDomTest: missing attr name @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove attr values can be queried and are correct
        KVariant        value;
        status = domRoot->GetAttribute(KIDomNode::QName(L"version"), value);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (wcscmp((WCHAR*)value, L"1.0") != 0)
        {
            KTestPrintf("CreateEmptyDomTest: invalid attr value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = domRoot->GetAttribute(KIDomNode::QName(L"encoding"), value);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (wcscmp((WCHAR*)value, L"UTF-8") != 0)
        {
            KTestPrintf("CreateEmptyDomTest: invalid attr value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //** Prove various children nodes can be added with various value types
        KIMutableDomNode::SPtr      child;
        status = domRoot->AddChild(KIDomNode::QName(L"child001"), child);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove attr and value can be added
        status = child->SetAttribute(KIDomNode::QName(L"testns", L"TestAttr"), KVariant::Create(L"Test Attr Value", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = child->SetValue(KVariant((LONG)-15));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->AddChild(KIDomNode::QName(L"child002"), child);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove attr and value can be added
        status = child->SetValue(KVariant((ULONGLONG)1500000));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove a null string value can be added
        KIMutableDomNode::SPtr      nullChild;
        status = domRoot->AddChild(KIDomNode::QName(L"nullChild"), nullChild);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = nullChild->SetValue(KVariant::Create(L"", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIDomNode::SPtr      nullChild1;
        status = domRoot->GetChild(KIDomNode::QName(L"nullChild"), nullChild1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (nullChild1->GetValue().Type() != KVariant::KVariantType::Type_KString_SPtr)
        {
            KTestPrintf("CreateEmptyDomTest: invalid KVariant type value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //** Prove that children and their values and attrs are correct
        KArray<KIDomNode::SPtr>     children(allocator);
        status = domRoot->GetAllChildren(children);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAllChildren failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (children.Count() != 3)
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove indexed GetChild fails as it should
        KIDomNode::SPtr         tChild;
        KIMutableDomNode::SPtr  tmChild;
        status = domRoot->GetChild(child->GetName(), tChild, 1);
        if (status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            KTestPrintf("CreateEmptyDomTest: indexed GetChild succeeded incorrectly@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        // prove positional insertion, retrieval, and value setting of new child and works
        status = domRoot->AddChild(child->GetName(), tmChild, 0);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KVariant firstDupValue = KVariant::Create(L"Value text for first child-02 element", allocator);
        status = domRoot->SetChildValue(child->GetName(), firstDupValue, 0);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetChild(child->GetName(), tChild, 0);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (tChild.RawPtr() != tmChild.RawPtr())
        {
            KTestPrintf("CreateEmptyDomTest: indexed GetChild or AddChild failed@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (wcscmp((WCHAR*)firstDupValue, (WCHAR*)(tChild->GetValue())) != 0)
        {
            KTestPrintf("CreateEmptyDomTest: value compare failed@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove a child can be appended (both forms)
        status = domRoot->AddChild(child->GetName(), tmChild);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KVariant thirdDupValue = KVariant::Create(L"Value text for 3rd child-02 element", allocator);
        status = domRoot->SetChildValue(child->GetName(), thirdDupValue, 2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetChild(child->GetName(), tChild, 2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (tChild.RawPtr() != tmChild.RawPtr())
        {
            KTestPrintf("CreateEmptyDomTest: indexed GetChild or AddChild failed@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (!tmChild->GetValueKtlType())
        {
            KTestPrintf("CreateEmptyDomTest: invalid GetValueKtlType failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = tmChild->SetValueKtlType(FALSE);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetValueKtlType failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (wcscmp((WCHAR*)thirdDupValue, (WCHAR*)(tChild->GetValue())) != 0)
        {
            KTestPrintf("CreateEmptyDomTest: value compare failed@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = domRoot->AppendChild(child->GetName(), tmChild);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AppendChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KVariant lastDupValue = KVariant::Create(L"Value text for last child-02 element", allocator);
        status = domRoot->SetChildValue(child->GetName(), lastDupValue, 3);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetChild(child->GetName(), tChild, 3);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (tChild.RawPtr() != tmChild.RawPtr())
        {
            KTestPrintf("CreateEmptyDomTest: indexed GetChild or AddChild failed@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (wcscmp((WCHAR*)lastDupValue, (WCHAR*)(tChild->GetValue())) != 0)
        {
            KTestPrintf("CreateEmptyDomTest: value compare failed@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //* Prove dup children are all present
        status = domRoot->GetChildren(child->GetName(), children);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (children.Count() != 4)
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

		//* Prove indexed last form works
		KIDomNode::SPtr      lastChild;
        status = domRoot->GetChild(KIDomNode::QName(L"child002"), lastChild, -1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = ValidateDomNode(lastChild, KIDomNode::QName(L"child002"), (WCHAR*)lastDupValue);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ValidateDomNode failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		KVariant lastValue;
		status = domRoot->GetChildValue(KIDomNode::QName(L"child002"), lastValue, -1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		if (wcscmp((WCHAR*)lastDupValue, (WCHAR*)lastValue) != 0)
		{
            KTestPrintf("CreateEmptyDomTest: invalid value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
		}

        //* Prove an out-of-range (indexed) SetChildValue call fails
        status = domRoot->SetChildValue(child->GetName(), lastDupValue, 7);
        if (status != STATUS_OBJECT_NAME_NOT_FOUND)
        {
            KTestPrintf("CreateEmptyDomTest: indexed SetChildValue succeeded incorrectly@%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

		//* Prove set of child value using last index form
        KVariant newLastDupValue = KVariant::Create(L"Updated: Value text for last child-02 element", allocator);
        status = domRoot->SetChildValue(child->GetName(), newLastDupValue, -1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		status = domRoot->GetChildValue(KIDomNode::QName(L"child002"), lastValue, -1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		if (wcscmp((WCHAR*)newLastDupValue, (WCHAR*)lastValue) != 0)
		{
            KTestPrintf("CreateEmptyDomTest: invalid value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
		}

		//* General validate
        KIDomNode::SPtr      child1;
        status = domRoot->GetChild(KIDomNode::QName(L"child001"), child1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = ValidateDomNode(child1, KIDomNode::QName(L"child001"), (LONG)-15);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ValidateDomNode failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetChild(KIDomNode::QName(L"child002"), child1, 1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = ValidateDomNode(child1, KIDomNode::QName(L"child002"), (ULONGLONG)1500000);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ValidateDomNode failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove a textual stream can be generated from the constructed DOM (with nested children)
        status = domRoot->AddChild(KIDomNode::QName(L"child003"), child);      // Make container element
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }
        status = child->SetAttribute(KIDomNode::QName(L"testns", L"TestAttr"), KVariant::Create(L"Attr on parent element", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr    child2;
        status = child->AddChild(KIDomNode::QName(L"child003-A"), child2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = child2->SetAttribute(KIDomNode::QName(L"testns", L"TestAttr"), KVariant::Create(L"Next Test Attr Value", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KGuid           guid;
        guid.CreateNew();

        status = child2->SetValue(KVariant(guid));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr    child3;
        status = child->AddChild(KIDomNode::QName(L"child003-B"), child3);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr    child3BA;
        status = child3->AddChild(KIDomNode::QName(L"child003-BA"), child3BA);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr    child3BAA;
        status = child3BA->AddChild(KIDomNode::QName(L"child003-BAA"), child3BAA);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = child3BAA->SetValue(KVariant::Create(L"Test Text Value String for 3BAA", allocator));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        OutputStream outStream;

        status = KDom::ToStream(domRoot, outStream);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove DOM can be loaded from produced stream and is the same as the original DOM
        KIMutableDomNode::SPtr  loadedDom;

        status = KDom::CreateFromBuffer(outStream.GetBuffer(), allocator, KTL_TAG_TEST, loadedDom);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CreateFromBuffer failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

    // Dump both to screen

    KString::SPtr Text1;
    status = KDom::ToString(domRoot, allocator, Text1);

    KString::SPtr Text2;
    status = KDom::ToString(loadedDom, allocator, Text2);

    // Uncomment for a visual check on the two DOMs
    // printf("DOM1=\n%S\n\n", LPWSTR(*Text1));
    // printf("DOM2=\n%S\n\n", LPWSTR(*Text2));


        status = Compare(loadedDom.RawPtr(), domRoot.RawPtr());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        OutputStream    outStream1;
        status = KDom::ToStream(KIDomNode::SPtr(domRoot.RawPtr()), outStream1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove KDom::UnicodeOutputStream works
        KDom::UnicodeOutputStream::SPtr     uioStream;
        status = KDom::CreateOutputStream(uioStream, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CreateOutputStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = KDom::ToStream(domRoot, *uioStream);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr  loadedDom1;
        KBuffer::SPtr           uioStreamBuffer = uioStream->GetBuffer();

        status = uioStreamBuffer->SetSize(uioStream->GetStreamSize(), TRUE);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: SetSize failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = KDom::CreateFromBuffer(uioStreamBuffer, allocator, KTL_TAG_TEST, loadedDom1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CreateFromBuffer failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = Compare(loadedDom1.RawPtr(), domRoot.RawPtr());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //** Prove read-only DOM construction
        KIDomNode::SPtr  loadedRODom;
        status = KDom::CreateReadOnlyFromBuffer(outStream1.GetBuffer(), allocator, KTL_TAG_TEST, loadedRODom);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CreateFromBuffer failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //* Prove CloneAs
        KIDomNode::SPtr  clonedRODom;
        status = KDom::CloneAs(loadedRODom, clonedRODom);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (clonedRODom->IsMutable())
        {
            KTestPrintf("CreateEmptyDomTest: IsMutatable failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        OutputStream    outStream2;
        status = KDom::ToStream(KIDomNode::SPtr(clonedRODom.RawPtr()), outStream2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = Compare(clonedRODom.RawPtr(), loadedRODom.RawPtr());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr  clonedDom;
        status = KDom::CloneAs(loadedRODom, clonedDom);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (!clonedDom->IsMutable())
        {
            KTestPrintf("CreateEmptyDomTest: IsMutatable failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = KDom::ToStream(KIDomNode::SPtr(clonedDom.RawPtr()), outStream2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = Compare(clonedDom.RawPtr(), loadedRODom.RawPtr());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        // Prove cloned DOM is mutable
        status = clonedDom->AddChild(KIDomNode::QName(L"child004"), child);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: AddChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = KDom::ToStream(KIDomNode::SPtr(clonedDom.RawPtr()), outStream2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        // And can be cloned again
        KIMutableDomNode::SPtr  clonedDom1;
        status = KDom::CloneAs(KIDomNode::SPtr(clonedDom.RawPtr()), clonedDom1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (!clonedDom1->IsMutable())
        {
            KTestPrintf("CreateEmptyDomTest: IsMutatable failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = KDom::ToStream(KIDomNode::SPtr(clonedDom1.RawPtr()), outStream2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = Compare(clonedDom1.RawPtr(), clonedDom.RawPtr());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        // Prove cloning a sub-tree
        KIDomNode::SPtr     child4;
        status = clonedDom1->GetChild(KIDomNode::QName(L"child003"), child4);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KIMutableDomNode::SPtr  clonedDom2;
        status = KDom::CloneAs(KIDomNode::SPtr(child4.RawPtr()), clonedDom2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = KDom::ToStream(KIDomNode::SPtr(clonedDom2.RawPtr()), outStream2);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        //* Prove !IsMutable and ToMutableForm
        if (loadedRODom->IsMutable())
        {
            KTestPrintf("CreateEmptyDomTest: IsMutatable failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        if (loadedRODom->ToMutableForm().RawPtr() != nullptr)
        {
            KTestPrintf("CreateEmptyDomTest: ToMutatableForm failed @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = Compare(loadedRODom.RawPtr(), domRoot.RawPtr());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		KIMutableDomNode::SPtr		savedRootDom;
		status = KDom::CloneAs(KIDomNode::SPtr(domRoot.RawPtr()), savedRootDom);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		//* Prove last form of delete array works
		status = savedRootDom->DeleteChild(KIDomNode::QName(L"child002"), -1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		status = savedRootDom->GetChildValue(KIDomNode::QName(L"child002"), lastValue, -1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetChildValue failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

		if (wcscmp((WCHAR*)thirdDupValue, (WCHAR*)lastValue) != 0)
		{
            KTestPrintf("CreateEmptyDomTest: invalid value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
		}

		//* Array delete works
        status = domRoot->GetAllChildren(children);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAllChildren failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        ULONG                   childrenBeforeDelete = children.Count();
        KIDomNode::QName        delName(L"child002");

        status = domRoot->DeleteChild(delName);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetAllChildren(children);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAllChildren failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (children.Count() != (childrenBeforeDelete - 1))
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = domRoot->DeleteChild(delName, 1);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetChildren(delName, children);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteChild failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (children.Count() != 2)
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = domRoot->DeleteChildren(delName);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteChildren failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->GetAllChildren(children);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAllChildren failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (children.Count() != (childrenBeforeDelete - 4))
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //** Prove that an api object for a delete node returns STATUS_INVALID_HANDLE
        KVariant        v1;
        status = child1->GetChildValue(delName, v1);
        if (status != STATUS_INVALID_HANDLE)
        {
            KTestPrintf("CreateEmptyDomTest: inmvalid status returned @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        //** Prove basic KDomPipe behaviors
        KDomPipe::SPtr      domPipe;
        status = KDomPipe::Create(domPipe, allocator, KTL_TAG_TEST);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Create failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        KDomPipe::DequeueOperation::SPtr    dQueueOp;
        status = domPipe->CreateDequeueOperation(dQueueOp);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: CreateDequeueOperation failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        class DomPipeRxr : public Synchronizer
        {
        public:
            KIDomNode::SPtr     _Dom;

            /* override */
            void
            OnCompletion(
                __in_opt KAsyncContextBase* const Parent,
                __in KAsyncContextBase& CompletingOperation)
            {
                UNREFERENCED_PARAMETER(Parent);

                if (NT_SUCCESS(CompletingOperation.Status()))
                {
                    _Dom = Ktl::Move(((KDomPipe::DequeueOperation*)&CompletingOperation)->GetDom());
                }
            }
        };

        Synchronizer    deactComp;
        DomPipeRxr      rxComp;
        Synchronizer    txComp;

        status = domPipe->Activate(nullptr, deactComp.AsyncCompletionCallback());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Activate failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        dQueueOp->StartDequeue(nullptr, rxComp.AsyncCompletionCallback());
        status = domPipe->StartWrite(*domRoot, nullptr, txComp.AsyncCompletionCallback());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: StartWrite failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = rxComp.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Rx failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (rxComp._Dom.RawPtr() != domRoot.RawPtr())
        {
            KTestPrintf("CreateEmptyDomTest: Rx failed incorrect dom sptr value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = txComp.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: Tx failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        dQueueOp->Reuse();
        rxComp._Dom = nullptr;
        dQueueOp->StartDequeue(nullptr, rxComp.AsyncCompletionCallback());

        domPipe->Deactivate();
        status = deactComp.WaitForCompletion();
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: deactComp failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = rxComp.WaitForCompletion();
        if (status != K_STATUS_SHUTDOWN_PENDING)
        {
            KTestPrintf("CreateEmptyDomTest: Rx failed incorrect status @%u with 0x%08X\n", __LINE__, status);
            return STATUS_UNSUCCESSFUL;
        }

        if (rxComp._Dom.RawPtr() != nullptr)
        {
            KTestPrintf("CreateEmptyDomTest: Rx failed incorrect dom sptr value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = domPipe->StartWrite(*domRoot, nullptr, txComp.AsyncCompletionCallback());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: StartWrite failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = txComp.WaitForCompletion();
        if (status != K_STATUS_SHUTDOWN_PENDING)
        {
            KTestPrintf("CreateEmptyDomTest: Tx failed incorrect status @%u with 0x%08X\n", __LINE__, status);
            return STATUS_UNSUCCESSFUL;
        }

        KNt::Sleep(500);        // Allow asyncs to finish completion

        //** Prove attributes can be deleted
        status = domRoot->DeleteAttribute(KIDomNode::QName(L"encoding"));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->DeleteAttribute(KIDomNode::QName(L"version"));
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteAttribute failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        status = domRoot->DeleteAttribute(KIDomNode::QName(L"version"));
        if (NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: DeleteAttribute succeeded @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        status = domRoot->GetAllAttributeNames(attrs);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmptyDomTest: GetAllAttributeNames failed @%u with 0x%08X\n", __LINE__, status);
            return status;
        }

        if (attrs.Count() != 0)
        {
            KTestPrintf("CreateEmptyDomTest: invalid Count value @%u\n", __LINE__);
            return STATUS_UNSUCCESSFUL;
        }

        {
            WCHAR               xml1[] = L" <MyDisks xmlns=\"http://schemas.microsoft.com/2012/03/MySchema\"\n"
                                         L"xmlns:ktl=\"http://schemas.microsoft.com/2012/03/ktl\"\n"
                                         L"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                                         L"xmlns:xs=\"http://www.w3.org/2001/XMLSchema\"\n"
                                         L">\n"
                                         L" <Disk>"
                                         L"   <DiskSize xsi:type=\"ktl:ULONGLONG\">123000000</DiskSize>\n"
                                         L"   <FreeSpace xsi:type=\"ktl:ULONGLONG\">87000000</FreeSpace>\n"
                                         L"   <DriveName xsi:type=\"ktl:STRING\">C:</DriveName>\n"
                                         L"   <VolumeLabel xsi:type=\"xs:string\">DRIVEC</VolumeLabel>\n"
                                         L"   <InventoryTimeStamp>2012-03-21T13:37:02Z</InventoryTimeStamp>\n"
                                         L" </Disk>\n"
                                         L" <Disk>\n"
                                         L"   <DiskSize xsi:type=\"ktl:ULONGLONG\">123000000</DiskSize>\n"
                                         L"   <FreeSpace xsi:type=\"ktl:ULONGLONG\">87000000</FreeSpace>\n"
                                         L"   <DriveName xsi:type=\"ktl:STRING\">D:</DriveName>\n"
                                         L"   <VolumeLabel xsi:type=\"xs:string\">DRIVED   </VolumeLabel>\n"
                                         L"   <InventoryTimeStamp>2012-03-21T13:37:02Z</InventoryTimeStamp>\n"
                                         L" </Disk>\n"
                                         L"</MyDisks>\n";

            xml1[0] = KTextFile::UnicodeBom;
            KBuffer::SPtr       buffer;

            status = KBuffer::Create(sizeof(xml1), buffer, allocator, KTL_TAG_TEST);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: Create failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            KMemCpySafe(buffer->GetBuffer(), buffer->QuerySize(), &xml1[0], sizeof(xml1));

            status = KDom::CreateFromBuffer(buffer, allocator, KTL_TAG_TEST, loadedDom);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: CreateFromBuffer failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            OutputStream    outStream3;
            status = KDom::ToStream(KIDomNode::SPtr(loadedDom.RawPtr()), outStream3);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            KIDomNode::SPtr  loadedRODom1;
            status = KDom::CreateReadOnlyFromBuffer(outStream3.GetBuffer(), allocator, KTL_TAG_TEST, loadedRODom1);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: CreateFromBuffer failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = Compare(loadedRODom1.RawPtr(), loadedDom.RawPtr());
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: Compare failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            //** Prove AddChildDom()
            KIMutableDomNode::SPtr      subDom;

            status = KDom::CloneAs(loadedRODom1, subDom);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = subDom->SetAttribute(KIDomNode::QName(L"Dup"), KVariant::Create(L"0", allocator));
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = loadedDom->AddChildDom(*subDom);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: AddChildDom failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = KDom::ToStream(KIDomNode::SPtr(loadedDom.RawPtr()), outStream3);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = KDom::CloneAs(loadedRODom1, subDom);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: CloneAs failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = subDom->SetAttribute(KIDomNode::QName(L"Dup"), KVariant::Create(L"1", allocator));
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: SetAttribute failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = loadedDom->AddChildDom(*subDom, 0);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: AddChildDom failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            KIDomNode::SPtr      subDomRO;
            status = loadedDom->GetChild(loadedRODom1->GetName(), subDomRO, 0);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            KVariant attValue;
            status = subDomRO->GetAttribute(KIDomNode::QName(L"Dup"), attValue);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: GetAttribute failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            if (*((WCHAR*)attValue) != L'1')
            {
                KTestPrintf("CreateEmptyDomTest: invalid attr value @%u\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            status = loadedDom->GetChild(loadedRODom1->GetName(), subDomRO, 1);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: GetChild failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            status = subDomRO->GetAttribute(KIDomNode::QName(L"Dup"), attValue);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: GetAttribute failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }

            if (*((WCHAR*)attValue) != L'0')
            {
                KTestPrintf("CreateEmptyDomTest: invalid attr value @%u\n", __LINE__);
                return STATUS_UNSUCCESSFUL;
            }

            status = KDom::ToStream(KIDomNode::SPtr(loadedDom.RawPtr()), outStream3);
            if (!NT_SUCCESS(status))
            {
                KTestPrintf("CreateEmptyDomTest: ToStream failed @%u with 0x%08X\n", __LINE__, status);
                return status;
            }
        }

        return STATUS_SUCCESS;
    }
}


NTSTATUS
RunTests()
{
    NTSTATUS Res;

    Res = DefaultedTypesTest();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = ExtendedTypesTest();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = DomPathTests();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = CreateEmptyDomTest();
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DomBasicTest(int Argc, WCHAR* Args[])
{
    UNREFERENCED_PARAMETER(Argc);
    UNREFERENCED_PARAMETER(Args);
	
    KtlSystem*  defaultSys;

    EventRegisterMicrosoft_Windows_KTL();
    KtlSystem::Initialize(&defaultSys);
    defaultSys->SetStrictAllocationChecks(TRUE);

    KTestPrintf("DomBasicTest: Starting\n");

    NTSTATUS        status = RunTests();

    KTestPrintf("DomBasicTest: End\n");
    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}
