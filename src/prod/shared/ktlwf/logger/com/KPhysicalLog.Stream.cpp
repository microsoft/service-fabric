// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "KPhysicalLog.h"
#include <FabricAsyncServiceBase.h>
#include <AsyncCallInAdapter.h>

namespace Ktl
{
    namespace Com
    {
        //*** IKPhysicalLogStream COM Implementation
        ComIKPhysicalLogStream::ComIKPhysicalLogStream(KtlLogStream& Stream)
            :   _BackingStream(&Stream)
        {
        }

        ComIKPhysicalLogStream::~ComIKPhysicalLogStream()
        {
        }

        HRESULT 
        ComIKPhysicalLogStream::Create(
            __in KAllocator& Allocator,  
            __in KtlLogStream& Stream, 
            __out ComIKPhysicalLogStream::SPtr& Result)
        {
            Result = _new(KTL_TAG_LOGGER, Allocator) ComIKPhysicalLogStream(Stream);
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

        
        //** IsFunctional
        HRESULT 
        ComIKPhysicalLogStream::IsFunctional()
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }

            return Common::ComUtility::OnPublicApiReturn(backingStream->IsFunctional() ? S_OK : S_FALSE);
        }

        HRESULT 
        ComIKPhysicalLogStream::QueryLogStreamId(__out KTL_LOG_STREAM_ID* const Result)
        {
            RvdLogStreamId      id;
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }

            backingStream->QueryLogStreamId(id);
            *Result = id.Get();

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }

        
        //** QueryReservedMetadataSize
        HRESULT 
        ComIKPhysicalLogStream::QueryReservedMetadataSize(__out ULONG* const Result)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            *Result = backingStream->QueryReservedMetadataSize();
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        //** QueryRecordRange
        struct QueryRecordRangeResults
        {
            KtlLogAsn OptionalLowestAsn;
            KtlLogAsn OptionalHighestAsn;
            KtlLogAsn OptionalLogTruncationAsn;
        };

        HRESULT 
        ComIKPhysicalLogStream::BeginQueryRecordRange(
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncQueryRecordRangeContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncQueryRecordRangeContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogStream::AsyncQueryRecordRangeContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<QueryRecordRangeResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<QueryRecordRangeResults>&)Adaptor;

                    Op.StartQueryRecordRange(
                        &adaptor.Extension.OptionalLowestAsn,
                        &adaptor.Extension.OptionalHighestAsn, 
                        &adaptor.Extension.OptionalLogTruncationAsn,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<QueryRecordRangeResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndQueryRecordRange(
            __in IFabricAsyncOperationContext* Context,
            __out_opt KTL_LOG_ASN* const OptionalLowestAsn,
            __out_opt KTL_LOG_ASN* const OptionalHighestAsn,
            __out_opt KTL_LOG_ASN* const OptionalLogTruncationAsn)
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncQueryRecordRangeContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (OptionalLowestAsn != nullptr)
            {
                *OptionalLowestAsn = ((AsyncCallInAdapter::Extended<QueryRecordRangeResults>*)Context)->Extension.OptionalLowestAsn.Get();
            }
            if (OptionalHighestAsn != nullptr)
            {
                *OptionalHighestAsn = ((AsyncCallInAdapter::Extended<QueryRecordRangeResults>*)Context)->Extension.OptionalHighestAsn.Get();
            }
            if (OptionalLogTruncationAsn != nullptr)
            {
                *OptionalLogTruncationAsn = ((AsyncCallInAdapter::Extended<QueryRecordRangeResults>*)Context)->Extension.OptionalLogTruncationAsn.Get();
            }

            return Common::ComUtility::OnPublicApiReturn(hr);
        }


        //** Write
        HRESULT 
        ComIKPhysicalLogStream::BeginWrite(
            __in KTL_LOG_ASN Asn,
            __in ULONGLONG Version,
            __in ULONG MetadataSize,
            __in IKIoBuffer* const MetadataBuffer,
            __in IKIoBuffer* const IoBuffer,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if ((Context == nullptr) || ((MetadataBuffer == nullptr) && (IoBuffer == nullptr)))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KSharedPtr<KtlLogStream::AsyncWriteContext>    op;

            NTSTATUS    status = backingStream->CreateAsyncWriteContext(op);
            if (!NT_SUCCESS(status))
            {
                 return Common::ComUtility::OnPublicApiReturn(KComUtility::ToHRESULT(status));
            }

            KIoBuffer::SPtr     metadataBuffer;
            KIoBuffer::SPtr     ioBuffer;

            if (MetadataBuffer != nullptr)
            {
                metadataBuffer = &((ComKIoBuffer*)MetadataBuffer)->GetBackingKIoBuffer();
            }
            if (IoBuffer != nullptr)
            {
                ioBuffer = &((ComKIoBuffer*)IoBuffer)->GetBackingKIoBuffer();
            }
            KAssert((metadataBuffer != nullptr) || (ioBuffer != nullptr));


            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogStream::AsyncWriteContext& Operation, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Operation.StartWrite(
                        KtlLogAsn(Asn),
                        Version,
                        MetadataSize,
                        metadataBuffer,
                        ioBuffer, 
                        0,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndWrite(__in IFabricAsyncOperationContext* Context)
        {
            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncWriteContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        //** Read
        struct ReadResults
        {
            KtlLogAsn       Asn;
            ULONGLONG       Version;
            ULONG           MetaDataLength;
            KIoBuffer::SPtr MetaDataBuffer;
            KIoBuffer::SPtr IoBuffer;
        };

        HRESULT 
        ComIKPhysicalLogStream::BeginRead(
            __in KTL_LOG_ASN Asn,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncReadContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncReadContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogStream::AsyncReadContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<ReadResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<ReadResults>&)Adaptor;

                    Op.StartRead(
                        KtlLogAsn(Asn),
                        &adaptor.Extension.Version,
                        adaptor.Extension.MetaDataLength,
                        adaptor.Extension.MetaDataBuffer,
                        adaptor.Extension.IoBuffer,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<ReadResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndRead(
            __in IFabricAsyncOperationContext* Context,
            __out_opt ULONGLONG* const OptionalVersion,
            __out ULONG* const ResultingMetadataSize,
            __out IKIoBuffer** const ResultingMetadata,
            __out IKIoBuffer** const ResultingIoBuffer)
        {
            if ((Context == nullptr) || 
                (ResultingMetadataSize == nullptr) ||
                (ResultingMetadata == nullptr) || 
                (ResultingIoBuffer == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncReadContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }


            ReadResults&        results = ((AsyncCallInAdapter::Extended<ReadResults>*)Context)->Extension;
            ComKIoBuffer::SPtr  metadataResult = _new(GetThisAllocationTag(), GetThisAllocator()) ComKIoBuffer(*results.MetaDataBuffer);
            ComKIoBuffer::SPtr  pageAlignedResult = _new(GetThisAllocationTag(), GetThisAllocator()) ComKIoBuffer(*results.IoBuffer);

            if ((metadataResult == nullptr) || (pageAlignedResult == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            if (!NT_SUCCESS(metadataResult->Status()))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(metadataResult->Status()));
            }

            if (!NT_SUCCESS(pageAlignedResult->Status()))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(pageAlignedResult->Status()));
            }

            if (OptionalVersion != nullptr)
            {
                *OptionalVersion = results.Version;
            }

            *ResultingMetadataSize = results.MetaDataLength;
            *ResultingMetadata = metadataResult.Detach();
            *ResultingIoBuffer = pageAlignedResult.Detach();

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        HRESULT 
        ComIKPhysicalLogStream::BeginReadContaining(
            __in KTL_LOG_ASN Asn,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncReadContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncReadContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogStream::AsyncReadContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<ReadResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<ReadResults>&)Adaptor;

                    Op.StartReadContaining(
                        KtlLogAsn(Asn),
                        &adaptor.Extension.Asn,
                        &adaptor.Extension.Version,
                        adaptor.Extension.MetaDataLength,
                        adaptor.Extension.MetaDataBuffer,
                        adaptor.Extension.IoBuffer,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<ReadResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndReadContaining(
            __in IFabricAsyncOperationContext* Context,
            __out KTL_LOG_ASN* const ContainingAsn,
            __out ULONGLONG* const ContainingAsnVersion,
            __out ULONG* const ResultingMetadataSize,
            __out IKIoBuffer** const ResultingMetadata,
            __out IKIoBuffer** const ResultingIoBuffer)
        {
            if ((Context == nullptr)                || 
                (ResultingMetadataSize == nullptr)  ||
                (ResultingMetadata == nullptr)      || 
                (ResultingIoBuffer == nullptr)      ||
                (ContainingAsn == nullptr)          ||
                (ContainingAsnVersion == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncReadContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }


            ReadResults&        results = ((AsyncCallInAdapter::Extended<ReadResults>*)Context)->Extension;
            ComKIoBuffer::SPtr  metadataResult = _new(GetThisAllocationTag(), GetThisAllocator()) ComKIoBuffer(*results.MetaDataBuffer);
            ComKIoBuffer::SPtr  pageAlignedResult = _new(GetThisAllocationTag(), GetThisAllocator()) ComKIoBuffer(*results.IoBuffer);

            if ((metadataResult == nullptr) || (pageAlignedResult == nullptr))
            {
                return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            if (!NT_SUCCESS(metadataResult->Status()))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(metadataResult->Status()));
            }

            if (!NT_SUCCESS(pageAlignedResult->Status()))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(pageAlignedResult->Status()));
            }

            *ContainingAsn = results.Asn.Get();
            *ContainingAsnVersion = results.Version;
            *ResultingMetadataSize = results.MetaDataLength;
            *ResultingMetadata = metadataResult.Detach();
            *ResultingIoBuffer = pageAlignedResult.Detach();

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        //** Multiple record Read
        struct MultiRecordReadResults
        {
            KIoBuffer::SPtr MetaDataBuffer;
            KIoBuffer::SPtr IoBuffer;
        };

        HRESULT
            ComIKPhysicalLogStream::BeginMultiRecordRead(
            __in KTL_LOG_ASN Asn,
            __in ULONG BytesToRead,
            __in IFabricAsyncOperationCallback* Callback,
            __out IFabricAsyncOperationContext** Context)
        {
                KtlLogStream::SPtr backingStream = _BackingStream;
                if (backingStream == nullptr)
                {
                    return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
                }

                if (nullptr == Context)
                {
                    return Common::ComUtility::OnPublicApiReturn(E_POINTER);
                }

                *Context = nullptr;
                KtlLogStream::AsyncMultiRecordReadContext::SPtr    op;

                NTSTATUS    status = backingStream->CreateAsyncMultiRecordReadContext(op);
                if (!NT_SUCCESS(status))
                {
                    return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
                }

                HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                    GetThisAllocator(),
                    op,
                    Callback,
                    Context,
                    [&](AsyncCallInAdapter& Adaptor,
                    KtlLogStream::AsyncMultiRecordReadContext& Op,
                    KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<MultiRecordReadResults>&   adaptor =
                        (AsyncCallInAdapter::Extended<MultiRecordReadResults>&)Adaptor;

                    const ULONG metadataSize = 4096;
                    ULONG paddedBytesToRead = ((BytesToRead + (metadataSize - 1)) / metadataSize) * metadataSize;
                    KIoBuffer::SPtr metadataBuffer;
                    PVOID metadataBufferPtr;
                    KIoBuffer::SPtr ioBuffer;
                    PVOID ioBufferPtr;

                    status = KIoBuffer::CreateSimple(metadataSize, metadataBuffer, metadataBufferPtr, GetThisAllocator());
                    if (! NT_SUCCESS(status))
                    {
                        return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
                    }

                    status = KIoBuffer::CreateSimple(paddedBytesToRead, ioBuffer, ioBufferPtr, GetThisAllocator());
                    if (!NT_SUCCESS(status))
                    {
                        return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
                    }

                    adaptor.Extension.MetaDataBuffer = metadataBuffer;
                    adaptor.Extension.IoBuffer = ioBuffer;

                    Op.StartRead(
                        KtlLogAsn(Asn),
                        *adaptor.Extension.MetaDataBuffer,
                        *adaptor.Extension.IoBuffer,
                        nullptr,
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom Adaptor factory
                [&](KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                    return AsyncCallInAdapter::Extended<MultiRecordReadResults>::Create(Allocator, Adapter);
                });

                return Common::ComUtility::OnPublicApiReturn(hr);
            }

        HRESULT
            ComIKPhysicalLogStream::EndMultiRecordRead(
            __in IFabricAsyncOperationContext* Context,
            __out IKIoBuffer** const ResultingMetadata,
            __out IKIoBuffer** const ResultingIoBuffer)
        {
                if ((Context == nullptr) ||
                    (ResultingMetadata == nullptr) ||
                    (ResultingIoBuffer == nullptr))
                {
                    return Common::ComUtility::OnPublicApiReturn(E_POINTER);
                }

                KtlLogStream::AsyncReadContext::SPtr    op;

                HRESULT     hr = AsyncCallInAdapter::End(Context, op);
                if (FAILED(hr))
                {
                    return Common::ComUtility::OnPublicApiReturn(hr);
                }


                MultiRecordReadResults&        results = ((AsyncCallInAdapter::Extended<MultiRecordReadResults>*)Context)->Extension;
                ComKIoBuffer::SPtr  metadataResult = _new(GetThisAllocationTag(), GetThisAllocator()) ComKIoBuffer(*results.MetaDataBuffer);
                ComKIoBuffer::SPtr  pageAlignedResult = _new(GetThisAllocationTag(), GetThisAllocator()) ComKIoBuffer(*results.IoBuffer);

                if ((metadataResult == nullptr) || (pageAlignedResult == nullptr))
                {
                    return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
                }

                if (!NT_SUCCESS(metadataResult->Status()))
                {
                    return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(metadataResult->Status()));
                }

                if (!NT_SUCCESS(pageAlignedResult->Status()))
                {
                    return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(pageAlignedResult->Status()));
                }

                *ResultingMetadata = metadataResult.Detach();
                *ResultingIoBuffer = pageAlignedResult.Detach();

                return Common::ComUtility::OnPublicApiReturn(S_OK);
            }

        //** QueryRecord
        struct QueryRecordResults
        {
            ULONGLONG       Version;
            RvdLogStream::RecordDisposition Disposition;
            ULONG           IoBufferSize;
            ULONGLONG       DebugInfo1;
        };

        HRESULT 
        ComIKPhysicalLogStream::BeginQueryRecord(
            __in KTL_LOG_ASN Asn,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncQueryRecordContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncQueryRecordContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogStream::AsyncQueryRecordContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<QueryRecordResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<QueryRecordResults>&)Adaptor;

                    Op.StartQueryRecord(
                        KtlLogAsn(Asn),
                        &adaptor.Extension.Version,
                        &adaptor.Extension.Disposition,
                        &adaptor.Extension.IoBufferSize,
                        &adaptor.Extension.DebugInfo1,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<QueryRecordResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndQueryRecord(
            __in IFabricAsyncOperationContext* Context,
            __out_opt ULONGLONG* const OptionalVersion,
            __out_opt KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION* const OptionalDisposition,
            __out_opt ULONG* const OptionalIoBufferSize,
            __out_opt ULONGLONG* const OptionalDebugInfo1)
        {
            if (Context == nullptr) 
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncQueryRecordContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }


            QueryRecordResults& results = ((AsyncCallInAdapter::Extended<QueryRecordResults>*)Context)->Extension;
            if (OptionalVersion != nullptr)
            {
                *OptionalVersion = results.Version;
            }
            if (OptionalDisposition != nullptr)
            {
                *OptionalDisposition = (KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION)results.Disposition;
            }
            if (OptionalIoBufferSize != nullptr)
            {
                *OptionalIoBufferSize = results.IoBufferSize;
            }
            if (OptionalDebugInfo1 != nullptr)
            {
                *OptionalDebugInfo1 = results.DebugInfo1;
            }

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        //** QueryRecords
        HRESULT 
        ComIKPhysicalLogStream::BeginQueryRecords(
            __in KTL_LOG_ASN LowestAsn,
            __in KTL_LOG_ASN HighestAsn,
            __in IKArray* const ResultArray,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncQueryRecordsContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncQueryRecordsContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            KArray<RvdLogStream::RecordMetadata>&   resultsArray = 
                (KArray<RvdLogStream::RecordMetadata>&)((ComIKArrayBase*)ResultArray)->GetBackingArray();

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogStream::AsyncQueryRecordsContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Op.StartQueryRecords(
                        KtlLogAsn(LowestAsn),
                        KtlLogAsn(HighestAsn),
                        resultsArray,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndQueryRecords(
            __in IFabricAsyncOperationContext* Context)
        {
            if (Context == nullptr) 
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncQueryRecordsContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            return Common::ComUtility::OnPublicApiReturn(hr);
        }


        //** Truncate
        HRESULT 
        ComIKPhysicalLogStream::Truncate(
            __in KTL_LOG_ASN TruncationPoint,
            __in KTL_LOG_ASN PreferredTruncationPoint)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            KtlLogStream::AsyncTruncateContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncTruncateContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            op->Truncate(KtlLogAsn(TruncationPoint), KtlLogAsn(PreferredTruncationPoint));
            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        //** WaitForNotification
        HRESULT 
        ComIKPhysicalLogStream::BeginWaitForNotification(
            __in KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE NotificationType,
            __in ULONGLONG NotificationValue,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncStreamNotificationContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncStreamNotificationContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter&,
                     KtlLogStream::AsyncStreamNotificationContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    Op.StartWaitForNotification(
                        (KtlLogStream::AsyncStreamNotificationContext::ThresholdTypeEnum)NotificationType,
                        NotificationValue,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndWaitForNotification(
            __in IFabricAsyncOperationContext* Context)
        {
            if (Context == nullptr) 
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncStreamNotificationContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            return Common::ComUtility::OnPublicApiReturn(hr);
        }


        //** Ioctl
        struct IoctlResults
        {
            KBuffer::SPtr   _OutBuffer;
            ULONG           _Result;
        };

        HRESULT 
        ComIKPhysicalLogStream::BeginIoctl(
            __in ULONG ControlCode,
            __in IKBuffer* const InBuffer,
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            KtlLogStream::SPtr backingStream = _BackingStream;
            if (backingStream == nullptr)
            {
                return(Common::ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE)));
            }
            
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KtlLogStream::AsyncIoctlContext::SPtr    op;

            NTSTATUS    status = backingStream->CreateAsyncIoctlContext(op);
            if (!NT_SUCCESS(status))
            {
                return Common::ComUtility::OnPublicApiReturn(Ktl::Com::KComUtility::ToHRESULT(status));
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                op,
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KtlLogStream::AsyncIoctlContext& Op, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<IoctlResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<IoctlResults>&)Adaptor;

                    Op.StartIoctl(
                        ControlCode,
                        (InBuffer == nullptr) ? nullptr : &((ComKBuffer*)InBuffer)->GetBackingBuffer(),
                        adaptor.Extension._Result,
                        adaptor.Extension._OutBuffer,
                        nullptr, 
                        AsyncCallback);

                    return STATUS_SUCCESS;
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adaptor) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<IoctlResults>::Create(Allocator, Adaptor);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndIoctl(
            __in IFabricAsyncOperationContext* Context,
            __out ULONG *const Result,
            __out IKBuffer** const OutBuffer)
        {
            if (Context == nullptr) 
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KtlLogStream::AsyncIoctlContext::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            IoctlResults& results = ((AsyncCallInAdapter::Extended<IoctlResults>*)Context)->Extension;

            if (Result != nullptr)
            {
                *Result = results._Result;
            }
            if (OutBuffer != nullptr)
            {
                ComKBuffer::SPtr        result;
                if (results._OutBuffer != nullptr)
                {
                    hr = ComKBuffer::Create(GetThisAllocator(), *(results._OutBuffer), result);
                    if (FAILED(hr))
                    {
                        *OutBuffer = nullptr;
                        return Common::ComUtility::OnPublicApiReturn(hr);
                    }
                } else {
                    *OutBuffer = nullptr;
                }

                *OutBuffer = result.Detach();
            }

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }


        //** CloseStream
        struct CloseStreamResults
        {
            class FakeAsync : public KAsyncContextBase
            {
            public:
                using KObject<KAsyncContextBase>::SetStatus;
            };

            KSharedPtr<FakeAsync>                   _FakeCloseOp;
            KAsyncContextBase::CompletionCallback   _OuterCallback;

            VOID CloseCallbackIntercept(
                KAsyncContextBase* const Parent,
                KtlLogStream&,
                NTSTATUS Status)
            {
                _FakeCloseOp->SetStatus(Status);
                _OuterCallback(Parent, *_FakeCloseOp);
            }
        };

        HRESULT 
        ComIKPhysicalLogStream::BeginClose(
            __in IFabricAsyncOperationCallback* Callback,       
            __out IFabricAsyncOperationContext** Context)
        {
            if (nullptr == Context)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            *Context = nullptr;
            KSharedPtr<CloseStreamResults::FakeAsync>    op;

            op = _new(GetThisAllocationTag(), GetThisAllocator()) CloseStreamResults::FakeAsync();
            if (op == nullptr)
            {
                 return Common::ComUtility::OnPublicApiReturn(E_OUTOFMEMORY);
            }

            HRESULT hr = AsyncCallInAdapter::CreateAndStart(
                GetThisAllocator(),
                Callback,
                Context,
                [&] (AsyncCallInAdapter& Adaptor,
                     KAsyncContextBase&, 
                     KAsyncContextBase::CompletionCallback AsyncCallback) -> NTSTATUS
                {
                    AsyncCallInAdapter::Extended<CloseStreamResults>&   adaptor = 
                        (AsyncCallInAdapter::Extended<CloseStreamResults>&)Adaptor;

                    adaptor.Extension._FakeCloseOp = Ktl::Move(op);
                    adaptor.Extension._OuterCallback = AsyncCallback;
                    return  _BackingStream->StartClose(
                        nullptr, 
                        KtlLogStream::CloseCompletionCallback(
                            &adaptor.Extension, 
                            &CloseStreamResults::CloseCallbackIntercept));
                },

                // Custom Adaptor factory
                [&] (KAllocator& Allocator, AsyncCallInAdapter::SPtr& Adapter) -> NTSTATUS
                {
                   return AsyncCallInAdapter::Extended<CloseStreamResults>::Create(Allocator, Adapter);
                });

            return Common::ComUtility::OnPublicApiReturn(hr);
        }

        HRESULT 
        ComIKPhysicalLogStream::EndClose(
            __in IFabricAsyncOperationContext* Context)
        {
            _BackingStream = nullptr;

            if (Context == nullptr)
            {
                return Common::ComUtility::OnPublicApiReturn(E_POINTER);
            }

            KAsyncContextBase::SPtr    op;

            HRESULT     hr = AsyncCallInAdapter::End(Context, op);
            if (FAILED(hr))
            {
                return Common::ComUtility::OnPublicApiReturn(hr);
            }

            return Common::ComUtility::OnPublicApiReturn(S_OK);
        }
    }
}


