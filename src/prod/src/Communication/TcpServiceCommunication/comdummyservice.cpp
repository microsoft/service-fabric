// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComDummyservice.h"
#include "api/wrappers/ApiWrappers.h"

using namespace Common;
using namespace std;
using  namespace Communication::TcpServiceCommunication;
using namespace Api;

ComDummyService::ComDummyService(ComPointer<IFabricServiceCommunicationMessage>  reply)
{
    this->reply_ = reply;
}


HRESULT ComDummyService::BeginProcessRequest(
    LPCWSTR clientId,
    IFabricServiceCommunicationMessage *message,
    DWORD timeoutMilliseconds,
    IFabricAsyncOperationCallback * callback,
    IFabricAsyncOperationContext ** context)
{
	UNREFERENCED_PARAMETER(message);
	UNREFERENCED_PARAMETER(timeoutMilliseconds);

    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();
    HRESULT hr = operation->Initialize(ComponentRootSPtr(), callback);

    if (FAILED(hr))
    {
        return hr;
    }

    wstring sclientId;
    hr = StringUtility::LpcwstrToWstring(clientId,
                                         false,
                                         ParameterValidator::MinStringSize,
                                         ParameterValidator::MaxStringSize,
                                         sclientId);
    if (FAILED(hr))
    {
        return hr;
    }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(operation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr))
    {
        return hr;
    }

    *context = baseOperation.DetachNoRelease();
    return S_OK;
}

HRESULT ComDummyService::EndProcessRequest(
    IFabricAsyncOperationContext * context,
    IFabricServiceCommunicationMessage ** reply)
{
    reply_->QueryInterface(IID_IFabricServiceCommunicationMessage, reinterpret_cast<void**>(reply));
    return ComCompletedAsyncOperationContext::End(context);
}

HRESULT  ComDummyService::HandleOneWay(
    LPCWSTR clientId,
    IFabricServiceCommunicationMessage *message)
{
	UNREFERENCED_PARAMETER(message);

    wstring sclientId;
    auto hr = StringUtility::LpcwstrToWstring(
        clientId,
        false,
        ParameterValidator::MinStringSize,
        ParameterValidator::MaxStringSize,
        sclientId);

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
