// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace V1ReplPerf;

ServiceEntry::ServiceEntry(int64 key)
    : key_(key)
{
}

HRESULT ServiceEntry::BeginReplicate(
    ComPointer<IFabricStateReplicator> & replicator, 
    ComPointer<ComAsyncOperationCallbackHelper> & callback, 
    ComPointer<IFabricAsyncOperationContext> & context, 
    FABRIC_SEQUENCE_NUMBER & newVersion,
    int newValueSize) 
{ 
    ComPointer<OperationData> operationData = make_com<OperationData>(newValueSize);
    
    HRESULT hr = replicator->BeginReplicate(
        operationData.GetRawPointer(),
        callback.GetRawPointer(),
        &newVersion,
        context.InitializationAddress());

    return hr;
}
