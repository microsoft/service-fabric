// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AsyncOperationRoot;
using Common::AsyncOperationSPtr;
using Common::ComAsyncOperationContext;
using Common::ComAsyncOperationContextCPtr;
using Common::ComPointer;
using Common::ErrorCode;
using Common::make_com;
using Common::ComUtility;

ComOperationDataAsyncEnumerator::ComOperationDataAsyncEnumerator(
    Common::ComponentRoot const & root,
    DispatchQueueSPtr const & dispatchQueue)
    :   IFabricOperationDataStream(),
        Common::ComUnknownBase(),
        rootSPtr_(root.shared_from_this()),
        dispatchQueue_(dispatchQueue),
        errorsEncountered_(false),
        closeCalled_(false)
{
}

ComOperationDataAsyncEnumerator::~ComOperationDataAsyncEnumerator()
{
}

// {EA50D5A4-A150-4FE2-B667-DAADC13A199C}
static const GUID CLSID_ComOperationDataAsyncEnumerator_GetNextOperation = 
    { 0xea50d5a4, 0xa150, 0x4fe2, { 0xb6, 0x67, 0xda, 0xad, 0xc1, 0x3a, 0x19, 0x9c } };
class ComOperationDataAsyncEnumerator::GetNextOperation : public ComAsyncOperationContext
{
    DENY_COPY(GetNextOperation)

    COM_INTERFACE_LIST2(
        GetNextOperation,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_ComOperationDataAsyncEnumerator_GetNextOperation,
        GetNextOperation)
public:
    explicit GetNextOperation(ComOperationDataAsyncEnumerator const & enumerator)
        :   enumerator_(enumerator),
            operationData_(),
            queue_()
    {
    }

    virtual ~GetNextOperation() {}

    HRESULT Initialize(
        __in IFabricAsyncOperationCallback * inCallback)
    {
        {
            Common::AcquireExclusiveLock lock(this->enumerator_.lock_);

            if (this->enumerator_.errorsEncountered_ || this->enumerator_.closeCalled_)
            {
                // The copy process encountered an error;
                // let the state provider know by completing with error
                return ComUtility::OnPublicApiReturn(E_ABORT);
            }
        }

        HRESULT hr = this->ComAsyncOperationContext::Initialize(enumerator_.rootSPtr_, inCallback);
        return ComUtility::OnPublicApiReturn(hr);
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context, 
        __out IFabricOperationData ** operationData)
    {
        if (context == NULL || operationData == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }
        
        ComPointer<GetNextOperation> asyncOperation(context, CLSID_ComOperationDataAsyncEnumerator_GetNextOperation);
        HRESULT hr = asyncOperation->ComAsyncOperationContext::End();
        if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }
        *operationData = asyncOperation->operationData_.DetachNoRelease();
        return ComUtility::OnPublicApiReturn(asyncOperation->Result);
    }

protected:
    virtual void OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        {
            Common::AcquireExclusiveLock lock(this->enumerator_.lock_);

            if (this->enumerator_.errorsEncountered_ || this->enumerator_.closeCalled_)
            {
                // The copy encountered an error, so
                // let the state provider know by cancelling this operation
                this->TryComplete(proxySPtr, E_ABORT);

                return;
            }

            // if we can continue, grab the queue
            this->queue_ = this->enumerator_.dispatchQueue_;
        }

        auto inner = this->queue_->BeginDequeue(
            Common::TimeSpan::MaxValue, 
            [this](AsyncOperationSPtr const & asyncOperation) { this->DequeueCallback(asyncOperation); },
            proxySPtr);

        if (inner->CompletedSynchronously)
        {
            this->FinishDequeue(inner);
        }
    }

private:

    void DequeueCallback(AsyncOperationSPtr const & asyncOperation)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            this->FinishDequeue(asyncOperation);
        }
    }

    void FinishDequeue(AsyncOperationSPtr const & asyncOperation)
    {
        std::unique_ptr<ComOperationCPtr> operation;
        auto error = this->queue_->EndDequeue(asyncOperation, operation);

        if (error.IsSuccess())
        {
            if (enumerator_.errorsEncountered_)
            {
                this->TryComplete(asyncOperation->Parent, E_ABORT);

                return;
            }

            if (operation)
            {
                ComOperationCPtr & opPtr = *(operation.get());
                operationData_ = make_com<ComOperationData,IFabricOperationData>(std::move(opPtr));
            }            
        }

        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    ComOperationDataAsyncEnumerator const & enumerator_;
    ComPointer<IFabricOperationData> operationData_;
    DispatchQueueSPtr queue_;
};
            
HRESULT STDMETHODCALLTYPE ComOperationDataAsyncEnumerator::BeginGetNext(
    /*[in]*/ IFabricAsyncOperationCallback *callback,
    /*[out, retval]*/ IFabricAsyncOperationContext **context)
{
    if (context == NULL) { return ComUtility::OnPublicApiReturn(E_INVALIDARG); }

    ComPointer<GetNextOperation> getOperation = make_com<GetNextOperation>(*this);

    HRESULT hr = getOperation->Initialize(callback);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(getOperation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

    *context = baseOperation.DetachNoRelease();
    return ComUtility::OnPublicApiReturn(S_OK);
}

HRESULT STDMETHODCALLTYPE ComOperationDataAsyncEnumerator::EndGetNext(
    /*[in]*/ IFabricAsyncOperationContext *context,
    /*[out, retval]*/ IFabricOperationData ** operationData)
{
    if ((context == NULL) || (operationData == NULL)) { return ComUtility::OnPublicApiReturn(E_INVALIDARG); }

    return ComUtility::OnPublicApiReturn(GetNextOperation::End(context, operationData));
}

void ComOperationDataAsyncEnumerator::Close(bool errorsEncountered)
{
    Common::AcquireExclusiveLock lock(this->lock_);

    this->errorsEncountered_ = errorsEncountered;
    this->closeCalled_ = true;

    this->rootSPtr_.reset();
}

} // end namespace ReplicationComponent
} // end namespace Reliability
