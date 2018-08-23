//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Transport;
using namespace std;
using namespace ClientServerTransport;
using namespace Management::ClusterManager;

AsyncOperationSPtr DeleteVolumeAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    DeleteVolumeMessageBody body;
    if (this->Request.GetBody(body))
    {
        return this->Replica.VolMgr->BeginDeleteVolume(
            body.VolumeName,
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

ErrorCode DeleteVolumeAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.VolMgr->EndDeleteVolume(operation);
}

