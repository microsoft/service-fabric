
class KServiceSynchronizer : public KObject<KServiceSynchronizer>
{
    K_DENY_COPY(KServiceSynchronizer);

public:

    FAILABLE
    KServiceSynchronizer(__in BOOLEAN IsManualReset = FALSE);

    virtual ~KServiceSynchronizer();

    KAsyncServiceBase::OpenCompletionCallback	OpenCompletionCallback();
    KAsyncServiceBase::CloseCompletionCallback	CloseCompletionCallback();

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

