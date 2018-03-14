// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"

#include "Management/RepairManager/NodeImpactLevel.h"
#include "Management/RepairManager/NodeImpact.h"
#include "Management/RepairManager/RepairImpactDescriptionBase.h"
#include "Management/RepairManager/RepairTargetDescriptionBase.h"
#include "Management/RepairManager/RepairResourceIdentifierBase.h"
#include "Management/RepairManager/NodeRepairImpactDescription.h"
#include "Management/RepairManager/NodeRepairTargetDescription.h"
#include "Management/RepairManager/ClusterRepairResourceIdentifier.h"
#include "Management/RepairManager/RepairTaskHistory.h"
#include "Management/RepairManager/RepairTask.h"
#include "Management/RepairManager/ApproveRepairTaskMessageBody.h"
#include "Management/RepairManager/CancelRepairTaskMessageBody.h"
#include "Management/RepairManager/DeleteRepairTaskMessageBody.h"
#include "Management/RepairManager/UpdateRepairTaskMessageBody.h"
#include "Management/RepairManager/UpdateRepairTaskReplyMessageBody.h"
#include "Management/RepairManager/UpdateRepairTaskHealthPolicyMessageBody.h"
