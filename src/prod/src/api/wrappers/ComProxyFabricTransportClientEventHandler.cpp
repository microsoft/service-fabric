// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyFabricTransportClientEventHandler  Implementation
//
ComProxyFabricTransportClientEventHandler::ComProxyFabricTransportClientEventHandler(
    ComPointer<IFabricTransportClientEventHandler > const & comImpl)
    : ComponentRoot(),
    IServiceConnectionEventHandler(),
    comImpl_(comImpl)
{
}

ComProxyFabricTransportClientEventHandler::~ComProxyFabricTransportClientEventHandler()
{
}

void ComProxyFabricTransportClientEventHandler::OnConnected(wstring const & address)
{
    comImpl_->OnConnected(address.c_str());
}

void ComProxyFabricTransportClientEventHandler::OnDisconnected(wstring const & address,
                                                           ErrorCode const & error)
{
    comImpl_->OnDisconnected(address.c_str(), error.ToHResult());
}
