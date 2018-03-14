// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "api/definitions/ApiDefinitions.h"
#include "systemservices/SystemServices.Service.h"
#include "client/ClientServerTransport/ClientServerTransport.external.h"
#include "Management/RepairManager/RepairManager.External.h"

#include "Management/RepairManager/Constants.h"
#include "Management/RepairManager/RepairManagerPointers.h"
#include "Management/RepairManager/RepairManagerConfig.h"
#include "Management/RepairManager/RepairManagerState.h"
#include "Management/RepairManager/RepairManagerEventSource.h"

#include "Management/RepairManager/RepairTaskContext.h"

#include "Management/RepairManager/CleanupAsyncOperation.h"
#include "Management/RepairManager/ClientRequestAsyncOperation.h"

#include "Management/RepairManager/HealthCheckStoreData.h"

#include "Management/RepairManager/RepairManagerServiceFactory.h"
#include "Management/RepairManager/RepairManagerServiceReplica.h"

#include "Management/RepairManager/ProcessQueryAsyncOperation.h"
#include "Management/RepairManager/CancelRepairRequestAsyncOperation.h"
#include "Management/RepairManager/CreateRepairRequestAsyncOperation.h"
#include "Management/RepairManager/DeleteRepairRequestAsyncOperation.h"
#include "Management/RepairManager/ForceApproveRepairAsyncOperation.h"
#include "Management/RepairManager/UpdateRepairExecutionStateAsyncOperation.h"
#include "Management/RepairManager/UpdateRepairTaskHealthPolicyAsyncOperation.h"
#include "Management/RepairManager/RepairManagerQueryMessageHandler.h"

#include "Management/RepairManager/RepairTaskHealthCheckHelper.h"

#include "Management/RepairManager/RepairPreparationAsyncOperation.h"
#include "Management/RepairManager/RepairRestorationAsyncOperation.h"
#include "Management/RepairManager/ProcessRepairTaskContextAsyncOperation.h"
#include "Management/RepairManager/ProcessRepairTaskContextsAsyncOperation.h"

#include "Management/RepairManager/HealthCheckAsyncOperation.h"
