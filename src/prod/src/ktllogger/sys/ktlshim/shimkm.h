// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if defined(PLATFORM_UNIX)
#undef IsListEmpty
#endif

class FileObjectTable;

class KtlLogManagerKernel;
class KtlLogContainerKernel;
class KtlLogStreamKernel;

template <class T, class KeyType>
class ActiveListNoSpinlock
{
    public:
        ActiveListNoSpinlock(ULONG LinkOffset) :
                _List(LinkOffset),
                _EmptyEvent(TRUE,         // Manual Reset
                            FALSE),       // Initial state
                _EmptyAsyncEvent(TRUE,    // Manual Reset
                                 FALSE),  // Initial state
                _SignalWhenEmpty(FALSE)
        {
        }

        ~ActiveListNoSpinlock()
        {
        }

        NTSTATUS CreateWaitContext(
            __in KAllocator& Allocator,
            __in ULONG AllocationTag,
            __out KAsyncEvent::WaitContext::SPtr& WaitContext
            )
        {
            NTSTATUS status;
            
            status = _EmptyAsyncEvent.CreateWaitContext(AllocationTag, Allocator, WaitContext);
            
            return(status);
        }
        
        NTSTATUS Add(
            __in T* Entry
        )
        {
            NTSTATUS status;
            
            if (! _SignalWhenEmpty)
            {            
                _List.AppendTail(Entry);
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_FILE_CLOSED;
            }
            
            return(status);
        }

        BOOLEAN Remove(
            __in T* Entry
        )
        {
            BOOLEAN b = FALSE;
            
            b = (_List.Remove(Entry) != NULL);

            if (b)
            {
                if (_SignalWhenEmpty && (_List.IsEmpty()))
                {
                    _EmptyEvent.SetEvent();
                    _EmptyAsyncEvent.SetEvent();
                }
            }
            
            return(b);
        }

        T* RemoveHead(
        )
        {
            T* entry = NULL;
            
            entry = _List.RemoveHead();

            if (_SignalWhenEmpty && (_List.IsEmpty()))
            {
                _EmptyEvent.SetEvent();
                _EmptyAsyncEvent.SetEvent();
            }
            
            return(entry);
        }

        T*
        PeekHead(
            )
        {
            return(_List.PeekHead());
        }

        T*
        Next(
            T* Previous
            )
        {
            T* entry = _List.Successor(Previous);
            return(entry);
        }

        T*
        Find(
            KeyType* Key
        )
        {
            T* entry = NULL;

            entry = _List.PeekHead();
            while (entry)
            {
                if (entry->IsMyKey(*Key))
                {
                    return(entry);
                }

                entry = _List.Successor(entry);
            }
            
            return(NULL);
        }
        
        BOOLEAN IsListEmpty(
        )
        {
            BOOLEAN b = FALSE;
            
            b = _List.IsEmpty();
            
            return(b);
        }
        
        BOOLEAN IsOnList(
          __in T* Entry
        )
        {
            BOOLEAN b = FALSE;
            
            b = _List.IsOnList(Entry);
            
            return(b);
        }
        
        VOID WaitForEmpty(
        )
        {
            KInvariant(_SignalWhenEmpty == TRUE);

            if (_List.IsEmpty())
            {
                return;
            }

            _EmptyEvent.WaitUntilSet();
        }

        VOID SignalWhenEmpty(
        )
        {
            _SignalWhenEmpty = TRUE;
            
            if (_List.IsEmpty())
            {
                _EmptyEvent.SetEvent();
                _EmptyAsyncEvent.SetEvent();
            }
        }
        
    protected:
        KNodeList<T> _List;
        KEvent _EmptyEvent;
        KAsyncEvent _EmptyAsyncEvent;
        BOOLEAN _SignalWhenEmpty;
};

template <class T, class KeyType>
class ActiveList : public ActiveListNoSpinlock<T, KeyType>
{
    public:
        ActiveList(ULONG LinkOffset) :
            ActiveListNoSpinlock<T, KeyType>(LinkOffset)
        {
        }

        ~ActiveList()
        {
        }

        NTSTATUS Add(
            __in T* Entry
        )
        {
            NTSTATUS status = STATUS_SUCCESS;
            
            K_LOCK_BLOCK(_Lock)
            {
                status = __super::Add(Entry);
            }

            return(status);
        }

        BOOLEAN Remove(
            __in T* Entry
        )
        {
            BOOLEAN b = FALSE;
            
            K_LOCK_BLOCK(_Lock)
            {
                b = __super::Remove(Entry);
            }
            return(b);
        }

        T* RemoveHead(
        )
        {
            T* entry = NULL;
            
            K_LOCK_BLOCK(_Lock)
            {
                entry = __super::RemoveHead();
            }
            
            return(entry);
        }

        T*
        PeekHead(
            )
        {
            T* entry = NULL;
            
            K_LOCK_BLOCK(_Lock)
            {
                entry = __super::PeekHead();
            }
            
            return(entry);
        }

        T*
        Next(
            T* Previous
            )
        {
            T* entry = NULL;
            
            K_LOCK_BLOCK(_Lock)
            {
                entry = __super::Next(Previous);
            }
            
            return(entry);
        }

        T*
        Find(
            KeyType* Key
        )
        {
            T* entry = NULL;

            K_LOCK_BLOCK(_Lock)
            {
                entry = __super::Find(Key);
            }
            
            return(entry);
        }
        
        BOOLEAN IsListEmpty(
        )
        {
            BOOLEAN b = FALSE;
            
            K_LOCK_BLOCK(_Lock)
            {
                b = __super::IsListEmpty();
            }
            
            return(b);
        }
        
        BOOLEAN IsOnList(
          __in T* Entry
        )
        {
            BOOLEAN b = FALSE;
            
            K_LOCK_BLOCK(_Lock)
            {
                b = __super::IsOnList(Entry);
            }
            
            return(b);
        }
        
        VOID WaitForEmpty(
        )
        {
            KInvariant((this->_SignalWhenEmpty) == TRUE);

            K_LOCK_BLOCK(_Lock)
            {
                if (__super::IsListEmpty())
                {
                    return;
                }
            }

            this->_EmptyEvent.WaitUntilSet();
        }

        VOID SignalWhenEmpty(
        )
        {
            K_LOCK_BLOCK(_Lock)
            {
                __super::SignalWhenEmpty();
            }
        }
        
    private:
        KSpinLock _Lock;
};

//
// This class allows contexts to be kept on an ActiveList and managed
// by the active list
//
class RequestMarshallerKernel;

class ActiveContext : public KObject<ActiveContext>
{
    public:        
        ActiveContext(
            );
        
        ~ActiveContext(
            );
    
        VOID
        SetMarshaller(__in RequestMarshallerKernel& Marshaller
            )
        {
            _Marshaller = &Marshaller;
        }

#if defined(UDRIVER) || defined(KDRIVER)
        RequestMarshallerKernel* GetMarshaller()
        {
            return(_Marshaller);
        }
#endif
#ifdef UPASSTHROUGH
        class DummyMarshaller
        {
            public:
                DummyMarshaller() { _ObjectMethod = RequestMarshaller::OBJECT_METHOD::ObjectMethodNone; };
                ~DummyMarshaller() {};
            
                ULONGLONG GetRequestId() { return(0); };
                
                RequestMarshaller::OBJECT_METHOD GetObjectMethod()
                {
                    return(_ObjectMethod);
                }

                VOID SetObjectMethod(__in RequestMarshaller::OBJECT_METHOD ObjectMethod) { _ObjectMethod = ObjectMethod; };
            private:
                RequestMarshaller::OBJECT_METHOD _ObjectMethod;
                
        };

        DummyMarshaller _DummyMarshaller;
        DummyMarshaller* GetMarshaller()
        {
            return(&_DummyMarshaller);
        }
#endif
        
        VOID SetParent(
            PVOID Parent
        )
        {
            _Parent = Parent;
        }

        PVOID& GetParent()
        {
            return(_Parent);
        }

        BOOLEAN IsCancelled()
        {
            return(_IsCancelled);
        }
        
        VOID SetCancelFlag()
        {
            _IsCancelled = TRUE;
        }
        
    public:
        static const ULONG _LinkOffset;
        
    private:
        KListEntry _ListEntry;
        BOOLEAN _IsCancelled;
        BOOLEAN _IsStarted;
        RequestMarshallerKernel* _Marshaller;
        PVOID _Parent;
};

//************************************************************************************
//
// KtlLogStream Implementation
//
//************************************************************************************
class KtlLogStreamKernel : public KtlLogStream
{
    K_FORCE_SHARED(KtlLogStreamKernel);

    friend FileObjectTable;
    friend KtlLogContainerKernel;
    friend RequestMarshallerKernel;

    public:
        KtlLogStreamKernel(
            __in KtlLogContainerKernel& LogContainer,
            __in KGuid DiskId,
            __in ULONG CoreLoggerMetadataOverhead
                           
        );

#if defined(UDRIVER) || defined(KDRIVER)
        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(RequestMarshaller::LogStream);
        }

        VOID SetObjectId(
            ULONGLONG ObjectId
            )
        {
            KInvariant(_ObjectId == 0);
            _ObjectId = ObjectId;
        }
        
        ULONGLONG GetObjectId()
        {
            KInvariant(_ObjectId != 0);
            return(_ObjectId);
        }
#endif
        
        KtlLogStreamId GetLogStreamId()
        {
            return(_LogStreamId);
        }

        OverlayStream* GetOverlayStream()
        {
            return(_BaseLogStream.RawPtr());
        }
        
        static const ULONG OverlayLinkageOffset;

    public:
        BOOLEAN IsFunctional() override ;

        VOID
        QueryLogStreamId(__out KtlLogStreamId& LogStreamId) override;


        ULONG
        QueryReservedMetadataSize() override ;

        class AsyncQueryRecordRangeContextKernel : public AsyncQueryRecordRangeContext
        {
            K_FORCE_SHARED(AsyncQueryRecordRangeContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KtlLogAsn* _LowestAsn;
                KtlLogAsn* _HighestAsn;
                KtlLogAsn* _LogTruncationAsn;
                                
                //
                // Members needed for functionality
                //
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        NTSTATUS
        CreateAsyncQueryRecordRangeContext(__out AsyncQueryRecordRangeContext::SPtr& Context) override ;        
        
        class AsyncReservationContextKernel : public AsyncReservationContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReservationContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
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

                VOID
                StartUpdateReservation(
                    __in LONGLONG ReservationDelta,
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
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID ReservationSizeChanged(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                LONGLONG _ReservationDelta;
                
                //
                // Members needed for functionality
                //
                OverlayStream::AsyncReservationContext::SPtr _BaseAsyncReservation;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncReservationContextKernel;
        
        NTSTATUS
        CreateAsyncReservationContext(__out AsyncReservationContext::SPtr& Context) override ;

        ULONGLONG
        QueryReservationSpace();
        
        class AsyncWriteContextKernel : public AsyncWriteContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
    
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                
                VOID WriteCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                
            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
				KInstrumentedOperation _WriteOperation;
                
                //
                // Parameters to api
                //
                KtlLogAsn _RecordAsn;
                ULONGLONG _Version;
                ULONG _MetaDataLength;
                ULONGLONG _ReservationSpace;
                KIoBuffer::SPtr _MetaDataBuffer;
                KIoBuffer::SPtr _IoBuffer;
                ULONGLONG* _LogSize;
                ULONGLONG* _LogSpaceRemaining;

                //
                // Members needed for functionality
                //
                OverlayStream::AsyncWriteContext::SPtr _BaseAsyncWrite;              
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncWriteContextKernel;

        NTSTATUS
        CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context) override ;
        
        class AsyncReadContextKernel : public AsyncReadContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
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
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID ReadCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
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
                KBuffer::SPtr _MetaDataKBuffer;
                OverlayStream::AsyncReadContext::SPtr _BaseAsyncRead;                
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncReadContextKernel;

        NTSTATUS
        CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context) override ;

        class AsyncMultiRecordReadContextKernel : public AsyncMultiRecordReadContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncMultiRecordReadContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartRead(
                    __in KtlLogAsn RecordAsn,
                    __inout KIoBuffer& MetaDataBuffer,
                    __inout KIoBuffer& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

#if defined(K_UseResumable)
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
                ktl::Awaitable<NTSTATUS>
                ReadAsync(
                    __in KtlLogAsn RecordAsn,
                    __inout KIoBuffer& MetaDataBuffer,
                    __inout KIoBuffer& IoBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
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

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID ReadCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KtlLogAsn _RecordAsn;
                KIoBuffer::SPtr _MetaDataBuffer;
                KIoBuffer::SPtr _IoBuffer;

                //
                // Members needed for functionality
                //
                OverlayStream::AsyncMultiRecordReadContextOverlay::SPtr _BaseAsyncRead;                
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncMultiRecordReadContextKernel;

        NTSTATUS
        CreateAsyncMultiRecordReadContext(__out KtlLogStream::AsyncMultiRecordReadContext::SPtr& Context);
        
        class AsyncQueryRecordContextKernel : public AsyncQueryRecordContext
        {
            K_FORCE_SHARED(AsyncQueryRecordContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
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
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        NTSTATUS
        CreateAsyncQueryRecordContext(__out AsyncQueryRecordContext::SPtr& Context) override ;        
                

        class AsyncQueryRecordsContextKernel : public AsyncQueryRecordsContext
        {
            K_FORCE_SHARED(AsyncQueryRecordsContextKernel);

            friend RequestMarshallerKernel;
            friend KtlLogStreamKernel;
            
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;
                
            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
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
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        NTSTATUS
        CreateAsyncQueryRecordsContext(__out AsyncQueryRecordsContext::SPtr& Context) override ;

        
        class AsyncTruncateContextKernel : public AsyncTruncateContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncTruncateContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                Truncate(
                    __in KtlLogAsn TruncationPoint,
                    __in KtlLogAsn PreferredTruncationPoint) override ;

                VOID
                StartTruncate(
                    __in KtlLogAsn TruncationPoint,
                    __in KtlLogAsn PreferredTruncationPoint,         
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
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KtlLogAsn _TruncationPoint;
                KtlLogAsn _PreferredTruncationPoint;

                //
                // Members needed for functionality
                //
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncTruncateContextKernel;
        
        NTSTATUS
        CreateAsyncTruncateContext(__out AsyncTruncateContext::SPtr& Context);

        
        class AsyncWriteMetadataContextKernel : public AsyncWriteMetadataContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncWriteMetadataContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartWriteMetadata(
                    __in const KIoBuffer::SPtr& MetadataBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

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

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID WriteMetadataCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KIoBuffer::SPtr _MetadataBuffer;

                //
                // Members needed for functionality
                //
                OverlayStream::AsyncWriteMetadataContextOverlay::SPtr _AsyncWriteFileContext;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncWriteMetadataContextKernel;
        
        NTSTATUS
        CreateAsyncWriteMetadataContext(__out AsyncWriteMetadataContext::SPtr& Context) override;

        class AsyncReadMetadataContextKernel : public AsyncReadMetadataContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncReadMetadataContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartReadMetadata(
                    __out KIoBuffer::SPtr& MetadataBuffer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

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

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID ReadMetadataCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KIoBuffer::SPtr* _MetadataBuffer;

                //
                // Members needed for functionality
                //
                OverlayStream::AsyncReadMetadataContextOverlay::SPtr _AsyncReadFileContext;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogStreamKernel::AsyncReadMetadataContextKernel;
        
        NTSTATUS
        CreateAsyncReadMetadataContext(__out AsyncReadMetadataContext::SPtr& Context) override ;

        class AsyncIoctlContextKernel : public AsyncIoctlContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncIoctlContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
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
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID IoctlCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;         
                ActiveContext _ActiveContext;
                
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
                OverlayStream::AsyncIoctlContextOverlay::SPtr _AsyncIoctlContext;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
                
        };

        friend KtlLogStreamKernel::AsyncIoctlContextKernel;
        
        NTSTATUS
        CreateAsyncIoctlContext(__out AsyncIoctlContext::SPtr& Context) override ;
   
        class AsyncCloseContextKernel : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCloseContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
            public:
                void BindToParent(
                    __in KtlLogStreamKernel& LogStream
                    )
                {
                    KInvariant(_LogStream == nullptr);
                    _LogStream = &LogStream;
                }
                
                KtlLogStreamKernel::SPtr GetLogStream()
                {
                    return(_LogStream);
                }
                
                VOID
                StartClose(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

#ifdef UPASSTHROUGH
                VOID StartClose(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt CloseCompletionCallback CloseCallbackPtr
                );              
#endif
                
#if defined(K_UseResumable)   
                ktl::Awaitable<NTSTATUS>
                CloseAsync(
                    __in_opt KAsyncContextBase* const ParentAsyncContext
                    );
#endif
                
            private:
                enum { Initial, StartOperationDrain, CloseOverlayStream, Completed, CompletedWithError } _State;
                
                VOID FSMContinue(
                    __in NTSTATUS Status
                    );
                
                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

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

                VOID
                OnCancel(
                    ) override ;

            private:
                //
                // General members
                //
                KtlLogStreamKernel::SPtr _LogStream;
                KAsyncEvent::WaitContext::SPtr _WaitContext;
                OverlayLog::AsyncCloseLogStreamContextOverlay::SPtr _BaseAsyncCloseStream;
                
                //
                // Parameters to api
                //
#ifdef UPASSTHROUGH
                CloseCompletionCallback _CloseCallbackPtr;
                KAsyncContextBase* _ParentAsyncContext;             
#endif
                //
                // Members needed for functionality
                //
        };

        friend KtlLogStreamKernel::AsyncCloseContextKernel;
        
        NTSTATUS
        CreateAsyncCloseContext(__out AsyncCloseContextKernel::SPtr& Context);
        
        NTSTATUS StartClose(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        );
        
#if defined(K_UseResumable)
        // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
        // TODO: when kernel supports coroutines, actually implement this
        ktl::Awaitable<NTSTATUS>
        CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif

        AsyncCloseContextKernel::SPtr& GetAutoCloseContext()
        {
            return(_AutoCloseContext);
        }

        class AsyncStreamNotificationContextKernel : public AsyncStreamNotificationContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncStreamNotificationContextKernel);

            friend KtlLogStreamKernel;
            friend RequestMarshallerKernel;
            
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
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                //
                // General members
                //                
                KtlLogStreamKernel::SPtr _LogStream;
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                ThresholdTypeEnum _ThresholdType;
                ULONGLONG _ThresholdValue;

                //
                // Members needed for functionality
                //
        };

        friend KtlLogStreamKernel::AsyncStreamNotificationContextKernel;
        
        NTSTATUS
        CreateAsyncStreamNotificationContext(__out AsyncStreamNotificationContext::SPtr& Context);      

        
    private:
#ifdef UPASSTHROUGH
        NTSTATUS
        AdjustReservationSpace(__in LONGLONG ReservationDelta)
        {
            K_LOCK_BLOCK(_Lock)
            {
                if ((ReservationDelta < 0) && (_ReservationSpaceHeld < (-1 * ReservationDelta)))
                {
                    //
                    // Assert if removing more than is being held
                    //
                    return(STATUS_INVALID_PARAMETER);
                }

                _ReservationSpaceHeld += ReservationDelta;
            }
            return(STATUS_SUCCESS);
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
#endif
        
        NTSTATUS GenerateMetadataFileName(
            __out KWString& FileName
            );

        //
        // General routine for including an async as one running under
        // this stream. This should be called within the context's
        // Start(). It checks if the stream is still open and also
        // checks if the request had been previously cancelled.
        //
        NTSTATUS AddActiveContext(
            __in ActiveContext& Context
        );

        //
        // This is called when the request is ready to be completed. If
        // the routine returns TRUE then the thread calling has the
        // exclusive right to complete it. If FALSE then the thread may
        // not complete it as it has been completed by another thread
        //
        BOOLEAN RemoveActiveContext(
            __in ActiveContext& Context
        );

        BOOLEAN RemoveActiveContextNoSpinLock(
            __in ActiveContext& Context
        );

        //
        // This sets the cancel flag associated with the active
        // context. If the context has not been added to the list then
        // AddActiveContext will see the flag and cancel the context.
        //
        VOID SetCancelFlag(
            __in ActiveContext& Context
        );                         
        
        //
        // This will cancel all threshold contexts that are outstanding
        // with STATUS_CANCELLED
        //
        VOID CancelAllThresholdContexts(
        );

    public:
        //
        // This will perform a check of the threshold contexts for a
        // stream and complete those that are triggered based upon the
        // passed percentage
        //
        VOID CheckThresholdContexts(
            __in ULONG Percentage,
            __in NTSTATUS CompletionStatus = STATUS_SUCCESS
        );
                    
    private:
        KtlLogStreamId _LogStreamId;        // Cached value of the log stream id
        KGuid _DiskId;                      // Disk Id on which the stream lies
        OverlayStream::SPtr _BaseLogStream;
#if defined(UDRIVER) || defined(KDRIVER)
        ULONGLONG _ObjectId;
#endif
        
#ifdef UPASSTHROUGH
        KSpinLock _Lock;
        LONGLONG _ReservationSpaceHeld;
#endif

        KSharedPtr<KtlLogContainerKernel> _LogContainer;        
        ActiveContext _ActiveContext;

        KListEntry _OverlayStreamEntry;
        
        ActiveListNoSpinlock<ActiveContext, ULONGLONG> _ContextList;
        KSpinLock _ThisLock;
        BOOLEAN _IsClosed;
        BOOLEAN _IsOnContainerActiveList;
        
        ULONG _CoreLoggerMetadataOverhead;
        
        AsyncCloseContextKernel::SPtr _AutoCloseContext;        
		KInstrumentedComponent::SPtr _WriteIC;
};

//************************************************************************************
//
// KtlLogContainer Implementation
//
//************************************************************************************

class KtlLogContainerKernel : public KtlLogContainer
{
    K_FORCE_SHARED(KtlLogContainerKernel);

    friend FileObjectTable;
    friend KtlLogManagerKernel;
    
    public:
        KtlLogContainerKernel(
            __in KtlLogContainerId LogContainerId,
            __in KGuid DiskId
        );                         

#if defined(UDRIVER) || defined(KDRIVER)
        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(RequestMarshaller::LogContainer);
        }
        
        VOID SetObjectId(
            ULONGLONG ObjectId
            )
        {
            KInvariant(_ObjectId == 0);
            _ObjectId = ObjectId;
        }
        
        ULONGLONG GetObjectId()
        {
            KInvariant(_ObjectId != 0);
            return(_ObjectId);
        }
#endif
        
        static const ULONG OverlayLinkageOffset;
        
    public:     
        BOOLEAN IsFunctional() override;        

        KtlLogContainerId& GetLogContainerId()
        {
            return(_LogContainerId);
        }
        
        class AsyncAssignAliasContextKernel : public AsyncAssignAliasContext
        {
            K_FORCE_SHARED(AsyncAssignAliasContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
            public:

                VOID
                StartAssignAlias(
                    __in KString::CSPtr& Alias,
                    __in KtlLogStreamId LogStreamId,                             
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                    
#if defined(K_UseResumable)
            // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
            // TODO: when kernel supports coroutines, actually implement this
            ktl::Awaitable<NTSTATUS>
            AssignAliasAsync(
                __in KString::CSPtr& Alias,
                __in KtlLogStreamId LogStreamId,                             
                __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
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

                VOID
                OnCancel(
                    ) override ;
                
            private:
                VOID AliasAssigned(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;
                KString::CSPtr _Alias;

                //
                // Members needed for functionality
                //
                OverlayLog::AliasOperationContext::SPtr _AsyncAliasOperation;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };
        
        friend KtlLogContainerKernel::AsyncAssignAliasContextKernel;

        NTSTATUS
        CreateAsyncAssignAliasContext(__out AsyncAssignAliasContext::SPtr& Context) override ;

        class AsyncRemoveAliasContextKernel : public AsyncRemoveAliasContext
        {
            K_FORCE_SHARED(AsyncRemoveAliasContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartRemoveAlias(
                    __in KString::CSPtr& Alias,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                    
#if defined(K_UseResumable)
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
                ktl::Awaitable<NTSTATUS>
                RemoveAliasAsync(
                    __in KString::CSPtr& Alias,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
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

                VOID
                OnCancel(
                    ) override ;
                
            private:
                VOID AliasRemoved(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KString::CSPtr _Alias;

                //
                // Members needed for functionality
                //
                OverlayLog::AliasOperationContext::SPtr _AsyncAliasOperation;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogContainerKernel::AsyncRemoveAliasContextKernel;
        
        NTSTATUS
        CreateAsyncRemoveAliasContext(__out AsyncRemoveAliasContext::SPtr& Context) override ;
        
        class AsyncResolveAliasContextKernel : public AsyncResolveAliasContext
        {
            K_FORCE_SHARED(AsyncResolveAliasContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartResolveAlias(
                    __in KString::CSPtr& Alias,
                    __out KtlLogStreamId* LogStreamId,                           
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                    
#if defined(K_UseResumable)
            // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
            // TODO: when kernel supports coroutines, actually implement this
            ktl::Awaitable<NTSTATUS>
            ResolveAliasAsync(
                __in KString::CSPtr& Alias,
                __out KtlLogStreamId* LogStreamId,                           
                __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
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

                VOID
                OnCancel(
                    ) override ;
                
            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KString::CSPtr _Alias;
                KtlLogStreamId* _LogStreamId;
                
                //
                // Members needed for functionality
                //
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };
        
        friend KtlLogContainerKernel::AsyncAssignAliasContextKernel;
        
        NTSTATUS
        CreateAsyncResolveAliasContext(__out AsyncResolveAliasContext::SPtr& Context) override ;        
        
        
        class AsyncCreateLogStreamContextKernel : public AsyncCreateLogStreamContext
        {
            K_FORCE_SHARED(AsyncCreateLogStreamContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
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
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;
                
            private:
                enum { Initial, CreateStream } _State;
                
                VOID LogStreamCreateFSM(
                    __in NTSTATUS Status
                    );
                
                VOID LogStreamCreateCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
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
                KtlLogStreamKernel::SPtr _LogStream;
                OverlayLog::AsyncCreateLogStreamContextOverlay::SPtr _BaseAsyncCreateLogStream;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogContainerKernel::AsyncCreateLogStreamContextKernel;
        
        NTSTATUS
        CreateAsyncCreateLogStreamContext(__out AsyncCreateLogStreamContext::SPtr& Context) override;

        class AsyncDeleteLogStreamContextKernel : public AsyncDeleteLogStreamContext
        {
            K_FORCE_SHARED(AsyncDeleteLogStreamContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartDeleteLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

#if defined(K_UseResumable)
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
                ktl::Awaitable<NTSTATUS>
                DeleteLogStreamAsync(
                    __in const KtlLogStreamId& LogStreamId,
                    __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif
                
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

                VOID
                OnCancel(
                    ) override ;
                
            private:
                VOID LogStreamDeleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;

                //
                // Members needed for functionality
                //
                OverlayLog::AsyncDeleteLogStreamContext::SPtr _BaseAsyncDeleteLogStream;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogContainerKernel::AsyncDeleteLogStreamContextKernel;
        
        NTSTATUS
        CreateAsyncDeleteLogStreamContext(__out AsyncDeleteLogStreamContext::SPtr& Context) override;

        class AsyncOpenLogStreamContextKernel : public AsyncOpenLogStreamContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncOpenLogStreamContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
            public:
                VOID
                StartOpenLogStream(
                    __in const KtlLogStreamId& LogStreamId,
                    __out ULONG* MaximumMetaDataSize,
                    __out KtlLogStream::SPtr& LogStream,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override;

#if defined(K_UseResumable)
                // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
                // TODO: when kernel supports coroutines, actually implement this
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;
                
            private:
                VOID LogStreamOpened(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KtlLogStreamId _LogStreamId;
                ULONG* _MaximumMetaDataLength;
                KtlLogStream::SPtr* _ResultLogStreamPtr;

                //
                // Members needed for functionality
                //
                KtlLogStreamKernel::SPtr _LogStream;
                OverlayLog::AsyncOpenLogStreamContextOverlay::SPtr _BaseAsyncOpenLogStream;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogContainerKernel::AsyncOpenLogStreamContextKernel;
        
        NTSTATUS
        CreateAsyncOpenLogStreamContext(__out AsyncOpenLogStreamContext::SPtr& Context) override;


        class AsyncEnumerateStreamsContextKernel : public AsyncEnumerateStreamsContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncEnumerateStreamsContextKernel);

            friend KtlLogContainerKernel;
            friend RequestMarshallerKernel;
            
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;
                
            private:
                VOID Completion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                ActiveContext _ActiveContext;
                
                //
                // Parameters to api
                //
                KArray<KtlLogStreamId>* _LogStreamIds;

                //
                // Members needed for functionality
                //
                OverlayLog::AsyncEnumerateStreamsContextOverlay::SPtr _BaseAsyncEnumerateLogStreams;
#ifdef UPASSTHROUGH
                BOOLEAN _IsOnActiveList;
#endif
        };

        friend KtlLogContainerKernel::AsyncEnumerateStreamsContextKernel;
        
        NTSTATUS
        CreateAsyncEnumerateStreamsContext(__out AsyncEnumerateStreamsContext::SPtr& Context) override;

        
        class AsyncCloseContextKernel : public KAsyncContextBase
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncCloseContextKernel);

            friend KtlLogContainerKernel;
            
            public:
                void BindToParent(
                    __in KtlLogContainerKernel& LogContainer
                    )
                {
                    KInvariant(_LogContainer == nullptr);
                    _LogContainer = &LogContainer;
                }
                
                VOID OperationCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );
                                
                VOID
                StartClose(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

#ifdef UPASSTHROUGH
                VOID
                AsyncCloseContextKernel::StartClose(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt CloseCompletionCallback CloseCallbackPtr
                    );
#endif              
#if defined(K_UseResumable)
                ktl::Awaitable<NTSTATUS>
                CloseAsync(
                    __in_opt KAsyncContextBase* const ParentAsyncContext
                    );
#endif

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

                VOID
                OnCancel(
                    ) override ;

            private:
                enum { Initial, StartOperationDrain, CloseOverlayLog,
                       Completed, CompletedWithError } _State;
                
                VOID
                FSMContinue(
                    __in NTSTATUS Status
                    );
                
                
            private:
                //
                // General members
                //
                KtlLogContainerKernel::SPtr _LogContainer;         
                
                //
                // Parameters to api
                //
#ifdef UPASSTHROUGH
                CloseCompletionCallback _CloseCallbackPtr;
                KAsyncContextBase* _ParentAsyncContext;             
#endif

                //
                // Members needed for functionality
                //
                OverlayManager::AsyncCloseLogOverlay::SPtr _BaseAsyncCloseLog;
                KAsyncEvent::WaitContext::SPtr _WaitContext;
        };

        friend KtlLogContainerKernel::AsyncCloseContextKernel;
        
        NTSTATUS
        CreateAsyncCloseContext(__out AsyncCloseContextKernel::SPtr& Context);
        
        NTSTATUS StartClose(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt CloseCompletionCallback CloseCallbackPtr
        );

#if defined(K_UseResumable)
        ktl::Awaitable<NTSTATUS>
        CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif

        AsyncCloseContextKernel::SPtr& GetAutoCloseContext()
        {
            return(_AutoCloseContext);
        }

        //
        // General routine for including an async as one running under
        // this container. This should be called within the context's
        // Start(). It checks if the container is still open and also
        // checks if the request had been previously cancelled.
        //
        // This is also used by streams that are created under this
        // container. If the container is closed then this will fail
        // and the stream unable to be added to the list and the stream
        // creation/open will fail.
        //
        NTSTATUS AddActiveContext(
            __in ActiveContext& Context
        );

        //
        // This is called when the request is ready to be completed. If
        // the routine returns TRUE then the thread calling has the
        // exclusive right to complete it. If FALSE then the thread may
        // not complete it as it has been completed by another thread
        //
        BOOLEAN RemoveActiveContext(
            __in ActiveContext& Context
        );

        BOOLEAN RemoveActiveContextNoSpinLock(
            __in ActiveContext& Context
        );

        //
        // This sets the cancel flag associated with the active
        // context. If the context has not been added to the list then
        // AddActiveContext will see the flag and cancel the context.
        //
        VOID SetCancelFlag(
            __in ActiveContext& Context
        );                         
                
    private:
        KtlLogContainerId _LogContainerId;
        KGuid _DiskId;         // Disk Id on which the container lies
        OverlayLog::SPtr _BaseLogContainer;
#if defined(UDRIVER) || defined(KDRIVER)
        ULONGLONG _ObjectId;
#endif

        KListEntry _OverlayContainerEntry;
        
        KSpinLock _ThisLock;
        BOOLEAN _IsClosed;
        ActiveListNoSpinlock<ActiveContext, ULONGLONG> _ContextList;

        AsyncCloseContextKernel::SPtr _AutoCloseContext;
};

//************************************************************************************
//
// KtlLogManager Implementation
//
//************************************************************************************
class KtlLogManagerKernel : public KtlLogManager
{
    K_FORCE_SHARED(KtlLogManagerKernel);

    friend KtlLogManager;
    friend RequestMarshallerKernel;
    friend FileObjectTable;
    friend KtlLogStreamKernel;
    
    public:
#if defined(KDRIVER) || defined(UDRIVER) || defined(DAEMON)
        KtlLogManagerKernel(__in OverlayManager& GlobalOverlayManager);
#endif
        
#if defined(UDRIVER) || defined(KDRIVER)
        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(RequestMarshaller::LogManager);
        }
        
        VOID SetObjectId(
            ULONGLONG ObjectId
            )
        {
            KInvariant(_ObjectId == 0);
            _ObjectId = ObjectId;
        }
        
        ULONGLONG GetObjectId()
        {
            KInvariant(_ObjectId != 0);
            return(_ObjectId);
        }

#endif
        //
        // Service stubs
        //
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

#if defined(UPASSTHROUGH)
        ktl::Task OpenTask();
        ktl::Task CloseTask();
#endif
        
        class AsyncCreateLogContainerKernel : public AsyncCreateLogContainer
        {
            K_FORCE_SHARED(AsyncCreateLogContainerKernel);

            friend KtlLogManagerKernel;
            
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
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

                VOID
                StartCreateLogContainer(
                    __in KString const & Path,
                    __in KtlLogContainerId const& LogContainerId,
                    __in LONGLONG LogSize,
                    __in ULONG MaximumNumberStreams,
                    __in ULONG MaximumRecordSize,
                    __out KtlLogContainer::SPtr& LogContainer,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

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
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

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
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;

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
                OnCompleted(
                    ) override ;

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
                VOID LogContainerCreated(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );


            private:
                //
                // General members
                //
                KtlLogManagerKernel::SPtr _LogManager;         
                
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
                KtlLogContainerKernel::SPtr _LogContainer;
                OverlayManager::AsyncCreateLogOverlay::SPtr _BaseAsyncCreateLogContainer;
        };

        friend KtlLogManagerKernel::AsyncCreateLogContainerKernel;
    
        NTSTATUS
        CreateAsyncCreateLogContainerContext(__out AsyncCreateLogContainer::SPtr& Context) override ;

        
        class AsyncOpenLogContainerKernel : public AsyncOpenLogContainer
        {
            K_FORCE_SHARED(AsyncOpenLogContainerKernel);

            friend KtlLogManagerKernel;
            
        public:
            VOID
            StartOpenLogContainer(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogId,
                __out KtlLogContainer::SPtr& LogContainer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override ;
                
#if defined(K_UseResumable)
            // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
            // TODO: when kernel supports coroutines, actually implement this
            ktl::Awaitable<NTSTATUS>
            OpenLogContainerAsync(
                __in const KGuid& DiskId,
                __in const KtlLogContainerId& LogContainerId,
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
            // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
            // TODO: when kernel supports coroutines, actually implement this
            ktl::Awaitable<NTSTATUS>
            OpenLogContainerAsync(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __out KtlLogContainer::SPtr& LogContainer) override;
#endif
            
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
                VOID DoComplete(
                    __in NTSTATUS status
                    );
                        
                VOID OpenLogContainerCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                enum { Initial, LookupLogId, OpenBaseLogContainer } _State;
                VOID OpenLogContainerFSM(
                    __in NTSTATUS Status
                    );
                
            private:
                //
                // General members
                //
                KtlLogManagerKernel::SPtr _LogManager;         
                
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
                KtlLogContainerKernel::SPtr _LogContainer;
                OverlayManager::AsyncOpenLogOverlay::SPtr _BaseAsyncOpenLogContainer;
                OverlayManager::AsyncQueryLogIdOverlay::SPtr _BaseAsyncQueryLogIdContainer;
        };
        
        friend KtlLogManagerKernel::AsyncOpenLogContainerKernel;

        NTSTATUS
        CreateAsyncOpenLogContainerContext(__out AsyncOpenLogContainer::SPtr& Context) override ;


        class AsyncDeleteLogContainerKernel : public AsyncDeleteLogContainer
        {
            K_FORCE_SHARED(AsyncDeleteLogContainerKernel);

            friend KtlLogManagerKernel;
            
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
            // Only included here to satisfy the compiler when compiling UDRIVER project.  Should never be called today.
            // TODO: when kernel supports coroutines, actually implement this
            ktl::Awaitable<NTSTATUS>
            DeleteLogContainerAsync(
                __in KString const & Path,
                __in const KtlLogContainerId& LogContainerId,
                __in_opt KAsyncContextBase* const ParentAsyncContext) override;
#endif   
            
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
                VOID DoComplete(
                    __in NTSTATUS status
                    );
                        
                VOID DeleteLogContainerCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                enum { Initial, LookupLogId, DeleteBaseLogContainer } _State;
                VOID DeleteLogContainerFSM(
                    __in NTSTATUS Status
                    );

            private:
                //
                // General members
                //
                KtlLogManagerKernel::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                KGuid _DiskId;
                KString::CSPtr _Path;
                KtlLogContainerId _LogContainerId;

                //
                // Members needed for functionality
                //
                OverlayManager::AsyncDeleteLog::SPtr _BaseAsyncDeleteLogContainer;
                OverlayManager::AsyncQueryLogIdOverlay::SPtr _BaseAsyncQueryLogIdContainer;
        };
        
        friend KtlLogManagerKernel::AsyncDeleteLogContainerKernel;

        NTSTATUS
        CreateAsyncDeleteLogContainerContext(__out AsyncDeleteLogContainer::SPtr& Context) override;

        class AsyncEnumerateLogContainersKernel : public AsyncEnumerateLogContainers
        {
            K_FORCE_SHARED(AsyncEnumerateLogContainersKernel);

            friend KtlLogManagerKernel;
            
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;
                
            private:
                VOID LogContainersEnumerated(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogManagerKernel::SPtr _LogManager;         
                
                //
                // Parameters to api
                //
                KGuid _DiskId;
                KArray<KtlLogContainerId>* _LogIdArray;

                //
                // Members needed for functionality
                //
                KtlLogContainerKernel::SPtr _LogContainer;
                OverlayManager::AsyncEnumerateLogsOverlay::SPtr _BaseAsyncEnumerateLogs;
        };
        
        friend KtlLogManagerKernel::AsyncEnumerateLogContainersKernel;

        NTSTATUS
        CreateAsyncEnumerateLogContainersContext(__out AsyncEnumerateLogContainers::SPtr& Context) override;

        
        class AsyncConfigureContextKernel : public AsyncConfigureContext
        {
            K_FORCE_SHARED_WITH_INHERITANCE(AsyncConfigureContextKernel);

            friend KtlLogManagerKernel;
            friend RequestMarshallerKernel;
            
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
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                VOID ConfigureCompleted(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                //
                // General members
                //
                KtlLogManagerKernel::SPtr _LogManager;         
                
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
                OverlayManager::AsyncConfigureContextOverlay::SPtr _BaseAsyncConfigureContext;
        };

        friend KtlLogManagerKernel::AsyncConfigureContextKernel;
        
        NTSTATUS
        CreateAsyncConfigureContext(__out AsyncConfigureContext::SPtr& Context) override ;
        

        
    protected:
        using KAsyncContextBase::Cancel;

        VOID OnServiceOpen() override ;   // StartOpen() was called and accepted; default: calls CompleteOpen(STATUS_SUCCESS)
        VOID OnServiceClose() override ;  // StartClose() was called and accepted; default: calls CompleteClose(STATUS_SUCCESS)
        VOID OnServiceReuse() override ;  // Reuse() was called; default: nothing 

    public:
        KWString _LogContainerType;
        
    private:
        NTSTATUS _FinalStatus;
#if defined(UDRIVER) || defined(KDRIVER)
        ULONGLONG _ObjectId;
#endif
        
        OverlayManager::SPtr _OverlayManager;

#if defined(UDRIVER) || defined(KDRIVER)
    // TODO: FOR DEBUGGING PURPOSES, REMOVE WHEN FIXED
    public:     
        FileObjectTable* _FileObjectTable;
        BOOLEAN _CleanupCalled;
    // TODO: FOR DEBUGGING PURPOSES, REMOVE WHEN FIXED
#endif
};

#if defined(UDRIVER) || defined(KDRIVER)
class KtlLogObjectHolder
{
    public:
        KtlLogObjectHolder() :
                _ObjectPtr(nullptr),
                _ObjectType(RequestMarshaller::NullObject),
                _ObjectId(0)
        {
        }
                
        ~KtlLogObjectHolder()
        {
        }
        
        KtlLogManager* GetLogManager()
        {
            KInvariant((_ObjectType == RequestMarshaller::LogManager));
            return(static_cast<KtlLogManager*>(_ObjectPtr.RawPtr()));
        }
        
        KtlLogContainer* GetLogContainer()
        {
            KInvariant((_ObjectType == RequestMarshaller::LogContainer));
            return(static_cast<KtlLogContainer*>(_ObjectPtr.RawPtr()));
        }
        
        KtlLogStream* GetLogStream()
        {
            KInvariant((_ObjectType == RequestMarshaller::LogStream));
            return(static_cast<KtlLogStream*>(_ObjectPtr.RawPtr()));
        }

        PVOID GetObjectPointer()
        {
            return(_ObjectPtr.RawPtr());
        }

        RequestMarshaller::OBJECT_TYPE GetObjectType()
        {
            return(_ObjectType);
        }

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

        VOID ResetCloseContext(
        )
        {
            _CloseContext = nullptr;
        }
        
        VOID ClearObject(
        )
        {
            //
            // Force close any log containers or log streams
            //
            if (_ObjectType == RequestMarshaller::LogContainer)
            {
                if (_CloseContext)
                {
                    KtlLogContainerKernel::AsyncCloseContextKernel::SPtr closeContext;
                    closeContext = down_cast<KtlLogContainerKernel::AsyncCloseContextKernel>(_CloseContext);
                    
                    KDbgPrintfInformational("ClearContainerObject %p\n", closeContext.RawPtr());
                    
                    closeContext->StartClose(nullptr,
                                             nullptr);
                    _CloseContext = nullptr;
                }
            }

            if (_ObjectType == RequestMarshaller::LogStream)
            {
                if (_CloseContext)
                {
                    
                    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr closeContext;
                    closeContext = down_cast<KtlLogStreamKernel::AsyncCloseContextKernel>(_CloseContext);
                    
                    KDbgPrintfInformational("ClearStreamObject %x %p\n", closeContext->GetLogStream() ? closeContext->GetLogStream()->GetLogStreamId().Get().Data1 : 0,
                                             closeContext.RawPtr());

                    closeContext->StartClose(nullptr,
                                             nullptr);
                    _CloseContext = nullptr;
                }
            }

            _ObjectPtr = nullptr;
        }
        
        VOID SetObject(
            __in RequestMarshaller::OBJECT_TYPE ObjectType,
            __in ULONGLONG ObjectId,
            __in KSharedBase* ObjectPtr,
            __in KSharedBase::SPtr& CloseContext
        )                          
        {
            KInvariant(_ObjectType == RequestMarshaller::NullObject);
            KInvariant(_ObjectPtr == nullptr);
            
            KInvariant((ObjectType == RequestMarshaller::NullObject) ||
                    (ObjectType == RequestMarshaller::LogManager) ||
                    (ObjectType == RequestMarshaller::LogContainer) ||
                    (ObjectType == RequestMarshaller::LogStream));
    
            
            _ObjectId = ObjectId;
            _ObjectType = ObjectType;
            _ObjectPtr = ObjectPtr;
            _CloseContext = CloseContext;
        }
        
    private:
        RequestMarshaller::OBJECT_TYPE _ObjectType;
        ULONGLONG _ObjectId;        
        KSharedBase::SPtr _ObjectPtr;
        KSharedBase::SPtr _CloseContext;
};

class FileObjectTable;

class KIoBufferElementKernel : public KIoBufferElement
{
    K_FORCE_SHARED(KIoBufferElementKernel);

    friend FileObjectTable;
    
    public:
        static NTSTATUS Create(
            __in FileObjectTable& FOT,                                      
            __in BOOLEAN IsReadOnly,
#ifdef UDRIVER
            __in KIoBufferElement& IoBufferElementUser,
#endif
            __in PVOID Src,
            __in ULONG Size,
            __out KIoBufferElement::SPtr& IoBufferElement,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_IO_BUFFER
        );
            
    private:
        KListEntry _ListEntry;
        KSharedPtr<FileObjectTable> _FOT;
#ifdef UDRIVER
        KIoBufferElement::SPtr _IoBufferElementUser;
#endif
#ifdef KDRIVER
        PMDL _Mdl;
#endif

};
#endif   

#include "FileObjectTable.h"


#if defined(UDRIVER) || defined(KDRIVER)
//
// This is a struct that is used by the top layer to dispatch
// requests received from user mode. 
//
class RequestMarshallerKernel : public RequestMarshaller
{
    K_FORCE_SHARED(RequestMarshallerKernel);

    friend FileObjectTable;
    
    public:

        static NTSTATUS Create(
            __out RequestMarshallerKernel::SPtr& Result,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

    public:     
        NTSTATUS ReadKIoBuffer(
            __out KIoBuffer::SPtr& IoBuffer,
            __in BOOLEAN IsReadOnly
        );

        NTSTATUS ReadKIoBufferElement(
            __out KIoBufferElement::SPtr& IoBufferElement,
            __in BOOLEAN IsReadOnly
            );
        
        NTSTATUS WriteObject(
            __in OBJECT_TYPE ObjectType,
            __in ULONGLONG ObjectId
        );

        NTSTATUS WriteKIoBuffer(
            __in KIoBuffer& IoBuffer
        );
        
        NTSTATUS WriteKIoBufferPreAlloc(
            __in KIoBuffer& IoBuffer,
            __in KIoBuffer& UserIoBuffer
        );

        NTSTATUS InitializeRequestFromUser(
            __in FileObjectTable::SPtr& FOT,
            __out KtlLogObjectHolder& Object
            );

        NTSTATUS VerifyOutBufferSize(
            __in ULONG OutBufferSizeNeeded
        );

        VOID SetOutBufferSizeNeeded(
            __in ULONG OutBufferSizeNeeded
        )
        {
            _WriteIndex = OutBufferSizeNeeded;
        }
                                     
        
        VOID FormatCreateObjectResponseToUser(
            __in OBJECT_TYPE ObjectType,
            __in ULONGLONG ObjectId
            );            

        VOID FormatResponseToUser(
            );            
        
        VOID Reset(
        );

        NTSTATUS MarshallRequest(
            __in PVOID WdfRequest,
            __in FileObjectTable::SPtr& FOT,
            __in DevIoControlKernel::SPtr& DevIoCtl,
            __out BOOLEAN* CompleteRequest,
            __out ULONG* OutBufferSize
            );

        NTSTATUS PerformRequest(
            __in FileObjectTable::SPtr& FOT,
            __in DevIoControlKernel::SPtr& DevIoCtl
            );

        VOID FormatOutputBuffer(
            __out ULONG& OutBufferSize,
            __inout NTSTATUS& Status                                    
        );
        
        VOID PerformCancel(
            );
        
        static VOID Cleanup(
            __in FileObjectTable& FOT
#ifdef UDRIVER
          , __in KAllocator& Allocator
#endif
            );

        RequestMarshaller::OBJECT_METHOD GetObjectMethod()
        {
            return(_ObjectMethod);
        }

        ULONGLONG GetRequestId()
        {
            return(_RequestId);
        }
        
    private:

        VOID
        CloseLogManagerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase& Async,
            __in NTSTATUS Status
            );
        
        VOID CreateLogManagerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase& Async,
            __in NTSTATUS Status
            );

        NTSTATUS CreateLogManagerMethod(
            );

        NTSTATUS CreateLogManagerCancel(
            );

        VOID CreateLogContainerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS CreateLogContainerMethod(
        );

        NTSTATUS CreateLogContainerMarshall(
        );
        
        VOID OpenLogContainerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS OpenLogContainerMethod(
         );

        NTSTATUS OpenLogContainerMarshall(
         );

        VOID DeleteLogContainerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS DeleteLogContainerMethod(
            );

        NTSTATUS DeleteLogContainerMarshall(
            );
        
        VOID EnumerateLogContainersCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS EnumerateLogContainersMethod(
            );

        NTSTATUS EnumerateLogContainersMarshall(
            );

        VOID ConfigureLogManagerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS ConfigureLogManagerMethod(
            );

        NTSTATUS ConfigureLogManagerMarshall(
            );

        
        NTSTATUS
        LogManagerObjectMethodRequest(
            __in OBJECT_METHOD ObjectMethod
            );

        VOID CreateLogStreamCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS CreateLogStreamMethod(
            );

        NTSTATUS CreateLogStreamMarshall(
            );

        VOID DeleteLogStreamCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS DeleteLogStreamMethod(
            );

        NTSTATUS DeleteLogStreamMarshall(
            );

        VOID OpenLogStreamCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS OpenLogStreamMethod(
            );

        NTSTATUS OpenLogStreamMarshall(
            );

        VOID CloseContainerCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS CloseContainerMethod(
            );

        NTSTATUS CloseContainerMarshall(
            );

        VOID AssignAliasCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS AssignAliasMethod(
            );

        NTSTATUS AssignAliasMarshall(
            );

        VOID RemoveAliasCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
        NTSTATUS RemoveAliasMethod(
            );
        
        NTSTATUS RemoveAliasMarshall(
            );
        
        VOID ResolveAliasCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
                
        NTSTATUS ResolveAliasMethod(
            );
                
        NTSTATUS ResolveAliasMarshall(
            );
                
        VOID EnumerateStreamsCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
                
        NTSTATUS EnumerateStreamsMethod(
            );
                
        NTSTATUS EnumerateStreamsMarshall(
            );
                
        NTSTATUS
        LogContainerObjectMethodRequest(
            __in OBJECT_METHOD ObjectMethod
            );


        VOID QueryRecordRangeCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS QueryRecordRangeMethod(
            );

        NTSTATUS QueryRecordRangeMarshall(
            );

        VOID QueryRecordCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS QueryRecordMethod(
            );

        NTSTATUS QueryRecordMarshall(
            );

        VOID QueryRecordsCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS QueryRecordsMethod(
            );

        NTSTATUS QueryRecordsMarshall(
            );

        VOID WriteCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS WriteMethod(
            );

        NTSTATUS WriteMarshall(
            );

        VOID ReadCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS ReadMethod(
            );

        NTSTATUS ReadMarshall(
            );

        VOID MultiRecordReadCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS MultiRecordReadMethod(
            );

        NTSTATUS MultiRecordReadMarshall(
            );

        VOID TruncateCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS TruncateMethod(
            );

        NTSTATUS TruncateMarshall(
            );

        VOID WriteMetadataCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS WriteMetadataMethod(
            );

        NTSTATUS WriteMetadataMarshall(
            );

        VOID ReservationCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS ReservationMethod(
            );

        NTSTATUS ReservationMarshall(
            );

        VOID ReadMetadataCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
        NTSTATUS ReadMetadataMethod(
            );
        
        NTSTATUS ReadMetadataMarshall(
            );
        
        VOID IoctlCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
        NTSTATUS IoctlMethod(
            );
        
        NTSTATUS IoctlMarshall(
            );
        
        VOID CloseStreamCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS CloseStreamMethod(
            );

        NTSTATUS CloseStreamMarshall(
            );

        VOID StreamNotificationCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        NTSTATUS StreamNotificationMethod(
            );

        NTSTATUS StreamNotificationMarshall(
            );

        NTSTATUS
        LogStreamObjectMethodRequest(
            __in OBJECT_METHOD ObjectMethod
            );
                
        NTSTATUS CreateObjectRequest(
            );
        
        NTSTATUS DeleteObjectMethod(
            );
        
        NTSTATUS DeleteObjectRequest(
            );
        
        NTSTATUS ObjectMethodRequest(
            );

        NTSTATUS
        SingleContextCancel(
            );      
        
        VOID CompleteRequest(
            __in NTSTATUS Status
            );

        typedef KDelegate<NTSTATUS(
            )> ObjectMethodCallback;

        typedef KDelegate<NTSTATUS(
            )> ObjectCancelCallback;

    public:

        BOOLEAN IsMyKey(
            __in ULONGLONG& Key
            )
        {
            return(_RequestId == Key);
        }
        
    private:
        //
        // General members
        //
        PVOID _WdfRequest;      
        KListEntry _ListEntry;
        ULONGLONG _RequestId;
        RequestMarshallerKernel::ObjectCancelCallback _ObjectCancelCallback;
        RequestMarshaller::OBJECT_METHOD _ObjectMethod;

        //
        // Parameters to api
        //
        FileObjectTable::SPtr _FOT;
        DevIoControlKernel::SPtr _DevIoCtl;
        
        //
        // Members needed for functionality
        //        
        RequestMarshaller::REQUEST_BUFFER* _Request;
        
        KtlLogObjectHolder _ParentObject;

        KtlLogManagerKernel::SPtr _LogManager;
        KtlLogContainerKernel::SPtr _LogContainer;
        KtlLogStreamKernel::SPtr _LogStream;


        FileObjectEntry::SPtr _FileObjectEntry;
        
        //
        // Parameters to the various methods that need to be invoked
        //
        KAsyncContextBase::SPtr _Context;       
        ObjectMethodCallback _ObjectMethodCallback;


        // CONSIDER: Unionize these parameters
        
        //
        // CreateLogManager
        //
        OBJECT_TYPEANDID _ObjectTypeAndId;

        //
        // DeleteObject
        //

        //
        // CreateLogContainer
        //
        KGuid _DiskId;
        KtlLogContainerId _LogContainerId;
        LONGLONG _LogSize;
        ULONG _MaximumRecordSize;
        ULONG _MaximumNumberStreams;
        DWORD _Flags;

        //
        // OpenLogContainer
        //
//      KGuid _DiskId;
//      KtlLogContainerId _LogContainerId;
        
        //
        // DeleteLogContainer
        //
//      KGuid _DiskId;
//      KtlLogContainerId _LogContainerId;

        //
        // EnumerateLogContainers
        //
//      KGuid _DiskId;
        KArray<KtlLogContainerId> _LogIdArray;

        //
        // ConfigureLogManager
        //
        ULONG _Code;
        KBuffer::SPtr _InBuffer;
        ULONG _Result;
        KBuffer::SPtr _OutBuffer;
        
        //
        // CreateLogStream
        //
        KtlLogStreamId _LogStreamId;
        KtlLogStreamType _LogStreamType;
        KString::CSPtr _Alias;
        KString::CSPtr _Path;
        ULONG _MaximumMetaDataLength;
        LONGLONG _MaximumStreamSize;
//        ULONG _MaximumRecordSize;
//      DWORD _Flags;
        KBuffer::SPtr _SecurityDescriptor;
        
        //
        // DeleteLogStream
        //
//      KtlLogStreamId _LogStreamId;

        //
        // AssignAlias
        //
//      KtlLogStreamId _LogStreamId;
//      KString::CSPtr _Alias;

        //
        // RemoveAlias
        //
//      KString::CSPtr _Alias;

        //
        // ResolveAlias
        //
//      KString::CSPtr _Alias;

        //
        // EnumerateStreams
        //
        KArray<KtlLogStreamId> _LogStreamIdArray;

        //
        // LogStream::QueryRecordRange
        //
        KtlLogAsn _LowestAsn;
        KtlLogAsn _HighestAsn;
        KtlLogAsn _LogTruncationAsn;

        //
        // LogStream::QueryRecord
        //
        KtlLogAsn _Asn;
        
        ULONGLONG _Version;
#ifdef USE_RVDLOGGER_OBJECTS
        RvdLogStream::RecordDisposition _Disposition;
#else
        KtlLogStream::RecordDisposition _Disposition;
#endif
        ULONG _IoBufferSize;
        ULONGLONG _DebugInfo;

        //
        // LogStream::QueryRecords
        //
//      KtlLogAsn _LowestAsn;
//      KtlLogAsn _HighestAsn;
        
#ifdef USE_RVDLOGGER_OBJECTS
        KArray<RvdLogStream::RecordMetadata> _ResultsArray;
#else
        KArray<RecordMetadata> _ResultsArray;
#endif

        //
        // LogStream::Write
        //
        KtlLogAsn _RecordAsn;
//      ULONGLONG _Version;
        ULONG _MetaDataLength;
        ULONGLONG _ReservationSpace;
        KIoBuffer::SPtr _MetaDataBuffer;
        KIoBuffer::SPtr _IoBuffer;
        KIoBuffer::SPtr _PreAllocMetaDataBuffer;
        KIoBuffer::SPtr _PreAllocIoBuffer;
        ULONGLONG _CurrentLogSize;
        ULONGLONG _CurrentLogSpaceRemaining;

        //
        // LogStream::Reservation
        //
        LONGLONG _ReservationDelta;
        
        //
        // LogStream::Read
        //
        RvdLogStream::AsyncReadContext::ReadType _ReadType;
//      KtlLogAsn _RecordAsn;
        KtlLogAsn _ActualAsn;
//      ULONGLONG _Version;
//      ULONG _MetaDataLength;
//        KIoBuffer::SPtr _MetaDataBuffer;
//        KIoBuffer::SPtr _IoBuffer;

        //
        // LogStream::MultiRecordRead
        //
        //        KtlLogAsn _RecordAsn;
        //        KIoBuffer::SPtr _MetaDataBuffer;
        //        KIoBuffer::SPtr _IoBuffer;


        //
        // LogStream:Truncate
        //
        KtlLogAsn _TruncateAsn;
        KtlLogAsn _PreferredTruncateAsn;

        //
        // WriteMetadata
        //
//      KIoBuffer::SPtr _MetaDataBuffer;

        //
        // ReadMetadata
        //
//      KIoBuffer::SPtr _PreAllocMetaDataBuffer;
//      KIoBuffer::SPtr _MetaDataBuffer;

        //
        // Ioctl
        //
        ULONG _ControlCode;
//        KBuffer::SPtr _InBuffer;
//        ULONG _Result;
//        KBuffer::SPtr _OutBuffer;
        
        //
        // StreamNotification
        //
        KtlLogStream::AsyncStreamNotificationContext::ThresholdTypeEnum _ThresholdType;
        ULONGLONG _ThresholdValue;      
};
#endif
