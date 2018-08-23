//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace std;
using namespace ClientServerTransport;
using namespace Management::ClusterManager;

AsyncOperationSPtr CreateVolumeAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    CreateVolumeMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.VolMgr->BeginCreateVolume(
            body.VolumeDescription,
            move(clientRequest),
            timeout,
            callback,
            root);
    }
    else
    {
        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }
}

ErrorCode CreateVolumeAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.VolMgr->EndCreateVolume(operation);
}


