// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#if KTL_USER_MODE

#include "Common/Common.h"
#include "ktl.h"
#include "KUnknown.h"
#include "KComUtility.h"

//* Grandfathered to/from KTL/COM adaptors
#include "FabricAsyncContextBase.h"
#include "FabricAsyncServiceBase.h"
#include "AsyncCallInAdapter.h"
#include "AsyncCalloutAdapter.h"


namespace Ktl
{
    namespace Com
    {
        using ::_delete;

        //*** KBuffer implementation that aliases a COM interface of type <ComType>
        //
        //  Requirement: <ComType> MUST implement:
        //      HRESULT GetBytes(ULONG* Size, BYTE const& StatePtr);
        //
        //  Usage Example:
        //      Common::ComPointer<IFabricBtreeValue> testPtr
        //      KBuffer::SPtr                         allocatedBuffer;
        //      
        //      NTSTATUS = KComBuffer<IFabricBtreeValue>::Create(GetMyAllocator(), *testPtr, allocatedBuffer);
        //
        template <typename ComType>
        class KComBuffer : public KBuffer
        {
            K_FORCE_SHARED(KComBuffer);

        public:
            //* Create a KBuffer aliasing the supplied COM object of type <ComType>; no copying is done
            //
            //  Parameters:
            //      Allocator - Allocator to use for allocation of the resulting KBuffer instance
            //      ComObjToWrap - Wraped COM interface imp that exposes GetBytes()
            //      Result - SPtr of allocated KBuffer
            //
            static NTSTATUS Create(__in KAllocator& Allocator, __in ComType& ComObjToWrap, __out KBuffer::SPtr& Result);

        //* KBuffer implementation - see KBuffer.h for behavior except as noted below
        private:

            //* NOTE: Returns: STATUS_NOT_IMPLEMENTED
            NTSTATUS
            SetSize(
                __in ULONG NewSize,
                __in BOOLEAN PreserveOldContents = FALSE,
                __in ULONG AllocationTag = KTL_TAG_BUFFER) override;

            VOID const *
            GetBuffer() const override;

            VOID *
            GetBuffer() override;

            ULONG 
            QuerySize() const override;

            VOID 
            Zero() override;

            VOID 
            Zero(__in ULONG Offset, __in ULONG Length) override;

            VOID
            CopyFrom(
                __in ULONG TargetOffset,
                __in KBuffer const& SourceBuffer,
                __in ULONG SourceOffset,
                __in ULONG Length) override;

            //* NOTE: Returns: STATUS_NOT_IMPLEMENTED
            NTSTATUS
            CloneFrom(__in KBuffer const& SourceBuffer, __in ULONG AllocationTag = KTL_TAG_BUFFER) override;

            VOID
            Move(__in ULONG TargetOffset, __in ULONG SourceOffset, __in ULONG Length) override;

            BOOLEAN 
            operator==(__in KBuffer const& Right) override;
        
            BOOLEAN 
            operator!=(__in KBuffer const& Right) override;

        private:
            KComBuffer(ComType& BackingState);

        private:
            Common::ComPointer<ComType> _BackingState;
            const BYTE*                 _BackingStatePtr;
            ULONG                       _BackingStateSize;
        };

        //** COM interface implementation of type <ComBufferType> and with an IID reference of <IIDRef>
        //   that aliases a provided KBuffer.
        //
        //   Requirements: Interface <ComBufferType> MUST expose:  
        //
        //      HRESULT STDMETHODCALLTYPE GetBytes(__out ULONG* Count, __out BYTE const** Buffer);
        // 
        template <typename ComBufferType, IID const& IIDRef>
        class ComKBuffer : public ComBufferType, Common::ComUnknownBase
        {
            BEGIN_COM_INTERFACE_LIST(ComKBuffer)
                COM_INTERFACE_ITEM(IID_IUnknown, ComBufferType)
                COM_INTERFACE_ITEM(IIDRef, ComBufferType)
            END_COM_INTERFACE_LIST()

        public:
            // Create a KBuffer alias instance of type <ComBufferType>  
            //
            //  Parameters:
            //      BackingBuffer - Reference of the KBuffer instance that is to be aliased 
            //      Result - Resulting interface pointer to the created <ComBufferType> instance
            //      
            HRESULT static
            Create(__in KBuffer& BackingBuffer, __out ComBufferType** Result);

        private:
            HRESULT STDMETHODCALLTYPE
            GetBytes(__out ULONG* count, __out const BYTE** buffer) override;

            ComKBuffer(KBuffer& BackingBuffer);

            ComKBuffer() {}         // Disallowed

        private:
            KBuffer::SPtr       _BackingKBuffer;
        };


        //** Class that aliases a KArray<KBuffer::SPtr> as a IFabricOperationData implementation
        //
        //  Usage Example:
        //      
        //      KArray<KBuffer::SPtr>   backingBuffers(GetMyAllocator());
        //      IFabricOperationData*   iod1;
        //      hr = FabricOperationDataKArray::Create(backingBuffers, &iod1);
        //
        class FabricOperationDataKArray : public IFabricOperationData, Common::ComUnknownBase
        {
            BEGIN_COM_INTERFACE_LIST(FabricOperationDataKArray)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
                COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
            END_COM_INTERFACE_LIST()

        public:
            // Create a KArray<KBuffer::SPtr> alias instance of type IFabricOperationData  
            //
            //  Parameters:
            //      BackingBuffers - Reference of the KArray<KBuffer::SPtr> instance that is to be aliased
            //      Result - Resulting interface pointer to the created IFabricOperationData instance
            //
            //   NOTE: Backing buffers array is Moved and this its contents will be cleared by this operation.
            //      
            HRESULT static
            Create(__in KArray<KBuffer::SPtr>& BackingBuffers, __out IFabricOperationData** Result);

        private:
            HRESULT STDMETHODCALLTYPE
            GetData(__out ULONG* Count, __out const FABRIC_OPERATION_DATA_BUFFER** Buffer) override;

            FabricOperationDataKArray(KArray<KBuffer::SPtr>& BackingBuffers);
            FabricOperationDataKArray();       // Disallowed
            ~FabricOperationDataKArray();

        private:
            KArray<KBuffer::SPtr>           _Buffers;
            FABRIC_OPERATION_DATA_BUFFER*   _AliasBufferDescs; 
        };


        //* Adapter class (KFabricOperationDataArray : KShared) for exposing an IFabricOperationData as a KArray<KBuffer::Ptr>

        //** KArray<Buffer::SPtr> that aliases a IFabricOperationData implementation
        //
        //  Usage Example:
        //      
        //      KFabricOperationDataArray   opDataArray(GetMyAllocator(), *iod1);
        //      if (!NT_SUCCESS(opDataArray.Status())
        //      {
        //          // ctor of KArray failed
        //      }
        //
        //  See KArray.h for further usage information
        //
        class KFabricOperationDataArray : public KArray<KBuffer::SPtr>
        {
            K_DENY_COPY(KFabricOperationDataArray);

        public:
            FAILABLE 
            KFabricOperationDataArray(__in KAllocator& Allocator, __in IFabricOperationData& BackingState);
            ~KFabricOperationDataArray();

        private:
            KFabricOperationDataArray();         // Disallow

        private:
            class FODKBuffer;

            Common::ComPointer<IFabricOperationData> _BackingArrayState;
        };
    }
}


//*** KComBuffer<ComType> Implementation
template <typename ComType>
Ktl::Com::KComBuffer<ComType>::KComBuffer(ComType& BackingState)
{
    _BackingState.SetAndAddRef(&BackingState);

    if (FAILED(BackingState.GetBytes(&_BackingStateSize, &_BackingStatePtr)))
    {
        // BUG: Consider adding some trace output or a better HRESULT -> NTSTATUS conversion
        SetConstructorStatus(STATUS_UNSUCCESSFUL);
    }
}

template <typename ComType>
Ktl::Com::KComBuffer<ComType>::~KComBuffer()
{
}

template <typename ComType>
NTSTATUS 
Ktl::Com::KComBuffer<ComType>::Create( __in KAllocator& Allocator, __in ComType& ComObjToWrap,__out KBuffer::SPtr& Result)
{
    Result = _new(KTL_TAG_BUFFER, Allocator) Ktl::Com::KComBuffer<ComType>(ComObjToWrap);
    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return Result->Status();
}

template <typename ComType>
VOID* 
Ktl::Com::KComBuffer<ComType>::GetBuffer()
{
    return (VOID*)_BackingStatePtr;
}

template <typename ComType>
VOID const *
Ktl::Com::KComBuffer<ComType>::GetBuffer() const
{
    return (VOID*)_BackingStatePtr;
}

template <typename ComType>
ULONG 
Ktl::Com::KComBuffer<ComType>::QuerySize() const
{
    return _BackingStateSize;
}

template <typename ComType>
NTSTATUS
Ktl::Com::KComBuffer<ComType>::SetSize(
    __in ULONG NewSize,
    __in BOOLEAN PreserveOldContents,
    __in ULONG AllocationTag)
{
    UNREFERENCED_PARAMETER(NewSize);
    UNREFERENCED_PARAMETER(PreserveOldContents);
    UNREFERENCED_PARAMETER(AllocationTag);
    return STATUS_NOT_IMPLEMENTED;
}

template <typename ComType>
VOID
Ktl::Com::KComBuffer<ComType>::Zero()
{
    if (_BackingStateSize > 0) 
    {
        RtlZeroMemory((UCHAR*)_BackingStatePtr, _BackingStateSize);
    }
}

template <typename ComType>
VOID
Ktl::Com::KComBuffer<ComType>::Zero(__in ULONG Offset, __in ULONG Length)
{
    KFatal(Offset <= _BackingStateSize);
    KFatal(_BackingStateSize - Offset >= Length);
    if (Length > 0) 
    {
        RtlZeroMemory((UCHAR*)_BackingStatePtr + Offset, Length);
    }
}

template <typename ComType>
VOID
Ktl::Com::KComBuffer<ComType>::CopyFrom(
    __in ULONG TargetOffset,
    __in KBuffer const& SourceBuffer,
    __in ULONG SourceOffset,
    __in ULONG Length)
{
    KFatal(TargetOffset <= _BackingStateSize);
    KFatal(_BackingStateSize - TargetOffset >= Length);
    KFatal(SourceOffset <= SourceBuffer.QuerySize());
    KFatal(SourceBuffer.QuerySize() - SourceOffset >= Length);
    if (Length > 0) 
    {
        KMemCpySafe(
            (BYTE*)_BackingStatePtr + TargetOffset, 
            _BackingStateSize - TargetOffset, 
            (BYTE*)SourceBuffer.GetBuffer() + SourceOffset, 
            Length);
    }
}

template <typename ComType>
NTSTATUS
Ktl::Com::KComBuffer<ComType>::CloneFrom(
    __in KBuffer const& SourceBuffer,
    __in ULONG AllocationTag)
{
    UNREFERENCED_PARAMETER(SourceBuffer);
    UNREFERENCED_PARAMETER(AllocationTag);
    return STATUS_NOT_IMPLEMENTED;
}

template <typename ComType>
VOID
Ktl::Com::KComBuffer<ComType>::Move(
    __in ULONG TargetOffset,
    __in ULONG SourceOffset,
    __in ULONG Length)
{
    KFatal(SourceOffset <= _BackingStateSize);
    KFatal(_BackingStateSize - SourceOffset >= Length);
    KFatal(TargetOffset <= _BackingStateSize);
    KFatal(_BackingStateSize - TargetOffset >= Length);
    if (Length > 0) 
    {
        RtlMoveMemory((BYTE*)_BackingStatePtr + TargetOffset, _BackingStatePtr + SourceOffset, Length);
    }
}

template <typename ComType>
BOOLEAN
Ktl::Com::KComBuffer<ComType>::operator==(__in KBuffer const& Right)
{
    if (QuerySize() != Right.QuerySize())
    {
        return FALSE;
    }

    size_t n = RtlCompareMemory(GetBuffer(), Right.GetBuffer(), QuerySize());
    if (n != QuerySize())
    {
        return FALSE;
    }

    return TRUE;
}

template <typename ComType>
BOOLEAN
Ktl::Com::KComBuffer<ComType>::operator!=(__in KBuffer const& Right)
{
    return !(*this == Right);
}


//*** ComKBuffer<ComType, IIDRef> Implementation
template <typename ComBufferType, REFIID IIDRef>
Ktl::Com::ComKBuffer<ComBufferType, IIDRef>::ComKBuffer(KBuffer& BackingBuffer)
    :   _BackingKBuffer(&BackingBuffer)
{
}

template <typename ComBufferType, REFIID IIDRef>
HRESULT 
Ktl::Com::ComKBuffer<ComBufferType, IIDRef>::Create(__in KBuffer& BackingBuffer, __out ComBufferType** Result)
{
    if (nullptr == Result)
    {
        return E_POINTER;
    }

    *Result = nullptr;

    ComKBuffer<ComBufferType, IIDRef>* resultPtr = new(std::nothrow) ComKBuffer<ComBufferType, IIDRef>(BackingBuffer); 
        // ref count is already 1
    
    if (nullptr == resultPtr)
    {
        return E_OUTOFMEMORY;
    }

    *Result = static_cast<ComBufferType*>(resultPtr);

    return S_OK;
}

template <typename ComBufferType, REFIID IIDRef>
HRESULT STDMETHODCALLTYPE
Ktl::Com::ComKBuffer<ComBufferType, IIDRef>::GetBytes(__out ULONG* count, __out const BYTE** buffer)
{
    if (nullptr == count || nullptr == buffer)
    {
        return E_POINTER;
    }

    if (_BackingKBuffer == nullptr)
    {
        *count = 0;
        *buffer = nullptr;
    }
    else
    {
        *count = _BackingKBuffer->QuerySize();
        *buffer = (BYTE*)(_BackingKBuffer->GetBuffer());
    }

    return Common::ComUtility::OnPublicApiReturn(S_OK);
}

#endif
