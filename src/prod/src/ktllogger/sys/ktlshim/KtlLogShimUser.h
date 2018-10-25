// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <ktl.h>
#include <ktrace.h>
#include <minmax.h>
#include <kinstrumentop.h>

#include "KtlPhysicalLog.h"
#include "../inc/ktllogger.h"
#include "../inc/KLogicalLog.h"

#include "fileio.h"

#include "KtlLogMarshal.h"
#include "DevIoUmKm.h"

class FileObjectTable;

class RequestMarshallerUser : public RequestMarshaller
{
    K_FORCE_SHARED(RequestMarshallerUser);
        
    public:
        static NTSTATUS Create(
            __out RequestMarshallerUser::SPtr& Result,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __in ULONG InKBufferSize = _InitialInBufferSize,
            __in ULONG OutKBufferSize = _InitialOutBufferSize
        );

        static const ULONG _InitialInBufferSize = 512;
        static const ULONG _InitialOutBufferSize = 512;
        
        NTSTATUS InitializeRequest(
            __in REQUEST_TYPE RequestType,
            __in ULONG RequestVersion,
            __in ULONGLONG RequestId,
            __in OBJECT_TYPE ObjectType,
            __in ULONGLONG ObjectId
        );
        
        NTSTATUS InitializeForCreateObject(
            OBJECT_TYPE ObjectType
            );        

        NTSTATUS InitializeForDeleteObject(
            __in OBJECT_TYPE ObjectType,
            __in ULONGLONG ObjectId
            );

        NTSTATUS InitializeForObjectMethod(
            __in OBJECT_TYPE ObjectType,
            __in ULONGLONG RequestId,
            __in ULONGLONG ObjectId,
            __in OBJECT_METHOD ObjectMethod
            );        

        NTSTATUS InitializeForResponse(
            __in ULONG OutBufferSize
        );
        
        VOID Reset();

        NTSTATUS ReadObject(
            __in OBJECT_TYPE ObjectTypeExpected,
            __out ULONGLONG& ObjectId
        );
        
        NTSTATUS ReadKIoBufferPreAlloc(
            __out KIoBuffer::SPtr& IoBuffer,
            __in KIoBuffer::SPtr& PreAllocIoBuffer,
            __out ULONG& Size
        );

        NTSTATUS WriteKIoBufferElement(
            __in KIoBufferElement& IoBufferElement,
            __in BOOLEAN MakeReadOnly
            );
            
        NTSTATUS WriteKIoBuffer(
            __in KIoBuffer& IoBuffer,
            __in BOOLEAN MakeReadOnly
        );

        KBuffer::SPtr& GetInKBuffer()
        {
            return(_InKBuffer);
        }

        KBuffer::SPtr& GetOutKBuffer()
        {
            return(_OutKBuffer);
        }

        ULONG GetMaxOutBufferSize()
        {
            return(_OutKBuffer->QuerySize());
        }
        
    private:
        KBuffer::SPtr _InKBuffer;
        KBuffer::SPtr _OutKBuffer;                  
};

class KtlLogManagerUser;
class KtlLogContainerUser;
class KtlLogStreamUser;

//************************************************************************************
//
// KtlLogStream Implementation
//
//************************************************************************************
class KtlLogStreamUser : public KtlLogStream
{
    K_FORCE_SHARED(KtlLogStreamUser);

    friend KtlLogContainerUser;

    public:
        KtlLogStreamUser(
            __in OpenCloseDriver::SPtr DeviceHandle
        );

        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(RequestMarshaller::LogStream);
        }
        
        ULONGLONG GetObjectId()
        {
            return(_ObjectId);
        }

        inline KActivityId GetActivityId()
        {
            return((KActivityId)(_LogStreamId.Get().Data1));
        }

#ifdef UDRIVER
        PVOID GetKernelPointer();
#endif
        
    protected:
        static VOID DeleteObjectCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
        VOID SetObjectId(
            ULONGLONG ObjectId
            )
        {
            _ObjectId = ObjectId;
        }

    private:
        ULONGLONG _ObjectId;

    public:     
        
        BOOLEAN IsFunctional() override ;

        VOID
        QueryLogStreamId(__out KtlLogStreamId& LogStreamId) override;


        ULONG
        QueryReservedMetadataSize() override ;

        class AsyncQueryRecordRangeContextUser : public AsyncQueryRecordRangeContext
        {
            K_FORCE_SHARED(AsyncQueryRecordRangeContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartQueryRecordRange(
                    __out_opt KtlLogAsn* const LowestAsn,
                    __out_opt KtlLogAsn* const HighestAsn,
                    __out_opt KtlLogAsn* const LogTruncationAsn,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID RecordRangeQueried(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KtlLogAsn* _LowestAsn;
                KtlLogAsn* _HighestAsn;
                KtlLogAsn* _LogTruncationAsn;
                                
                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        NTSTATUS
        CreateAsyncQueryRecordRangeContext(__out AsyncQueryRecordRangeContext::SPtr& Context) override ;        

        class AsyncReservationContextUser : public AsyncReservationContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReservationContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartExtendReservation(
                    __in ULONGLONG ReservationSize,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartContractReservation(
                    __in ULONGLONG ReservationSize,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
            
            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID ReservationSizeChanged(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                LONGLONG _ReservationDelta;
                
                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncReservationContextUser;
        
        NTSTATUS
        CreateAsyncReservationContext(__out AsyncReservationContext::SPtr& Context) override ;

        ULONGLONG
        QueryReservationSpace();
        
        
        class AsyncWriteContextUser : public AsyncWriteContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContextUser);

            friend KtlLogStreamUser;
    
            public:
                VOID
                StartWrite(
                    __in KtlLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in ULONG MetaDataLength,
                    __in const KIoBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in ULONGLONG ReservationSpace,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartWrite(
                    __in KtlLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in ULONG MetaDataLength,
                    __in const KIoBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in ULONGLONG ReservationSpace,
                    __out ULONGLONG& LogSize,
                    __out ULONGLONG& LogSpaceRemaining,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                WriteAsync(
                    __in KtlLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in ULONG MetaDataLength,
                    __in const KIoBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in ULONGLONG ReservationSpace,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;

                ktl::Awaitable<NTSTATUS>
                WriteAsync(
                    __in KtlLogAsn RecordAsn,
                    __in ULONGLONG Version,
                    __in ULONG MetaDataLength,
                    __in const KIoBuffer::SPtr& MetaDataBuffer,
                    __in const KIoBuffer::SPtr& IoBuffer,
                    __in ULONGLONG ReservationSpace,
                    __out ULONGLONG& LogSize,
                    __out ULONGLONG& LogSpaceRemaining,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                
                VOID WriteCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;
				KInstrumentedOperation _InstrumentedOperation;
                
                //
                // Parameters to api
                //
                KtlLogAsn _RecordAsn;
                ULONGLONG _Version;
                ULONG _MetaDataLength;
                KIoBuffer::SPtr _MetaDataBuffer;
                KIoBuffer::SPtr _IoBuffer;
                ULONGLONG _ReservationSpace;
                ULONGLONG* _LogSize;
                ULONGLONG* _LogSpaceRemaining;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncWriteContextUser;

        NTSTATUS
        CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override ;
        
        class AsyncReadContextUser : public AsyncReadContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartRead(
                    __in KtlLogAsn RecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out ULONG& MetaDataLength,
                    __out KIoBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;


                VOID
                StartReadPrevious(
                    __in KtlLogAsn NextRecordAsn,
                    __out_opt KtlLogAsn* RecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out ULONG& MetaDataLength,
                    __out KIoBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartReadNext(
                    __in KtlLogAsn PreviousRecordAsn,
                    __out_opt KtlLogAsn* RecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out ULONG& MetaDataLength,
                    __out KIoBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartReadContaining(
                    __in KtlLogAsn ContainingRecordAsn,
                    __out_opt KtlLogAsn* RecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out ULONG& MetaDataLength,
                    __out KIoBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;              

#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                ReadContainingAsync(
                    __in KtlLogAsn ContainingRecordAsn,
                    __out_opt KtlLogAsn* RecordAsn,
                    __out_opt ULONGLONG* const Version,
                    __out ULONG& MetaDataLength,
                    __out KIoBuffer::SPtr& MetaDataBuffer,
                    __out KIoBuffer::SPtr& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );
                
                VOID ReadCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KtlLogAsn _RecordAsn;
                KtlLogAsn* _ActualReadAsnPtr;
                ULONGLONG* _Version;
                ULONG* _MetaDataLength;
                KIoBuffer::SPtr* _MetaDataBuffer;
                KIoBuffer::SPtr* _IoBuffer;

                //
                // Members needed for functionality
                //
                RvdLogStream::AsyncReadContext::ReadType _ReadType;

                KIoBuffer::SPtr _PreAllocDataBuffer;
                KIoBuffer::SPtr _PreAllocMetadataBuffer;
                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncReadContextUser;

        NTSTATUS
        CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override ;

        class AsyncMultiRecordReadContextUser : public AsyncMultiRecordReadContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncMultiRecordReadContextUser);

            friend KtlLogStreamUser;
            
            public:

#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                ReadAsync(
                    __in KtlLogAsn RecordAsn,
                    __inout KIoBuffer& MetaDataBuffer,
                    __inout KIoBuffer& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif

                VOID
                StartRead(
                    __in KtlLogAsn RecordAsn,
                    __inout KIoBuffer& MetaDataBuffer,
                    __inout KIoBuffer& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );
                
                VOID ReadCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KtlLogAsn _RecordAsn;
                KIoBuffer::SPtr _MetaDataBuffer;
                KIoBuffer::SPtr _IoBuffer;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncMultiRecordReadContextUser;

        NTSTATUS
        CreateAsyncMultiRecordReadContext(__out AsyncMultiRecordReadContext::SPtr& Context);

        
        class AsyncQueryRecordContextUser : public AsyncQueryRecordContext
        {
            K_FORCE_SHARED(AsyncQueryRecordContextUser);

            friend KtlLogStreamUser;
            
        public:
            VOID
            StartQueryRecord(
                __in KtlLogAsn RecordAsn,
                __out_opt ULONGLONG* const Version,
#ifdef USE_RVDLOGGER_OBJECTS
                __out_opt RvdLogStream::RecordDisposition* const Disposition,
#else
                __out_opt KtlLogStream::RecordDisposition* const Disposition,
#endif
                __out_opt ULONG* const IoBufferSize,
                __out_opt ULONGLONG* const DebugInfo,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID QueryRecordCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //              
                KtlLogAsn _RecordAsn;
                ULONGLONG* _Version;
#ifdef USE_RVDLOGGER_OBJECTS
                RvdLogStream::RecordDisposition* _Disposition;
#else
                KtlLogStream::RecordDisposition* _Disposition;
#endif
                ULONG* _IoBufferSize;
                ULONGLONG* _DebugInfo;
                

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        NTSTATUS
        CreateAsyncQueryRecordContext(__out AsyncQueryRecordContext::SPtr& Context) override ;        
                

        class AsyncQueryRecordsContextUser : public AsyncQueryRecordsContext
        {
            K_FORCE_SHARED(AsyncQueryRecordsContextUser);

            friend KtlLogStreamUser;
            
        public:
            VOID
            StartQueryRecords(
                __in KtlLogAsn LowestAsn,
                __in KtlLogAsn HighestAsn,
#ifdef USE_RVDLOGGER_OBJECTS
                __in KArray<RvdLogStream::RecordMetadata>& ResultsArray,
#else
                __in KArray<RecordMetadata>& ResultsArray,
#endif
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);


            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID QueryRecordsCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KtlLogAsn _LowestAsn;
                KtlLogAsn _HighestAsn;
#ifdef USE_RVDLOGGER_OBJECTS
                KArray<RvdLogStream::RecordMetadata>* _ResultsArray;
#else
                KArray<RecordMetadata>* _ResultsArray;     
#endif

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        NTSTATUS
        CreateAsyncQueryRecordsContext(__out AsyncQueryRecordsContext::SPtr& Context) override ;


        class AsyncTruncateContextUser : public AsyncTruncateContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncTruncateContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                Truncate(
                    __in KtlLogAsn TruncationPoint,
                    __in KtlLogAsn PreferredTruncationPoint
                ) override ;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID
                StartTruncate(
                    __in KtlLogAsn TruncationPoint,
                    __in KtlLogAsn PreferredTruncationPoint,         
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);
                
                VOID TruncationCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KtlLogAsn _TruncationPoint;
                KtlLogAsn _PreferredTruncationPoint;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncTruncateContextUser;

        
        NTSTATUS
        CreateAsyncTruncateContext(__out AsyncTruncateContext::SPtr& Context) override ;

        
        class AsyncStreamNotificationContextUser : public AsyncStreamNotificationContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncStreamNotificationContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartWaitForNotification(
                    __in ThresholdTypeEnum ThresholdType,
                    __in ULONGLONG ThresholdValue,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID WaitCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
                static VOID
                CancelWaitCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
            
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                ThresholdTypeEnum _ThresholdType;
                ULONGLONG _ThresholdValue;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;                
        };

        friend KtlLogStreamUser::AsyncStreamNotificationContextUser;
        
        NTSTATUS
        CreateAsyncStreamNotificationContext(__out AsyncStreamNotificationContext::SPtr& Context);

                
        class AsyncWriteMetadataContextUser : public AsyncWriteMetadataContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteMetadataContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartWriteMetadata(
                    __in const KIoBuffer::SPtr& MetadataBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID WriteMetadataCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KIoBuffer::SPtr _MetadataBuffer;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncWriteMetadataContextUser;
        
        NTSTATUS
        CreateAsyncWriteMetadataContext(__out AsyncWriteMetadataContext::SPtr& Context) override;

        class AsyncReadMetadataContextUser : public AsyncReadMetadataContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadMetadataContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartReadMetadata(
                    __out KIoBuffer::SPtr& MetadataBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );
                
                VOID ReadMetadataCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                KIoBuffer::SPtr* _MetadataBuffer;

                //
                // Members needed for functionality
                //
                KIoBuffer::SPtr _PreAllocMetadataBuffer;
                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncReadMetadataContextUser;
        
        NTSTATUS
        CreateAsyncReadMetadataContext(__out AsyncReadMetadataContext::SPtr& Context) override ;


        class AsyncIoctlContextUser : public AsyncIoctlContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncIoctlContextUser);

            friend KtlLogStreamUser;
            
            public:
                VOID
                StartIoctl(
                    __in ULONG ControlCode,
                    __in_opt KBuffer* const InBuffer,
                    __out ULONG& Result,
                    __out KBuffer::SPtr& OutBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

#if defined(K_UseResumable)   
                ktl::Awaitable<NTSTATUS>
                IoctlAsync(
                    __in ULONG ControlCode,
                    __in_opt KBuffer* const InBuffer,
                    __out ULONG& Result,
                    __out KBuffer::SPtr& OutBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );
                
                VOID IoctlCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                ULONG _ControlCode;
                KBuffer::SPtr _InBuffer;
                ULONG* _Result;
                KBuffer::SPtr* _OutBuffer;

                //
                // Members needed for functionality
                //                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncIoctlContextUser;
        
        NTSTATUS
        CreateAsyncIoctlContext(__out AsyncIoctlContext::SPtr& Context) override ;
   
        
        class AsyncCloseContextUser : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCloseContextUser);

            friend KtlLogStreamUser;
            
            public:

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

            private:
                VOID
                StartClose(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt CloseCompletionCallback CloseCallbackPtr
                );
                
#if defined(K_UseResumable)   
                ktl::Awaitable<NTSTATUS>
                CloseAsync(
                    __in_opt KAsyncContextBase* const ParentAsyncContext
                    );
#endif
                
                VOID CloseCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamUser::SPtr _LogStream;         
                
                //
                // Parameters to api
                //
                CloseCompletionCallback _CloseCallbackPtr;
                KAsyncContextBase* _ParentAsyncContext;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogStreamUser::AsyncCloseContextUser;
        
        NTSTATUS
        CreateAsyncCloseContext(__out AsyncCloseContextUser::SPtr& Context);

        NTSTATUS StartClose(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        ) override;
        
#if defined(K_UseResumable)
        ktl::Awaitable<NTSTATUS>
        CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
        
        private:
            
        VOID
        AdjustReservationSpace(__in LONGLONG ReservationDelta)
        {
            K_LOCK_BLOCK(_Lock)
            {
                if ((ReservationDelta < 0) && (_ReservationSpaceHeld < (-1 * ReservationDelta)))
                {
                    //
                    // Assert if removing more than is being held
                    //
                    KInvariant(FALSE);
                }

                _ReservationSpaceHeld += ReservationDelta;
            }
        }

        BOOLEAN IsReservationSpaceAvailable(__in LONGLONG ReservationSpace)
        {
            K_LOCK_BLOCK(_Lock)
            {
                if (ReservationSpace > _ReservationSpaceHeld)
                {
                    return(FALSE);
                }
            }

            return(TRUE);
        }
        
    private:
        OpenCloseDriver::SPtr _DeviceHandle;       
        DevIoControlUser::SPtr _DevIoCtl;
        RequestMarshallerUser::SPtr _Marshaller;
        
        KtlLogStreamId _LogStreamId;   // Cached value of the log stream id
        KAsyncEvent _DummyTruncationEvent;         // AutomaticReset, InitialStateFalse

        KSpinLock _Lock;
        LONGLONG _ReservationSpaceHeld;

        BOOLEAN _IsClosed;
        
        ULONG _PreAllocDataSize;
        ULONG _PreAllocRecordMetadataSize;
        ULONG _PreAllocMetadataSize;

        ULONG _CoreLoggerMetadataOverhead;
		KInstrumentedComponent::SPtr _InstrumentedComponent;
};

//************************************************************************************
//
// KtlLogContainer Implementation
//
//************************************************************************************

class KtlLogContainerUser : public KtlLogContainer
{
    K_FORCE_SHARED(KtlLogContainerUser);

    friend KtlLogManagerUser;
    
    public:
        KtlLogContainerUser(
            __in KtlLogContainerId LogContainerId,
            __in OpenCloseDriver& DeviceHandle
        );
        
        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(RequestMarshaller::LogContainer);
        }
        
    protected:
        ULONGLONG GetObjectId()
        {
            return(_ObjectId);
        }
        
        VOID SetObjectId(
            ULONGLONG ObjectId
            )
        {
            _ObjectId = ObjectId;
        }

        static VOID DeleteObjectCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
    private:
        ULONGLONG _ObjectId;

    public:     
        BOOLEAN IsFunctional() override;        

        class AsyncAssignAliasContextUser : public AsyncAssignAliasContext
        {
            K_FORCE_SHARED(AsyncAssignAliasContextUser);

            friend KtlLogContainerUser;
            
            public:

                VOID
                StartAssignAlias(
                    __in KString::CSPtr& Alias,
                    __in KtlLogStreamId LogStreamId,                             
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                    
#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                AssignAliasAsync(
                    __in KString::CSPtr& Alias,
                    __in KtlLogStreamId LogStreamId,                             
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
            private:
                VOID AliasAssigned(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;
                KString::CSPtr _Alias;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };
        
        friend KtlLogContainerUser::AsyncAssignAliasContextUser;

        NTSTATUS
        CreateAsyncAssignAliasContext(__out AsyncAssignAliasContext::SPtr& Context) override ;

        class AsyncRemoveAliasContextUser : public AsyncRemoveAliasContext
        {
            K_FORCE_SHARED(AsyncRemoveAliasContextUser);

            friend KtlLogContainerUser;
            
            public:
                VOID
                StartRemoveAlias(
                    __in KString::CSPtr& Alias,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                    
#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                RemoveAliasAsync(
                    __in KString::CSPtr& Alias,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
            private:
                VOID AliasRemoved(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                KString::CSPtr _Alias;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogContainerUser::AsyncRemoveAliasContextUser;
        
        NTSTATUS
        CreateAsyncRemoveAliasContext(__out AsyncRemoveAliasContext::SPtr& Context) override ;
        
        class AsyncResolveAliasContextUser : public AsyncResolveAliasContext
        {
            K_FORCE_SHARED(AsyncResolveAliasContextUser);

            friend KtlLogContainerUser;
            
            public:
                VOID
                StartResolveAlias(
                    __in KString::CSPtr& Alias,
                    __out KtlLogStreamId* LogStreamId,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                    
#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                ResolveAliasAsync(
                    __in KString::CSPtr& Alias,
                    __out KtlLogStreamId* LogStreamId,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
            private:
                VOID AliasResolved(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                KString::CSPtr _Alias;
                KtlLogStreamId* _LogStreamId;
                
                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };
        
        friend KtlLogContainerUser::AsyncResolveAliasContextUser;
        
        NTSTATUS
        CreateAsyncResolveAliasContext(__out AsyncResolveAliasContext::SPtr& Context) override ;        
        
        
        class AsyncCreateLogStreamContextUser : public AsyncCreateLogStreamContext
        {
            K_FORCE_SHARED(AsyncCreateLogStreamContextUser);

            friend KtlLogContainerUser;
            
            public:
                
                VOID
                StartCreateLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __in_opt const KString::CSPtr& LogStreamAlias,
                    __in_opt const KString::CSPtr& Path,
                    __in_opt KBuffer::SPtr& SecurityDescriptor,
                    __in ULONG MaximumMetaDataLength,
                    __in LONGLONG MaximumStreamSize,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                
                VOID
                StartCreateLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __in const KtlLogStreamType& LogStreamType,
                    __in_opt const KString::CSPtr& LogStreamAlias,
                    __in_opt const KString::CSPtr& Path,
                    __in_opt KBuffer::SPtr& SecurityDescriptor,
                    __in ULONG MaximumMetaDataLength,
                    __in LONGLONG MaximumStreamSize,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                
                VOID
                StartCreateLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __in const KtlLogStreamType& LogStreamType,
                    __in_opt const KString::CSPtr& LogStreamAlias,
                    __in_opt const KString::CSPtr& Path,
                    __in_opt KBuffer::SPtr& SecurityDescriptor,
                    __in ULONG MaximumMetaDataLength,
                    __in LONGLONG MaximumStreamSize,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

#if defined(K_UseResumable)
				ktl::Awaitable<NTSTATUS>
				CreateLogStreamAsync(
					__in const KtlLogStreamId& LogStreamId,
					__in const KtlLogStreamType& LogStreamType,
					__in_opt const KString::CSPtr& LogStreamAlias,
					__in_opt const KString::CSPtr& Path,
					__in_opt KBuffer::SPtr& SecurityDescriptor,
					__in ULONG MaximumMetaDataLength,
					__in LONGLONG MaximumStreamSize,
					__in ULONG MaximumRecordSize,
					__in DWORD Flags,
					__out KtlLogStream::SPtr& LogStream,
					__in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );

                VOID
                OnCompleted(
                    );

                VOID
                OnReuse(
                    );
                
            private:
                VOID LogStreamCreated(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;
                KtlLogStreamType _LogStreamType;
                KString::CSPtr _LogStreamAlias;
                KString::CSPtr _Path;
                KBuffer::SPtr _SecurityDescriptor;
                ULONG _MaximumMetaDataLength;
                LONGLONG _MaximumStreamSize;
                ULONG _MaximumRecordSize;
                DWORD _Flags;
                KtlLogStream::SPtr* _ResultLogStreamPtr;

                //
                // Members needed for functionality
                //
                KtlLogStreamUser::SPtr _LogStream;
                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogContainerUser::AsyncCreateLogStreamContextUser;
        
        NTSTATUS
        CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) override;

        class AsyncDeleteLogStreamContextUser : public AsyncDeleteLogStreamContext
        {
            K_FORCE_SHARED(AsyncDeleteLogStreamContextUser);

            friend KtlLogContainerUser;
            
            public:
                VOID
                StartDeleteLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                DeleteLogStreamAsync(
                    __in const KtlLogStreamId& LogStreamId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
            private:
                VOID LogStreamDeleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogContainerUser::AsyncDeleteLogStreamContextUser;
        
        NTSTATUS
        CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) override;

        class AsyncOpenLogStreamContextUser : public AsyncOpenLogStreamContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogStreamContextUser);

            friend KtlLogContainerUser;
            
            public:
                VOID
                StartOpenLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __out ULONG* MaximumMetaDataSize,
                    __out KtlLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

#if defined(K_UseResumable)
				ktl::Awaitable<NTSTATUS>
				OpenLogStreamAsync(
					__in const KtlLogStreamId& LogStreamId,
					__out ULONG* MaximumMetaDataSize,
					__out KtlLogStream::SPtr& LogStream,
					__in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
            private:
                VOID LogStreamOpened(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;
                ULONG* _MaximumMetaDataLength;
                KtlLogStream::SPtr* _ResultLogStreamPtr;

                //
                // Members needed for functionality
                //
                KtlLogStreamUser::SPtr _LogStream;
                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogContainerUser::AsyncOpenLogStreamContextUser;
        
        NTSTATUS
        CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) override;

        class AsyncEnumerateStreamsContextUser : public AsyncEnumerateStreamsContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncEnumerateStreamsContextUser);

            friend KtlLogContainerUser;
            
            public:
                VOID
                StartEnumerateStreams(
                    __out KArray<KtlLogStreamId>& LogStreamIds,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;
                    
#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                EnumerateStreamsAsync(
                    __out KArray<KtlLogStreamId>& LogStreamIds,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
            private:
                VOID StreamsEnumerated(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                KArray<KtlLogStreamId>* _LogStreamIdArray;

                //
                // Members needed for functionality
                //                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogContainerUser::AsyncEnumerateStreamsContextUser;
        
        NTSTATUS
        CreateAsyncEnumerateStreamsContext(__out AsyncEnumerateStreamsContext::SPtr& Context) override;

        class AsyncCloseContextUser : public KAsyncContextBase
        {
            K_FORCE_SHARED(AsyncCloseContextUser);

            friend KtlLogContainerUser;
            
            public:
                VOID
                StartClose(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt CloseCompletionCallback CloseCallbackPtr
                );

#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                CloseAsync(
                    __in_opt KAsyncContextBase* const ParentAsyncContext
                );
#endif
                
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
                VOID
                OnCompleted(
                    ) override ;

            private:
                VOID CloseCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerUser::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
                CloseCompletionCallback _CloseCallbackPtr;
                KAsyncContextBase* _ParentAsyncContext;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogContainerUser::AsyncCloseContextUser;
        
        NTSTATUS
        CreateAsyncCloseContext(__out AsyncCloseContextUser::SPtr& Context);

        NTSTATUS StartClose(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        );

#if defined(K_UseResumable)
        ktl::Awaitable<NTSTATUS>
        CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
        
    private:
        OpenCloseDriver::SPtr _DeviceHandle;       
        DevIoControlUser::SPtr _DevIoCtl;
        RequestMarshallerUser::SPtr _Marshaller;
        BOOLEAN _IsClosed;

        KtlLogContainerId _LogContainerId;
};

//************************************************************************************
//
// KtlLogManager Implementation
//
//************************************************************************************
class KtlLogManagerUser : public KtlLogManager
{
    K_FORCE_SHARED(KtlLogManagerUser);

    friend KtlLogManager;
    friend KtlLogStreamUser;
    
    public:
        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(RequestMarshaller::LogManager);
        }
        
        ULONGLONG GetObjectId()
        {
            return(_ObjectId);
        }
        
    protected:
        VOID SetObjectId(
            ULONGLONG ObjectId
            )
        {
            _ObjectId = ObjectId;
        }

        static VOID DeleteObjectCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
                
    private:
        ULONGLONG _ObjectId;

    public:
        NTSTATUS
        StartOpenLogManager(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in_opt OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr) override;

#if defined(K_UseResumable)
        ktl::Awaitable<NTSTATUS>
        OpenLogManagerAsync(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr) override;
#endif

        NTSTATUS
        StartCloseLogManager(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in_opt CloseCompletionCallback CloseCallbackPtr) override;

#if defined(K_UseResumable)
        ktl::Awaitable<NTSTATUS>
        CloseLogManagerAsync(
            __in_opt KAsyncContextBase* const ParentAsync) override;
#endif
                
        class AsyncCreateLogContainerUser : public AsyncCreateLogContainer
        {
            K_FORCE_SHARED(AsyncCreateLogContainerUser);

            friend KtlLogManagerUser;
            
            public:
                VOID
                StartCreateLogContainer(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartCreateLogContainer(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartCreateLogContainer(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

                VOID
                StartCreateLogContainer(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;

                ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;

                ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KGuid const& DiskId,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;

                ktl::Awaitable<NTSTATUS>
                CreateLogContainerAsync(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __in DWORD Flags,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif

            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
                VOID
                OnCompleted(
                    );
            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID LogContainerCreated(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );


            private:
                //
                // General members
                //
                KtlLogManagerUser::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                KtlLogContainerId _LogContainerId;
                LONGLONG _LogSize;
                ULONG _MaximumRecordSize;
                ULONG _MaximumNumberStreams;
                ULONG _Flags;
                KtlLogContainer::SPtr* _ResultLogContainerPtr;

                //
                // Members needed for functionality
                //
                KtlLogContainerUser::SPtr _LogContainer;

                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogManagerUser::AsyncCreateLogContainerUser;
    
        NTSTATUS
        CreateAsyncCreateLogContainerContext(__out AsyncCreateLogContainer::SPtr& Context) override ;

        
        class AsyncOpenLogContainerUser : public AsyncOpenLogContainer
        {
            K_FORCE_SHARED(AsyncOpenLogContainerUser);

            friend KtlLogManagerUser;
            
        public:
            VOID
            StartOpenLogContainer(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogId,
                __out KtlLogContainer::SPtr& LogContainer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

#if defined(K_UseResumable)
            ktl::Awaitable<NTSTATUS>
            OpenLogContainerAsync(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __out KtlLogContainer::SPtr& LogContainer) override;
#endif

            VOID
            StartOpenLogContainer(
                __in KString const & Path,
                __in const KtlLogContainerId& LogId,
                __out KtlLogContainer::SPtr& LogContainer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

#if defined(K_UseResumable)
            ktl::Awaitable<NTSTATUS>
            OpenLogContainerAsync(
                __in KString const & Path,
                __in const KtlLogContainerId& LogId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __out KtlLogContainer::SPtr& LogContainer) override;
#endif

            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
                VOID
                OnCompleted(
                    );
            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID OpenLogContainerCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogManagerUser::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                KtlLogContainerId _LogContainerId;
                KtlLogContainer::SPtr* _ResultLogContainerPtr;

                //
                // Members needed for functionality
                //
                KtlLogContainerUser::SPtr _LogContainer;

                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };
        
        friend KtlLogManagerUser::AsyncOpenLogContainerUser;

        NTSTATUS
        CreateAsyncOpenLogContainerContext(__out AsyncOpenLogContainer::SPtr& Context) override ;


        class AsyncDeleteLogContainerUser : public AsyncDeleteLogContainer
        {
            K_FORCE_SHARED(AsyncDeleteLogContainerUser);

            friend KtlLogManagerUser;
            
        public:
            VOID
            StartDeleteLogContainer(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
            
            VOID
            StartDeleteLogContainer(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

#if defined(K_UseResumable)
            ktl::Awaitable<NTSTATUS>
            DeleteLogContainerAsync(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif   
            
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
                VOID
                OnCompleted(
                    );
            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID LogContainerDeleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogManagerUser::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                KtlLogContainerId _LogContainerId;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;

        };
        
        friend KtlLogManagerUser::AsyncDeleteLogContainerUser;
 
        NTSTATUS
        CreateAsyncDeleteLogContainerContext(__out AsyncDeleteLogContainer::SPtr& Context) override;

        class AsyncEnumerateLogContainersUser : public AsyncEnumerateLogContainers
        {
            K_FORCE_SHARED(AsyncEnumerateLogContainersUser);

            friend KtlLogManagerUser;
            
        public:
            VOID
            StartEnumerateLogContainers(
                __in const KGuid& DiskId,
                __out KArray<KtlLogContainerId>& LogIdArray,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
            
            protected:
                VOID
                OnStart(
                    );
                
                VOID
                OnReuse(
                    );
                
                VOID
                OnCompleted(
                    );
            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );

                VOID LogContainersEnumerated(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogManagerUser::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                KGuid _DiskId;
                KArray<KtlLogContainerId>* _LogIdArray;

                //
                // Members needed for functionality
                //
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;

        };
        
        friend KtlLogManagerUser::AsyncEnumerateLogContainersUser;
 
        NTSTATUS
        CreateAsyncEnumerateLogContainersContext(__out AsyncEnumerateLogContainers::SPtr& Context) override;

        
        class AsyncConfigureContextUser : public AsyncConfigureContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncConfigureContextUser);

            friend KtlLogManagerUser;
            
            public:
                VOID
                StartConfigure(
                    __in ULONG Code,
                    __in_opt KBuffer* const InBuffer,
                    __out ULONG& Result,
                    __out KBuffer::SPtr& OutBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

            protected:
                VOID
                OnStart(
                    );

                VOID
                OnReuse(
                    );

            private:
                VOID DoComplete(
                    __in NTSTATUS Status
                    );
                
                VOID ConfigureCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogManagerUser::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                ULONG _Code;
                KBuffer::SPtr _InBuffer;
                ULONG* _Result;
                KBuffer::SPtr* _OutBuffer;

                //
                // Members needed for functionality
                //                
                DevIoControlUser::SPtr _DevIoCtl;
                RequestMarshallerUser::SPtr _Marshaller;
        };

        friend KtlLogManagerUser::AsyncConfigureContextUser;
        
        NTSTATUS
        CreateAsyncConfigureContext(__out AsyncConfigureContext::SPtr& Context) override ;
        
        
    protected:
        using KAsyncContextBase::Cancel;

        VOID OnServiceOpen() override ;   // StartOpen() was called and accepted; default: calls CompleteOpen(STATUS_SUCCESS)
        VOID OnServiceClose() override ;  // StartClose() was called and accepted; default: calls CompleteClose(STATUS_SUCCESS)
        VOID OnServiceReuse() override ;  // Reuse() was called; default: nothing 

    private:

        typedef enum { Initial, OpenDeviceHandle, CreateLogManager, CloseDeviceHandle, Completed, CompletedWithError } FSMState;
        
        FSMState _CloseState;
        VOID CloseLogManagerFSM(
            __in NTSTATUS Status
        );

        VOID CloseLogManagerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
        FSMState _OpenState;
        VOID OpenLogManagerFSM(
            __in NTSTATUS Status
        );

        VOID OpenLogManagerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID DoCompleteOpen(
            __in NTSTATUS Status
        );
        
        VOID
        BaseLogManagerDeactivated(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async

            );        

    public:
        FileObjectTable* GetFileObjectTable()
        {
            return(_DeviceHandle->GetFileObjectTable());
        }

    private:
        OpenCloseDriver::SPtr _DeviceHandle;
        DevIoControlUser::SPtr _DevIoCtl;
        RequestMarshallerUser::SPtr _Marshaller;
};

