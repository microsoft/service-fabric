// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyServiceConnectionHandler  Implementation
//
class ComProxyServiceConnectionHandler::DisconnectAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(DisconnectAsyncOperation)

public:
    DisconnectAsyncOperation(
        __in IFabricServiceConnectionHandler & comImpl,
        wstring clientId,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
        , clientId_(clientId)
    {
    }

    virtual ~DisconnectAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DisconnectAsyncOperation>(operation);

        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        return comImpl_.BeginProcessDisconnect(
            clientId_.c_str(),
            GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        auto hr = comImpl_.EndProcessDisconnect(context);

        return hr;
    }


    IFabricServiceConnectionHandler & comImpl_;
    wstring clientId_;
    TimeSpan const timeout_;
};

class ComProxyServiceConnectionHandler::ConnectAsyncOperation : public ComProxyAsyncOperation
{
    DENY_COPY(ConnectAsyncOperation)

public:
    ConnectAsyncOperation(
        __in IFabricServiceConnectionHandler & comImpl,
        IClientConnectionPtr clientConnection,
        TimeSpan const & timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , comImpl_(comImpl)
        , timeout_(timeout)
        , clientConnection_(clientConnection)
    {
    }

    virtual ~ConnectAsyncOperation() {}

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ConnectAsyncOperation>(operation);

        return thisPtr->Error;
    }

protected:
    DWORD GetDWORDTimeout() { return static_cast<DWORD>(timeout_.TotalMilliseconds()); }

    HRESULT BeginComAsyncOperation(IFabricAsyncOperationCallback * callback, IFabricAsyncOperationContext ** context)
    {
        ComPointer<IFabricClientConnection> clientConnectionCPTr = make_com<ComFabricClientConnection,IFabricClientConnection>(clientConnection_);
        return comImpl_.BeginProcessConnect(
            clientConnectionCPTr.GetRawPointer(),
            GetDWORDTimeout(),
            callback,
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        auto hr = comImpl_.EndProcessConnect(context);
     
        return hr;
    }


    IFabricServiceConnectionHandler & comImpl_;
    IClientConnectionPtr clientConnection_;
    TimeSpan const timeout_;
};
ComProxyServiceConnectionHandler::ComProxyServiceConnectionHandler(
    ComPointer<IFabricServiceConnectionHandler > const & comImpl)
    : ComponentRoot(),
    IServiceConnectionHandler(),
    comImpl_(comImpl)
{
}

ComProxyServiceConnectionHandler::~ComProxyServiceConnectionHandler()
{
}

AsyncOperationSPtr ComProxyServiceConnectionHandler::BeginProcessConnect(
    IClientConnectionPtr clientConnection,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return  AsyncOperation::CreateAndStart<ConnectAsyncOperation>(
        *comImpl_.GetRawPointer(),
        clientConnection,
        timeout,
        callback,
        parent);
}

ErrorCode ComProxyServiceConnectionHandler::EndProcessConnect(
    AsyncOperationSPtr const &  operation)
{
    return ConnectAsyncOperation::End(operation);

}

AsyncOperationSPtr ComProxyServiceConnectionHandler::BeginProcessDisconnect(
    std::wstring const & clientId,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return  AsyncOperation::CreateAndStart<DisconnectAsyncOperation>(
        *comImpl_.GetRawPointer(),
        clientId,
        timeout,
        callback,
        parent);
}

ErrorCode ComProxyServiceConnectionHandler::EndProcessDisconnect(
    AsyncOperationSPtr const &  operation)
{
    return DisconnectAsyncOperation::End(operation);

}

