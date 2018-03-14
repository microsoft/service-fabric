// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Failover/common/FailoverUnitDescription.h"

#include "Reliability/Failover/common/FailoverConfig.h"
#include "Reliability/Failover/fm/NodeDeactivationInfo.h"
#include "Reliability/Failover/ra/FailoverUnitStates.h"
#include "Reliability/Failover/common/ReplicaRole.h"
#include "Reliability/Failover/common/ReplicaStates.h"
#include "Reliability/Failover/ra/ReplicaStates.h"
#include "Reliability/Failover/ra/FailoverUnitReconfigurationStage.h"
#include "Reliability/Failover/ra/ReplicaMessageStage.h"
#include "Reliability/Failover/fm/FailoverUnitHealthState.h"
#include "Reliability/Failover/TestApi.FailoverPointers.h"
#include "Reliability/Failover/TestApi.Reliability.h"
#include "Reliability/Failover/ra/TestApi.RA.h"
#include "Reliability/Failover/ra/TestApi.RAP.h"
#include "Reliability/Failover/fm/TestApi.FM.h"
