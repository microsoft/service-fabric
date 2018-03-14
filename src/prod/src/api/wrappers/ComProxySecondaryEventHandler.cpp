// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComProxySecondaryEventHandler  Implementation
//
ComProxySecondaryEventHandler::ComProxySecondaryEventHandler(
    ComPointer<IFabricSecondaryEventHandler > const & comImpl)
    : ComponentRoot(),
    ISecondaryEventHandler(),
    comImpl_(comImpl)
{
}

ComProxySecondaryEventHandler::~ComProxySecondaryEventHandler()
{
}

ErrorCode ComProxySecondaryEventHandler::OnCopyComplete(IStoreEnumeratorPtr const & ptr)
{
    auto comPtr = WrapperFactory::create_com_wrapper(ptr);
    return ErrorCode::FromHResult(comImpl_->OnCopyComplete(comPtr.GetRawPointer()), true); // capture managed exception stack
}

ErrorCode ComProxySecondaryEventHandler::OnReplicationOperation(IStoreNotificationEnumeratorPtr const & ptr)
{
    auto comPtr = WrapperFactory::create_com_wrapper(ptr);
    return ErrorCode::FromHResult(comImpl_->OnReplicationOperation(comPtr.GetRawPointer()), true); // capture managed exception stack
}
