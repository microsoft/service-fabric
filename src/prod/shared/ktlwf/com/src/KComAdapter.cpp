// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ktl.h"
#include "KComAdapter.h"

using namespace Ktl::Com;

//*** FabricOperationDataKArray Implementation
Ktl::Com::FabricOperationDataKArray::FabricOperationDataKArray(KArray<KBuffer::SPtr>& BackingBuffers)
    :   _AliasBufferDescs(nullptr),
        _Buffers(Ktl::Move(BackingBuffers))
{
    _AliasBufferDescs = (FABRIC_OPERATION_DATA_BUFFER*)new(std::nothrow) FABRIC_OPERATION_DATA_BUFFER[_Buffers.Count()];
    if (_AliasBufferDescs != nullptr)
    {
        for (ULONG ix = 0; ix < _Buffers.Count(); ix++)
        {
            if (_Buffers[ix])
            {
                _AliasBufferDescs[ix].Buffer = (BYTE*)(_Buffers[ix]->GetBuffer());
                _AliasBufferDescs[ix].BufferSize = _Buffers[ix]->QuerySize();
            }
            else
            {
                _AliasBufferDescs[ix].Buffer = nullptr;
                _AliasBufferDescs[ix].BufferSize = 0;
            }
        }
    }
}

Ktl::Com::FabricOperationDataKArray::~FabricOperationDataKArray()
{
    if (_AliasBufferDescs != nullptr)
    {
        delete[] _AliasBufferDescs;
        _AliasBufferDescs = nullptr;
    }
}

HRESULT 
Ktl::Com::FabricOperationDataKArray::Create(__in KArray<KBuffer::SPtr>& BackingBuffers, __out IFabricOperationData** Result)
{
    if (nullptr == Result)
    {
        return E_POINTER;
    }

    *Result = nullptr;

    FabricOperationDataKArray* resultPtr = new(std::nothrow) FabricOperationDataKArray(BackingBuffers); 
        // ref count is already 1
    
    if (resultPtr == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    if (resultPtr->_AliasBufferDescs == nullptr)
    {
        resultPtr->Release();
        return E_OUTOFMEMORY;
    }

    *Result = static_cast<IFabricOperationData*>(resultPtr);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE
Ktl::Com::FabricOperationDataKArray::GetData(__out ULONG* Count, __out const FABRIC_OPERATION_DATA_BUFFER** Buffers)
{
    if ((nullptr == Buffers) || (nullptr == Count))
    {
        return Common::ComUtility::OnPublicApiReturn(E_POINTER);
    }

    if (_AliasBufferDescs != nullptr)
    {
        *Buffers = _AliasBufferDescs;
        *Count = _Buffers.Count();
        return Common::ComUtility::OnPublicApiReturn(S_OK);
    }

    *Buffers = nullptr;
    *Count = 0;
    return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
}


//** Implementation of KFabricOperationDataArray

//* Private KBuffer implementation that aliases a FABRIC_OPERATION_DATA_BUFFER structure
class Ktl::Com::KFabricOperationDataArray::FODKBuffer : public KBuffer
{
    K_FORCE_SHARED(FODKBuffer);

public:
    static NTSTATUS Create(
        __in KAllocator& Allocator, 
        __in FABRIC_OPERATION_DATA_BUFFER const& BufferToWrap, 
        __out KBuffer::SPtr& Result);

private:
    NTSTATUS
    SetSize(
        __in ULONG NewSize,
        __in BOOLEAN PreserveOldContents = FALSE,
        __in ULONG AllocationTag = KTL_TAG_BUFFER) override;

    VOID* 
    GetBuffer() override;

    VOID const *
    GetBuffer() const override;

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

    NTSTATUS
    CloneFrom(__in KBuffer const& SourceBuffer, __in ULONG AllocationTag = KTL_TAG_BUFFER) override;

    VOID
    Move(__in ULONG TargetOffset, __in ULONG SourceOffset, __in ULONG Length) override;

    BOOLEAN 
    operator==(__in KBuffer const& Right) override;
        
    BOOLEAN 
    operator!=(__in KBuffer const& Right) override;

private:
    FODKBuffer(FABRIC_OPERATION_DATA_BUFFER const& BackingState);

private:
    FABRIC_OPERATION_DATA_BUFFER const&     _BackingState;
};


Ktl::Com::KFabricOperationDataArray::KFabricOperationDataArray(
    __in KAllocator& Allocator, 
    __in IFabricOperationData& BackingState)
    :   KArray<KBuffer::SPtr>(Allocator, 0)
{
    _BackingArrayState.SetAndAddRef(&BackingState);      // Take ref on backing store

    FABRIC_OPERATION_DATA_BUFFER const* buffers = nullptr;
    ULONG                               count = 0;

    HRESULT     hr = BackingState.GetData(&count, &buffers);
    if (FAILED(hr))
    {
        SetConstructorStatus(STATUS_UNSUCCESSFUL);
        return;
    }

    NTSTATUS    status = Reserve(count);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    for (ULONG ix = 0; ix < count; ix++)
    {
        KBuffer::SPtr   currBuffer;
        status = FODKBuffer::Create(Allocator, buffers[ix], currBuffer);
        if (FAILED(hr))
        {
            SetConstructorStatus(status);
            return;
        }

        KInvariant(NT_SUCCESS(Append(Ktl::Move(currBuffer))));
    }
}

Ktl::Com::KFabricOperationDataArray::~KFabricOperationDataArray()
{
}


//* Implementation of KFabricOperationDataArray::FODKBuffer
Ktl::Com::KFabricOperationDataArray::FODKBuffer::FODKBuffer(FABRIC_OPERATION_DATA_BUFFER const& BufferToWrap)
    :   _BackingState(BufferToWrap)
{
}

Ktl::Com::KFabricOperationDataArray::FODKBuffer::~FODKBuffer()
{
}

NTSTATUS 
Ktl::Com::KFabricOperationDataArray::FODKBuffer::Create(
    __in KAllocator& Allocator, 
    __in FABRIC_OPERATION_DATA_BUFFER const& BufferToWrap,
    __out KBuffer::SPtr& Result)
{
    Result = _new(KTL_TAG_BUFFER, Allocator) FODKBuffer(BufferToWrap);

    if (!Result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return Result->Status();
}

VOID* 
Ktl::Com::KFabricOperationDataArray::FODKBuffer::GetBuffer()
{
    return (VOID*)_BackingState.Buffer;
}

VOID const *
Ktl::Com::KFabricOperationDataArray::FODKBuffer::GetBuffer() const
{
    return (VOID*)_BackingState.Buffer;
}

ULONG 
Ktl::Com::KFabricOperationDataArray::FODKBuffer::QuerySize() const
{
    return _BackingState.BufferSize;
}

NTSTATUS
Ktl::Com::KFabricOperationDataArray::FODKBuffer::SetSize(
    __in ULONG NewSize,
    __in BOOLEAN PreserveOldContents,
    __in ULONG AllocationTag)
{
    UNREFERENCED_PARAMETER(NewSize);
    UNREFERENCED_PARAMETER(PreserveOldContents);
    UNREFERENCED_PARAMETER(AllocationTag);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
Ktl::Com::KFabricOperationDataArray::FODKBuffer::Zero()
{
    if (_BackingState.BufferSize > 0) 
    {
        RtlZeroMemory(_BackingState.Buffer, _BackingState.BufferSize);
    }
}

VOID
Ktl::Com::KFabricOperationDataArray::FODKBuffer::Zero(__in ULONG Offset, __in ULONG Length)
{
    KFatal(Offset <= _BackingState.BufferSize);
    KFatal(_BackingState.BufferSize - Offset >= Length);
    if (Length > 0) 
    {
        RtlZeroMemory(_BackingState.Buffer + Offset, Length);
    }
}

VOID
Ktl::Com::KFabricOperationDataArray::FODKBuffer::CopyFrom(
    __in ULONG TargetOffset,
    __in KBuffer const& SourceBuffer,
    __in ULONG SourceOffset,
    __in ULONG Length)
{
    KFatal(TargetOffset <= _BackingState.BufferSize);
    KFatal(_BackingState.BufferSize - TargetOffset >= Length);
    KFatal(SourceOffset <= SourceBuffer.QuerySize());
    KFatal(SourceBuffer.QuerySize() - SourceOffset >= Length);
    if (Length > 0) 
    {
        KMemCpySafe(
            _BackingState.Buffer + TargetOffset, 
            _BackingState.BufferSize - TargetOffset, 
            (BYTE*)SourceBuffer.GetBuffer() + SourceOffset, 
            Length);
    }
}

NTSTATUS
Ktl::Com::KFabricOperationDataArray::FODKBuffer::CloneFrom(
    __in KBuffer const& SourceBuffer,
    __in ULONG AllocationTag)
{
    UNREFERENCED_PARAMETER(SourceBuffer);
    UNREFERENCED_PARAMETER(AllocationTag);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
Ktl::Com::KFabricOperationDataArray::FODKBuffer::Move(
    __in ULONG TargetOffset,
    __in ULONG SourceOffset,
    __in ULONG Length)
{
    KFatal(SourceOffset <= _BackingState.BufferSize);
    KFatal((_BackingState.BufferSize - SourceOffset) >= Length);
    KFatal(TargetOffset <= _BackingState.BufferSize);
    KFatal((_BackingState.BufferSize - TargetOffset) >= Length);
    if (Length > 0) 
    {
        RtlMoveMemory(_BackingState.Buffer + TargetOffset, _BackingState.Buffer + SourceOffset, Length);
    }
}

BOOLEAN
Ktl::Com::KFabricOperationDataArray::FODKBuffer::operator==(__in KBuffer const& Right)
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

BOOLEAN
Ktl::Com::KFabricOperationDataArray::FODKBuffer::operator!=(__in KBuffer const& Right)
{
    return !(*this == Right);
}


