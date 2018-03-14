// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Transport;

// ********************************************************************************************************************
// ComProxyFabricTransportCallbackMessageHandler Implementation
//
ComProxyFabricTransportCallbackMessageHandler::ComProxyFabricTransportCallbackMessageHandler(
    ComPointer<IFabricTransportCallbackMessageHandler> const & comImpl)
    : IServiceCommunicationMessageHandler()
    , comImpl_(comImpl)
{
}


ComProxyFabricTransportCallbackMessageHandler::~ComProxyFabricTransportCallbackMessageHandler()
{
}
AsyncOperationSPtr ComProxyFabricTransportCallbackMessageHandler::BeginProcessRequest(
    wstring const &,
    MessageUPtr &&,
    TimeSpan const &,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    Assert::TestAssert("Not implemented");
    return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(ErrorCodeValue::NotImplemented, callback, parent);
}

ErrorCode ComProxyFabricTransportCallbackMessageHandler::EndProcessRequest(
    AsyncOperationSPtr const & asyncOperation,
    MessageUPtr &)
{
    Assert::TestAssert("Not implemented");
    return AsyncOperation::End<CompletedAsyncOperation>(asyncOperation)->Error;
}

ErrorCode ComProxyFabricTransportCallbackMessageHandler::HandleOneWay(
    wstring const & ,
    MessageUPtr && message)
{
    ComPointer<IFabricTransportMessage> msgCPointer = make_com<ComFabricTransportMessage,
        IFabricTransportMessage>(move(message));

    auto hr = comImpl_->HandleOneWay(
        msgCPointer.GetRawPointer());

    return ErrorCode::FromHResult(hr);

}
