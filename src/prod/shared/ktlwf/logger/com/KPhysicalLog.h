// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ktl.h"
#include "KUnknown.h"
#include "KComUtility.h"
#include "ktllogger.h"
#include "Common/Common.h"
#include "kphysicallog_.h"

// Validate KtlLogger and kphysicallog_ defs match
static_assert(KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE::LogSpaceUtilization == KtlLogStream::AsyncStreamNotificationContext::ThresholdTypeEnum::LogSpaceUtilization, "Definition Mismatch");
static_assert(KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION::DispositionPending == RvdLogStream::RecordDisposition::eDispositionPending, "Definition Mismatch");
static_assert(KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION::DispositionPersisted == RvdLogStream::RecordDisposition::eDispositionPersisted, "Definition Mismatch");
static_assert(KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION::DispositionNone == RvdLogStream::RecordDisposition::eDispositionNone, "Definition Mismatch");
static_assert(FIELD_OFFSET(KPHYSICAL_LOG_STREAM_RecordMetadata, Asn) == FIELD_OFFSET(RvdLogStream::RecordMetadata, Asn), "Definition Mismatch");
static_assert(FIELD_OFFSET(KPHYSICAL_LOG_STREAM_RecordMetadata, Version) == FIELD_OFFSET(RvdLogStream::RecordMetadata, Version), "Definition Mismatch");
static_assert(FIELD_OFFSET(KPHYSICAL_LOG_STREAM_RecordMetadata, Disposition) == FIELD_OFFSET(RvdLogStream::RecordMetadata, Disposition), "Definition Mismatch");
static_assert(FIELD_OFFSET(KPHYSICAL_LOG_STREAM_RecordMetadata, Size) == FIELD_OFFSET(RvdLogStream::RecordMetadata, Size), "Definition Mismatch");
static_assert(FIELD_OFFSET(KPHYSICAL_LOG_STREAM_RecordMetadata, Debug_LsnValue) == FIELD_OFFSET(RvdLogStream::RecordMetadata, Debug_LsnValue), "Definition Mismatch");


namespace Ktl
{
    namespace Com
    {
        using ::_delete;

        //** KIoBufferElement COM definitions - see IKIoBufferElement interface documentation in NativeLog.cs
        class ComKIoBufferElement : public KShared<ComKIoBufferElement>,
                                    public KObject<ComKIoBufferElement>,
                                    public IKIoBufferElement
        {
            K_FORCE_SHARED(ComKIoBufferElement)

            K_BEGIN_COM_INTERFACE_LIST(ComKIoBufferElement)
                COM_INTERFACE_ITEM(IID_IUnknown, IKIoBufferElement)
                COM_INTERFACE_ITEM(IID_IKIoBufferElement, IKIoBufferElement)
            K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT Create(__in KAllocator& Allocator,  __in ULONG Size, __out ComKIoBufferElement::SPtr& Result);

        private:
            HRESULT STDMETHODCALLTYPE GetBuffer(__out BYTE **const Result) override;
            HRESULT STDMETHODCALLTYPE QuerySize( __out ULONG *const Result) override;

        private:
            friend class ComKIoBuffer;
            ComKIoBufferElement(ULONG Size);
            ComKIoBufferElement(KIoBufferElement& BackingElement);

            KIoBufferElement& GetBackingElement() { return *_BackingElement; }

        private:
            KIoBufferElement::SPtr  _BackingElement;
        };

        
        //** KIoBuffer COM definitions - see IKIoBuffer interface documentation in NativeLog.cs
        class ComKIoBuffer : public KShared<ComKIoBuffer>,
                             public KObject<ComKIoBuffer>,
                             public IKIoBuffer
        {
            K_FORCE_SHARED(ComKIoBuffer)

            K_BEGIN_COM_INTERFACE_LIST(ComKIoBuffer)
                COM_INTERFACE_ITEM(IID_IUnknown, IKIoBuffer)
                COM_INTERFACE_ITEM(IID_IKIoBuffer, IKIoBuffer)
            K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT CreateSimple(__in KAllocator& Allocator,  __in ULONG Size, __out ComKIoBuffer::SPtr& Result);
            static HRESULT CreateEmpty(__in KAllocator& Allocator, __out ComKIoBuffer::SPtr& Result);

        private:
            HRESULT STDMETHODCALLTYPE AddIoBufferElement(__in IKIoBufferElement *const Element) override;
            HRESULT STDMETHODCALLTYPE QueryNumberOfIoBufferElements(__out ULONG *const Result) override;
            HRESULT STDMETHODCALLTYPE QuerySize(__out ULONG *const Result) override;
            HRESULT STDMETHODCALLTYPE First(__out void **const ResultToken) override;
            HRESULT STDMETHODCALLTYPE Next(__in void *const CurrentToken, __out void **const ResultToken) override;
            HRESULT STDMETHODCALLTYPE Clear() override;
            HRESULT STDMETHODCALLTYPE AddIoBuffer(__in IKIoBuffer *const ToAdd) override;
            HRESULT STDMETHODCALLTYPE AddIoBufferReference(
                __in IKIoBuffer *const SourceIoBuffer, 
                __in ULONG SourceStartingOffset,
                __in ULONG Size) override;

            HRESULT STDMETHODCALLTYPE QueryElementInfo(
                __in void *const Token, 
                __out BYTE **const ResultBuffer, 
                __out ULONG *const ResultSize) override;
        
            HRESULT STDMETHODCALLTYPE GetElement(__in void *const FromToken, __out IKIoBufferElement **const Result) override;
            HRESULT STDMETHODCALLTYPE GetElements(__out ULONG* const NumberOfElements, __out IKArray** const Result) override;

        private:
            friend class ComIKPhysicalLogStream;

            ComKIoBuffer(KIoBuffer& BackingIoBuffer);
            KIoBuffer& GetBackingKIoBuffer() { return *_BackingIoBuffer; }

        private:
            KIoBuffer::SPtr     _BackingIoBuffer;
        };

       
        //** KBuffer COM definitions - see IKBuffer interface documentation in NativeLog.cs
        class ComKBuffer : public KShared<ComKBuffer>,
                           public KObject<ComKBuffer>,
                           public IKBuffer
        {
            K_FORCE_SHARED(ComKBuffer)

            K_BEGIN_COM_INTERFACE_LIST(ComKBuffer)
                COM_INTERFACE_ITEM(IID_IUnknown, IKBuffer)
                COM_INTERFACE_ITEM(IID_IKBuffer, IKBuffer)
            K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT Create(__in KAllocator& Allocator,  __in ULONG Size, __out ComKBuffer::SPtr& Result);
            static HRESULT Create(__in KAllocator& Allocator,  __in KBuffer& From, __out ComKBuffer::SPtr& Result);

        private:
            HRESULT STDMETHODCALLTYPE GetBuffer(__out BYTE **const Result) override;
            HRESULT STDMETHODCALLTYPE QuerySize( __out ULONG *const Result) override;

        private:
            friend class ComIKPhysicalLogStream;
            friend class ComIKPhysicalLogContainer;
            ComKBuffer(ULONG Size);
            ComKBuffer(KBuffer& BackingBuffer);

            KBuffer& GetBackingBuffer() { return *_BackingBuffer; }

        private:
            KBuffer::SPtr  _BackingBuffer;
        };


        //** KArray<void*> COM Definitions - see IKArray interface documentation in NativeLog.cs
        class ComIKArrayBase : public KShared<ComIKArrayBase>,
                               public KObject<ComIKArrayBase>,
                               public IKArray
        {
            K_FORCE_SHARED_WITH_INHERITANCE(ComIKArrayBase)

            K_BEGIN_COM_INTERFACE_LIST(ComIKArrayBase)
                COM_INTERFACE_ITEM(IID_IUnknown, IKArray)
                COM_INTERFACE_ITEM(IID_IKArray, IKArray)
            K_END_COM_INTERFACE_LIST()

        private:
            HRESULT STDMETHODCALLTYPE GetStatus() override;
            HRESULT STDMETHODCALLTYPE GetCount(__out ULONG* const Result) override;
            HRESULT STDMETHODCALLTYPE GetArrayBase(__out void** const Result) override;
            HRESULT STDMETHODCALLTYPE AppendGuid(__in GUID *const ToAdd);
            HRESULT STDMETHODCALLTYPE AppendRecordMetadata(__in KPHYSICAL_LOG_STREAM_RecordMetadata *const ToAdd);

        protected:
            friend class ComIKPhysicalLogStream;
            virtual KArray<void*>& GetBackingArray() = 0;
        };

        template <typename ArrayType>
        class ComIKArray : public ComIKArrayBase
        {
            K_FORCE_SHARED(ComIKArray)

        public:
            ComIKArray(ULONG ReserveSize);
            static HRESULT Create(__in KAllocator& Allocator,  __in ULONG ReserveSize, __out KSharedPtr<ComIKArray<ArrayType>>& Result);

            //* For test support only
            __inline KArray<void*>& GetBackingArray() override
            {
                return (KArray<void*>&)_BackingArray;
            }

        private:
            KArray<ArrayType>   _BackingArray;
        };


        //* IKPhysicalLogManager COM Definitions - see IKPhysicalLogManager interface documentation in NativeLog.cs
        class ComIKPhysicalLogManager : public KShared<ComIKPhysicalLogManager>,
                                        public KObject<ComIKPhysicalLogManager>,
                                        public IKPhysicalLogManager
        {
            K_FORCE_SHARED(ComIKPhysicalLogManager)

            ComIKPhysicalLogManager(__in BOOLEAN Inproc);
                    
            K_BEGIN_COM_INTERFACE_LIST(ComIKPhysicalLogManager)
                COM_INTERFACE_ITEM(IID_IUnknown, IKPhysicalLogManager)
                COM_INTERFACE_ITEM(IID_IKPhysicalLogManager, IKPhysicalLogManager)
            K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT Create(__in KAllocator& Allocator, __out ComIKPhysicalLogManager::SPtr& Result);
            static HRESULT CreateInproc(__in KAllocator& Allocator, __out ComIKPhysicalLogManager::SPtr& Result);

        private:
            HRESULT STDMETHODCALLTYPE BeginOpen( 
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE EndOpen( 
                __in IFabricAsyncOperationContext *const Context) override;

        
            HRESULT STDMETHODCALLTYPE BeginClose( 
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE EndClose( 
                __in IFabricAsyncOperationContext *const Context) override;

        
            HRESULT STDMETHODCALLTYPE BeginCreateLogContainer( 
                __in LPCWSTR FullyQualifiedLogFilePath,
                __in KTL_LOG_ID LogId,
                __in LPCWSTR LogType,
                __in LONGLONG LogSize,
                __in ULONG MaxAllowedStreams,
                __in ULONG MaxRecordSize,
                __in DWORD CreationFlags,
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE EndCreateLogContainer( 
                __in IFabricAsyncOperationContext *const Context,
                __out IKPhysicalLogContainer **const Result) override;

        
            HRESULT STDMETHODCALLTYPE BeginOpenLogContainerById( 
                __in KTL_DISK_ID DiskId,
                __in KTL_LOG_ID LogId,
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE BeginOpenLogContainer( 
                __in LPCWSTR FullyQualifiedLogFilePath,
                __in KTL_LOG_ID LogId,
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE EndOpenLogContainer( 
                __in IFabricAsyncOperationContext *const Context,
                __out IKPhysicalLogContainer **const Result) override;
        

            HRESULT STDMETHODCALLTYPE BeginDeleteLogContainer( 
                __in LPCWSTR FullyQualifiedLogFilePath,
                __in KTL_LOG_ID LogId,
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE BeginDeleteLogContainerById( 
                __in KTL_DISK_ID DiskId,
                __in KTL_LOG_ID LogId,
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE EndDeleteLogContainer( 
                __in IFabricAsyncOperationContext *const Context) override;
        

            HRESULT STDMETHODCALLTYPE BeginEnumerateLogContainers( 
                __in KTL_DISK_ID DiskId,
                __in IFabricAsyncOperationCallback *const Callback,
                __out IFabricAsyncOperationContext **const Context) override;
        
            HRESULT STDMETHODCALLTYPE EndEnumerateLogContainers( 
                __in IFabricAsyncOperationContext *const Context,
                __out IKArray **const Result) override;

#if !defined(PLATFORM_UNIX)
            HRESULT STDMETHODCALLTYPE BeginGetVolumeIdFromPath(
                __in LPCWSTR Path,
                __in IFabricAsyncOperationCallback* const Callback,     
                __out IFabricAsyncOperationContext** const Context) override;

            HRESULT STDMETHODCALLTYPE EndGetVolumeIdFromPath(
                __in IFabricAsyncOperationContext* const Context,
                __out GUID* const Result) override;
#endif

        private:
            KtlLogManager::SPtr     _BackingManager;
        };
    
        //* IKPhysicalLogManager COM Definitions - see IKPhysicalLogContainer interface documentation in NativeLog.cs
        class ComIKPhysicalLogContainer :   public KShared<ComIKPhysicalLogContainer>,
                                            public KObject<ComIKPhysicalLogContainer>,
                                            public IKPhysicalLogContainer
        {
            K_FORCE_SHARED(ComIKPhysicalLogContainer)

            K_BEGIN_COM_INTERFACE_LIST(ComIKPhysicalLogContainer)
                COM_INTERFACE_ITEM(IID_IUnknown, IKPhysicalLogContainer)
                COM_INTERFACE_ITEM(IID_IKPhysicalLogContainer, IKPhysicalLogContainer)
            K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT Create(__in KAllocator& Allocator, __in KtlLogContainer& Containter, __out ComIKPhysicalLogContainer::SPtr& Result);

        private:
            ComIKPhysicalLogContainer(KtlLogContainer& Containter);

            HRESULT STDMETHODCALLTYPE IsFunctional() override;

            HRESULT STDMETHODCALLTYPE BeginCreateLogStream(
                __in KTL_LOG_STREAM_ID LogStreamId,
                __in KTL_LOG_STREAM_TYPE_ID LogStreamTypeId,
                __in_opt LPCWSTR OptionalLogStreamAlias,
                __in LPCWSTR Path,
                __in_opt IKBuffer *const OptionalSecurityInfo,
                __in LONGLONG MaximumSize,
                __in ULONG MaximumBlockSize,
                __in DWORD CreationFlags,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndCreateLogStream(
                __in IFabricAsyncOperationContext* Context,
                __out IKPhysicalLogStream** Result) override;


            HRESULT STDMETHODCALLTYPE BeginDeleteLogStream(
                __in KTL_LOG_STREAM_ID LogStreamId,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndDeleteLogStream(
                __in IFabricAsyncOperationContext* Context) override;


            HRESULT STDMETHODCALLTYPE BeginOpenLogStream(
                __in KTL_LOG_STREAM_ID LogStreamId,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndOpenLogStream(
                __in IFabricAsyncOperationContext* Context,
                __out IKPhysicalLogStream** Result) override;


            HRESULT STDMETHODCALLTYPE BeginAssignAlias(
                __in LPCWSTR Alias,
                __in KTL_LOG_STREAM_ID LogStreamId,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndAssignAlias(
                __in IFabricAsyncOperationContext* Context) override;


            HRESULT STDMETHODCALLTYPE BeginRemoveAlias(
                __in LPCWSTR Alias,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndRemoveAlias(
                __in IFabricAsyncOperationContext* Context) override;


            HRESULT STDMETHODCALLTYPE BeginResolveAlias(
                __in LPCWSTR Alias,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndResolveAlias(
                __in IFabricAsyncOperationContext* Context,
                __out KTL_LOG_STREAM_ID* const Result) override;
    

            HRESULT STDMETHODCALLTYPE BeginClose(
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndClose(
                __in IFabricAsyncOperationContext* Context) override;

        private:
            template <typename OpType>
            HRESULT CommonEndOpenLogStream(
                __in IFabricAsyncOperationContext* Context,
                __out IKPhysicalLogStream** Result);

            static RvdLogStreamType const& KtlDefaultStreamType();

        private:
            KtlLogContainer::SPtr   _BackingContainer;
        };
        
        //* IKPhysicalLogStream COM Definitions - see IKPhysicalLogStream interface documentation in NativeLog.cs
        class ComIKPhysicalLogStream :  public KShared<ComIKPhysicalLogStream>,
                                        public KObject<ComIKPhysicalLogStream>,
                                        public IKPhysicalLogStream2
        {
            K_FORCE_SHARED(ComIKPhysicalLogStream)

            K_BEGIN_COM_INTERFACE_LIST(ComIKPhysicalLogStream)
                COM_INTERFACE_ITEM(IID_IUnknown, IKPhysicalLogStream)
                COM_INTERFACE_ITEM(IID_IKPhysicalLogStream, IKPhysicalLogStream)
                COM_INTERFACE_ITEM(IID_IKPhysicalLogStream2, IKPhysicalLogStream2)
            K_END_COM_INTERFACE_LIST()

        public:
            static HRESULT Create(
                __in KAllocator& Allocator, 
                __in KtlLogStream& Stream, 
                __out ComIKPhysicalLogStream::SPtr& Result);

        private:
            ComIKPhysicalLogStream(KtlLogStream& Stream);

            HRESULT STDMETHODCALLTYPE IsFunctional() override;

            HRESULT STDMETHODCALLTYPE QueryLogStreamId(__out KTL_LOG_STREAM_ID* const Result) override;

            HRESULT STDMETHODCALLTYPE QueryReservedMetadataSize(__out ULONG* const Result) override;


            HRESULT STDMETHODCALLTYPE BeginQueryRecordRange(
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndQueryRecordRange(
                __in IFabricAsyncOperationContext* Context,
                __out_opt KTL_LOG_ASN* const OptionalLowestAsn,
                __out_opt KTL_LOG_ASN* const OptionalHighestAsn,
                __out_opt KTL_LOG_ASN* const OptionalLogTruncationAsn) override;


            HRESULT STDMETHODCALLTYPE BeginWrite(
                __in KTL_LOG_ASN Asn,
                __in ULONGLONG Version,
                __in ULONG MetadataSize,
                __in IKIoBuffer* const MetadataBuffer,
                __in IKIoBuffer* const IoBuffer,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndWrite(__in IFabricAsyncOperationContext* Context) override;


            HRESULT STDMETHODCALLTYPE BeginRead(
                __in KTL_LOG_ASN Asn,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndRead(
                __in IFabricAsyncOperationContext* Context,
                __out_opt ULONGLONG* const OptionalVersion,
                __out ULONG* const ResultingMetadataSize,
                __out IKIoBuffer** const ResultingMetadata,
                __out IKIoBuffer** const ResultingIoBuffer) override;

            HRESULT STDMETHODCALLTYPE BeginReadContaining(
                __in KTL_LOG_ASN Asn,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndReadContaining(
                __in IFabricAsyncOperationContext* Context,
                __out_opt ULONGLONG* const ResultingAsn,
                __out_opt ULONGLONG* const ResultingVersion,
                __out ULONG* const ResultingMetadataSize,
                __out IKIoBuffer** const ResultingMetadata,
                __out IKIoBuffer** const ResultingIoBuffer) override;


            HRESULT STDMETHODCALLTYPE BeginQueryRecord(
                __in KTL_LOG_ASN Asn,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndQueryRecord(
                __in IFabricAsyncOperationContext* Context,
                __out_opt ULONGLONG* const OptionalVersion,
                __out_opt KPHYSICAL_LOG_STREAM_RECORD_DISPOSITION* const OptionalDisposition,
                __out_opt ULONG* const OptionalIoBufferSize,
                __out_opt ULONGLONG* const OptionalDebugInfo1) override;


            HRESULT STDMETHODCALLTYPE BeginQueryRecords(
                __in KTL_LOG_ASN LowestAsn,
                __in KTL_LOG_ASN HighestAsn,
                __in IKArray* const ResultArray,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndQueryRecords(
                __in IFabricAsyncOperationContext* Context) override;


            HRESULT STDMETHODCALLTYPE Truncate(
                __in KTL_LOG_ASN TruncationPoint,
                __in KTL_LOG_ASN PreferredTruncationPoint) override;


            HRESULT STDMETHODCALLTYPE BeginWaitForNotification(
                __in KPHYSICAL_LOG_STREAM_NOTIFICATION_TYPE NotificationType,
                __in ULONGLONG NotificationValue,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndWaitForNotification(
                __in IFabricAsyncOperationContext* Context) override;


            HRESULT STDMETHODCALLTYPE BeginIoctl(
                __in ULONG ControlCode,
                __in IKBuffer* const InBuffer,
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndIoctl(
                __in IFabricAsyncOperationContext* Context,
                __out ULONG *const Result,
                __out IKBuffer** const OutBuffer) override;

            HRESULT STDMETHODCALLTYPE BeginMultiRecordRead(
                __in KTL_LOG_ASN Asn,
                __in ULONG BytesToRead,
                __in IFabricAsyncOperationCallback* Callback,
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndMultiRecordRead(
                __in IFabricAsyncOperationContext* Context,
                __out IKIoBuffer** const ResultingMetadata,
                __out IKIoBuffer** const ResultingIoBuffer) override;

            HRESULT STDMETHODCALLTYPE BeginClose(
                __in IFabricAsyncOperationCallback* Callback,       
                __out IFabricAsyncOperationContext** Context) override;

            HRESULT STDMETHODCALLTYPE EndClose(
                __in IFabricAsyncOperationContext* Context) override;

        private:
            KtlLogStream::SPtr      _BackingStream;
        };
    }
}

//** Templated implementations
namespace Ktl
{
    namespace Com
    {
        //** KArray<type> COM wrapper class implementation
        template <typename ArrayType>
        ComIKArray<ArrayType>::ComIKArray(ULONG ReserveSize)
            :   _BackingArray(GetThisAllocator(), ReserveSize)
        {
        }

        template <typename ArrayType>
        ComIKArray<ArrayType>::~ComIKArray()
        {
        }

        template <typename ArrayType>
        HRESULT 
        ComIKArray<ArrayType>::Create(__in KAllocator& Allocator,  __in ULONG ReserveSize, __out KSharedPtr<ComIKArray<ArrayType>>& Result)
        {
            Result = _new(KTL_TAG_BUFFER, Allocator) ComIKArray<ArrayType>(ReserveSize);
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
    }
}
