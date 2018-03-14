// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;
using namespace ImageModel;

StringLiteral const TracePrincipalsProvider("PrincipalsProvider");

GlobalWString PrincipalsProvider::GlobalMutexName = make_global<wstring>(L"Global\\WinFabEnvMgr");

PrincipalsProvider::PrincipalsProvider(ComponentRoot const & root, wstring const & nodeId)
    : RootedObject(root),
    FabricComponent(),
    nodeId_(nodeId)
{
}

PrincipalsProvider::~PrincipalsProvider()
{
}

// ********************************************************************************************************************
// FabricComponent methods

ErrorCode PrincipalsProvider::OnOpen()
{
    MutexHandleUPtr mutex;
    auto error = SecurityPrincipalHelper::AcquireMutex(PrincipalsProvider::GlobalMutexName, mutex);
    if (error.IsSuccess())
    {
        // Cleanup all principals for current nodeId
        ApplicationPrincipals::CleanupEnvironment(nodeId_);
        // Cleanup errors are ignored here. Since we use randomly generated names the 
        // cleanup is only an optimization. If it fails it will be taken care of next time the
        // environment or node is cleaned up or when a re-image happens
    }

    return error;
}

ErrorCode PrincipalsProvider::OnClose()
{
    MutexHandleUPtr mutex;
    auto error = SecurityPrincipalHelper::AcquireMutex(PrincipalsProvider::GlobalMutexName, mutex);
    if (error.IsSuccess())
    {
        // Cleanup all principals for current nodeId
        ApplicationPrincipals::CleanupEnvironment(nodeId_);
        // Cleanup errors are ignored here. Since we use randomly generated names the 
        // cleanup is only an optimization. If it fails it will be taken care of next time the
        // environment or node is cleaned up or when a re-image happens
    }

    return ErrorCode::Success();
}

void PrincipalsProvider::OnAbort()
{
    MutexHandleUPtr mutex;
    auto error = SecurityPrincipalHelper::AcquireMutex(PrincipalsProvider::GlobalMutexName, mutex);
    if (error.IsSuccess())
    {
        // Cleanup all principals for current nodeId
        ApplicationPrincipals::CleanupEnvironment(nodeId_);
        // Cleanup errors are ignored here. Since we use randomly generated names the 
        // cleanup is only an optimization. If it fails it will be taken care of next time the
        // environment or node is cleaned up or when a re-image happens
    }

    error.ReadValue();
}

