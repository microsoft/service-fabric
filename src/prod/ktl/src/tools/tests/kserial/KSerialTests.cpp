/*+

Copyright (c) Microsoft Corporation

Module Name:

    KSerialTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KSerialTests.h.
    2. Add an entry to the gs_KuTestCases table in KSerialTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/

#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef PLATFORM_UNIX
#include <unistd.h>
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KSerialTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


#if KTL_USER_MODE
 extern volatile LONGLONG gs_AllocsRemaining;
#endif

KAllocator* g_Alloc = nullptr;



///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test 1 Region, simple object with ULONG/ULONGLONG
//

// {4D2E03B3-379B-41c9-A4D9-264CBCF55707}
static const GUID GUID_Test1 =
{ 0x4d2e03b3, 0x379b, 0x41c9, { 0xa4, 0xd9, 0x26, 0x4c, 0xbc, 0xf5, 0x57, 0x7 } };


// {5A770582-28E6-440b-8FAD-D168AF549D7A}
static const GUID NULL_GUID =
{ 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };


// {E16AD969-6206-4811-A5B2-C0C7B8CD8E31}
static const GUID GUID_TestObject =
{ 0xe16ad969, 0x6206, 0x4811, { 0xa5, 0xb2, 0xc0, 0xc7, 0xb8, 0xcd, 0x8e, 0x31 } };

class TestObject
{
public:
    TestObject() { v1 = 0; v2 = 0; g1 = NULL_GUID; }

    TestObject(ULONGLONG x, ULONG y)
    {
        v1 = x;
        v2 = y;
    }

    ULONGLONG get_v1() const { return v1; }
    ULONG get_v2() const { return v2; }
    GUID& get_g1() { return g1; }
    void set_g1(const GUID g) { g1 = g; }

private:
    ULONGLONG v1;
    GUID g1;
    ULONG v2;

    K_SERIALIZABLE(TestObject);
};

K_SERIALIZE_POD_MESSAGE(TestObject, GUID_TestObject);

K_DESERIALIZE_POD_MESSAGE(TestObject, GUID_TestObject);


NTSTATUS DeserializeThem(KInChannel& In)
{
    TestObject a;
    TestObject b;

    In >> a >> b;
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Deserialization failed.\n");
        return In.Status();
    }

    if (!(a.get_v1() == 3 && a.get_v2() == 5))
    {
        KTestPrintf("Object 'a' failed to deserialize\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (a.get_g1() != GUID_Test1)
    {
        KTestPrintf("GUID failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (!(b.get_v1() == 10 && b.get_v2() == 12))
    {
        KTestPrintf("Object 'b' failed to deserialize\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS Test1_b()
{
    NTSTATUS Result;
    KOutChannel Out(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Failed to allocate Out\n");
        return Out.Status();
    }

    TestObject x(3, 5);
    x.set_g1(GUID_Test1);

    TestObject y(10, 12);

    Out << x << y;
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Serialization failed.\n");
        return Out.Status();
    }

    // Now, transfer blocks to deserializer
    KInChannel In(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Failed to create In\n");
        return In.Status();
    }

    Result = _KIoChannelTransfer(In, Out,KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(Result))
    {
        KTestPrintf("_KIoChannelTransfer error.\n");
        return Result;
    }

    Result = DeserializeThem(In);
    return Result;
}

#if KTL_USER_MODE

NTSTATUS SimpleTest()
{
    NTSTATUS Res;

    for (unsigned i = 0; i < 500; i++)
    {
        Res = Test1_b();
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("Test1_b() FAILED\n");
            return Res;
        }
    }

    return STATUS_SUCCESS;
}

#else

NTSTATUS SimpleTest()
{
    return Test1_b();
}

#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Test2 region

// {F4C4DC5D-8542-4134-B5F3-0E7CE2FCC794}
static const GUID GUID_SampleObject =
{ 0xf4c4dc5d, 0x8542, 0x4134, { 0xb5, 0xf3, 0xe, 0x7c, 0xe2, 0xfc, 0xc7, 0x94 } };

class SampleObject : public KObject<SampleObject>, public KShared<SampleObject>
{
public:
    typedef KSharedPtr<SampleObject> SPtr;

    FAILABLE SampleObject()
        :   _Val3(GetThisAllocator())
    {
        _Val1 = 0;
        _Val2 = 0;
        if (NT_SUCCESS(_Val3.Status()))
        {
            _Val3 = L"<nullstr>";
        }
        else
        {
            KTestPrintf("Failed to allocate _Val3\n");
            SetConstructorStatus(_Val3.Status());
            return;
        }

        NTSTATUS status;

        status = KIoBuffer::CreateEmpty(_IoBuf, KtlSystem::GlobalNonPagedAllocator());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create empty _IoBuf\n");
            SetConstructorStatus(status);
            return;
        }

        status = KBuffer::Create(256, _Buf, KtlSystem::GlobalNonPagedAllocator());
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create _Buf\n");
            SetConstructorStatus(status);
            return;
        }
    }

    NTSTATUS SetValues()
    {
        NTSTATUS status = STATUS_SUCCESS;

        _Val1 = 33;
        _Val2 = 44;
        _Val3 = L"<ExplicitlyAssignedString>";

        // Set KBuffer
        memset(_Buf->GetBuffer(), 'z', 256);

        // Set KIoBuffer
        KIoBufferElement::SPtr El_a;
        VOID* RawMem_a = NULL;
        status = KIoBufferElement::CreateNew(32, El_a, RawMem_a, KtlSystem::GlobalNonPagedAllocator());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create KIoBufferElement a\n");
            return status;
        }
        _IoBuf->AddIoBufferElement(*El_a);
        memset(RawMem_a, 'a', 32);

        // And a second one
        KIoBufferElement::SPtr El_b;
        VOID* RawMem_b = 0;
        status = KIoBufferElement::CreateNew(64, El_b, RawMem_b, KtlSystem::GlobalNonPagedAllocator());
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("Failed to create KIoBufferElement b\n");
            return status;
        }
        _IoBuf->AddIoBufferElement(*El_b);
        memset(RawMem_b, 'b', 64);

        return STATUS_SUCCESS;
    }

    KWString GetStr() { return _Val3; }
    ULONG Get1() { return _Val1; }
    ULONGLONG Get2() { return _Val2; }

    BOOLEAN CheckIt()
    {
        if (_Val1 != 33)
        {
            KTestPrintf("_Val1 value incorrect.\nExpected: 33\nActual: %lu\n", _Val1);
            return FALSE;
        }
        if (_Val2 != 44)
        {
            KTestPrintf("_Val2 value incorrect.\nExpected: 44\nActual: %I64u\n", _Val2);
            return FALSE;
        }
        if (_Val3.CompareTo(L"<ExplicitlyAssignedString>") != 0)
        {
            KTestPrintf("_Val3 value incorrect.\nExpected: <ExplicitlyAssignedString>\n");
            return FALSE;
        }

        if (_Buf->QuerySize() != 256)
        {
            KTestPrintf("_Buf->QuerySize() value incorrect.\nExpected: 256\nActual: %lu\n", _Buf->QuerySize());
            return FALSE;
        }

        const void* memcheck;
        memcheck = _Buf->GetBuffer();

        // Visit the KIoBufferElements

        if (_IoBuf->QueryNumberOfIoBufferElements() != 2)
        {
            KTestPrintf("_IoBuf->QueryNumberOfIoBufferElements() value incorrect.\nExpected: 2\nActual: %lu\n", _IoBuf->QueryNumberOfIoBufferElements());
            return FALSE;
        }

        KIoBufferElement* First = _IoBuf->First();
        if (!First)
        {
            KTestPrintf("_IoBuf->First == nullptr\n");
            return FALSE;
        }

        if (First->QuerySize() != 32)
        {
            KTestPrintf("First->QuerySize() value incorrect.\nExpected: 32\nActual: %lu\n", First->QuerySize());
            return FALSE;
        }

        memcheck = First->GetBuffer();

        KIoBufferElement* Next = _IoBuf->Next(*First);
        if (!Next)
        {
            KTestPrintf("_IoBuf->Next(*First) == nullptr\n");
            return FALSE;
        }

        if (Next->QuerySize() != 64)
        {
            KTestPrintf("Next->QuerySize() value incorrect.\nExpected: 64\nActual: %lu\n", Next->QuerySize());
            return FALSE;
        }

        memcheck = Next->GetBuffer();

        return TRUE;
    }

private:
    ULONG  _Val1;
    ULONGLONG _Val2;
    KWString _Val3;
    KBuffer::SPtr _Buf;
    KIoBuffer::SPtr _IoBuf;

    K_SERIALIZABLE(SampleObject);
};


K_SERIALIZE(SampleObject)
{
    return Out << This._Val1 << This._Val2 << This._Val3 << This._Buf << This._IoBuf;
}

K_DESERIALIZE(SampleObject)
{
    return In >> This._Val1 >> This._Val2 >> This._Val3 >> This._Buf >> This._IoBuf;
}

/////////////

// {DA8A4A2B-43A4-465e-85F6-4DBC00AF9F4E}
static const GUID GUID_Container =
{ 0xda8a4a2b, 0x43a4, 0x465e, { 0x85, 0xf6, 0x4d, 0xbc, 0x0, 0xaf, 0x9f, 0x4e } };

class Container : public KObject<Container>, public KShared<Container>
{
public:
    typedef KSharedPtr<Container> SPtr;

    FAILABLE Container(KAllocator& Alloc) : _Array(Alloc)
    {
        _uVal1 = 0;
        _uVal2 = 0;
        _Samp = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) SampleObject();
        if (!_Samp || !NT_SUCCESS(_Samp->Status()))
        {
            SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
        if (! NT_SUCCESS(_Array.Status()))
        {
            SetConstructorStatus(_Array.Status());
        }
    }

    NTSTATUS SetValues()
    {
        NTSTATUS status = STATUS_SUCCESS;

        _uVal1 = 99;
        _uVal2 = 101;

        status = _Samp->SetValues();
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        status = _Array.Append(1000);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        status = _Array.Append(2000);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        status = _Array.Append(3000);
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        return STATUS_SUCCESS;
    }

    ULONG Get1() const { return _uVal1; }
    ULONG Get2() const { return _uVal2; }
    SampleObject::SPtr GetPtr() { return _Samp; }

    BOOLEAN CheckArray() const
    {
        if (_Array[0] != 1000)
        {
            KTestPrintf("_Array[0] value incorrect.\nExpected: 1000\nActual: %lu\n", _Array[0]);
            return FALSE;
        }

        if (_Array[1] != 2000)
        {
            KTestPrintf("_Array[1] value incorrect.\nExpected: 2000\nActual: %lu\n", _Array[1]);
            return FALSE;
        }

        if (_Array[2] != 3000)
        {
            KTestPrintf("_Array[2] value incorrect.\nExpected: 3000\nActual: %lu\n", _Array[2]);
            return FALSE;
        }

        return TRUE;
    }

private:

    ULONG _uVal1;
    SampleObject::SPtr _Samp;
    KArray<ULONG> _Array;
    ULONG _uVal2;

    K_SERIALIZABLE(Container);
};

K_CONSTRUCT(Container)
{
    UNREFERENCED_PARAMETER(In);

    _This = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) Container(KtlSystem::GlobalNonPagedAllocator());
    if (!_This || !NT_SUCCESS(_This->Status()))
    {
        KTestPrintf("Failed to allocate container\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

K_SERIALIZE(Container)
{
    K_EMIT_MESSAGE_GUID(GUID_Container);

    // Write some metadata
    DWORD Tmp = 33;
    Out.Meta << Tmp;
    Tmp = 99;
    Out.Meta << Tmp;

    return Out << This._uVal1 << This._Samp << This._uVal2 << This._Array;
}

K_DESERIALIZE(Container)
{
    K_VERIFY_MESSAGE_GUID(GUID_Container);

    // Read metadata
    DWORD Tmp = 0;
    In.Meta >> Tmp;
    In.Meta >> Tmp;

    return In >> This._uVal1 >> This._Samp >> This._uVal2 >> This._Array;
}


NTSTATUS SerializeIt(KOutChannel& Out, Container::SPtr sp)
{
    Out << *sp;
    return Out.Status();
}

NTSTATUS DeserializeIt(KInChannel& In, Container::SPtr newObj)
{
    In >> *newObj;
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Deserialization failed.\n");
        return In.Status();
    }

    In._DeferredQ.ResetCursor();
    KMemRef ref;
    while (In._DeferredQ.Next(ref))
    {
        ULONG TotalRead = 0;
        NTSTATUS status = In.Data.Read(ref._Size, ref._Address, &TotalRead);
        if (!NT_SUCCESS(status) || TotalRead != ref._Size)
        {
            KDbgErrorWData(0, "Deserialization failed.\n", status, TotalRead, ref._Size, 0, 0);
            return status;
        }
    }

    return In.Status();
}

NTSTATUS CheckIt(Container::SPtr newObj)
{
    ULONG v1 = newObj->Get1();
    ULONG v2 = newObj->Get2();

    SampleObject::SPtr s = newObj->GetPtr();

    if (s->CheckIt() != TRUE)
    {
        KTestPrintf("Embedded object failure\n");
        return STATUS_UNSUCCESSFUL;
    }

    KWString str = s->GetStr();
    ULONG sv1 = s->Get1();
    ULONGLONG sv2 = s->Get2();

    if (v1 != 99)
    {
        KTestPrintf("CheckIt() failure: v1 value incorrect.\nExpected: 99\nActual: %lu\n", v1);
        return STATUS_UNSUCCESSFUL;
    }

    if (v2 != 101)
    {
        KTestPrintf("CheckIt() failure: v2 value incorrect.\nExpected: 101\nActual: %lu\n", v2);
        return STATUS_UNSUCCESSFUL;
    }

    if (sv1 != 33)
    {
        KTestPrintf("CheckIt() failure: sv1 value incorrect.\nExpected: 33\nActual: %lu\n", sv1);
        return STATUS_UNSUCCESSFUL;
    }

    if (sv2 != 44)
    {
        KTestPrintf("CheckIt() failure: sv2 value incorrect.\nExpected: 44\nActual: %I64u\n", sv2);
        return STATUS_UNSUCCESSFUL;
    }

    if (str.CompareTo(L"<ExplicitlyAssignedString>") != 0)
    {
        KTestPrintf("CheckIt() failure: str value incorrect.\nExpected: <ExplicitlyAssignedString>\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (newObj->CheckArray() == FALSE)
    {
        KTestPrintf("CheckIt() failure: newObj->CheckArray() returned FALSE\n");
        return STATUS_UNSUCCESSFUL;
    }

    KTestPrintf("CheckIt() was successful\n");
    return STATUS_SUCCESS;
}

NTSTATUS ComplexTest()
{
    NTSTATUS status = STATUS_SUCCESS;

    Container::SPtr s = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) Container(KtlSystem::GlobalNonPagedAllocator());
    if (!s || !NT_SUCCESS(s->Status()))
    {
        KTestPrintf("Failed to allocate s\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = s->SetValues();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("s->SetValues() failed\n");
        return status;
    }

    KOutChannel Out(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Failed to allocate Out\n");
        return Out.Status();
    }

    status = SerializeIt(Out, s);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Serialization failed.\n");
        return status;
    }

    KInChannel In(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Failed to allocate In\n");
        return In.Status();
    }

    status = _KIoChannelTransfer(In, Out, KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("_KIoChannelTransfer failure.\n");
        return status;
    }

    // Now check things

    Container::SPtr snew = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) Container(KtlSystem::GlobalNonPagedAllocator());
    if (!snew || !NT_SUCCESS(snew->Status()))
    {
        KTestPrintf("Failed to allocate snew\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = DeserializeIt(In, snew);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("DeserializeIt(In, snew) failed\n");
        return status;
    }

    status = CheckIt(snew);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Checkit(snew) failed\n");
        return status;
    }

    snew = 0;

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// KVariant tests

NTSTATUS KVariantTests()
{
    NTSTATUS status = STATUS_SUCCESS;

    KVariant v1(ULONGLONG(100));
    KVariant v2(LONG(-22));

    KVariant v3(GUID_SampleObject);
    KVariant v4 = KVariant::Create(L"Blah", *g_Alloc);
    if (! NT_SUCCESS(v4.Status()))
    {
        KTestPrintf("Failed to allocate v4\n");
        return v4.Status();
    }

    KOutChannel Out(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Failed to allocate Out\n");
        return Out.Status();
    }

    Out << v1 << v2 << v3 << v4;
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Serialization failed.\n");
        return Out.Status();
    }

    KInChannel In(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Failed to allocate In\n");
        return In.Status();
    }

    status = _KIoChannelTransfer(In, Out, KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("_KIoChannelTransfer failure.\n");
        return status;
    }

    KVariant v10, v11, v12, v13;
    In >> v10 >> v11 >> v12 >> v13;
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Deserialization failed.\n");
        return In.Status();
    }

    if (ULONGLONG(v10) != 100)
    {
        KTestPrintf("v10 value incorrect.\nExpected: 100\nActual: %I64u\n", ULONGLONG(v10));
        return STATUS_UNSUCCESSFUL;
    }

    if (LONG(v11) != -22)
    {
        KTestPrintf("v11 value incorrect.\nExpected: -22\nActual: %lu\n", LONG(v11));
        return STATUS_UNSUCCESSFUL;
    }

    if (GUID(v12) != GUID_SampleObject)
    {
        KTestPrintf("v12 value incorrect GUID\n");
        return STATUS_UNSUCCESSFUL;
    }

    KWString Verify(KtlSystem::GlobalNonPagedAllocator(), L"Blah");
    if (! NT_SUCCESS(Verify.Status()))
    {
        KTestPrintf("Failed to allocate Verify\n");
        return Verify.Status();
    }

    KString::SPtr Tmp = (KString::SPtr) v13;
    if (!Tmp || !NT_SUCCESS(Tmp->Status()))
    {
        KTestPrintf("Failed to allocate Tmp\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Verify.CompareTo(PWSTR(v13)) != 0)
    {
        KTestPrintf("v13 value incorrect\nExpected: Blah\n");
        return STATUS_UNSUCCESSFUL;
    }

    return status;
}

NTSTATUS VirtualNetworkTests()
{
    NTSTATUS status = STATUS_SUCCESS;

    KSerializer::SPtr Sz = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KSerializer();
    if (!Sz || !NT_SUCCESS(Sz->Status()))
    {
        KTestPrintf("Failed to allocate Sz\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Container::SPtr s = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) Container(KtlSystem::GlobalNonPagedAllocator());
    if (!s || !NT_SUCCESS(s->Status()))
    {
        KTestPrintf("Failed to allocate s\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = s->SetValues();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("s->SetValues() failed\n");
        return status;
    }

    // Serialize our object.
    //
    status = Sz->Serialize(*s);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to serialize s\n");
        return status;
    }

    // This acts as a virtual network layer.
    //
    _KTransferAgent Xa(KtlSystem::GlobalNonPagedAllocator());
    if (! NT_SUCCESS(Xa.Status()))
    {
        KTestPrintf("Failed to allocate Xa\n");
        return Xa.Status();
    }

    // Read the the serialized data and copy it.
    //
    status = Xa.Read(*Sz);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Xa.Read(*Sz) failed\n");
        return status;
    }


    // Now all the memory is in the transfer agent.
    // Create a deserializer and feed it.
    //
    KDeserializer::SPtr Dz = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KDeserializer();
    if (!Dz || !NT_SUCCESS(Dz->Status()))
    {
        KTestPrintf("Failed to allocate Dz\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Feed the Meta & Data channels.
    //
    status = Xa.WritePass1(*Dz);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Xa.WritePass1(*Dz) failed\n");
        return status;
    }

    // Now we can deserialize the object.
    //
    Container* p = 0;

    // Deserialize, pass 1.
    //
    status = Dz->Deserialize(NULL, p);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Dz->Deserialize(NULL, p) failed\n");
        return status;
    }

    // Fill in all the deferred blocks
    //
    status = Xa.WritePass2(*Dz);   // Fill in the deferred blocks
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Xa.WritePass2(*Dz) failed\n");
        return status;
    }

    Container::SPtr sp = p;

    status = CheckIt(sp);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("CheckIt(sp) failed\n");
        return status;
    }

    return status;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// TODO: add KBuffer test

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// KIDomNode tests

NTSTATUS KIDomNodeTests()
{
    const WCHAR xml[] =
        {L"\xFEFF<NodeConfig>"
                    L"<NodeId xsi:type=\"ktl:STRING\">0</NodeId>"
                    L"<NodeAddress xsi:type=\"ktl:STRING\">127.0.0.1:9000</NodeAddress>"
                    L"<LeaseAgentAddress xsi:type=\"ktl:STRING\">127.0.0.1:9001</LeaseAgentAddress>"
                    L"<RuntimeServiceAddress xsi:type=\"ktl:STRING\">127.0.0.1:9002</RuntimeServiceAddress>"
                    L"<ClientConnectionAddress xsi:type=\"ktl:STRING\">127.0.0.1:19000</ClientConnectionAddress>"
                    L"<NodeProperties>"
                        L"<Property>"
                            L"<Name xsi:type=\"ktl:STRING\">NodeType</Name>"
                            L"<Value xsi:type=\"ktl:STRING\">SeedNode</Value>"
                        L"</Property>"
                    L"</NodeProperties>"
                L"</NodeConfig>"};

    KAllocator& Alloc = *g_Alloc;

    KBuffer::SPtr       buf;
    NTSTATUS            status = STATUS_SUCCESS;

    HRESULT hr;
    size_t result;
    size_t bufferSize;
    hr = SizeTAdd(wcslen(xml), 1, &result);
    KInvariant(SUCCEEDED(hr));
    hr = SizeTMult(result, sizeof(WCHAR), &bufferSize);
    KInvariant(SUCCEEDED(hr));
    if (bufferSize > ULONG_MAX)
    {
        KTestPrintf("bufferSize > ULONG_MAX\n");
        return STATUS_INVALID_BUFFER_SIZE;
    }

    status = KBuffer::CreateOrCopy(buf, xml, static_cast<ULONG>(bufferSize), Alloc);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("Failed to create buf and copy xml to it\n");
        return status;
    }

    KIDomNode::SPtr dom1;
    KIMutableDomNode::SPtr dom2;

    KString::SPtr str1;
    KString::SPtr str2;

    status = KDom::CreateReadOnlyFromBuffer(buf, Alloc, KTL_TAG_TEST, dom1);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("Failed to create dom1\n");
        return status;
    }

    status = KDom::ToString(dom1, Alloc, str1);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("Failed to convert dom1 to str1\n");
        return status;
    }

    status = KDom::CreateFromBuffer(buf, Alloc, KTL_TAG_TEST, dom2);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("Failed to create dom2 from buf\n");
        return status;
    }

    status = KDom::ToString(dom2, Alloc, str2);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("Failed to convert dom2 to str2\n");
        return status;
    }

    //
    // Serialize then deserialize the objects
    //

    KOutChannel Out(Alloc);
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Failed to allocate Out.\n");
        return Out.Status();
    }

    Out << dom1 << dom2;
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Serialization failed.\n");
        return Out.Status();
    }

    KInChannel In(Alloc);
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Failed to allocate In.\n");
        return In.Status();
    }

    status = _KIoChannelTransfer(In, Out, Alloc);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("_KIoChannelTransfer failure.\n");
        return status;
    }

    KIDomNode::SPtr dom3;
    KIMutableDomNode::SPtr dom4;

    KString::SPtr str3;
    KString::SPtr str4;

    In >> dom3 >> dom4;
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Deserialization failed.\n");
        return In.Status();
    }

    status = KDom::ToString(dom3, Alloc, str3);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KDom::ToString(dom3, Alloc, str3) failed\n");
        return status;
    }

    status = KDom::ToString(dom4, Alloc, str4);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("KDom::ToString(dom4, Alloc, str4) failed\n");
        return status;
    }

    //
    // Compare objects
    //

    if (str1->Compare(*str3) != 0)
    {
        KTestPrintf("str1 != str3\n");
        return STATUS_DATA_ERROR;
    }

    if (str2->Compare(*str4) != 0)
    {
        KTestPrintf("str2 != str4\n");
        return STATUS_DATA_ERROR;
    }

    KIDomNode::SPtr emptyDom1;
    KIMutableDomNode::SPtr emptyDom2;

    Out << emptyDom1 << emptyDom2;
    if (! NT_SUCCESS(Out.Status()))
    {
        KTestPrintf("Serialization failed.\n");
        return Out.Status();
    }

    status = _KIoChannelTransfer(In, Out, Alloc);
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("_KIoChannelTransfer failure.\n");
        return status;
    }

    KIDomNode::SPtr emptyDom3;
    KIMutableDomNode::SPtr emptyDom4;

    In >> emptyDom3 >> emptyDom4;
    if (! NT_SUCCESS(In.Status()))
    {
        KTestPrintf("Deserialization failed.\n");
        return In.Status();
    }

    if (emptyDom3.RawPtr())
    {
        KTestPrintf("emptyDom3 != nullptr\n");
        return STATUS_DATA_ERROR;
    }

    if (emptyDom4.RawPtr())
    {
        KTestPrintf("emptyDom4 != nullptr\n");
        return STATUS_DATA_ERROR;
    }

    return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

NTSTATUS TestSequence()
{
    NTSTATUS Result = 0;

    KTestPrintf("Starting TestSequence...\n");

    KTestPrintf("Running SimpleTest\n");
    Result = SimpleTest();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("SimpleTest failure\n");
        return Result;
    }

    KTestPrintf("Running ComplexTest\n");
    Result = ComplexTest();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("ComplexTest failure\n");
        return Result;
    }

    KTestPrintf("Running KVariantTests\n");
    Result = KVariantTests();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KVariantTests failure\n");
        return Result;
    }

    KTestPrintf("Running VirtualNetworkTests\n");
    Result = VirtualNetworkTests();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("VirtualNetworkTests failure\n");
        return Result;
    }

    KTestPrintf("Running KIDomNodeTests\n");
    Result = KIDomNodeTests();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("KIDomNodeTests failure\n");
        return Result;
    }

    KTestPrintf("SUCCESS\n");
    return STATUS_SUCCESS;
}

NTSTATUS
KSerialTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status;
#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KSerialTest: STARTED\n");

    EventRegisterMicrosoft_Windows_KTL();

    KtlSystem* underlyingSystem;
    status = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("%s: %i: KtlSystem::Initialize failed\n", __FUNCTION__, __LINE__);
        return status;
    }
    KFinally([&](){ KtlSystem::Shutdown(); });

    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);
    g_Alloc = &underlyingSystem->GlobalNonPagedAllocator();
    LONGLONG InitialAllocations = KAllocatorSupport::gs_AllocsRemaining;

    status = TestSequence();

#ifndef PLATFORM_UNIX
    KNt::Sleep(500); // Sleep before checking for leaks
#else
    sleep(0.5);
#endif
    if (KAllocatorSupport::gs_AllocsRemaining != InitialAllocations)
    {
        KTestPrintf("Outstanding allocations\n");
        KtlSystemBase::DebugOutputKtlSystemBases();
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    EventUnregisterMicrosoft_Windows_KTL();
    
    KTestPrintf("KSerialTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return status;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    return KSerialTest(argc,args);
}
#endif
