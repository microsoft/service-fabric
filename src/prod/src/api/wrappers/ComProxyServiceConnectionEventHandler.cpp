// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyServiceConnectionEventHandler  Implementation
//
ComProxyServiceConnectionEventHandler::ComProxyServiceConnectionEventHandler(
    ComPointer<IFabricServiceConnectionEventHandler > const & comImpl)
    : ComponentRoot(),
    IServiceConnectionEventHandler(),
    comImpl_(comImpl)
{
}

ComProxyServiceConnectionEventHandler::~ComProxyServiceConnectionEventHandler()
{
}

void ComProxyServiceConnectionEventHandler::OnConnected(wstring const & address)
{
    comImpl_->OnConnected(address.c_str());
}

void ComProxyServiceConnectionEventHandler::OnDisconnected(wstring const & address,
                                                           ErrorCode const & error)
{
    comImpl_->OnDisconnected(address.c_str(), error.ToHResult());
}
