// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#if !defined(PLATFORM_UNIX)
#include <sal.h>
#endif
#include "Common/Common.h"
#include <ktl.h>
#include <KTpl.h>
#include <kPriorityQueue.h>

// External dependencies
#include "../data.public.h"
#include "../utilities/Data.Utilities.Public.h"
#include "../txnreplicator/common/TransactionalReplicator.Common.Public.h"
#include "../txnreplicator/statemanager/StateManager.Public.h"
#include "../txnreplicator/TransactionalReplicator.Public.h"
#include "../txnreplicator/loggingreplicator/VersionManager.Public.h"
#include <SfStatus.h>
#include "../common/DataCommon.h"

namespace Data
{
    namespace TStore
    {
        using ::_delete;
        using namespace Data::Utilities;
    }
}

#include "KBufferSerializer.h"
#include "KBufferComparer.h"
#include "IFilterableEnumerator.h"
#include "StoreBehavior.h"
#include "StoreTraceComponent.h"
#include "Sorter.h"
#include "StreamPool.h"
#include "SortedItemComparer.h"
#include "FastSkipList.h"
#include "FastSkipListEnumerator.h"
#include "FastSkipListKeysEnumerator.h"
#include "ConcurrentDictionary2.h"
#include "KSharedArrayEnumerator.h"
#include "ConcurrentSkipListFilterableEnumerator.h"
#include "ConcurrentSkipListKeysEnumerator.h"
#include "ReadOnlySortedList.h"
#include "ReadOnlySortedListEnumerator.h"
#include "ReadOnlySortedListKeysEnumerator.h"
#include "ComparableSortedSequenceEnumerator.h"
#include "SharedPriorityQueue.h"
#include "SortedSequenceMergeEnumerator.h"
#include "IPartition.h"
#include "StoreUtilities.h"
#include "SharedBinaryWriter.h"
#include "SharedBinaryReader.h"
#include "PartitionComparer.h"
#include "PartitionSearchKey.h"
#include "Index.h"
#include "KeyValueList.h"
#include "Partition.h"
#include "PartitionedListEnumerator.h"
#include "PartitionedListKeysEnumerator.h"
#include "PartitionedList.h"
#include "PartitionedSortedList.h"
#include "PartitionedSortedListFilterableEnumerator.h"
#include "PartitionedSortedListKeysFilterableEnumerator.h"
#include "StoreEventSource.h"
#include "StorePerformanceCounters.h"
#include "PerformanceCounterWriter.h"
#include "CheckpointPerformanceCounterWriter.h"
#include "CopyPerformanceCounterWriter.h"
#include "Constants.h"
#include "StoreTransactionSettings.h"
#include "StoreApi.h"
#include "StoreInitializationParameters.h"
#include "RecordKind.h"
#include "ReadMode.h"
#include "MergePolicy.h"
#include "VersionedItem.h"
#include "InsertedVersionedItem.h"
#include "UpdatedVersionedItem.h"
#include "DeletedVersionedItem.h"
#include "KeyVersionedItemComparer.h"
#include "IReadableStoreComponent.h"
#include "Diagnostics.h"
#include "DifferentialStateVersions.h"
#include "DifferentialKeysEnumerator.h"
#include "DifferentialKeyValueEnumerator.h"
#include "ArrayKeyVersionedItemEnumerator.h"
#include "RedoUndoOperationData.h"
#include "MetadataOperationData.h"
#include "WriteSetStoreComponent.h"
#include "IStoreTransaction.h"
#include "StoreTransaction.h"
#include "IDictionaryChangeHandler.h"
#include "IStore.h"
#include "StoreFactory.h"
#include "FileCountMergeConfiguration.h"
#include "PropertyId.h"
#include "ByteAlignedReaderWriterHelper.h"
#include "FilePropertySection.h"
#include "PropertyChunkMetadata.h"
#include "MetadataManagerFileProperties.h"
#include "KeyCheckpointFileProperties.h"
#include "ValueCheckpointFileProperties.h"
#include "KeyData.h"
#include "KeyChunkMetadata.h"
#include "KeyCheckpointFile.h"
#include "ValueCheckpointFile.h"
#include "ValueBlockAlignedWriter.h"
#include "KeyBlockAlignedWriter.h"
#include "KeyCheckpointFileAsyncEnumerator.h"
#include "BlockAlignedWriter.h"
#include "CheckpointFile.h"
#include "FileMetadata.h"
#include "FileMetaDataComparer.h"
#include "MetadataTable.h"
#include "PropertyChunkMetadata.h"
#include "MetadataManager.h"
#include "MemoryBuffer.h"
#include "IStoreCopyProvider.h"
#include "ICopyManager.h"
#include "StoreCopyStream.h"
#include "CopyManager.h"
#include "MergeHelper.h"
#include "StoreTransaction.h"
#include "KeySizeEstimator.h"
#include "RecoveryStoreEnumerator.h"
#include "RecoveryStoreComponent.h"
#include "StoreComponentReadResult.h"
#include "SnapshotComponent.h"
#include "SnapshotContainer.h"
#include "DifferentialStoreComponent.h"
#include "DifferentialStateEnumerator.h"
#include "DeltaEnumerator.h"
#include "SweepEnumerator.h"
#include "DifferentialData.h"
#include "DifferentialDataEnumerator.h"
#include "ConsolidatedStoreComponent.h"
#include "AggregatedStoreComponent.h"
#include "PostMergeMetadataTableInformation.h"
#include "IConsolidationProvider.h"
#include "ConsolidationManager.h"
#include "ConsolidationTask.h"
#include "ISweepProvider.h"
#include "SweepManager.h"
#include "VolatileStoreCopyStream.h"
#include "VolatileCopyManager.h"
#include "RebuiltStateEnumerator.h"
#include "StoreKeysEnumerator.h"
#include "StoreKeyValueEnumerator.h"
#include "Store.h"

namespace TStoreTests
{
    using ::_delete;
}

#include "../testcommon/TestCommon.Public.h"

// The following are test headers
#include "MockStateManager.h"
#include "StoreStateProviderFactory.h"
#include "TestTransactionContext.h"
#include "MockTransactionalReplicator.h"
#include "NullableStringStateSerializer.h"
#include "WriteTransaction.h"
#include "TStoreTestBase.h"
#include "TestStateSerializer.h"
#include "StringStateSerializer.h"
#include "TestDictionaryChangeHandler.h"

// The following are perf test headers
#include "SharedLong.h"
#include "DataStructures.Perf.h"
#include "TStorePerfTestBase.h"
