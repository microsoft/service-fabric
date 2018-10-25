// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Naming;

using namespace Management::ResourceManager;

Context::Context(
	ComponentRoot const & componentRoot,
	Store::PartitionedReplicaId const & partitionedReplicaId,
	IReplicatedStore * replicatedStore,
	wstring const & resourceTypeName,
	wstring const & traceSubComponent,
	QueryMetadataFunction const & queryResourceMetadataFunction,
	TimeSpan const & maxRetryDelay)
	: partitionedReplicaId_(partitionedReplicaId)
	, resourceManager_(partitionedReplicaId, replicatedStore, resourceTypeName)
	, queryResourceMetadataFunction_(queryResourceMetadataFunction)
	, requestTracker_(componentRoot, traceSubComponent, partitionedReplicaId)
	, maxRetryDelay_(maxRetryDelay)
{
}
