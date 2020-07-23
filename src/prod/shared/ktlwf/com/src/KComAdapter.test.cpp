// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "WexTestClass.h"
#include "KComAdapter.h"

using namespace Ktl::Com;

namespace ApiTest
{
using ::_delete;

#define VERIFY_REF_COUNT(object, expected) \
    object->AddRef(); \
    VERIFY_ARE_EQUAL((LONG)expected, (LONG)(object->Release()), L""  L## #object L"(" L ## #expected L")");

    class KComAdapterTest : public WEX::TestClass<KComAdapterTest>
    {
    public:
        TEST_CLASS(KComAdapterTest)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(Simple)

        static KAllocator*  _Allocator;
        static KtlSystem*   _System;
    };

    KAllocator* KComAdapterTest::_Allocator = nullptr;
    KtlSystem* KComAdapterTest::_System = nullptr;

    bool KComAdapterTest::Setup()
    {
        NTSTATUS status = STATUS_SUCCESS;

        status = KtlSystem::Initialize(&_System);
        _System->SetStrictAllocationChecks(TRUE);

        if (!NT_SUCCESS(status))
        {
            return false;
        }

        _Allocator = &KtlSystem::GlobalPagedAllocator();

        return true;
    }

    bool KComAdapterTest::Cleanup()
    {
        KtlSystem::Shutdown();

        return true;
    } 

    // Simple GetBytes interface
    interface ITestGetBytes : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE
        GetBytes(__out ULONG* count, __out const BYTE** buffer) = 0;
    };

    EXTERN_C const IID IID_ITestGetBytes = {1,2,3,{4,5,6,7,8,9,10,11}};
    ULONG               TestITestGetBytesInstances = 0;
    
    class TestITestGetBytes : public ITestGetBytes, Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(TestITestGetBytes)
            COM_INTERFACE_ITEM(IID_IUnknown, ITestGetBytes)
            COM_INTERFACE_ITEM(IID_ITestGetBytes, ITestGetBytes)
        END_COM_INTERFACE_LIST()

    public:
        TestITestGetBytes(KStringView& String)
            :   _String(String)
        {
            TestITestGetBytesInstances++;
        }

        ~TestITestGetBytes()
        {
            TestITestGetBytesInstances--;
        }

    private:
        HRESULT STDMETHODCALLTYPE
        ITestGetBytes::GetBytes(__out ULONG* count, __out const BYTE** buffer) override
        {
            *count = _String.LengthInBytes();
            *buffer = (BYTE*)PVOID(_String);
            return S_OK;
        }

    private:
        KStringView             _String;
    };

    
    // Simple IFabricOperationData test imp
    ULONG               TestIFabricOperationDataInstances = 0;
    
    template<ULONG Size>
    class TestIFabricOperationData : public IFabricOperationData, Common::ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(TestIFabricOperationData)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()

    public:
        TestIFabricOperationData(LPCWSTR StringPreamble)
        {
            TestIFabricOperationDataInstances++;

            KStringView         stringPreamble(StringPreamble);

            for (ULONG ix = 0; ix < Size; ix++)
            {
                KLocalString<128>   str;
                KInvariant(str.FromULONG(ix));
                KInvariant(str.Prepend(stringPreamble));
                KInvariant(str.SetNullTerminator());

                UCHAR*              buffer = new UCHAR[str.LengthInBytes() + 2];
                KInvariant(buffer != nullptr);

                KStringView         outStr;
                outStr.Set((PWCHAR)buffer, 0, str.Length() + 1);
                KInvariant(outStr.CopyFrom(str, TRUE));

                _BuffersDescs[ix].BufferSize = str.LengthInBytes() + 2;
                _BuffersDescs[ix].Buffer = (BYTE*)buffer;
            }
        }

        ~TestIFabricOperationData()
        {
            for (ULONG ix = 0; ix < Size; ix++)
            {
                delete _BuffersDescs[ix].Buffer;
            }

            TestIFabricOperationDataInstances--;
        }

        BOOLEAN
        Validate(LPCWSTR StringPreamble, ULONG Ix, LPCWSTR ToCompare)
        {
            KInvariant(Ix < Size);

            KLocalString<128>   str;
            KStringView         stringPreamble(StringPreamble);
            KInvariant(str.FromULONG(Ix));
            KInvariant(str.Prepend(stringPreamble));
            KInvariant(str.SetNullTerminator());

            KStringView         toCompare(ToCompare);

            return str == toCompare;
        }

    private:
        TestIFabricOperationData();

        HRESULT STDMETHODCALLTYPE GetData( 
            /* [out] */ ULONG *count,
            /* [retval][out] */ const FABRIC_OPERATION_DATA_BUFFER **buffers) override
        {
            *count = Size;
            *buffers = &_BuffersDescs[0];

            return S_OK;
        }

    private:
        FABRIC_OPERATION_DATA_BUFFER    _BuffersDescs[Size];
    };


    void KComAdapterTest::Simple()
    {
        LONGLONG        baseKtlAllocs = KAllocatorSupport::gs_AllocsRemaining;

        VERIFY_IS_TRUE(TestIFabricOperationDataInstances == 0);
        VERIFY_IS_TRUE(TestITestGetBytesInstances == 0);

        {
            KStringView                         testStr(L"Test String");

            //* Prove we can alias a COM Object supporting GetBytes() as a KBuffer
            Common::ComPointer<ITestGetBytes>   testPtr;
            testPtr.SetNoAddRef(new TestITestGetBytes(testStr));
            VERIFY_IS_TRUE((testPtr.GetRawPointer() != nullptr));
            VERIFY_IS_TRUE(TestITestGetBytesInstances == 1);

            KBuffer::SPtr           testBfr;
            NTSTATUS                status = KComBuffer<ITestGetBytes>::Create(*_Allocator, *testPtr.GetRawPointer(), testBfr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KStringView             testStr1((WCHAR*)testBfr->GetBuffer(), testBfr->QuerySize() / 2, testBfr->QuerySize() / 2);
            VERIFY_IS_TRUE((testStr1 == testStr) == TRUE);

            //* Prove we can alias a KBuffer as a COM Object supporting GetBytes()
            Common::ComPointer<ITestGetBytes>   testPtr1;
            HRESULT hr = ComKBuffer<ITestGetBytes, IID_ITestGetBytes>::Create(*testBfr, testPtr1.InitializationAddress()); 
            VERIFY_SUCCEEDED(hr, L"ComKBuffer<>::Create");

            ULONG           size = 0;
            BYTE const*     ptr = nullptr;

            hr = testPtr1->GetBytes(&size, &ptr);
            VERIFY_SUCCEEDED(hr, L"ComKBuffer<>::GetBytes");

            testStr1.Clear();
            VERIFY_IS_TRUE(testStr1.Length() == 0);
            
            testStr1.Set((PWCHAR)ptr, size / 2, size / 2);
            VERIFY_IS_TRUE((testStr1 == testStr) == TRUE);

            // Prove we can alias an IFabricOperationData as a KArray<KBuffer::SPtr>
            Common::ComPointer<TestIFabricOperationData<1000>>      testFOD;
            testFOD.SetNoAddRef((TestIFabricOperationData<1000>*)new TestIFabricOperationData<1000>(L"Test String Number: "));

            KFabricOperationDataArray       testKFabricOperationDataArray(*_Allocator, *testFOD.GetRawPointer());
            VERIFY_IS_TRUE(NT_SUCCESS(testKFabricOperationDataArray.Status()));

            for (ULONG ix = 0; ix < testKFabricOperationDataArray.Count(); ix++)
            {
                VERIFY_IS_TRUE((testFOD->Validate(L"Test String Number: ", ix, LPCWSTR(testKFabricOperationDataArray[ix]->GetBuffer()))) == TRUE);
            }

            // Prove we can alias an IFabricOperationData over a KArray<KBuffer::SPtr>
            Common::ComPointer<IFabricOperationData>    testFabricOperationData;
            hr = FabricOperationDataKArray::Create(testKFabricOperationDataArray, testFabricOperationData.InitializationAddress());
            VERIFY_SUCCEEDED(hr, L"FabricOperationDataKArray::Create");

            ULONG                               bufferCount;
            FABRIC_OPERATION_DATA_BUFFER const* buffers;

            hr = testFabricOperationData->GetData(&bufferCount, &buffers);
            VERIFY_SUCCEEDED(hr, L"FabricOperationDataKArray::GetData");

            for (ULONG ix = 0; ix < bufferCount; ix++)
            {
                KLocalString<128>   str;
                KStringView         stringPreamble(L"Test String Number: ");

                KInvariant(str.FromULONG(ix));
                KInvariant(str.Prepend(stringPreamble));
                KInvariant(str.SetNullTerminator());

                KStringView         toCompare(LPCWSTR(buffers[ix].Buffer));

                VERIFY_IS_TRUE((str == toCompare) == TRUE);
            }

            // Prove null entries in a KArray<KBuffer::SPtr> work for aliasing
            testFabricOperationData.Release();
            KArray<KBuffer::SPtr>	testArray1(*_Allocator, 1);
            VERIFY_IS_TRUE(NT_SUCCESS(testArray1.Status()));
            VERIFY_IS_TRUE(NT_SUCCESS(testArray1.Append(nullptr)));
            VERIFY_IS_TRUE(testArray1.Count() == 1);
            VERIFY_IS_TRUE(!testArray1[0]);

            hr = FabricOperationDataKArray::Create(testArray1, testFabricOperationData.InitializationAddress());
            VERIFY_SUCCEEDED(hr, L"FabricOperationDataKArray::Create");

            hr = testFabricOperationData->GetData(&bufferCount, &buffers);
            VERIFY_SUCCEEDED(hr, L"FabricOperationDataKArray::GetData");
            VERIFY_IS_TRUE(bufferCount == 1);
            VERIFY_IS_TRUE(buffers[0].Buffer == nullptr);
            VERIFY_IS_TRUE(buffers[0].BufferSize == 0);
        }

        VERIFY_IS_TRUE(TestITestGetBytesInstances == 0);
        VERIFY_IS_TRUE(TestIFabricOperationDataInstances == 0);
        VERIFY_IS_TRUE(baseKtlAllocs == KAllocatorSupport::gs_AllocsRemaining);
    }
}

