// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <sal.h>
#if !defined(PLATFORM_UNIX)
#include <esent.h>
#endif

#include "Common/Common.h"
#include "Common/KtlSF.Common.h"
#include "api/definitions/ApiDefinitions.h"
#include "api/wrappers/ApiWrappers.h"
#include "ktllogger/ktlloggerX.h"
#include "../../test/TestHooks/TestHooks.h"

#include "Store/StoreConfig.h"
#include "Store/StorePointers.h"
#include "Store/StoreCounters.h"

#include "Store/PartitionedReplicaId.h"
#include "Store/ReplicaActivityId.h"
#include "Store/PartitionedReplicaTraceComponent.h"
#include "Store/ReplicaActivityTraceComponent.h"
#include "Store/RepairPolicy/RepairPolicy.h"

#include "Store/StoreType.h"
#include "Store/StoreFactoryParameters.h"
#include "Store/IStoreFactory.h"

#include "Store/StoreBackupOption.h"
#include "Store/SecondaryNotificationMode.h"
#include "Store/FullCopyMode.h"
#include "Store/ReplicatedStoreSettings.h"
#include "Store/TransactionSettings.h"

#include "Store/StoreBase.h"
#include "Store/IStoreBase.h"
#include "Store/ILocalStore.h"
#include "Store/IReplicatedStoreTxEventHandler.h"
#include "Store/IReplicatedStore.h"

#include "Store/StoreData.h"
#include "Store/StoreTransactionTemplate.h"
#include "Store/DataSerializer.h"

#include "Store/EseLocalStoreSettings.h"
#include "Store/TSReplicatedStoreSettings.h"

#include "Store/MigrationSettings.h"
#include "Store/MigrationInitData.h"
#include "Store/AzureStorageClient.h"

#if !defined(PLATFORM_UNIX)
#include "Store/KeyValueStoreMigrator.h"
#else
#include "Store/KeyValueStoreMigrator_Linux.h"
#endif

#include "Store/DeadlockDetector.h"
#include "Store/KeyValueStoreReplica.h"
#include "Store/StoreFactory.h"
#include "Store/KeyValueStoreReplicaFactory.h"
#include "Store/KeyValueStoreQueryResultWrapper.h"

