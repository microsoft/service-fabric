// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "CompletionTask.h"
#include "ILoggingReplicatorToVersionManager.h"

#include "RecoveryInformation.h"
#include "ILoggingReplicator.h"
#include "IStateProviderManager.h"
#include "IVersionProvider.h"
#include "IInternalVersionManager.h"
#include "IDataLossHandler.h"
#include "IBackupFolderInfo.h"

#include "Epoch.h" 
#include "TRInternalSettings.h"
#include "SLInternalSettings.h"

#include "IBackupRestoreProvider.h"

#include "../../../Reliability/Replication/Replication.Public.h"
#include "ApiMonitoringWrapper.h"

#include "AbortReason.h"
#include "TransactionStateMachine.h"
#include "TREventSource.h"
#include "IOMonitor.h"
