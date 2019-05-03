// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

//
// System headers
//

#include <sal.h>
#include <algorithm>
#include <vector>
#include <map>
#include <random>

#include "Common/Common.h"
#include "Transport/Transport.h"

#include "resources/Resources.h"
#include "inc/resourceids.h"
#include "ServiceModel/ServiceModel.h"
#include "ServiceModel/federation/NodeId.h"
#include "ServiceModel/federation/NodeInstance.h"
#include "ServiceModel/Constants.h"
#include "ServiceModel/LoadMetricInformation.h"
#include "ServiceModel/ClusterLoadInformationQueryResult.h"
#include "ServiceModel/NodeLoadMetricInformation.h"
#include "ServiceModel/NodeLoadInformationQueryResult.h"

#include "Reliability/Failover/common/ReplicaFlags.h"
#include "Reliability/Failover/common/ReplicaStates.h"
#include "Reliability/Failover/common/FailoverUnitFlags.h"
