// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Storage;
using namespace Storage::Api;

class FaultInjectionAdapter::FaultInjectionAsyncOperation : public Common::AsyncOperation
{
public:
    FaultInjectionAsyncOperation(
        bool isAsync,
        ErrorCodeValue::Enum error,
        IKeyValueStoreSPtr const & innerStore,
        OperationType::Enum operationType,
        RowIdentifier const & id,
        RowData && bytes,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent) :
        isAsync_(isAsync),
        result_(error),
        innerStore_(innerStore),
        operationType_(operationType),
        id_(id),
        bytes_(move(bytes)),
        timeout_(timeout),
        AsyncOperation(callback, parent)
    {
    }

public:
    void FinishAsync(AsyncOperationSPtr const & thisSPtr)
    {
        ASSERT_IF(!isAsync_, "Trying to finish non async");
        Finish(thisSPtr);
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr) override
    {
        // Call the store if fault is not to be induced
        if (result_ != ErrorCodeValue::Success)
        {
            if (!isAsync_)
            {
                Finish(thisSPtr);
            }

            return;
        }

        innerStore_->BeginStoreOperation(
            operationType_,
            id_,
            std::move(bytes_),
            timeout_,
            [this](AsyncOperationSPtr op)
            {
                auto result = innerStore_->EndStoreOperation(op);

                // If the adapter is in async mode
                // then thisSPtr must be completed by explicitly
                // calling finish async
                // save the error so that it is returned correctly
                result_ = result.ReadValue();
                if (!isAsync_)
                {
                    Finish(op->Parent);
                }
                else
                {
                    ASSERT_IF(IsCompleted, "store fault injection op in async mode completed prior to inner store completing");
                }
            },
            thisSPtr);
    }

private:
    void Finish(AsyncOperationSPtr const & thisSPtr)
    {
        bool result = TryComplete(thisSPtr, result_);
        ASSERT_IF(!result, "Finish must successfully complete fault injected op");        
    }

    bool isAsync_;
    ErrorCodeValue::Enum result_;
    IKeyValueStoreSPtr innerStore_;
    OperationType::Enum operationType_;
    RowIdentifier id_;
    RowData bytes_;
    TimeSpan timeout_;
};

FaultInjectionAdapter::FaultInjectionAdapter(IKeyValueStoreSPtr const & inner) :
    inner_(inner),
    faultError_(ErrorCodeValue::Success),
    isAsync_(false)
{
    ASSERT_IF(inner == nullptr, "fault injection adapter, inner cant be null");
}

void FaultInjectionAdapter::EnableFaultInjection(ErrorCodeValue::Enum err)
{
    AcquireExclusiveLock grab(lock_);
    faultError_ = err;
}

void FaultInjectionAdapter::DisableFaultInjection()
{
    AcquireExclusiveLock grab(lock_);
    faultError_ = ErrorCodeValue::Success;
}

size_t FaultInjectionAdapter::FinishPendingCommits()
{
    std::vector<Common::AsyncOperationSPtr> snap;

    {
        AcquireExclusiveLock grab(lock_);
        swap(snap, pendingCommits_);
    }

    for (auto it : snap)
    {
        auto casted = static_pointer_cast<FaultInjectionAsyncOperation>(it);
        casted->FinishAsync(it);
    }

    return snap.size();
}

ErrorCode FaultInjectionAdapter::Enumerate(
    RowType::Enum type,
    __out std::vector<Row>&  rows)
{
    return inner_->Enumerate(type, rows);
}

AsyncOperationSPtr FaultInjectionAdapter::BeginStoreOperation(
    OperationType::Enum operationType,
    RowIdentifier const & id,
    RowData && bytes,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    AcquireExclusiveLock grab(lock_);
        
    auto op = AsyncOperation::CreateAndStart<FaultInjectionAsyncOperation>(
        isAsync_, 
        faultError_, 
        inner_, 
        operationType, 
        id, 
        move(bytes), 
        timeout, 
        callback, 
        parent);

    if (isAsync_)
    {
        pendingCommits_.push_back(op);
    }

    return op;
}

ErrorCode FaultInjectionAdapter::EndStoreOperation(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End(operation);
}

void FaultInjectionAdapter::Close()
{
    inner_->Close();
}

