// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "KPhysicalLog.h"
#include <FabricAsyncServiceBase.h>
#include <AsyncCallInAdapter.h>
#include <ktrace.h>

namespace Ktl
{
    namespace Com
    {
        //** KBuffer COM support implementation
        ComKBuffer::ComKBuffer(ULONG Size)
        {
            NTSTATUS    status = KBuffer::Create(Size, _BackingBuffer, GetThisAllocator(), GetThisAllocationTag());

            SetConstructorStatus(status);
        }

        ComKBuffer::ComKBuffer(KBuffer& BackingBuffer)
            :   _BackingBuffer(&BackingBuffer)
        {
        }

        ComKBuffer::~ComKBuffer()
        {
        }

        HRESULT
        ComKBuffer::Create(__in KAllocator& Allocator, __in ULONG Size, __out ComKBuffer::SPtr& Result)
        {
            Result = _new(KTL_TAG_BUFFER, Allocator) ComKBuffer(Size);
            if (Result == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            NTSTATUS    status = Result->Status();
            if (!NT_SUCCESS(status))
            {
                Result = nullptr;
                return Ktl::Com::KComUtility::ToHRESULT(status);
            }

            return S_OK;
        }

        HRESULT
        ComKBuffer::Create(__in KAllocator& Allocator,  __in KBuffer& From, __out ComKBuffer::SPtr& Result)
        {
            Result = _new(KTL_TAG_BUFFER, Allocator) ComKBuffer(From);
            if (Result == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            NTSTATUS    status = Result->Status();
            if (!NT_SUCCESS(status))
            {
                Result = nullptr;
                return Ktl::Com::KComUtility::ToHRESULT(status);
            }

            return S_OK;
        }


        HRESULT  
        ComKBuffer::GetBuffer(__out BYTE **const Result)
        {
            ASSERT_IF(nullptr == Result, "parameter is null");

            *Result = (BYTE*)(_BackingBuffer->GetBuffer());
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        HRESULT  
        ComKBuffer::QuerySize( __out ULONG *const Result)
        {
            ASSERT_IF(nullptr == Result, "parameter is null");

            *Result = _BackingBuffer->QuerySize();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        
        
        //** KIoBufferElement COM support implementation
        ComKIoBufferElement::ComKIoBufferElement(ULONG Size)
        {
            void*       bfr = nullptr;
            NTSTATUS    status = KIoBufferElement::CreateNew(Size, _BackingElement, bfr, GetThisAllocator(), GetThisAllocationTag());

            SetConstructorStatus(status);
        }

        ComKIoBufferElement::ComKIoBufferElement(KIoBufferElement& BackingElement)
            :   _BackingElement(&BackingElement)
        {
        }

        ComKIoBufferElement::~ComKIoBufferElement()
        {
        }

        HRESULT
        ComKIoBufferElement::Create(__in KAllocator& Allocator, __in ULONG Size, __out ComKIoBufferElement::SPtr& Result)
        {
            Result = _new(KTL_TAG_BUFFER, Allocator) ComKIoBufferElement(Size);
            if (Result == nullptr)
            {
                return E_OUTOFMEMORY;
            }

            NTSTATUS    status = Result->Status();
            if (!NT_SUCCESS(status))
            {
                Result = nullptr;
                return Ktl::Com::KComUtility::ToHRESULT(status);
            }

            return S_OK;
        }

        HRESULT  
        ComKIoBufferElement::GetBuffer(__out BYTE **const Result)
        {
            ASSERT_IF(nullptr == Result, "parameter is null");

            *Result = (BYTE*)(_BackingElement->GetBuffer());
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        HRESULT  
        ComKIoBufferElement::QuerySize( __out ULONG *const Result)
        {
            ASSERT_IF(nullptr == Result, "parameter is null");

            *Result = _BackingElement->QuerySize();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        
        
        //** KIoBuffer COM support
        ComKIoBuffer::ComKIoBuffer(KIoBuffer& BackingIoBuffer)
            :   _BackingIoBuffer(&BackingIoBuffer)
        {
        }

        ComKIoBuffer::~ComKIoBuffer()
        {
        }

        HRESULT 
        ComKIoBuffer::CreateSimple(__in KAllocator& Allocator,  __in ULONG Size, __out ComKIoBuffer::SPtr& Result)
        {
            void*           buffer = nullptr;
            KIoBuffer::SPtr sptr;
            HRESULT hr;

            NTSTATUS    status = KIoBuffer::CreateSimple(Size, sptr, buffer, Allocator);
            if (!NT_SUCCESS(status))
            {
                hr = Ktl::Com::KComUtility::ToHRESULT(status);
                KDbgErrorWData(0, "ComKIoBuffer::CreateSimple", status, 
                    (ULONGLONG)0, 
                    hr, 
                    (ULONGLONG)Size, 
                    0);
                return(hr);
            }

            Result = _new(KTL_TAG_BUFFER, Allocator) ComKIoBuffer(*sptr);
            if (Result == nullptr)
            {
                hr = E_OUTOFMEMORY;
                KDbgErrorWData(0, "ComKIoBuffer::CreateSimple", STATUS_INSUFFICIENT_RESOURCES, 
                    (ULONGLONG)0, 
                    hr, 
                    (ULONGLONG)Size, 
                    0);
                return hr;
            }

            hr = S_OK;
            return hr;
        }

        HRESULT 
        ComKIoBuffer::CreateEmpty(__in KAllocator& Allocator, __out ComKIoBuffer::SPtr& Result)
        {
            KIoBuffer::SPtr sptr;
            HRESULT hr;

            NTSTATUS    status = KIoBuffer::CreateEmpty(sptr, Allocator);
            if (!NT_SUCCESS(status))
            {
                hr = Ktl::Com::KComUtility::ToHRESULT(status);
                KDbgErrorWData(0, "ComKIoBuffer::CreateEmpty", status, 
                    (ULONGLONG)0, 
                    hr, 
                    (ULONGLONG)0, 
                    0);
                return(hr);
            }

            Result = _new(KTL_TAG_BUFFER, Allocator) ComKIoBuffer(*sptr);
            if (Result == nullptr)
            {
                hr = E_OUTOFMEMORY;
                KDbgErrorWData(0, "ComKIoBuffer::CreateEmpty", STATUS_INSUFFICIENT_RESOURCES, 
                    (ULONGLONG)0, 
                    hr, 
                    (ULONGLONG)0, 
                    0);
                return hr;
            }

            hr = S_OK;
            return hr;
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::AddIoBufferElement(__in IKIoBufferElement *const Element)
        {
            _BackingIoBuffer->AddIoBufferElement(((ComKIoBufferElement*)Element)->GetBackingElement());
            HRESULT hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::QueryNumberOfIoBufferElements(__out ULONG *const Result)
        {
            *Result = _BackingIoBuffer->QueryNumberOfIoBufferElements();
            HRESULT hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::QuerySize(__out ULONG *const Result)
        {
            *Result = _BackingIoBuffer->QuerySize();
            HRESULT hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::First(__out void **const ResultToken)
        {
            *ResultToken =  _BackingIoBuffer->First();
            HRESULT hr = (*ResultToken) == nullptr ? S_FALSE : S_OK;

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::Next(__in void *const CurrentToken, __out void **const ResultToken)
        {
            *ResultToken =  _BackingIoBuffer->Next(*((KIoBufferElement*)CurrentToken));
            HRESULT hr = (*ResultToken) == nullptr ? S_FALSE : S_OK;

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::Clear()
        {
            _BackingIoBuffer->Clear();
            HRESULT hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::AddIoBuffer(__in IKIoBuffer *const ToAdd)
        {
            _BackingIoBuffer->AddIoBuffer(*((ComKIoBuffer*)ToAdd)->_BackingIoBuffer);

            HRESULT hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::AddIoBufferReference(
            __in IKIoBuffer *const SourceIoBuffer, 
            __in ULONG SourceStartingOffset,
            __in ULONG Size)
        {
            HRESULT hr;

            NTSTATUS status = _BackingIoBuffer->AddIoBufferReference(
                *((ComKIoBuffer*)SourceIoBuffer)->_BackingIoBuffer,
                SourceStartingOffset,
                Size);

            if (!NT_SUCCESS(status))
            {
                hr = Ktl::Com::KComUtility::ToHRESULT(status);
                KDbgErrorWData(0, "ComKIoBuffer::AddIoBufferReference", status, 
                    (ULONGLONG)_BackingIoBuffer, 
                    hr, 
                    (ULONGLONG)((ComKIoBuffer*)SourceIoBuffer)->_BackingIoBuffer, 
                    ((ULONGLONG)SourceStartingOffset << 32) + Size);
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }


        HRESULT STDMETHODCALLTYPE ComKIoBuffer::QueryElementInfo(
            __in void *const Token, 
            __out BYTE **const ResultBuffer, 
            __out ULONG *const ResultSize)
        {
            *ResultBuffer = (BYTE*)((KIoBufferElement*)Token)->GetBuffer();
            *ResultSize = ((KIoBufferElement*)Token)->QuerySize();

            HRESULT hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }
        
        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::GetElement(__in void *const FromToken, __out IKIoBufferElement **const Result)
        {
            ComKIoBufferElement::SPtr   sptr;
            HRESULT hr;

            sptr = _new(KTL_TAG_BUFFER, GetThisAllocator()) ComKIoBufferElement(*(KIoBufferElement*)FromToken);
            if (sptr == nullptr)
            {
                *Result = nullptr;

                hr = E_OUTOFMEMORY;
                KDbgErrorWData(0, "ComKIoBuffer::GetElement", STATUS_INSUFFICIENT_RESOURCES, 
                    (ULONGLONG)_BackingIoBuffer, 
                    hr, 
                    (ULONGLONG)FromToken, 
                    0);
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            *Result = (IKIoBufferElement*)sptr.Detach();
            
            hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT STDMETHODCALLTYPE 
        ComKIoBuffer::GetElements(__out ULONG* const NumberOfElements, __out IKArray** const Result)
        {
            *NumberOfElements =  _BackingIoBuffer->QueryNumberOfIoBufferElements();
            *Result = nullptr;

            ComIKArray<KIOBUFFER_ELEMENT_DESCRIPTOR>::SPtr  result;

            HRESULT     hr = ComIKArray<KIOBUFFER_ELEMENT_DESCRIPTOR>::Create(GetThisAllocator(), *NumberOfElements, result);
            if (FAILED(hr))
            {
                KDbgErrorWData(0, "ComKIoBuffer::GetElements", STATUS_INSUFFICIENT_RESOURCES, 
                    (ULONGLONG)_BackingIoBuffer, 
                    hr, 
                    0, 
                    0);
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            KIoBufferElement*  currentElement = _BackingIoBuffer->First();
            while (currentElement != nullptr)
            {
                KIOBUFFER_ELEMENT_DESCRIPTOR    newItem;
                newItem.ElementBaseAddress = (ULONGLONG)currentElement->GetBuffer();
                newItem.Size = currentElement->QuerySize();

                NTSTATUS    status = ((KArray<KIOBUFFER_ELEMENT_DESCRIPTOR>&)result->GetBackingArray()).Append(newItem);
                if (!NT_SUCCESS(status))
                {
                    hr = Ktl::Com::KComUtility::ToHRESULT(status);
                    KDbgErrorWData(0, "ComKIoBuffer::GetElements", STATUS_INSUFFICIENT_RESOURCES, 
                        (ULONGLONG)_BackingIoBuffer, 
                        hr, 
                        0, 
                        0);

                    return Common::ComUtility::OnPublicApiReturn(hr);
                }

                currentElement = _BackingIoBuffer->Next(*currentElement);
            }

            *Result = result.Detach();

            hr = S_OK;
            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        
        
        //** KArray<void*> COM wrapper class implementation
        ComIKArrayBase::ComIKArrayBase()
        {
        }

        ComIKArrayBase::~ComIKArrayBase()
        {
        }

        HRESULT 
        ComIKArrayBase::GetStatus()
        {
            return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(Status()));
        }

        HRESULT 
        ComIKArrayBase::GetCount(__out ULONG* const Result)
        {
            *Result = GetBackingArray().Count();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        HRESULT 
        ComIKArrayBase::GetArrayBase(__out void** const Result)
        {
            *Result = GetBackingArray().Data();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        HRESULT 
        ComIKArrayBase::AppendGuid(__in GUID *const ToAdd)
        {
            KArray<GUID>&   backingArray = (KArray<GUID>&)GetBackingArray();

            NTSTATUS        status = backingArray.Append(*ToAdd);
            return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
        }

        HRESULT 
        ComIKArrayBase::AppendRecordMetadata(__in KPHYSICAL_LOG_STREAM_RecordMetadata *const ToAdd)
        {
            KArray<KPHYSICAL_LOG_STREAM_RecordMetadata>&   backingArray = 
                (KArray<KPHYSICAL_LOG_STREAM_RecordMetadata>&)GetBackingArray();

            NTSTATUS        status = backingArray.Append(*ToAdd);
            return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
        }
    }
}


