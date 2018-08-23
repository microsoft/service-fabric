// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Store;

using namespace Management::ResourceManager;
using namespace Management::CentralSecretService;

StringLiteral const TraceComponent("ClientRequestAsyncOperation");

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
	Management::CentralSecretService::SecretManager & secretManager,
	IpcResourceManagerService & resourceManager,
	MessageUPtr requestMsg,
	IpcReceiverContextUPtr receiverContext,
	TimeSpan const & timeout,
	AsyncCallback const & callback,
	AsyncOperationSPtr const & parent)
	: IpcAutoRetryAsyncOperation(
		CentralSecretServiceConfig::GetConfig().MaxOperationRetryDelay,
		move(requestMsg),
		move(receiverContext),
		timeout,
		callback,
		parent)
	, ReplicaActivityTraceComponent<TraceTaskCodes::CentralSecretService>(
		secretManager.PartitionedReplicaId,
		this->ActivityId)
	, secretManager_(secretManager)
	, resourceManager_(resourceManager)
{
}

ErrorCode ClientRequestAsyncOperation::End(
	AsyncOperationSPtr const & asyncOperation,
	__out MessageUPtr & replyMsg,
	__out IpcReceiverContextUPtr & receiverContext)
{
	return IpcAutoRetryAsyncOperation::End(
		asyncOperation,
		replyMsg,
		receiverContext);
}