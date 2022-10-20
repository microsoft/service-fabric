// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include "Common/Common.h"
#include "KUnknown.h"
#include <kphysicallog.h>
#include "WexTestClass.h"

using Common::ErrorCode;
using Common::ComPointer;
using namespace Ktl::Com;

namespace ApiTest
{
using ::_delete;

#define VERIFY_REF_COUNT(object, expected) \
    object->AddRef(); \
    VERIFY_ARE_EQUAL((LONG)expected, (LONG)(object->Release()), L""  L## #object L"(" L ## #expected L")");

    class KPhysicalLogTest : public WEX::TestClass<KPhysicalLogTest>
    {
    public:
        TEST_CLASS(KPhysicalLogTest)

        TEST_CLASS_SETUP(Setup)
        TEST_CLASS_CLEANUP(Cleanup)

        TEST_METHOD(Simple)

        template <typename ElementFactory, 
                  typename IoBufferFactory,
                  typename ArrayFactory>
        static void CommonTest(
            __in ElementFactory& ElementFactory, 
            __in IoBufferFactory& IoBufferFactory, 
            __in ArrayFactory& ArrayFactory,
            __in ULONG InitialIoBufferSize = 0);

        static KAllocator*  _Allocator;
        static KtlSystem*   _System;
    };

    KAllocator* KPhysicalLogTest::_Allocator = nullptr;
    KtlSystem* KPhysicalLogTest::_System = nullptr;

    bool KPhysicalLogTest::Setup()
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

    bool KPhysicalLogTest::Cleanup()
    {
        KtlSystem::Shutdown();

        return true;
    }

    template <typename ElementFactory, typename IoBufferFactory, typename ArrayFactory>
    void 
    KPhysicalLogTest::CommonTest(
        __in ElementFactory& ElementFactory, 
        __in IoBufferFactory& IoBufferFactory, 
        __in ArrayFactory& ArrayFactory,
        __in ULONG InitialIoBufferSize)
    {
        {
            //*  ComKIoBufferElement/IKIoBufferElement test
            BYTE*       buffer = nullptr;
            ULONG       bufferSize = 0;
            Common::ComPointer<IKIoBufferElement>   iobe;

            *iobe.InitializationAddress() = ElementFactory(4096);
            HRESULT     hr  = iobe->GetBuffer(&buffer);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(buffer != nullptr);

            hr  = iobe->QuerySize(&bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize ==  4096);

            memset(buffer, 0xFE, bufferSize);           // trigger overwrite detection in AppVerifier if buffer misallocated

            //*  ComKIoBuffer/IKIoBuffer test
            Common::ComPointer<IKIoBuffer>   iob;
            *iob.InitializationAddress() = IoBufferFactory(InitialIoBufferSize);

            ULONG       elements = 0;
            hr = iob->QueryNumberOfIoBufferElements(&elements);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(elements == ULONG((InitialIoBufferSize == 0) ? 0 : 1));

            hr = iob->QuerySize(&bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == InitialIoBufferSize);

            void*       token = "5";
            hr = iob->First(&token);
            if (InitialIoBufferSize == 0)
            {
                VERIFY_IS_TRUE(hr == S_FALSE);
                VERIFY_IS_TRUE(token == nullptr);
            }
            else
            {
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(token != nullptr);
            }


            // Test appending an element
            hr = iob->AddIoBufferElement(iobe.GetRawPointer());
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            
            hr = iob->QueryNumberOfIoBufferElements(&elements);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(elements == ULONG((InitialIoBufferSize == 0) ? 1 : 2));

            hr = iob->QuerySize(&bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == InitialIoBufferSize + 4096);

            // Test basic enumeration token behavior
            token = nullptr;
            void*       nextToken = nullptr;
            hr = iob->First(&token);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            hr = iob->Next(token, &nextToken);
            if (InitialIoBufferSize == 0)
            {
                VERIFY_IS_TRUE(hr == S_FALSE);
            }
            else
            {
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                token = nextToken;
                hr = iob->Next(token, &nextToken);
                VERIFY_IS_TRUE(hr == S_FALSE);
            }

            buffer = nullptr;
            bufferSize = 0;
            hr = iob->QueryElementInfo(token, &buffer, &bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == 4096);
            VERIFY_IS_TRUE(buffer != nullptr);
            VERIFY_IS_TRUE(buffer[0] == 0xFE);
            VERIFY_IS_TRUE(memcmp(&buffer[0], &buffer[1], 4095) == 0);

            Common::ComPointer<IKIoBufferElement>   iobe2;
            hr = iob->GetElement(token, iobe2.InitializationAddress());
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            hr = iobe2->GetBuffer(&buffer);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            hr = iobe2->QuerySize(&bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            VERIFY_IS_TRUE(bufferSize == 4096);
            VERIFY_IS_TRUE(buffer != nullptr);
            VERIFY_IS_TRUE(buffer[0] == 0xFE);
            VERIFY_IS_TRUE(memcmp(&buffer[0], &buffer[1], 4095) == 0);

            // Prove appending an additional element
            iobe2.Release();
            *iobe2.InitializationAddress() = ElementFactory(16*4096);

            hr  = iobe2->GetBuffer(&buffer);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(buffer != nullptr);

            hr  = iobe2->QuerySize(&bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == 16*4096);

            hr = iob->AddIoBufferElement(iobe2.GetRawPointer());
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            hr = iob->QueryNumberOfIoBufferElements(&elements);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(elements == ULONG((InitialIoBufferSize == 0) ? 2 : 3));

            hr = iob->QuerySize(&bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == InitialIoBufferSize + 4096 + (16*4096));

            hr = iob->First(&token);
            VERIFY_IS_TRUE(SUCCEEDED(hr) && (hr != S_FALSE));

            if (InitialIoBufferSize > 0)
            {
                hr = iob->QueryElementInfo(token, &buffer, &bufferSize);
                VERIFY_IS_TRUE(SUCCEEDED(hr));
                VERIFY_IS_TRUE(bufferSize == InitialIoBufferSize);
                VERIFY_IS_TRUE(buffer != nullptr);

                hr = iob->Next(token, &token);
                VERIFY_IS_TRUE(SUCCEEDED(hr) && (hr != S_FALSE));
                VERIFY_IS_TRUE(token != nullptr);
            }

            hr = iob->QueryElementInfo(token, &buffer, &bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == 4096);
            VERIFY_IS_TRUE(buffer != nullptr);

            hr = iob->Next(token, &token);
            VERIFY_IS_TRUE(SUCCEEDED(hr) && (hr != S_FALSE));
            VERIFY_IS_TRUE(token != nullptr);

            hr = iob->QueryElementInfo(token, &buffer, &bufferSize);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(bufferSize == 4096*16);
            VERIFY_IS_TRUE(buffer != nullptr);

            hr = iob->Next(token, &token);
            VERIFY_IS_TRUE(hr == S_FALSE);
            VERIFY_IS_TRUE(token == nullptr);
        }

        //* Test IKArray behavior
        {
            Common::ComPointer<IKArray> aPtr;

            *aPtr.InitializationAddress() = ArrayFactory(0);

            HRESULT     hr = aPtr->GetStatus();
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            KArray<GUID>&   kArray = (KArray<GUID>&)(((ComIKArray<GUID>*)aPtr.GetRawPointer())->GetBackingArray());

            const ULONG     arraySize = 100;
            KGuid           guids[arraySize];

            // Stuff the test array
            for (ULONG ix = 0; ix < arraySize; ix++)
            {
                guids[ix].CreateNew();
                kArray.Append(guids[ix]);
                VERIFY_IS_TRUE(NT_SUCCESS(kArray.Status()));
            }

            ULONG       count = 0;
            hr = aPtr->GetCount(&count);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(count == arraySize);

            GUID*       arrayBase = nullptr;
            hr = aPtr->GetArrayBase((void**)&arrayBase);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            VERIFY_IS_TRUE(arrayBase != nullptr);

            // verify the test array
            for (ULONG ix = 0; ix < arraySize; ix++)
            {
                VERIFY_IS_TRUE(arrayBase[ix] == guids[ix]);
            }
        }
    }

    void KPhysicalLogTest::Simple()
    {
        LONGLONG        baseKtlAllocs = KAllocatorSupport::gs_AllocsRemaining;

        // Lib and DLL Element factories
        auto            LibElementFactory = [](ULONG Size) -> IKIoBufferElement*
        {
            ComKIoBufferElement::SPtr ckioBufferElement;

            HRESULT     hr = ComKIoBufferElement::Create(*_Allocator, Size, ckioBufferElement);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            IKIoBufferElement*   iobe;

            hr = ckioBufferElement->QueryInterface(IID_IKIoBufferElement, (void**)&iobe);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            return iobe;
        };

        auto            DllElementFactory = [](ULONG Size) -> IKIoBufferElement*
        {
            IKIoBufferElement*   iobe;
            HRESULT     hr = KCreateKIoBufferElement(Size, &iobe);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            return iobe;
        };

        // Lib and DLL IoBuffer factories
        auto            LibIoBufferFactory = [](ULONG Size) -> IKIoBuffer*
        {
            ComKIoBuffer::SPtr  comIoBuffer;
            HRESULT     hr =  S_OK;

            if (Size == 0)
            {
                hr = ComKIoBuffer::CreateEmpty(*_Allocator, comIoBuffer);
            }
            else
            {
                hr = ComKIoBuffer::CreateSimple(*_Allocator, Size, comIoBuffer);
            }
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            IKIoBuffer*   iob;

            hr = comIoBuffer->QueryInterface(IID_IKIoBuffer, (void**)&iob);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            return iob;
        };

        auto            DllIoBufferFactory = [](ULONG Size) -> IKIoBuffer*
        {
            IKIoBuffer*     iob;
            HRESULT         hr =  S_OK;

            if (Size == 0)
            {
                hr = KCreateEmptyKIoBuffer(&iob);
            }
            else
            {
                hr = KCreateSimpleKIoBuffer(Size, &iob);
            }
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            return iob;
        };

        //** Lib and DLL Array Factories
        auto            LibArrayFactory = [](ULONG InitialSize) -> IKArray*
        {
            ComIKArray<GUID>::SPtr    sptr;
            HRESULT                   hr = ComIKArray<GUID>::Create(*_Allocator, InitialSize, sptr);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            IKArray*            result;
            hr = sptr->QueryInterface(IID_IKArray, (void**)&result);
            VERIFY_IS_TRUE(SUCCEEDED(hr));

            return result;
        };

        auto            DllArrayFactory = [](ULONG InitialSize) -> IKArray*
        {
            IKArray*            result = nullptr;
            HRESULT             hr = KCreateGuidIKArray(InitialSize, &result);
            VERIFY_IS_TRUE(SUCCEEDED(hr));
            return result;
        };

        //** Simple Lib test
        {
            CommonTest(LibElementFactory, LibIoBufferFactory, LibArrayFactory);
            CommonTest(LibElementFactory, LibIoBufferFactory, LibArrayFactory, 1024*1024);
        }

        //* Simple DLL test
        {
            CommonTest(DllElementFactory, DllIoBufferFactory, DllArrayFactory);
            CommonTest(DllElementFactory, DllIoBufferFactory, DllArrayFactory, 1024*1024);
        }

        VERIFY_IS_TRUE(baseKtlAllocs == KAllocatorSupport::gs_AllocsRemaining);
    }

}

