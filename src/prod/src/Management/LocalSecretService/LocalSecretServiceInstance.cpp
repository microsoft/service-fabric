//+---------------------------------------------------------------------------
//  Copyright (C) Microsoft. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Naming;
using namespace Reliability;
using namespace Transport;
using namespace Store;
using namespace SystemServices;
using namespace Api;
using namespace Hosting2;
using namespace Federation;
using namespace Client;
using namespace TestHooks;
using namespace Management;
using namespace Management::LocalSecretService;
using namespace Management::ClusterManager;

LocalSecretServiceInstance::LocalSecretServiceInstance()
{
}

LocalSecretServiceInstance::~LocalSecretServiceInstance()
{
}

HRESULT LocalSecretServiceInstance::BeginOpen(
    __in IFabricStatelessServicePartition* partition,
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
)
{
    UNREFERENCED_PARAMETER(partition);
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);
    return S_OK;
}

HRESULT LocalSecretServiceInstance::EndOpen(
    __in IFabricAsyncOperationContext* context,
    __out IFabricStringResult** serviceAddress
)
{
    UNREFERENCED_PARAMETER(context);
    UNREFERENCED_PARAMETER(serviceAddress);
    return S_OK;
}

HRESULT LocalSecretServiceInstance::BeginClose(
    __in IFabricAsyncOperationCallback* callback,
    __out IFabricAsyncOperationContext** context
)
{
    UNREFERENCED_PARAMETER(callback);
    UNREFERENCED_PARAMETER(context);
    return S_OK;
}

HRESULT LocalSecretServiceInstance::EndClose(
    __in IFabricAsyncOperationContext* context
)
{
    UNREFERENCED_PARAMETER(context);
    return S_OK;
}

void LocalSecretServiceInstance::Abort(void)
{
}