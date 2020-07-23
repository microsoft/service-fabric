// TODO: Separate into .h and .c


const LONGLONG DefaultTestLogFileSize = (LONGLONG)1024*1024*1024;
const LONGLONG DEFAULT_STREAM_SIZE = (LONGLONG)256*1024*1024;
const ULONG DEFAULT_MAX_RECORD_SIZE = (ULONG)16*1024*1024;

class StreamCloseSynchronizer : public KObject<StreamCloseSynchronizer>
{
    K_DENY_COPY(StreamCloseSynchronizer);

public:

    FAILABLE
    StreamCloseSynchronizer(__in BOOLEAN IsManualReset = FALSE)
        :   _Event(IsManualReset, FALSE),
            _CompletionStatus(STATUS_SUCCESS)
    {
        _Callback.Bind(this, &StreamCloseSynchronizer::AsyncCompletion);
        SetConstructorStatus(_Event.Status());
    }

    ~StreamCloseSynchronizer()
    {
    }

    KtlLogStream::CloseCompletionCallback CloseCompletionCallback()
    {
        return(_Callback);
    }

    NTSTATUS
    WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr)
    {
        KInvariant(! KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());

        BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
        if (!b)
        {
            if (IsCompleted)
            {
                *IsCompleted = FALSE;
            }

            return STATUS_IO_TIMEOUT;
        }
        else
        {        
            if (IsCompleted)
            {
                *IsCompleted = TRUE;
            }

            return _CompletionStatus;
        }
    }
            
    
    VOID
    Reset()
    {
        _Event.ResetEvent();
        _CompletionStatus = STATUS_SUCCESS;
    }
            

protected:
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogStream& LogStream,
        __in NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(LogStream);

        _CompletionStatus = Status;

        _Event.SetEvent();
    }    
                
private:
    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KtlLogStream::CloseCompletionCallback       _Callback;
};


class ContainerCloseSynchronizer : public KObject<ContainerCloseSynchronizer>
{
    K_DENY_COPY(ContainerCloseSynchronizer);

public:

    FAILABLE
    ContainerCloseSynchronizer(__in BOOLEAN IsManualReset = FALSE)
        :   _Event(IsManualReset, FALSE),
            _CompletionStatus(STATUS_SUCCESS)
    {
        _Callback.Bind(this, &ContainerCloseSynchronizer::AsyncCompletion);
        SetConstructorStatus(_Event.Status());
    }

    ~ContainerCloseSynchronizer()
    {
    }

    KtlLogContainer::CloseCompletionCallback CloseCompletionCallback()
    {
        return(_Callback);
    }

    NTSTATUS
    WaitForCompletion(
        __in_opt ULONG TimeoutInMilliseconds = KEvent::Infinite,
        __out_opt PBOOLEAN IsCompleted = nullptr)
    {
        KInvariant(! KtlSystem::GetDefaultKtlSystem().DefaultThreadPool().IsCurrentThreadOwned());

        BOOLEAN b = _Event.WaitUntilSet(TimeoutInMilliseconds);
        if (!b)
        {
            if (IsCompleted)
            {
                *IsCompleted = FALSE;
            }

            return STATUS_IO_TIMEOUT;
        }
        else
        {        
            if (IsCompleted)
            {
                *IsCompleted = TRUE;
            }

            return _CompletionStatus;
        }
    }
            
    
    VOID
    Reset()
    {
        _Event.ResetEvent();
        _CompletionStatus = STATUS_SUCCESS;
    }
            

protected:
    VOID
    AsyncCompletion(
        __in_opt KAsyncContextBase* const Parent,
        __in KtlLogContainer& LogContainer,
        __in NTSTATUS Status)
    {
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(LogContainer);

        _CompletionStatus = Status;

        _Event.SetEvent();
    }    
                
private:
    KEvent                                      _Event;
    NTSTATUS                                    _CompletionStatus;
    KtlLogContainer::CloseCompletionCallback       _Callback;
};
