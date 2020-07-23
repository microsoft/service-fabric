// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace ServiceModel;
using namespace Management::GatewayResourceManager;

StringLiteral const TraceComponent("ComFabricClient");

// ********************************************************************************************************************
// ComProxyAsyncOperation Classes
//

class ComProxyGatewayResourceManager::CreateGatewayResourceAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(CreateGatewayResourceAsyncOperation)

public:
    CreateGatewayResourceAsyncOperation(
        __in IFabricGatewayResourceManager & comImpl,
        wstring const & resourceDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , resourceDescription_(resourceDescription)
        , timeout_(timeout)
    {
    }

    virtual ~CreateGatewayResourceAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricStringResult ** reply)
    {
        auto casted = AsyncOperation::End<CreateGatewayResourceAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result_);
        }
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginCreateOrUpdateGatewayResource(
            resourceDescription_.c_str(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndCreateOrUpdateGatewayResource(context, &result_);
    }

    IFabricGatewayResourceManager & comImpl_;
    wstring const resourceDescription_;
    IFabricStringResult * result_;
    TimeSpan const timeout_;
};

class ComProxyGatewayResourceManager::DeleteGatewayResourceAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(DeleteGatewayResourceAsyncOperation)

public:
    DeleteGatewayResourceAsyncOperation(
        __in IFabricGatewayResourceManager & comImpl,
        wstring const & resourceName,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , resourceName_(resourceName)
        , timeout_(timeout)
    {
    }

    virtual ~DeleteGatewayResourceAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto casted = AsyncOperation::End<DeleteGatewayResourceAsyncOperation>(operation);
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginDeleteGatewayResource(
            resourceName_.c_str(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndDeleteGatewayResource(context);
    }

    IFabricGatewayResourceManager & comImpl_;
    wstring const resourceName_;
    TimeSpan const timeout_;
};

class ComProxyGatewayResourceManager::ProcessQueryAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ProcessQueryAsyncOperation)

public:
    ProcessQueryAsyncOperation(
        __in IFabricGatewayResourceManager & comImpl,
        wstring const & queryDescription,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , queryDescription_(queryDescription)
        , comImpl_(comImpl)
        , timeout_(timeout)
    {
    }

    virtual ~ProcessQueryAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation, IFabricStringListResult ** reply)
    {
        auto casted = AsyncOperation::End<ProcessQueryAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            *reply = move(casted->result_);
        }
        return casted->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginGetGatewayResourceList(
            queryDescription_.c_str(),
            this->GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return comImpl_.EndGetGatewayResourceList(context, &result_);
    }

    IFabricGatewayResourceManager & comImpl_;
    wstring const queryDescription_;
    IFabricStringListResult * result_;
    TimeSpan const timeout_;
};

// ********************************************************************************************************************
// ComProxyGatewayResourceManager Implementation
//
ComProxyGatewayResourceManager::ComProxyGatewayResourceManager(
    ComPointer<IFabricGatewayResourceManager> const & comImpl)
    : IGatewayResourceManager()
    , ComProxySystemServiceBase<IFabricGatewayResourceManager>(comImpl)
{
}

ComProxyGatewayResourceManager::~ComProxyGatewayResourceManager()
{
}

AsyncOperationSPtr ComProxyGatewayResourceManager::BeginCreateOrUpdateGatewayResource(
    wstring const & resourceDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<CreateGatewayResourceAsyncOperation>(
        *comImpl_.GetRawPointer(),
        resourceDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyGatewayResourceManager::EndCreateOrUpdateGatewayResource(
    AsyncOperationSPtr const & asyncOperation,
    IFabricStringResult ** resourceDescription
)
{
    return CreateGatewayResourceAsyncOperation::End(asyncOperation, resourceDescription);
}

AsyncOperationSPtr ComProxyGatewayResourceManager::BeginGetGatewayResourceList(
    wstring const & queryDescription,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
        *comImpl_.GetRawPointer(),
        queryDescription,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyGatewayResourceManager::EndGetGatewayResourceList(
    AsyncOperationSPtr const & asyncOperation,
    IFabricStringListResult ** queryResult)
{
    return ProcessQueryAsyncOperation::End(asyncOperation, queryResult);
}


AsyncOperationSPtr ComProxyGatewayResourceManager::BeginDeleteGatewayResource(
    std::wstring const & resourceName,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<DeleteGatewayResourceAsyncOperation>(
        *comImpl_.GetRawPointer(),
        resourceName,
        timeout,
        callback,
        parent);

    return operation;
}

ErrorCode ComProxyGatewayResourceManager::EndDeleteGatewayResource(
    AsyncOperationSPtr const & asyncOperation)
{
    return DeleteGatewayResourceAsyncOperation::End(asyncOperation);
}