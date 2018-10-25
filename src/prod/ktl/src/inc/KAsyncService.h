/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    KAsyncService.cpp

    Description:
      Kernel Template Library (KTL): Async Service Support Definitions

    History:
      richhas          25-Jun-2012        Initial version.

    (c) 2010 by Microsoft Corp. All Rights Reserved.

--*/

#pragma once

//** KAsyncServiceBase class
//
//    Description:
//        This class provides a standard base implementation for those wishing to build
//        KTL based services - KAsyncServices. 
//
//        From a derivation imp's point of view, KAsyncServices support a common async Open/Close 
//        pattern via the StartOpen() and StartClose() protected methods. KAsyncServices will 
//        invoke OpenCompletionCallback and CloseCompletionCallback callbacks within the 
//        (optional) ParentAsync passed to StartOpen() and StartClose(). 
//
//        When either StartOpen() or StartClose() return an error status, their corresponding
//        callback will not be invoked. It is not valid to issue a StartClose() prior to 
//        control being returned from StartOpen() - if this is done a failfast may occur. It
//        is best practice to not call StartClose() until after StartOpen() has completed via
//        its callback. It may, however, be necessary to do so at times. In this case the Close
//        logic will be held internally until the open completes either sucessfully or with and 
//        error. In the later case, the open and close callbacks will race, and it is the 
//        user's responsibility to sync it's use of Renew() across these racing callbacks 
//        if needed.
//
//        NOTE: The Start(), Cancel(), and Complete() methods are not available. Casting to 
//              call these will result in ramdom behavior.
//
//        From a derivation imp's point of view, after StartOpen() is called, the deravation 
//        API OnServiceOpen() method is called to trigger the specific open process. This open
//        process is completed by the derivation calling the support method CompleteOpen() - 
//        passing the open completion status. If the open completion status does not indicate
//        success, the KAsyncService will itself be completed with the error status. In either
//        case the open completion callback is invoked with the open completion status passed to
//        CompleteOpen(). If StartClose() is then issued on such a failed KAsyncService, 
//        StartClose() will return the failed open status. It is valid to issue a StartClose()
//        at any time after successful return from StartOpen() - best practice is to do so only
//        after the StartOpen's() callback has been invoked however.
//
//        Once a KAsyncService has been opened via a successful call to CompleteOpen(), a call to
//        StartClose() will result in the OnServiceClose() deravation method being invoked -
//        triggering specific closure behavior. This specific behavior is completed by the derivation
//        via a call to CompleteClose(). Once CompleteClose() has been called (with a close
//        completion status), the optional close completion callback supplied to the StartClose()
//        method will be invoked in the apartment of the (optional) ParentAsync passed to StartClose().
//
//        In order to support specific open-delayed custom APIs, the KAsyncService derivation 
//        support API supplies the GetOpenStatus() method. 
//
//        A derivation wishing to delay its specific APIs until after open is done would first 
//        call TryAcquireActivities() and assuming a success (indicating the instance is locked in an
//        active, non-completed, stat) GetOpenStatus() would be called to determine if open might still
//        be active. If not, it is free to complete it API work without waiting. If however GetOpenStatus()
//        indicates that the open might still be active, it can return an error.
//
//        There is also support for "deferred closure" - See Deferred Closure API below.
//

class KAsyncServiceBase : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KAsyncServiceBase);

public:     // Public API:
    typedef KDelegate<VOID(
        KAsyncContextBase* const,           // Parent; can be nullptr
        KAsyncServiceBase&,                 // BeginOpen()'s Service instance
        NTSTATUS                            // Completion status of the Open
        )> OpenCompletionCallback;

    typedef KDelegate<VOID(
        KAsyncContextBase* const,           // Parent; can be nullptr
        KAsyncServiceBase&,                 // BeginClose()'s Service instance
        NTSTATUS                            // Completion status of the Close
        )> CloseCompletionCallback;


    //* Obtain the opening state of the current KAsyncContextBase
    //
    //    Returns:
    //        STATUS_PENDING        - The Open process is still pending. If there is a reason to delay/filter
    //                                API invocation until after open has completed, this is the indicator
    //                                to do so.
    //        NT_SUCCESS            - The instance has been opened successfully
    //        !NT_SUCCESS           - The instance failed to open for the reason returned
    //
    NTSTATUS
    GetOpenStatus();

    //* Determine if the state of the service instance is opened
    BOOLEAN 
    IsOpen();

protected:  // Derivation Support API:

    //* OPEN A SERVICE INSTANCE.
    //
    //  Parameters:
    //      ParentAsync           - an optional pointer to an optional parent kasynccontextbase instance that is to
    //                              receive and dispatch open completions thru. The value of this parameter may be nullptr 
    //                              to disable the parent relationship. A referenced parent will not complete as 
    //                              long as the corresponding open callback (*callbackptr) has not been invoked.
    //
    //      OpenCallbackPtr       - Optional open completion callback. This function will be called when the instance 
    //                              has completed its open process internally. NOTE: even if parentasync is empty, this 
    //                              callback will still be called.
    //
    //      GlobalContextOverride - Optional pointer to a kasyncglobalcontext derivation for the kasyncservicebase 
    //                              operational life. If one is not supplied and there is a parentasync, that parent's global 
    //                              context pointer will be used. access to the current operation's KAsyncGlobalContext is 
    //                              made available to the derivation via GetGlobalContext().
    //
    //  returns:
    //      STATUS_PENDING                  - Open is in progress
    //      STATUS_SHARING_VIOLATION        - Attempt to open and already open service 
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //      <!NT_SUCCESS>                   - Open failed - KasyncContextBase instance is not active
    //
    NTSTATUS
    StartOpen(      
        __in_opt KAsyncContextBase* const ParentAsync, 
        __in_opt OpenCompletionCallback OpenCallbackPtr,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr);

    //* Close a Service Instance.
    //
    //  Parameters:
    //      ParentAsync           - An optional pointer to an optional parent KAsyncContextBase instance that is to
    //                              receive and dispatch close completions thru. The value of this parameter may be 
    //                              nullptr to disable the parent relationship. A referenced parent will not complete as 
    //                              long as the corresponding close callback (*CallbackPtr) has not been invoked.
    //
    //      CloseCallbackPtr      - Optional close completion callback. This function will be called when the instance 
    //                              has completed its close process internally. NOTE: even if ParentAsync is empty, this 
    //                              callback will still be called.
    //
    //  Returns:
    //      STATUS_PENDING                  - Requested close is in progress
    //      STATUS_UNSUCCESSFUL             - Attempt to close and non-open service
    //      STATUS_INSUFFICIENT_RESOURCES   - Out of memory
    //      <!NT_SUCCESS>                   - Open failed - KAsyncContextBase instance is not active
    //
    NTSTATUS
    StartClose(
        __in_opt KAsyncContextBase* const ParentAsync, 
        __in_opt CloseCompletionCallback CloseCallbackPtr);

    //* Complete the open process started by an OnServiceOpen() call.
    //
    //    If the OpenStatus indicates a failure, the current KAsyncContextBase instance is internally completed
    //    and the OpenCallbackPtr function (from StartOpen) is invoked. In addition, if there is a close pending 
    //    on the current KAsyncContextBase, that close is completed via its callback with a status of OpenStatus.
    //
    //    If OpenStatus does not indicate a failure, the OpenCallbackPtr function is invoked with OpenStatus. 
    //    
    //    NOTE: This method may be called multiple times but only the first call will be recongnized.
    //
    //  Parameters:
    //      OpenStatus            - Resulting status of the open process
    //      OpenCompleteCtx       - Will be passed to OnUnsafeOpenCompleteCalled() while KAsyncContextBase is
    //                              locked during the completion of the internal open async
    //  Returns:
    //      TRUE                  - Completion accepted; else an earlier call was made
    //
    BOOLEAN
    CompleteOpen(NTSTATUS OpenStatus, void* OpenCompleteCtx = nullptr);

    //* Complete the close process started by an OnServiceClose() call.
    //
    //    The current KAsyncContextBase instance is internally completed and the CloseCallbackPtr function 
    //    (from StartClose) is invoked with CloseStatus.
    //
    //    NOTE: This method may be called multiple times but only the first call will be recongnized.
    //
    //  Parameters:
    //      CloseStatus           - Resulting status of the open process
    //      CloseCompleteCtx      - Will be passed to OnUnsafeCloseCompleteCalled() while KAsyncContextBase is
    //                              locked during the completion of the internal close async
    //  Returns:
    //      TRUE                  - Completion accepted; else an earlier call was made
    //
    BOOLEAN
    CompleteClose(NTSTATUS CloseStatus, void* CloseCompleteCtx = nullptr);

    //* Deferred Service Closure API:
    //
    //  To support scenarios where StartClose() calls can race with other operations and methods on a Service,
    //  first class support for deferred closure is provided as an option behavior. There are actually two patterns
    //  that are supported: 1) The closure process can be started in parallel to other API activity, and 2) when the
    //  internal close process must be cordinated with other racing activity.  
    //
    //  In case #1 OnServiceClose() will execute within the apt of the service, so simply using the TryAcquireActivities()
    //  and IsOpen() methods will be enough:
    //
    //      Start...()
    //      {
    //          if (TryAcquireActivities())
    //          {
    //              if (IsOpen())
    //              {
    //                  // Start the internal activity --- NOTE: That activity MUST call ReleaseActivities() when done!!!
    //                  // indicated started
    //              }
    //          }
    //          
    //          // indicate closing...
    //      }
    //
    //  In case #2, the following API (optional) can be used to defer the OnServiceClose() call into the derivation until
    //  all service activities acquired thru TryAcquireServiceActivity() have been released via ReleaseServiceActivity().
    //
    //  There are some semantics changes that must be understood when using this API. Unlike in case #1, the call to 
    //  OnServiceClose() may or may not be called within the apt of the service. If that is required, then consider using 
    //  a sub-op to transfer control back into the service's appt. Typically when using the Deferred Close facility in a
    //  consistent manner, all async activities while have ceased once OnServiceClose() is called so executing within the
    //  service's appt is not interesting.
    //
    //  What is guarenteed is that if the caller of ReleaseServiceActivity() is executing within the service's appt, then 
    //  OnServiceClose() will be called within the appt. A typical pattern is:
    //
    //      SomeOp::StartSomeOp(...)
    //      {
    //          if (_MyService->TryAcquireServiceActivity())
    //          {
    //              ... set params...
    //              Start(...);
    //              return ...started...
    //          }
    //          return ... closed indication ...
    //      }
    //
    //      SomeOp::OnCompleted()
    //      {
    //          _MyService->ReleaseServiceActivity();
    //      }
    //

    //* Enable Deferred Close API and behavior
    //
    //  NOTE: Can open be called while the service is not opened; cleared on Reuse(). Usage
    //        example:
    //
    //      SomeOp::OnServiceOpen()
    //      {
    //          SetDeferredCloseBehavior();
    //      }
    //
    VOID
    SetDeferredCloseBehavior();

    //* Try and acquire a service activity count
    //
    //  Returns: TRUE - Activity was acquired - a matching ReleaseServiceActivity() must be called.
    //
    //  NOTE: Will FailFast if SetDeferredCloseBehavior() was not called.
    //
    BOOLEAN
    TryAcquireServiceActivity();

    //* Release service activity count acquired via TryAcquireServiceActivity()
    //
    //  Returns: TRUE - Last activity was released
    //
    //  NOTE: Will FailFast if SetDeferredCloseBehavior() was not called.
    //
    BOOLEAN
    ReleaseServiceActivity();

protected:  // Derivation extension API:

    //
    //  StartOpen() was called and accepted; default: calls CompleteOpen(STATUS_SUCCESS)
    //
    virtual VOID OnServiceOpen();

    //
    //  StartClose() was called on a service instance marked as DeferredClose. This allows such a service to trigger
    //  its internal close process. The default is to do nothing. An overriding method is always called within the
    //  service's appartment.
    //
    virtual VOID OnDeferredClosing();

    //
    //  StartClose() was called and accepted; default: calls CompleteClose(STATUS_SUCCESS).  If deferred closure is not
    //  being employed, will always execute in the service's apartment.  If deferred closure is being employed, may
    //  or may not execute in the service's apartment (see: Deferred Close API)
    //
    virtual VOID OnServiceClose();

    //
    //  Reuse() was called; default: nothing 
    //
    virtual VOID OnServiceReuse();

    // These derivation extention methods will be called when the corrsponding CompleteOpen/Close is called
    virtual VOID OnUnsafeOpenCompleteCalled(void* OpenedCompleteCtx)    { UNREFERENCED_PARAMETER(OpenedCompleteCtx); }
    virtual VOID OnUnsafeCloseCompleteCalled(void* ClosedCompleteCtx)   { UNREFERENCED_PARAMETER(ClosedCompleteCtx); }

private:
    // Disallow (hide) access to normal KAsyncContextBase API subset
    using KAsyncContextBase::Start;
    using KAsyncContextBase::Cancel;
    using KAsyncContextBase::Complete;

    // Private imp
    virtual VOID OnStart() override;
    virtual VOID OnCancel() override;
    virtual VOID OnReuse() override;

    VOID
    OnOpenCallback(KAsyncContextBase* const Parent, KAsyncContextBase& OpenContext);
    
    VOID
    OnOpenContinuation(NTSTATUS ApiStatus);

    VOID
    OnCloseCallback(KAsyncContextBase* const Parent, KAsyncContextBase& CloseContext);

    VOID
    OnInternalCompletion(KAsyncContextBase* const Parent, KAsyncContextBase& This);

    VOID
    OnCloseContinuation(KAsyncContextBase* const Parent, KAsyncContextBase& This);

    VOID
    ScheduleOnServiceClose();

private:
    class OpenCloseContext;
    friend class TestService;

    #pragma warning(disable:4201)
    union DeferredCloseState
    {
        LONG volatile   _Value;
        struct
        {
            unsigned    _PendingActivities : 30;
            unsigned    _IsDeferredCloseEnabled : 1;
            unsigned    _IsClosePending : 1;
        };

        DeferredCloseState();
        DeferredCloseState(const DeferredCloseState& From);
    };
    #pragma warning(default:4201)

    DeferredCloseState              _DeferredCloseState;
    KSpinLock                       _ThisLock;
        BOOLEAN                     _IsOpen;
        BOOLEAN                     _OpenCompleted;

    KAsyncEvent                     _IsOpenedGate;
        NTSTATUS volatile           _OpenStatus;

    KSharedPtr<OpenCloseContext>    _OpenContext;
    OpenCompletionCallback          _OpenCallback;
    KSharedPtr<OpenCloseContext>    _CloseContext;
    CloseCompletionCallback         _CloseCallback;
    KAsyncContextBase*              _CloseParent;

    #if DBG
        static LONG volatile        Test_TryAcquireServiceActivitySpinCount;
        static LONG volatile        Test_ScheduleOnServiceCloseSpinCount;
        static LONG volatile        Test_ReleaseServiceActivitySpinCount;
    #endif
};


//
// Synchronizer class for service open/close
//
// This class can be used to provide synchronous semantics around
// opening and closing a service by implementing a custom service open
// and close callback tied to an event which is signalled when the
// callback is invoked.
//
// Typical usage pattern is as follows:
//
//
//        KServiceSynchronizer        sync;
//        logManager->StartOpenLogManager(nullptr,          // ParentAsync
//                                         sync.OpenCompletionCallback());
//        //
//        // Wait for log manager to close
//        //
//        sync.WaitForCompletion();
//
//        KServiceSynchronizer        sync;
//        logManager->StartCloseLogManager(nullptr,          // ParentAsync
//                                         sync.CloseCompletionCallback());
//        //
//        // Wait for log manager to close
//        //
//        sync.WaitForCompletion();
//      

class KServiceSynchronizer : public KObject<KServiceSynchronizer>
{
    K_DENY_COPY(KServiceSynchronizer);

public:

    FAILABLE
    KServiceSynchronizer(__in BOOLEAN IsManualReset = FALSE);

    virtual ~KServiceSynchronizer();

    KAsyncServiceBase::OpenCompletionCallback   OpenCompletionCallback();
    KAsyncServiceBase::CloseCompletionCallback  CloseCompletionCallback();

    NTSTATUS
    WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr);

    VOID
    Reset();

    //
    // Extension API for derivation.
    // It gets invoked when the async operation completes.
    //

    virtual VOID
    OnCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncServiceBase& CompletingOperationService);

protected:
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncServiceBase& Service,
        __in NTSTATUS Status);

    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KAsyncServiceBase::OpenCompletionCallback   _Callback;
};

#if defined(K_UseResumable)

namespace ktl
{
    namespace kservice
	{
        using ::_delete;

        //** General classes for starting and awaiting (via C++ await) on an open or close complete on a subject KASyncService
        // BUG: TODO: richhas merge into common base class.
        
        //* Service open Awaiter
        class OpenAwaiter : public KShared<OpenAwaiter>, public KObject<OpenAwaiter>
        {
            K_FORCE_SHARED(OpenAwaiter);

        public:
            // Add static factory
            static __declspec(noinline)
            NTSTATUS Create(__in KAllocator& Allocator,
                            __in ULONG Tag,
                            __in KAsyncServiceBase& Service,
                            __out OpenAwaiter::SPtr& Result,
                            __in_opt KAsyncContextBase* const Parent = nullptr,
                            __in_opt KAsyncGlobalContext* const GlobalContext = nullptr)
            {
                Result = _new(Tag, Allocator) OpenAwaiter(Service, Parent, GlobalContext);
                if (!Result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(Result->Status()))
                {
                    return (Ktl::Move(Result))->Status();
                }

                return STATUS_SUCCESS;
            }

            void Reuse(
                __in KAsyncServiceBase& Service, 
                __in_opt KAsyncContextBase* const Parent, 
                __in_opt KAsyncGlobalContext* const GlobalContext)
            {
                KInvariant(!_awaiterHandle);
                _service = &Service;
                _parentAsync = Parent;
                _globalContext = GlobalContext;
                _status = STATUS_SUCCESS;
            }

            bool await_ready() { return false; }

            NTSTATUS await_resume()
            {
                return _status;
            }

            __declspec(noinline) bool await_suspend(__in coroutine_handle<> CrHandle)
            {
                K_LOCK_BLOCK(_awaiterHandleLock)
                {
                    KInvariant(!_awaiterHandle);    // Only one co_await allowed
                    _awaiterHandle = CrHandle;
                }

                class PrivateAsyncDef : public KAsyncServiceBase
                {
                public:
                    using KAsyncServiceBase::StartOpen;
                };

                KAsyncServiceBase::OpenCompletionCallback callback;
                callback.Bind(this, &OpenAwaiter::OnCompletionCallback);

                NTSTATUS status = ((PrivateAsyncDef&)(*_service)).StartOpen(_parentAsync, callback, _globalContext);
                if (!NT_SUCCESS(status))
                {
                    // Failed open - no callback will occur - complete sync
                    _status = status;
                    _awaiterHandle = nullptr;
                    return false;
                }

                return true;    // Suspended: Callback will resume
            }

        private:
            OpenAwaiter(
                __in KAsyncServiceBase& Service, 
                __in_opt KAsyncContextBase* const Parent, 
                __in_opt KAsyncGlobalContext* const GlobalContext)
            {
                Reuse(Service, Parent, GlobalContext);
            }

            void OnCompletionCallback(
                __in_opt KAsyncContextBase* const,          // Parent; can be nullptr
                __in KAsyncServiceBase&,                    // BeginOpen()'s Service instance
                __in NTSTATUS Status)                       // Completion status of the Open
            {
                _status = Status;
                KInvariant(!_awaiterHandle.done());

                coroutine_handle<>      awaiterHandle = _awaiterHandle;
                _awaiterHandle = nullptr;
                _service = nullptr;
                _parentAsync = nullptr;
                _globalContext = nullptr;

                OpenAwaiter::SPtr thisRef = this;              // Keep alive until resume() returns
                awaiterHandle.resume();
                thisRef;
            }

        private:
            KAsyncServiceBase*      _service;
            KAsyncContextBase*      _parentAsync;
            KAsyncGlobalContext*    _globalContext;
            KSpinLock               _awaiterHandleLock;
            coroutine_handle<>      _awaiterHandle = nullptr;
            NTSTATUS                _status;
        };

        //* Service open Awaiter
        class CloseAwaiter : public KShared<CloseAwaiter>, public KObject<CloseAwaiter>
        {
            K_FORCE_SHARED(CloseAwaiter);

        public:
            // Add static factory
            static __declspec(noinline)
            NTSTATUS Create(__in KAllocator& Allocator,
                            __in ULONG Tag,
                            __in KAsyncServiceBase& Service,
                            __out CloseAwaiter::SPtr& Result,
                            __in_opt KAsyncContextBase* const Parent = nullptr,
                            __in_opt KAsyncGlobalContext* const GlobalContext = nullptr)
            {
                Result = _new(Tag, Allocator) CloseAwaiter(Service, Parent, GlobalContext);
                if (!Result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(Result->Status()))
                {
                    return (Ktl::Move(Result))->Status();
                }

                return STATUS_SUCCESS;
            }

            void Reuse(
                __in KAsyncServiceBase& Service, 
                __in_opt KAsyncContextBase* const Parent, 
                __in_opt KAsyncGlobalContext* const GlobalContext)
            {
                KInvariant(!_awaiterHandle);
                _service = &Service;
                _parentAsync = Parent;
                _globalContext = GlobalContext;
                _status = STATUS_SUCCESS;
            }

            bool await_ready() { return false; }

            NTSTATUS await_resume()
            {
                return _status;
            }

            __declspec(noinline) bool await_suspend(__in coroutine_handle<> CrHandle)
            {
                K_LOCK_BLOCK(_awaiterHandleLock)
                {
                    KInvariant(!_awaiterHandle);    // Only one co_await allowed
                    _awaiterHandle = CrHandle;
                }

                class PrivateAsyncDef : public KAsyncServiceBase
                {
                public:
                    using KAsyncServiceBase::StartClose;
                };

                KAsyncServiceBase::CloseCompletionCallback callback;
                callback.Bind(this, &CloseAwaiter::OnCompletionCallback);

                NTSTATUS status = ((PrivateAsyncDef&)(*_service)).StartClose(_parentAsync, callback);
                if (!NT_SUCCESS(status))
                {
                    // Failed open - no callback will occur - complete sync
                    _status = status;
                    _awaiterHandle = nullptr;
                    return false;
                }

                return true;    // Suspended: Callback will resume
            }

        private:
            CloseAwaiter(KAsyncServiceBase& Service, KAsyncContextBase* const Parent, KAsyncGlobalContext* const GlobalContext)
            {
                Reuse(Service, Parent, GlobalContext);
            }

            void OnCompletionCallback(
                __in_opt KAsyncContextBase* const,          // Parent; can be nullptr
                __in KAsyncServiceBase&,                    // BeginOpen()'s Service instance
                __in NTSTATUS Status)                       // Completion status of the Open
            {
                _status = Status;
                KInvariant(!_awaiterHandle.done());

                coroutine_handle<>      awaiterHandle = _awaiterHandle;
                _awaiterHandle = nullptr;
                _service = nullptr;
                _parentAsync = nullptr;
                _globalContext = nullptr;

                CloseAwaiter::SPtr thisRef = this;              // Keep alive until resume() returns
                awaiterHandle.resume();
                thisRef;
            }

        private:
            KAsyncServiceBase*      _service;
            KAsyncContextBase*      _parentAsync;
            KAsyncGlobalContext*    _globalContext;
            KSpinLock               _awaiterHandleLock;
            coroutine_handle<>      _awaiterHandle = nullptr;
            NTSTATUS                _status;
        };
    }
}

#endif


// API Entry support macros for KAsyncServiceBase derivation methods. These are used to
// make sure the life of the service is kept as stable as possible. There are two forms
// of KService API - fire and forget Tasks and async co_awaitables. There is also a 
// distinction wrt the service operating in DeferredCloseBehavior or not *HasDeferredClose). 
// If it is, then Service level activities are used to hold service closure stability (but 
// not stopping it from starting). If DeferredCloseBehavior is not used, then only KAsync
// activities are used so that the service as an Async 
// is to use

// The only code difference between coroutine and non-coroutine versions of this macro is the return expression, so share the code
#define __KService$ApiEntry(ClosedRetExp, KService$HasDeferredClose) \
    BOOLEAN  KService$EntryHasDeferredCloseBehavior = (KService$HasDeferredClose); \
	if (KService$EntryHasDeferredCloseBehavior) \
	{ \
		if (!this->TryAcquireServiceActivity()) \
		{ \
			ClosedRetExp;\
		} \
	}\
	else \
	{\
		if (!this->TryAcquireActivities())\
		{\
			ClosedRetExp;\
		}\
	}\
	KFinally([&] { if (KService$EntryHasDeferredCloseBehavior) this->ReleaseServiceActivity(); else this->ReleaseActivities(); })

// No code differences between coroutine and non-coroutine versions of this macro, so share the code
#define __KService$VoidEntry(KService$HasDeferredClose) \
	BOOLEAN  KService$EntryHasDeferredCloseBehavior = (KService$HasDeferredClose); \
	if (KService$EntryHasDeferredCloseBehavior) \
	{ \
		KInvariant(this->TryAcquireServiceActivity()); \
	}\
	else\
	{ \
		this->AcquireActivities(); \
	} \
	KFinally([&] { if (KService$EntryHasDeferredCloseBehavior) this->ReleaseServiceActivity(); else this->ReleaseActivities(); })

// Non-coroutine versions of the helper macros, defined outside K_UseResumable ifdef
#define KService$ApiEntry(KService$HasDeferredClose) __KService$ApiEntry(return K_STATUS_API_CLOSED, KService$HasDeferredClose);
#define KService$VoidEntry(KService$HasDeferredClose) __KService$VoidEntry(KService$HasDeferredClose);

// Coroutine versions
#if defined(K_UseResumable)
#define KCoService$TaskEntry(KCoService$HasDeferredClose) __KService$VoidEntry(KCoService$HasDeferredClose);
#define KCoService$ApiEntry(KCoService$HasDeferredClose) __KService$ApiEntry(co_return {(NTSTATUS)K_STATUS_API_CLOSED}, KCoService$HasDeferredClose);
#endif
