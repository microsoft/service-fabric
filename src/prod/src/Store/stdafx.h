// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if defined(PLATFORM_UNIX)
#include "stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

#endif

#define GET_STORE_RC( ResourceName ) \
    Common::StringResource::Get( IDS_STORE_##ResourceName )

#if defined(UNICODE) || defined(_UNICODE)
    // Turn on unicode support for esent.h.
    #define JET_UNICODE
#else
    static_assert(false, "\
The ESE wrapper code uses wstring and therefore wchar_t explicitly, and assumes \
any non-unicode usage will be converted at the public API if needed.  To enforce \
this assumption, we fail compilation here if that assumption is no longer valid.");
#endif // if defined(UNICODE) || defined(_UNICODE)

#include "Store/store.h" 

#include <locale>

#include "FabricRuntime.h"
#include "FabricRuntime_.h"

#include "Common/RetailSerialization.h"

#include "Store/Constants.h"

#include "Store/BackupRestoreData.h"
#include "Store/LocalStoreIncrementalBackupData.h"
#include "Store/PartialCopyProgressData.h"
#include "Store/FileStreamCopyOperationData.h"
#include "Store/FileStreamFullCopyContext.h"
#include "Store/FileStreamFullCopyManager.h"
#include "Store/ProgressVectorData.h"
#include "Store/CopyContextData.h"
#include "Store/CurrentEpochData.h"
#include "Store/TombstoneData.h"
#include "Store/ComReplicatedOperationData.h"
#include "Store/ComCopyContextEnumerator.h"
#include "Store/ReplicationOperationType.h"
#include "Store/ReplicationOperation.h"
#include "Store/AtomicOperation.h"
#include "Store/CopyType.h"
#include "Store/CopyOperation.h"
#include "Store/TombstoneLowWatermarkData.h"
#include "Store/ComCopyOperationEnumerator.h"
#include "Store/OpenLocalStoreJobQueue.h"

#if !defined(PLATFORM_UNIX)
#include "Store/EseError.h"
#include "Store/EseTableBuilder.h"
#include "Store/EseInstance.h"
#include "Store/EseSession.h"
#include "Store/EseCommitAsyncOperation.h"
#include "Store/EseDatabase.h"
#include "Store/EseCursor.h"
#include "Store/EseLocalStore.h"
#include "Store/EseDefragmenter.h"
#include "Store/EseLocalStoreUtility.h"
#include "Store/EseException.h"
#include "Store/EseStoreEventSource.h"
#endif

#include "Store/ScopedActiveBackupState.h"
#include "Store/ScopedActiveFlag.h"
#include "Store/CopyStatistics.h"
#include "Store/ReplicatedStorestate.h"
#include "Store/ReplicatedStore.h"
#include "Store/ReplicatedStoreEvent.h"
#include "Store/ReplicatedStoreEventSource.h"

#include "Store/ReplicatedStoreEnumeratorBase.h"
#include "Store/ReplicatedStore.ChangeRoleAsyncOperation.h"
#include "Store/ReplicatedStore.CloseAsyncOperation.h"
#include "Store/FabricTimeData.h"
#include "Store/FabricTimeController.h"
#include "Store/ReplicatedStore.OnDataLossAsyncOperation.h"
#include "Store/ReplicatedStore.UpdateEpochAsyncOperation.h"
#include "Store/ReplicatedStore.OpenAsyncOperation.h"
#include "Store/ReplicatedStore.BackupAsyncOperation.h"
#include "Store/ReplicatedStore.RestoreAsyncOperation.h"
#include "Store/ReplicatedStore.TransactionBase.h"
#include "Store/ReplicatedStore.Transaction.h"
#include "Store/ReplicatedStore.Enumeration.h"
#include "Store/ReplicatedStore.SimpleTransactionGroup.h"
#include "Store/ReplicatedStore.SimpleTransaction.h"
#include "Store/ReplicatedStore.TransactionReplicator.h"
#include "Store/ReplicatedStore.TransactionReplicator.ThrottleState.h"
#include "Store/ReplicatedStore.TransactionReplicator.ThrottleStateMachine.h"
#include "Store/ReplicatedStore.TransactionTracker.h"
#include "Store/ReplicatedStore.HealthTracker.h"
#include "Store/ReplicatedStore.NotificationManager.h"
#include "Store/ReplicatedStore.SecondaryPump.h"
#include "Store/ReplicatedStore.StateChangeAsyncOperation.h"
#include "Store/ReplicatedStore.StateMachine.h"
#if !defined(PLATFORM_UNIX)
#include "Store/EseReplicatedStore.h"
#endif

#include <ktl.h>
#include <KTpl.h>
#include "../data/tstore/stdafx.h"
#include "../data/txnreplicator/TransactionalReplicator.Public.h"

namespace Store
{
    using ::_delete;
}

#include "Store/TSComponent.h"
#include "Store/TSTransaction.h"
#include "Store/TSEnumerationBase.h"
#include "Store/TSEnumeration.h"
#include "Store/TSChangeHandler.h"
#include "Store/TSReplicatedStore.h"
#include "Store/TSLocalStore.h"
#include "Store/TSUnitTestStore.h"

#include "Store/KeyValueStoreTransaction.h"
#include "Store/KeyValueStoreEnumeratorBase.h"
#include "Store/KeyValueStoreItemEnumerator.h"
#include "Store/KeyValueStoreItemMetadataEnumerator.h"
#include "Store/KeyValueStoreItemResult.h"
#include "Store/KeyValueStoreItemMetadataResult.h"

#include "Store/StoreFactory.h"
