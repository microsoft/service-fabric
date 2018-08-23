// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

class WrappedServiceBase;
class ServiceWrapper : public KObject<ServiceWrapper>, public KShared<ServiceWrapper>
{
    K_FORCE_SHARED_WITH_INHERITANCE(ServiceWrapper);

    public:
        //
        // This activates the queue and starts it pumping operations.
        //
        // The supplied callback (Callback) will be invoked once DeactivateQueue() is called
        // and the instance has finished its shutdown process - meaning any queued item have been dispatched
        // for processing by a DequeueOperation instance - meaning the completion process started on the
        // corresponding DequeueOperation. NOTE: This means that the final successful DequeueOperation completion
        // can race with the completion of the owning KAsyncQueue triggered by Deactivate().
        //
        NTSTATUS
        ActivateQueue(
            );

        //
        // This deactivates the queue and drains currently queued
        // operations. Any new operations started will receive an error
        //
        typedef KDelegate<VOID(
            ServiceWrapper& Service
        )> DeactivateQueueCallback;
        
        VOID
        DeactivateQueue(
            __in_opt DeactivateQueueCallback Callback                       
            );

        //
        // This returns a pointer to the underlying service
        //
        KSharedPtr<WrappedServiceBase> GetUnderlyingService()
        {
            return(_UnderlyingService);
        }

        class GlobalContext : public KAsyncGlobalContext
        {
            K_FORCE_SHARED(GlobalContext);

            GlobalContext(
                __in KActivityId ActivityId
                );

        public:
            static NTSTATUS Create(
                __in KGuid Id,
                __in KAllocator& Allocator,
                __in ULONG AllocationTag,
                __out GlobalContext::SPtr& GlobalContext);
        };
        

        
        //
        // Users can use this async as is or derive from it
        //
        class Operation : public KAsyncContextBase
        {
            //
            // This class represents an operation against the wrapped
            // service and is pumped through the service wrapper queue
            // and executed in order. Basic operations include service
            // open, service close and service shutdown. Extended
            // operations include Create and Delete.
            //
            // Each operation is dispatched through to the appropriate
            // OnOperation* callback in the service wrapper class.
            // Operation callbacks for extended operations can be
            // overridden with methods in a derived class while basic
            // operations are not able to be overridden.
            //
            // If the OnOperation* method is overridden the derived
            // implementation must call CompleteOpAndStartNext() to
            // complete the current operation and dispatch the next one
            // in the queue
            //
            // An implementor can derive from this class in case
            // additional information about the operation should be
            // passed to the OnOperation method. For example the
            // Operation instance passed to StartOpenOperation() could
            // be a derived class which contains the initial
            // parameters for configuring a service and would be
            // accessed in the CreateUnderlyingService() callback.
            // Similarly the operation passed to StartCreateOperation
            // can have parameters for the creation of the underlying
            // service resources.
            //
            K_FORCE_SHARED_WITH_INHERITANCE(Operation);

            friend ServiceWrapper;
            
            protected:
                Operation(
                    __in ServiceWrapper& Service
                );
                
            public:
                static NTSTATUS Create(
                    __out Operation::SPtr& Context,
                    __in ServiceWrapper& Service,
                    __in KAllocator& Allocator,
                    __in ULONG AllocationTag
                );
                
                typedef enum { Open, Close, Shutdown, Creation, Delete, Recover } OperationType;

                OperationType GetOperationType()
                {
                    return(_Type);
                }

                static const ULONG ListEntryOffset;
                
                // This will start an open operation on the service. If the
                // service is already opened then it is a NO OP other than
                // incrementing the usage count. It is expected that any
                // successful completion of the StartOpenOperation must be
                // accompanied by a successful StartCloseOperation or otherwise
                // the underlying service may not be stopped.
                //
                // In the case of the first open operation for the
                // service, the services CreateUnderlyingService()
                // method is called to create the service object to
                // start.
                //
                VOID StartOpenOperation(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

                //
                // This will start an close operation on the service.
                // If this is the last close operation then the service
                // will be closed.
                //
                VOID StartCloseOperation(
                    __in VOID* ClosingService,
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

                //
                // This will start a shutdown operation on the service.
                // Regardless of usage count, the service is closed and
                // usage count reset to 0. The service is disassociated
                // with the service wrapper.
                //
                VOID StartShutdownOperation(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

                //
                // This will start an create operation for the
                // resources underlying the wrapped service. Note that
                // this does not instantiate the service object. 
                //
                VOID StartCreateOperation(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

                //
                // This will start an delete operation for the
                // resources underlying the wrapped service. Note that
                // this does not instantiate the service object. 
                //
                VOID StartDeleteOperation(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );
                
                //
                // This will start a recovery operation for the
                // resources underlying the wrapped service. Note that
                // this will perform an open, recovery and close for
                // the underlying service
                //
                VOID StartRecoverOperation(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );
                
                //
                // When an operation has finished processing, it should
                // call this method with the completion status to
                // complete the operation back to the caller and start
                // the next operation in the queue
                //
                VOID CompleteOpAndStartNext(
                    __in NTSTATUS Status
                    );
                
            protected:
                VOID SetOperationType(OperationType Type)
                {
                    _Type = Type;
                }
                
                VOID OnStart();
                
            private:
                VOID StartAndQueueOperation(
                    __in_opt KAsyncContextBase* const ParentAsyncContext,
                    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                    );

            protected:
                VOID* _ClosingService;
                
            private:
                ServiceWrapper::SPtr _Service;
                KListEntry _ListEntry;
                OperationType _Type;
        };

        typedef NTSTATUS (*CreateServiceFactory)(
            __out KSharedPtr<WrappedServiceBase>& Context,    
            __in ServiceWrapper& FreeService,
            __in ServiceWrapper::Operation& OperationOpen,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );
        
        ServiceWrapper::ServiceWrapper(
            __in CreateServiceFactory CreateUnderlyingService
            );
        
        
    private:
        class WaitForQueueToDeactiveContext : public KAsyncContextBase
        {
            K_FORCE_SHARED(WaitForQueueToDeactiveContext);

            friend ServiceWrapper;
            
            WaitForQueueToDeactiveContext(
                __in ServiceWrapper& Service
            );

            private:
                VOID OnStart()
                {
                }

                VOID OnReuse()
                {
                }

                VOID DeactivateCompletion(
                    __in_opt KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

            private:
                ServiceWrapper::SPtr _Service;
                DeactivateQueueCallback _DeactivateQueueCallback;                       
        };
                
    protected:
        //
        // OnOperationOpen, OnOperationClose and
        // OnOperationDeleteOrShutdown should not be overridden.
        //
        VOID OnOperationOpen(
            __in Operation& Op
        );
        
        virtual NTSTATUS OpenToCloseTransitionWorker(
            __in NTSTATUS Status,
            __in ServiceWrapper::Operation& Op,
            __in KAsyncServiceBase::CloseCompletionCallback CloseCompletion                                           
            );
        
        VOID OnOperationClose(
            __in Operation& Op
        );

        VOID OnOperationDeleteOrShutdown(
            __in Operation& Op
        );

        //
        // OnOperationRecover, OnOperationCreate and OnOperationDelete are expected to be overridden for specific
        // functionality. Default implementation just completes the Op
        // successfully and dequeues the next item
        //
        virtual VOID OnOperationCreate(
            __in Operation& Op
        );

        virtual VOID OnOperationDelete(
            __in Operation& Op
        );

    public:
        typedef enum { Opening, Opened, Closing, Closed } ObjectState;
        
        ObjectState GetObjectState()
        {
            return(_ObjectState);
        }

        VOID SetObjectState(ObjectState Os)
        {
            _ObjectState = Os;
        }                            

    private:
        VOID
        StartDequeueNextOperation(
            );

        VOID
        DroppedItemCallback(
            __in KAsyncQueue<Operation>& DeactivatingQueue,
            __in Operation& DroppedItem
            );

        VOID DequeueCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );

        VOID
        OpenCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase& Async,
            __in NTSTATUS Status
            );

        VOID
        CloseCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase& Async,
            __in NTSTATUS Status
            );
        
        VOID
        RecoveryOpenCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase& Async,
            __in NTSTATUS Status
            );

        VOID OuterOperationRecover(
            __in Operation& Op
            );

        NTSTATUS OperationCloseWorker(
            __in Operation& Op,
            __in KAsyncServiceBase::CloseCompletionCallback CloseCompletion                                           
            );

        NTSTATUS OperationOpenWorker(
            __in Operation& Op,
            __in KAsyncServiceBase::OpenCompletionCallback OpenCompletion                                            
            );              
        
        VOID
        ForceCloseCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase& Async,
            __in NTSTATUS Status
            );
        
    private:
        ObjectState _ObjectState;

        KAsyncQueue<Operation>::SPtr _OperationQueue;
        KAsyncQueue<Operation>::DequeueOperation::SPtr _DequeueItem;

        ULONG _UsageCount;

        WaitForQueueToDeactiveContext::SPtr _WaitForDeactivate;

        CreateServiceFactory _CreateUnderlyingService;
        KSharedPtr<WrappedServiceBase> _UnderlyingService;
};

//
// WrappedServiceBase is the base class from which any service managed by
// ServiceWrapper should be derived. Users need to implement all of the
// methods defined below:
//
// CreateUnderlyingService is the static factory method for the inner service
//     being managed by ServiceWrapper. It should be passed to the
//     ServiceWrapper constructor so that the ServiceWrapper can callback
//     when the service needs to be created.
//
// StartOpenService is the method that should be forwarded to the
//     KAsyncServiceBase::StartOpen() for the inner service
//
// StartCloseService is the method that should be forwarded to the
//     KAsyncServiceBase::StartClose() for the inner service
//
class WrappedServiceBase abstract : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(WrappedServiceBase);

    friend ServiceWrapper;
    
    protected:
        static NTSTATUS CreateUnderlyingService(
            __out WrappedServiceBase::SPtr& Context,    
            __in ServiceWrapper& FreeService,
            __in ServiceWrapper::Operation& OperationOpen,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        virtual NTSTATUS StartServiceOpen(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in_opt OpenCompletionCallback OpenCallbackPtr,
            __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        ) = 0;

        virtual NTSTATUS StartServiceClose(
            __in_opt KAsyncContextBase* const ParentAsync, 
            __in_opt CloseCompletionCallback CloseCallbackPtr
        ) = 0;      
};



