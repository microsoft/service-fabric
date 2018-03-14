// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::ComPointer;
using Common::ErrorCode;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using std::move;

ComProxyStateProvider::ComProxyStateProvider(ComPointer<IFabricStateProvider> && stateProvider)
    : stateProvider_(std::move(stateProvider))
{
    ASSERT_IF(stateProvider_.GetRawPointer() == nullptr,
        "State provider should have a valid pointer");

    auto casted = Common::ComPointer<::IFabricStateProviderSupportsCopyUntilLatestLsn>(stateProvider_, ::IID_IFabricStateProviderSupportsCopyUntilLatestLsn);
    supportsCopyUntilLatestLsn_ = (casted.GetRawPointer() != nullptr);
}

ComProxyStateProvider::ComProxyStateProvider(ComProxyStateProvider && other)
    : stateProvider_(std::move(other.stateProvider_))
    , supportsCopyUntilLatestLsn_(other.supportsCopyUntilLatestLsn_)
{
}

ComProxyStateProvider & ComProxyStateProvider ::operator=(ComProxyStateProvider const & other)
{
    if (this != &other)
    {
        stateProvider_ = other.stateProvider_;
        supportsCopyUntilLatestLsn_ = other.supportsCopyUntilLatestLsn_;
    }

    return *this;
}

ComProxyStateProvider::ComProxyStateProvider(ComProxyStateProvider const & other)
    : stateProvider_(other.stateProvider_)
    , supportsCopyUntilLatestLsn_(other.supportsCopyUntilLatestLsn_)
{
}

ComProxyStateProvider & ComProxyStateProvider ::operator=(ComProxyStateProvider && other)
{
    if (this != &other)
    {
        stateProvider_ = std::move(other.stateProvider_);
        supportsCopyUntilLatestLsn_ = other.supportsCopyUntilLatestLsn_;
    }

    return *this;
}

Common::ErrorCode ComProxyStateProvider::GetLastCommittedSequenceNumber( 
    __out FABRIC_SEQUENCE_NUMBER & sequenceNumber) const
{
    HRESULT result = stateProvider_->GetLastCommittedSequenceNumber(&sequenceNumber);
    if (SUCCEEDED(result))
    {
        // Any negative value and 0 means the state provider has no data
        if (sequenceNumber < Constants::InvalidLSN)
        {
            sequenceNumber = Constants::InvalidLSN;
        }
    }
            
    return ErrorCode::FromHResult(result);
}

class ComProxyStateProvider::UpdateEpochAsyncOperation : 
    public Common::ComProxyAsyncOperation
{
public:
    UpdateEpochAsyncOperation(
        ComProxyStateProvider const & parent,
        FABRIC_EPOCH const & epoch,
        FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & state)
        :   ComProxyAsyncOperation(callback, state, true),
            parent_(parent),
            epoch_(epoch),
            previousEpochLastSequenceNumber_(previousEpochLastSequenceNumber)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<UpdateEpochAsyncOperation>(asyncOperation);
        return casted->Error;
    }

protected:

    virtual void Start(AsyncOperationSPtr const &)
    {
    }

    virtual HRESULT BeginComAsyncOperation(
        __in IFabricAsyncOperationCallback * callback, 
        __out IFabricAsyncOperationContext ** context)
    {
        return parent_.stateProvider_->BeginUpdateEpoch(&epoch_, previousEpochLastSequenceNumber_, callback, context);
    }
     
    virtual HRESULT EndComAsyncOperation(
        __in IFabricAsyncOperationContext * context)
    {
        HRESULT hr = parent_.stateProvider_->EndUpdateEpoch(
            context);
        if (FAILED(hr)) { return hr; }
        return S_OK;
    }

private:
    ComProxyStateProvider const & parent_;
    FABRIC_EPOCH const & epoch_;
    FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber_;
};

Common::AsyncOperationSPtr ComProxyStateProvider::BeginUpdateEpoch( 
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & state) const
{
    return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
        *this,
        epoch,
        previousEpochLastSequenceNumber,
        callback,
        state);
}

Common::ErrorCode ComProxyStateProvider::EndUpdateEpoch( 
    Common::AsyncOperationSPtr const & asyncOperation) const
{
    return UpdateEpochAsyncOperation::End(asyncOperation);
}

Common::ErrorCode ComProxyStateProvider::GetCopyContext(
    __out ComProxyAsyncEnumOperationData & asyncEnumOperationData) const
{
    ASSERT_IF(stateProvider_.GetRawPointer() == nullptr, "State provider should have a valid pointer");
    Common::ComPointer<IFabricOperationDataStream> enumerator;
    HRESULT result = stateProvider_->GetCopyContext(
        enumerator.InitializationAddress());
    if (SUCCEEDED(result))
    {
        // If the returned enumerator is NULL, do not set the out parameter
        // (the default empty async enumerator proxy is used)
        if (enumerator.GetRawPointer())
        {
            asyncEnumOperationData = ComProxyAsyncEnumOperationData(move(enumerator));
        }
    }
            
    return ErrorCode::FromHResult(result);
}

Common::ErrorCode ComProxyStateProvider::GetCopyOperations(
    ComPointer<IFabricOperationDataStream> && context, 
    FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    __out ComProxyAsyncEnumOperationData & asyncEnumOperationData) const
{
    ASSERT_IF(stateProvider_.GetRawPointer() == nullptr, "State provider should have a valid pointer");
    Common::ComPointer<IFabricOperationDataStream> enumerator;

    HRESULT result = stateProvider_->GetCopyState(
        uptoSequenceNumber,
        context.GetRawPointer(),
        enumerator.InitializationAddress());
            
    if (SUCCEEDED(result))
    {
        // If the returned enumerator is NULL, do not set the out parameter
        // (the default empty async enumerator proxy is used)
        if (enumerator.GetRawPointer())
        {
            asyncEnumOperationData = ComProxyAsyncEnumOperationData(move(enumerator));
        }
    }
    
    if (result == E_POINTER)
    {
        Common::TraceWarning(
            Common::TraceTaskCodes::Replication,
            "ComProxyStateProvider",            
            "E_POINTER returned from stateProvider_->GetCopyState");
    }

    return ErrorCode::FromHResult(result);
}

class ComProxyStateProvider::OnDataLossAsyncOperation : 
    public Common::ComProxyAsyncOperation
{
public:
    OnDataLossAsyncOperation(
        ComProxyStateProvider const & parent,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & state)
        :   ComProxyAsyncOperation(callback, state, true),
            parent_(parent),
            isStateChanged_(false)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation, 
        __out BOOLEAN & isStateChanged)
    {
        auto casted = AsyncOperation::End<OnDataLossAsyncOperation>(asyncOperation);
        isStateChanged = casted->isStateChanged_;
        return casted->Error;
    }

protected:

    virtual void Start(AsyncOperationSPtr const &)
    {
    }

    virtual HRESULT BeginComAsyncOperation(
        __in IFabricAsyncOperationCallback * callback, 
        __out IFabricAsyncOperationContext ** context)
    {
        return parent_.stateProvider_->BeginOnDataLoss(callback, context);
    }
     
    virtual HRESULT EndComAsyncOperation(
        __in IFabricAsyncOperationContext * context)
    {
        HRESULT hr = parent_.stateProvider_->EndOnDataLoss(
            context,
            &isStateChanged_);
        if (FAILED(hr)) { return hr; }
        return S_OK;
    }

private:
    ComProxyStateProvider const & parent_;
    BOOLEAN isStateChanged_;
};

Common::AsyncOperationSPtr ComProxyStateProvider::BeginOnDataLoss(
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & state) const
{
    return AsyncOperation::CreateAndStart<OnDataLossAsyncOperation>(
        *this, 
        callback, 
        state);
}

Common::ErrorCode ComProxyStateProvider::EndOnDataLoss(
    Common::AsyncOperationSPtr const & asyncOperation,
    __out BOOLEAN & isStateChanged) const
{
    return OnDataLossAsyncOperation::End(asyncOperation, isStateChanged);
}

} // end namespace ReplicationComponent
} // end namespace Reliability
