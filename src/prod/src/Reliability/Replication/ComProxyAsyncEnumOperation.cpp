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
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::AsyncCallback;

ComProxyAsyncEnumOperationData::ComProxyAsyncEnumOperationData()
    : asyncEnumOperationData_()
{
}

ComProxyAsyncEnumOperationData::ComProxyAsyncEnumOperationData(
    ComPointer<IFabricOperationDataStream> && asyncEnumOperation)
    : asyncEnumOperationData_(std::move(asyncEnumOperation))
{
    ASSERT_IF(
        asyncEnumOperationData_.GetRawPointer() == nullptr,
        "ComProxyAsyncEnumOperationData.ctor: Async enumerator should have a valid pointer");
}

ComProxyAsyncEnumOperationData::ComProxyAsyncEnumOperationData(
    ComProxyAsyncEnumOperationData && other)
    : asyncEnumOperationData_(std::move(other.asyncEnumOperationData_))
{
}

ComProxyAsyncEnumOperationData & ComProxyAsyncEnumOperationData::operator=(
    ComProxyAsyncEnumOperationData && other)
{
    if (this != &other)
    {
        asyncEnumOperationData_ = std::move(other.asyncEnumOperationData_);
    }

    return *this;
}

ComProxyAsyncEnumOperationData::~ComProxyAsyncEnumOperationData() 
{
    if (asyncEnumOperationData_.GetRawPointer() == nullptr)
    {
        return;
    }

    ComPointer<IFabricDisposable> disposable(asyncEnumOperationData_, __uuidof(IFabricDisposable));
    if (disposable.GetRawPointer() == nullptr)
    {
        // type does not implement IFabricDisposable
        return;
    }

    disposable.GetRawPointer()->Dispose();
}

class ComProxyAsyncEnumOperationData::GetNextAsyncOperation : 
    public Common::ComProxyAsyncOperation
{
public:
    GetNextAsyncOperation(
        __in ComProxyAsyncEnumOperationData & enumerator,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        :   ComProxyAsyncOperation(callback, parent, true),
            enumerator_(enumerator),
            isLast_(false),
            opPointer_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation, 
        __out bool & isLast, 
        __out ComPointer<IFabricOperationData> & operation)
    {
        auto casted = AsyncOperation::End<GetNextAsyncOperation>(asyncOperation);
        if (casted->Error.IsSuccess())
        {
            isLast = casted->isLast_;
            std::swap(operation, casted->opPointer_);
        }
        return casted->Error;
    }

protected:

    void Start(AsyncOperationSPtr const &)
    {
    }

    virtual HRESULT BeginComAsyncOperation(
        __in IFabricAsyncOperationCallback * callback, 
        __out IFabricAsyncOperationContext ** context)
    {
        return enumerator_.asyncEnumOperationData_->BeginGetNext(callback, context);
    }
     
    virtual HRESULT EndComAsyncOperation(
        __in IFabricAsyncOperationContext * context)
    {
        HRESULT hr = enumerator_.asyncEnumOperationData_->EndGetNext(
            context, 
            opPointer_.InitializationAddress());
        if (FAILED(hr)) { return hr; }
        
        if (opPointer_.GetRawPointer() == nullptr)
        {
            isLast_ = true;
        }
        else
        {
            isLast_ = false;
        }
                
        return S_OK;
    }
    
private:
    ComProxyAsyncEnumOperationData & enumerator_;
    bool isLast_;
    ComPointer<IFabricOperationData> opPointer_;
};

class ComProxyAsyncEnumOperationData::GetNextEmptyAsyncOperation : 
    public Common::AsyncOperation
{
    DENY_COPY(GetNextEmptyAsyncOperation)

public:
    GetNextEmptyAsyncOperation(
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & state) 
        :   AsyncOperation(callback, state, true)
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation, 
        __out bool & isLast)
    {
        auto casted = AsyncOperation::End<GetNextEmptyAsyncOperation>(asyncOperation);
        isLast = true;
        return casted->Error;
    }

protected:
    void OnStart(Common::AsyncOperationSPtr const & thisSPtr)
    {
        TryComplete(thisSPtr, ErrorCode(Common::ErrorCodeValue::Success));
    }
};
 
Common::AsyncOperationSPtr ComProxyAsyncEnumOperationData::BeginGetNext(
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & state)
{
    if (asyncEnumOperationData_.GetRawPointer())
    {
        return AsyncOperation::CreateAndStart<GetNextAsyncOperation>(
            *this, 
            callback, 
            state);
    }
    else
    {
        return AsyncOperation::CreateAndStart<GetNextEmptyAsyncOperation>(
            callback, 
            state);
    }
}

Common::ErrorCode ComProxyAsyncEnumOperationData::EndGetNext(
    Common::AsyncOperationSPtr const & asyncOperation,
    __out bool & isLast,
    __out ComPointer<IFabricOperationData> & operation)
{
    if (asyncEnumOperationData_.GetRawPointer())
    {
        return GetNextAsyncOperation::End(asyncOperation, isLast, operation);
    }
    else
    {
        // Leave the operation unset
        return GetNextEmptyAsyncOperation::End(asyncOperation, isLast);
    }
}

} // end namespace ReplicationComponent
} // end namespace Reliability
