// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyClientConnectionEventHandler  Implementation
//
ComProxyClientConnectionEventHandler::ComProxyClientConnectionEventHandler(
    ComPointer<IFabricClientConnectionEventHandler > const & comImpl)
    : ComponentRoot()
    , IClientConnectionEventHandler()
    , comImpl_(comImpl)
    , heap_()
{
}

ComProxyClientConnectionEventHandler::~ComProxyClientConnectionEventHandler()
{
}

ErrorCode ComProxyClientConnectionEventHandler::OnConnected(Naming::GatewayDescription const & impl)
{
    auto comPtr = make_com<ComGatewayInformationResult, IFabricGatewayInformationResult>(impl);
    return ErrorCode::FromHResult(comImpl_->OnConnected(comPtr.GetRawPointer()), true);
}

ErrorCode ComProxyClientConnectionEventHandler::OnDisconnected(Naming::GatewayDescription const & impl, ErrorCode const & error)
{
    UNREFERENCED_PARAMETER( error );

    auto comPtr = make_com<ComGatewayInformationResult, IFabricGatewayInformationResult>(impl);
    return ErrorCode::FromHResult(comImpl_->OnDisconnected(comPtr.GetRawPointer()), true);
}

ErrorCode ComProxyClientConnectionEventHandler::OnClaimsRetrieval(
    shared_ptr<Transport::ClaimsRetrievalMetadata> const & metadata, 
    __out wstring & token)
{
    ComPointer<IFabricClientConnectionEventHandler2> comImpl2;

    auto hr = comImpl_->QueryInterface(IID_IFabricClientConnectionEventHandler2, comImpl2.VoidInitializationAddress());
    if (!SUCCEEDED(hr)) { return ErrorCode::FromHResult(hr); }

    auto publicMetadata = heap_.AddItem<FABRIC_CLAIMS_RETRIEVAL_METADATA>();
    metadata->ToPublicApi(heap_, *publicMetadata);

    ComPointer<IFabricStringResult> result;
    hr = comImpl2->OnClaimsRetrieval(publicMetadata.GetRawPointer(), result.InitializationAddress());
    if (!SUCCEEDED(hr)) { return ErrorCode::FromHResult(hr, true); } // capture managed exception stack

    hr = StringUtility::LpcwstrToWstring(result->get_String(), true, token);

    return ErrorCode::FromHResult(hr);
}
