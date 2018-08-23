// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Federation/Federation.h"
#include "ServiceModel/ServiceModel.h"
#include "Reliability/Failover/common/Constants.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"

//
// Header files internal to Query component
//

#include "query/QueryEventSource.h"
#include "query/QueryConfig.h"

#include "query/QueryMessage.h"

#include "query/QueryAddress.h"
#include "query/QuerySpecification.h"
#include "query/QuerySpecificationStore.h"
#include "query/ParallelQuerySpecification.h"
#include "query/SequentialQuerySpecification.h"
#include "query/AggregateHealthParallelQuerySpecificationBase.h"

#include "query/QueryMessageUtility.h"

#include "query/GetApplicationListQuerySpecification.h"
#include "query/GetApplicationServiceListQuerySpecification.h"
#include "query/GetDeployedCodePackageParallelQuerySpecification.h"
#include "query/GetSystemServiceListQuerySpecification.h"
#include "query/GetServicePartitionListQuerySpecification.h"
#include "query/GetServicePartitionReplicaListQuerySpecification.h"
#include "query/GetNodeListQuerySpecification.h"
#include "query/SwapReplicaQuerySpecificationHelper.h"
#include "query/GetUnplacedReplicaInformationQuerySpecificationHelper.h"
#include "query/GetServiceNameQuerySpecificationHelper.h"
#include "query/GetApplicationPagedListDeployedOnNodeQuerySpecification.h"
#include "query/GetApplicationResourceListQuerySpecification.h"
#include "query/GetServiceResourceListQuerySpecification.h"
#include "query/GetContainerCodePackageLogsQuerySpecification.h"
#include "query/GetReplicaResourceListQuerySpecification.h"
