// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxyServiceNotificationEventHandler  Implementation
//
ComProxyServiceNotificationEventHandler::ComProxyServiceNotificationEventHandler(
    ComPointer<IFabricServiceNotificationEventHandler > const & comImpl)
    : ComponentRoot(),
    IServiceNotificationEventHandler(),
    comImpl_(comImpl)
{
}

ComProxyServiceNotificationEventHandler::~ComProxyServiceNotificationEventHandler()
{
}

ErrorCode ComProxyServiceNotificationEventHandler::OnNotification(IServiceNotificationPtr const & ptr)
{
    auto comPtr = WrapperFactory::create_com_wrapper(ptr);
    return ErrorCode::FromHResult(comImpl_->OnNotification(comPtr.GetRawPointer()));
}
