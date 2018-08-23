// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Naming;
using namespace std;
using namespace ClientServerTransport;
using namespace Management::ClusterManager;
using namespace ServiceModel;

StringLiteral const TraceComponent("UpgradeComposeDeploymentAsyncOperation");

AsyncOperationSPtr UpgradeComposeDeploymentAsyncOperation::BeginAcceptRequest(
    ClientRequestSPtr && clientRequest,
    TimeSpan const timeout, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & root)
{
    UpgradeComposeDeploymentRequestHeaderBase requestHeader;
    vector<const_buffer> body;

    if (!this->Request.GetBody(body))
    {
        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }
    else
    {
        UpgradeComposeDeploymentRequestHeader newHeader;
        UpgradeComposeDeploymentRequestCompatibilityHeader oldHeader;
        if (this->Request.Headers.Contains(MessageHeaderId::Enum::UpgradeComposeDeploymentRequest)
            && this->Request.Headers.TryGetAndRemoveHeader(newHeader))
        {
            requestHeader = static_cast<UpgradeComposeDeploymentRequestHeaderBase>(newHeader);
        }
        else if (this->Request.Headers.TryGetAndRemoveHeader(oldHeader))
        {
            requestHeader = static_cast<UpgradeComposeDeploymentRequestHeaderBase>(oldHeader);
        }
        else
        {
            return this->Replica.RejectInvalidMessage(
                move(clientRequest),
                callback,
                root);
        }
    }

    //
    // Depending on the message chunk size used internally, the files might be split across multiple
    // const buffer's. So we need to aggregate them. When client sends overrides files and sf settings file
    // we will need to split the buffers into separate file data. 
    //
    if (requestHeader.ComposeFileSizes.size() != 1)
    {
        WriteInfo(
           TraceComponent, 
           "{0} More than one compose file not supported for app {1}",
           FabricActivityHeader::FromMessage(Request).ActivityId,
           requestHeader.ApplicationName);

        return this->Replica.RejectInvalidMessage(
            move(clientRequest),
            callback,
            root);
    }

    vector<ByteBuffer> composeFiles;
    vector<ByteBuffer> sfSettingsFiles;
    composeFiles.reserve(requestHeader.ComposeFileSizes.size());
    for (int i = 0; i < requestHeader.ComposeFileSizes.size(); ++i)
    {
        ByteBuffer temp;
        temp.reserve(requestHeader.ComposeFileSizes[i]);
        composeFiles.push_back(move(temp));
    }

    auto composeFileLength = requestHeader.ComposeFileSizes[0];
    auto it = composeFiles[0].begin();
    for (int i = 0; i < body.size(); ++i)
    {
        auto chunkLength = body[i].len;
        if (chunkLength < composeFileLength)
        {
            composeFiles[0].insert(it, &(body[i].buf[0]), &(body[i].buf[chunkLength]));
            composeFileLength = composeFileLength - chunkLength;
            it = composeFiles[0].end();
        }
        else // chunkLength >= composeFileLength
        {
            composeFiles[0].insert(it, &(body[i].buf[0]), &(body[i].buf[composeFileLength]));
            break;
        }
    }

    ComposeDeploymentUpgradeDescription upgradeDescription(
        requestHeader.TakeDeploymentName(),
        requestHeader.TakeApplicationName(),
        move(composeFiles),
        move(sfSettingsFiles),
        requestHeader.TakeRepoUserName(),
        requestHeader.TakeRepoPassword(),
        requestHeader.PasswordEncrypted,
        requestHeader.UpgradeType,
        requestHeader.UpgradeMode,
        requestHeader.ForceRestart,
        requestHeader.TakeUpgradeMonitoringPolicy(),
        requestHeader.HealthPolicyValid,
        requestHeader.TakeApplicationHealthPolicy(),
        requestHeader.ReplicaSetCheckTimeout);

    return this->Replica.BeginAcceptUpgradeComposeDeployment(
        upgradeDescription,
        move(clientRequest),
        timeout,
        callback,
        root);
}

ErrorCode UpgradeComposeDeploymentAsyncOperation::EndAcceptRequest(AsyncOperationSPtr const & operation)
{
    return this->Replica.EndAcceptUpgradeComposeDeployment(operation);
}

