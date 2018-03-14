// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

class WorkerAsync : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(WorkerAsync);

    
    public:     
        virtual VOID StartIt(
            __in PVOID Parameters,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

    protected:
        virtual VOID FSMContinue(
            __in NTSTATUS Status
            )
        {
            UNREFERENCED_PARAMETER(Status);
            Complete(STATUS_SUCCESS);
        }

        virtual VOID OnReuse() = 0;

        virtual VOID OnCompleted()
        {
        }
        
    private:
        VOID OnStart()
        {
            _Completion.Bind(this, &WorkerAsync::OperationCompletion);
            FSMContinue(STATUS_SUCCESS);
        }

        VOID OnCancel()
        {
            FSMContinue(STATUS_CANCELLED);
        }

        VOID OperationCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
        )
        {
            UNREFERENCED_PARAMETER(ParentAsync);
            FSMContinue(Async.Status());
        }

    protected:
        KAsyncContextBase::CompletionCallback _Completion;
        
};

