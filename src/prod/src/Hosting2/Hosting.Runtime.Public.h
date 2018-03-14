// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

//
// External Header Files required by public Hosting header files
//

#include "LeaseAgent/LeaseAgent.h"
#include "Common/Common.h"
#include "Transport/Transport.h"
#include "Management/Common/ManagementCommon.h"
#include "ServiceModel/ServiceModel.h"
#include "Management/ImageModel/ImageModel.h"
#include "Reliability/LoadBalancing/PLBConfig.h"
#include "Reliability/Failover/FailoverPointers.h"
#include "Reliability/Replication/ReplicationPointers.h"
#include "data/txnreplicator/TransactionalReplicatorPointers.h"

#include "FabricCommon.h"
#include "FabricRuntime.h"
#include "FabricRuntime_.h"

//
// Public Runtime header files
//

#include "Hosting2/HostingPointers.h"
#include "Hosting2/Hosting.RuntimePointers.h"
#include "Hosting2/IApplicationHost.h"
#include "Hosting2/IRuntimeSharingHelper.h"
