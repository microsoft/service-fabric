// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONSOLIDATIONMANAGER_TAG 'rgMC'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class ConsolidationManager :  
           public KObject<ConsolidationManager<TKey, TValue>>, 
           public KShared<ConsolidationManager<TKey, TValue>>,
           public IReadableStoreComponent<TKey, TValue>
        {
            K_FORCE_SHARED(ConsolidationManager)
            K_SHARED_INTERFACE_IMP(IReadableStoreComponent)

        public:
            static NTSTATUS Create(
                __in IConsolidationProvider<TKey, TValue> & consolidationProvider,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(CONSOLIDATIONMANAGER_TAG, allocator) ConsolidationManager(
                    consolidationProvider,
                    traceComponent);

                if (!output)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    return status;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            __declspec(property(get = get_NumberOfDeltasToBeConsolidated, put = set_NumberOfDeltasToBeConsolidated)) ULONG32 NumberOfDeltasToBeConsolidated;
            ULONG32 get_NumberOfDeltasToBeConsolidated() const
            {
               return numberOfDeltasToBeConsolidated_;
            }

            void set_NumberOfDeltasToBeConsolidated(__in ULONG32 value)
            {
               numberOfDeltasToBeConsolidated_ = value;
            }

            // Exposed for testing
            __declspec(property(get = get_AggregatedStoreComponent)) KSharedPtr<AggregatedStoreComponent<TKey, TValue>> AggregatedStoreComponentSPtr;
            KSharedPtr<AggregatedStoreComponent<TKey, TValue>> get_AggregatedStoreComponent()
            {
                return aggregatedStoreComponentSPtr_.Get();
            }
            
            static int CompareEnumerators(__in KSharedPtr<DifferentialStateEnumerator<TKey, TValue>> const & one, __in KSharedPtr<DifferentialStateEnumerator<TKey, TValue>> const & two)
            {
               return one->Compare(*two);
            }

            ktl::Task ConsolidateAsync(
               __in MetadataTable& metadataTable,
               __in ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>& completionSource,
               __in StorePerformanceCountersSPtr & perfCounters,
               __in ktl::CancellationToken const cancellationToken)
            {
               KShared$ApiEntry();

               // Snapshot parameters
               MetadataTable::SPtr metadataTableSPtr = &metadataTable;
               ktl::AwaitableCompletionSource<PostMergeMetadataTableInformation::SPtr>::SPtr completionSourceSPtr = &completionSource;

               PostMergeMetadataTableInformation::SPtr postMergeMetadataTableInformation = nullptr;

               try
               {
                  // Create a store transaction not associated with a replicator transaction
                  KSharedPtr<StoreTransaction<TKey, TValue>> rwtxSPtr;

                  auto status = StoreTransaction<TKey, TValue>::Create(
                     consolidationProviderSPtr_->CreateTransactionId(),
                     consolidationProviderSPtr_->StateProviderId,
                     *consolidationProviderSPtr_->KeyComparerSPtr,
                     *traceComponent_,
                     this->GetThisAllocator(), 
                      rwtxSPtr);
                  Diagnostics::Validate(status);

                  // Release all acquired store locks; if any.
                  KFinally([&] { rwtxSPtr->Close(); });

                  try
                  {
                     co_await rwtxSPtr->AcquirePrimeLockAsync(
                        *consolidationProviderSPtr_->LockManagerSPtr,
                        LockMode::Shared,
                        Common::TimeSpan::FromMilliseconds(256 * 1000), // TODO: make constant
                        true);

                     cancellationToken.ThrowIfCancellationRequested();
                  }
                  catch (const ktl::Exception & e)
                  {

                      AssertException(L"ConsolidateAsync", e);
                  }

                  // Assert that new aggregated state is null
                  // New aggregated state does not have to be cached since there will no writes when reads are in progress.
                  STORE_ASSERT(newAggregatedStoreComponentSPtr_ == nullptr, "newAggregatedStoreComponentSPtr_ == nullptr");

                  MetadataTable::SPtr mergeTableSPtr = consolidationProviderSPtr_->MergeMetadataTableSPtr;
                  STORE_ASSERT(mergeTableSPtr == nullptr, "merged metadata table should be null");

                  auto cachedAggregatedComponentSPtr = aggregatedStoreComponentSPtr_.Get();
                  STORE_ASSERT(cachedAggregatedComponentSPtr != nullptr, "cachedAggregatedComponentSPtr != nullptr");

                  snapshotOfHighestIndexOnConsolidation_ = cachedAggregatedComponentSPtr->Index;

                  // Index starts with 1, zero is not a valid index.
                  if (snapshotOfHighestIndexOnConsolidation_ > 0)
                  {
                     MovePreviousVersionItemsToSnapshotContainerIfNeeded(snapshotOfHighestIndexOnConsolidation_, *metadataTableSPtr);
                  }

                  if (cachedAggregatedComponentSPtr->Index < NumberOfDeltasToBeConsolidated)
                  {
                     completionSourceSPtr->SetResult(nullptr);
                     co_return;
                  }

                  // Assumption: new consolidated state's size will be similar to old consolidated state's size.
                  // todo: Add an overload that takes in count.
                  KSharedPtr<ConsolidatedStoreComponent<TKey, TValue>> newConsolidatedStateSPtr;
                  status = ConsolidatedStoreComponent<TKey, TValue>::Create(*consolidationProviderSPtr_->KeyComparerSPtr, this->GetThisAllocator(), newConsolidatedStateSPtr);
                  Diagnostics::Validate(status);
                   
                  // Create new aggregated state here. This method is the only writer, there can be no readers. Unlike managed, clear cannot contend here for writes.
                  status = AggregatedStoreComponent<TKey, TValue>::Create(*newConsolidatedStateSPtr, *consolidationProviderSPtr_->KeyComparerSPtr, *traceComponent_, this->GetThisAllocator(), newAggregatedStoreComponentSPtr_);
                  Diagnostics::Validate(status);

                  auto oldConsolidatedStateSPtr = cachedAggregatedComponentSPtr->GetConsolidatedState();
                  STORE_ASSERT(oldConsolidatedStateSPtr != nullptr, "oldConsolidatedStateSPtr != nullptr");

                  KSharedPtr<SharedPriorityQueue<KSharedPtr<DifferentialStateEnumerator<TKey, TValue>>>> priorityQueueSPtr = nullptr;

                  status = SharedPriorityQueue<KSharedPtr<DifferentialStateEnumerator<TKey, TValue>>>::Create(
                     CompareEnumerators,
                     this->GetThisAllocator(),
                     priorityQueueSPtr);

                  Diagnostics::Validate(status);

                  for (ULONG32 i = snapshotOfHighestIndexOnConsolidation_; i >= 1; i--)
                  {
                     KSharedPtr<DifferentialStoreComponent<TKey, TValue>> differentialState = nullptr;
                     bool result = cachedAggregatedComponentSPtr->DeltaDifferentialStateList->TryGetValue(i, differentialState);
                     STORE_ASSERT(result, "Delta list should contain value for this index: {1}", i);
                     STORE_ASSERT(differentialState != nullptr, "Diffrential state cannot be null");

                     KSharedPtr<DifferentialStateEnumerator<TKey, TValue>> diffStateEnumeratorSPtr = nullptr;
                     status = DifferentialStateEnumerator<TKey, TValue>::Create(
                        *differentialState,
                        *(consolidationProviderSPtr_->KeyComparerSPtr),
                        this->GetThisAllocator(),
                        diffStateEnumeratorSPtr);
                     Diagnostics::Validate(status);

                     if (diffStateEnumeratorSPtr->MoveNext())
                     {
                        priorityQueueSPtr->Push(diffStateEnumeratorSPtr);
                     }
                  }

                  auto consolidatedStateEnumeratorSPtr = oldConsolidatedStateSPtr->EnumerateEntries();
                  bool isConsolidatedStateDrained = !consolidatedStateEnumeratorSPtr->MoveNext();

                  KSharedPtr<DifferentialDataEnumerator<TKey, TValue>> differentialDataEnumeratorSPtr = nullptr;
                  status = DifferentialDataEnumerator<TKey, TValue>::Create(*priorityQueueSPtr, *(consolidationProviderSPtr_->KeyComparerSPtr), *this, *metadataTableSPtr, this->GetThisAllocator(), differentialDataEnumeratorSPtr);
                  Diagnostics::Validate(status);

                  STORE_ASSERT(differentialDataEnumeratorSPtr != nullptr, "differential enumerator cannot be null");

                   bool isDifferentialStateDrained = !differentialDataEnumeratorSPtr->MoveNext();

                  STORE_ASSERT(consolidatedStateEnumeratorSPtr != nullptr, "consolidated state enumerator cannot be null");

                  KSharedPtr<KSharedArray<KSharedPtr<VersionedItem<TValue>>>> versionedItems =
                     _new(CONSOLIDATIONMANAGER_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<VersionedItem<TValue>>>();
                  while (isDifferentialStateDrained == false && isConsolidatedStateDrained == false)
                  {
                     cancellationToken.ThrowIfCancellationRequested();
                     auto consolidatedStateEntry = consolidatedStateEnumeratorSPtr->Current();
                     TKey consolidatedStateKey = consolidatedStateEntry.Key;

                     auto differentialKeyValuePair = differentialDataEnumeratorSPtr->Current()->KeyAndVersionedItemPair;
                     auto oldDifferentialState = differentialDataEnumeratorSPtr->Current()->DifferentialStoreComponentSPtr;
                     TKey differntialStateKey = differentialKeyValuePair.Key;

                     int keyComparison = consolidationProviderSPtr_->KeyComparerSPtr->Compare(differntialStateKey, consolidatedStateKey);

                     // Key in consolidation is smaller than key in differential.
                     if (keyComparison > 0)
                     {
                        auto consolidatedVersionedItemSPtr = oldConsolidatedStateSPtr->Read(consolidatedStateKey);
                        STORE_ASSERT(consolidatedVersionedItemSPtr != nullptr, "consolidatedVersionedItemSPtr != nullptr");

                        // Note: On Hydrate, we load all the items including deleted because we could read in any order. Ignore adding them here.
                        if (consolidatedVersionedItemSPtr->GetRecordKind() != RecordKind::DeletedVersion)
                        {
                           newConsolidatedStateSPtr->Add(consolidatedStateKey, *consolidatedVersionedItemSPtr);
                        }

                        isConsolidatedStateDrained = !consolidatedStateEnumeratorSPtr->MoveNext();
                        continue;
                     }

                     // Key in differential is same as key in Consolidated.
                     else if (keyComparison == 0)
                     {
                        // Remove the value in consolidated.
                        auto valueInConsolidatedState = oldConsolidatedStateSPtr->Read(consolidatedStateKey);
                        STORE_ASSERT(valueInConsolidatedState != nullptr, "Old consolidated state must contain item");
                        versionedItems->Append(valueInConsolidatedState);

                        // Decrement number of valid entries.
                        if (consolidationProviderSPtr_->HasPersistedState)
                        {
                            FileMetadata::SPtr fileMetadataSPtr = nullptr;
                            if (!metadataTableSPtr->Table->TryGetValue(valueInConsolidatedState->GetFileId(), fileMetadataSPtr))
                            {
                                STORE_ASSERT(
                                    fileMetadataSPtr != nullptr,
                                    "Failed to find file metadata for versioned item in consolidated with file id {1}.",
                                    valueInConsolidatedState->GetFileId());
                            }

                            fileMetadataSPtr->DecrementValidEntries();
                        }

                        // Remove the value in differential.
                        auto differentialStateVersionsSPtr = oldDifferentialState->ReadVersions(differntialStateKey);
                        STORE_ASSERT(differentialStateVersionsSPtr != nullptr, "differential version should not be null");
                        auto currentVersionSPtr = differentialStateVersionsSPtr->CurrentVersionSPtr;
                        STORE_ASSERT(currentVersionSPtr != nullptr, "currentVersionSPtr != nullptr");
                        STORE_ASSERT(valueInConsolidatedState->GetVersionSequenceNumber() < currentVersionSPtr->GetVersionSequenceNumber(),
                           "valueInConsolidatedState->GetVersionSequenceNumber {1} < currentVersionSPtr->GetVersionSequenceNumber {2}",
                           valueInConsolidatedState->GetVersionSequenceNumber(),
                           currentVersionSPtr->GetVersionSequenceNumber());

                        auto previousVersion = differentialStateVersionsSPtr->PreviousVersionSPtr;
                        if (previousVersion != nullptr)
                        {
                           STORE_ASSERT(valueInConsolidatedState->GetVersionSequenceNumber() < previousVersion->GetVersionSequenceNumber(),
                              "valueInConsolidatedState->GetVersionSequenceNumber {1} < previousVersion->GetVersionSequenceNumber {2}",
                              valueInConsolidatedState->GetVersionSequenceNumber(),
                              previousVersion->GetVersionSequenceNumber());
                           versionedItems->Append(previousVersion);
                        }

                        versionedItems->Append(currentVersionSPtr);
                        ProcessToBeRemovedVersions(consolidatedStateKey, *versionedItems, *metadataTableSPtr);

                        if (currentVersionSPtr->GetRecordKind() != RecordKind::DeletedVersion)
                        {
                           newConsolidatedStateSPtr->Add(differntialStateKey, *currentVersionSPtr);
                        }

                        isConsolidatedStateDrained = !consolidatedStateEnumeratorSPtr->MoveNext();
                        isDifferentialStateDrained = !differentialDataEnumeratorSPtr->MoveNext();
                        continue;
                     }

                     // Key in Differential is smaller than key in Consolidated.
                     else if (keyComparison < 0)
                     {
                        // Move the value in differential to consolidated state.
                        auto differentialStateVersionsSPtr = oldDifferentialState->ReadVersions(differntialStateKey);
                        STORE_ASSERT(differentialStateVersionsSPtr != nullptr, "differentialStateVersionsSPtr != nullptr");

                        auto currentVersionSPtr = differentialStateVersionsSPtr->CurrentVersionSPtr;
                        STORE_ASSERT(currentVersionSPtr != nullptr, "currentVersionSPtr != nullptr");

                        auto previousVersionSPtr = differentialStateVersionsSPtr->PreviousVersionSPtr;
                        if (previousVersionSPtr != nullptr)
                        {
                           LONG64 previousVersionSequenceNumber = previousVersionSPtr->GetVersionSequenceNumber();
                           LONG64 currentVersionSequenceNumber = differentialStateVersionsSPtr->CurrentVersionSPtr->GetVersionSequenceNumber();
                           STORE_ASSERT(previousVersionSequenceNumber < currentVersionSequenceNumber, "Previous version lsn={1} should be less than current version lsn={2}", previousVersionSequenceNumber, currentVersionSequenceNumber);

                           versionedItems->Append(previousVersionSPtr);
                           versionedItems->Append(currentVersionSPtr);
                           ProcessToBeRemovedVersions(differntialStateKey, *versionedItems, *metadataTableSPtr);
                        }

                        if (differentialStateVersionsSPtr->CurrentVersionSPtr->GetRecordKind() != RecordKind::DeletedVersion)
                        {
                           newConsolidatedStateSPtr->Add(differntialStateKey, *(differentialStateVersionsSPtr->CurrentVersionSPtr));
                        }

                        isDifferentialStateDrained = !differentialDataEnumeratorSPtr->MoveNext();
                        continue;
                     }
                  }

                  if (isDifferentialStateDrained == true && isConsolidatedStateDrained == false)
                  {
                     do
                     {
                        auto consolidatedStateEntry = consolidatedStateEnumeratorSPtr->Current();
                        TKey consolidatedStateKey = consolidatedStateEntry.Key;
                        auto valueInConsolidatedStateSPtr = oldConsolidatedStateSPtr->Read(consolidatedStateKey);
                        STORE_ASSERT(valueInConsolidatedStateSPtr != nullptr, "valueInConsolidatedStateSPtr != nullptr");

                        if (valueInConsolidatedStateSPtr->GetRecordKind() != RecordKind::DeletedVersion)
                        {
                           newConsolidatedStateSPtr->Add(consolidatedStateKey, *valueInConsolidatedStateSPtr);
                        }

                        isConsolidatedStateDrained = !consolidatedStateEnumeratorSPtr->MoveNext();
                     } while (isConsolidatedStateDrained == false);
                  }
                  else if (isConsolidatedStateDrained == true && isDifferentialStateDrained == false)
                  {
                     do
                     {
                        STORE_ASSERT(versionedItems->Count() == 0, "versionedItems must be empty. count={1}", versionedItems->Count());
                        TKey differentialStateKey = differentialDataEnumeratorSPtr->Current()->KeyAndVersionedItemPair.Key;
                        auto oldDifferentialState = differentialDataEnumeratorSPtr->Current()->DifferentialStoreComponentSPtr;
                        auto differentialStateVersionsSPtr = oldDifferentialState->ReadVersions(differentialStateKey);
                        STORE_ASSERT(differentialStateVersionsSPtr != nullptr, "differentialStateVersionsSPtr != nullptr");

                        auto currentVersionSPtr = differentialStateVersionsSPtr->CurrentVersionSPtr;
                        STORE_ASSERT(currentVersionSPtr != nullptr, "(currentVersionSPtr != nullptr");

                        auto previousVersionSPtr = differentialStateVersionsSPtr->PreviousVersionSPtr;

                        if (previousVersionSPtr != nullptr)
                        {
                           STORE_ASSERT(currentVersionSPtr->GetVersionSequenceNumber() > previousVersionSPtr->GetVersionSequenceNumber(),
                              "currentVersionSPtr->GetVersionSequenceNumber {1} > previousVersionSPtr->GetVersionSequenceNumber {2}",
                              currentVersionSPtr->GetVersionSequenceNumber(),
                              previousVersionSPtr->GetVersionSequenceNumber());

                           versionedItems->Append(previousVersionSPtr);
                           versionedItems->Append(currentVersionSPtr);
                           ProcessToBeRemovedVersions(differentialStateKey, *versionedItems, *metadataTableSPtr);
                        }

                        if (currentVersionSPtr->GetRecordKind() != RecordKind::DeletedVersion)
                        {
                           newConsolidatedStateSPtr->Add(differentialStateKey, *currentVersionSPtr);
                        }

                        isDifferentialStateDrained = !differentialDataEnumeratorSPtr->MoveNext();
                     } while (isDifferentialStateDrained == false);
                  }

                  differentialDataEnumeratorSPtr = nullptr;

                  // Call merge before switching states

                  // If any files fall below the threshold, merge them together
                  KSharedArray<ULONG32>::SPtr mergeFileIds = nullptr;
                  if (
                      consolidationProviderSPtr_->HasPersistedState &&
                      co_await consolidationProviderSPtr_->MergeHelperSPtr->ShouldMerge(*metadataTableSPtr, mergeFileIds))
                  {
                     STORE_ASSERT(mergeFileIds != nullptr, "mergeFileIds != nullptr");
                     postMergeMetadataTableInformation = co_await MergeAsync(*metadataTableSPtr, *mergeFileIds, *newConsolidatedStateSPtr, perfCounters, cancellationToken);
                     STORE_ASSERT(postMergeMetadataTableInformation != nullptr, "Merge result cannot be null");
                     STORE_ASSERT(postMergeMetadataTableInformation->DeletedFileIdsSPtr != nullptr, "Deleted list cannot be null");
                  }

                  if (consolidationProviderSPtr_->EnableBackgroundConsolidation)
                  {
                     // It is safe to reset aggregated to new aggregated state here before next metadata table is set to temp metdata table.
                     // New file metadata comes in either from merge or perf checkpoint.
                     // File metadata that comes from perf checkpoint is always in memory, will never be swept so it is okay to set the temp metadata table to next after consolidation manager has been reset.
                     // Reads will not need to lookup the filemetadata (this is the state that comes from differential and is moved to new aggregared state).
                     // File metadata that comes from merge has been included to merge metadata table. Merge metadata table is included for lookup on reads.
                     ResetToNewAggregatedState();
                  }

                  completionSourceSPtr->SetResult(postMergeMetadataTableInformation);
                  co_return;
               }
               catch (const ktl::Exception & exception)
               {
                  completionSourceSPtr->SetException(exception);
                  co_return;
               }
            }

            void ResetToNewAggregatedState()
            {
               auto cachedAggregatedComponentSPtr_ = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregatedComponentSPtr_ != nullptr, "cachedAggregatedComponentSPtr_ != nullptr");

               // NewAggregatedState not null cannot be asserted since there can be cases when consolidation is not needed. Since this is co-ordintade by performcheckpoint,
               // not asserting here.

               if (newAggregatedStoreComponentSPtr_ != nullptr)
               {
                  K_LOCK_BLOCK(indexLock_)
                  {
                     // Fetch new delta states, if any from aggregatedstate, before restting it
                     for (ULONG32 i = snapshotOfHighestIndexOnConsolidation_ + 1; i <= cachedAggregatedComponentSPtr_->Index; i++)
                     {
                        KSharedPtr<DifferentialStoreComponent<TKey, TValue>> diffComponent = nullptr;
                        cachedAggregatedComponentSPtr_->DeltaDifferentialStateList->TryGetValue(i, diffComponent);
                        STORE_ASSERT(diffComponent != nullptr, "All indexes should be present");

                        // Add it in the correct order
                        newAggregatedStoreComponentSPtr_->AppendDeltaDifferentialState(*diffComponent);
                     }

                     aggregatedStoreComponentSPtr_.Put(Ktl::Move(newAggregatedStoreComponentSPtr_));
                     newAggregatedStoreComponentSPtr_ = nullptr;
                       
                     // Do not trigger sweep here. Background sweep will account for checkpoints
                  }
               }
            }

            void AppendDeltaDifferentialState(__in DifferentialStoreComponent<TKey, TValue>& deltaState)
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");;

               // Lock to protect against swapping with new aggregated state.
               K_LOCK_BLOCK(indexLock_)
               {
                  cachedAggregratedStoreComponentSPtr->AppendDeltaDifferentialState(deltaState);
               }

               if (cachedAggregratedStoreComponentSPtr->Index > NumberOfDeltasToBeConsolidated + 7)
               {
                  // Trace warning for a large difference with index which indicates that bg consolidation is making very slow progress.
                   StoreEventSource::Events->ConsolidationManagerSlowConsolidation(
                       traceComponent_->PartitionId, traceComponent_->TraceTag,
                       cachedAggregratedStoreComponentSPtr->Index,
                       Constants::DefaultNumberOfDeltasTobeConsolidated);
               }
            }

            void Add(__in TKey key, __in VersionedItem<TValue>& value)
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");;

               cachedAggregratedStoreComponentSPtr->GetConsolidatedState()->Add(key, value);
            }

            virtual bool ContainsKey(__in TKey& key) const override
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->ContainsKey(key);
            }

            virtual KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key) const override
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->Read(key);
            }

            KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key, __in LONG64 visbilityLsn) const
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->Read(key, visbilityLsn);
            }

            KSharedPtr<IEnumerator<TKey>> GetSortedKeyEnumerable(__in bool useFirstKey, __in TKey & firstKey, __in bool useLastKey, __in TKey & lastKey)
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->GetSortedKeyEnumerable(useFirstKey, firstKey, useLastKey, lastKey);
            }

            KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> GetAllKeysAndValuesEnumerator()
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->GetKeysAndValuesEnumerator();
            }

            LONG64 Count()
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->Count();
            }

            LONG64 GetMemorySize(__in LONG64 estimatedKeySize)
            {
               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->GetMemorySize(estimatedKeySize);
            }

            void AddToMemorySize(__in LONG64 size)
            {
               if (newAggregatedStoreComponentSPtr_ != nullptr)
               {
                  newAggregatedStoreComponentSPtr_->AddToMemorySize(size);
               }

               auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

               return cachedAggregratedStoreComponentSPtr->AddToMemorySize(size);
            }

            void ProcessToBeRemovedVersions(
               __in TKey & key,
               __in VersionedItem<TValue> & nextVersion,
               __in VersionedItem<TValue> & versionToBeDeleted,
               __in MetadataTable & metadataTable)
            {
               // Assert that the versionsequence is in the correct order.
               LONG64 sequenceNumberToBeDeleted = versionToBeDeleted.GetVersionSequenceNumber();
               LONG64 nextSequenceNumber = nextVersion.GetVersionSequenceNumber();

               STORE_ASSERT(sequenceNumberToBeDeleted < nextSequenceNumber, "sequenceNumber < nextSequenceNumber");

               TxnReplicator::TryRemoveVersionResult::SPtr removeVersionResult;
               NTSTATUS status = consolidationProviderSPtr_->TransactionalReplicatorSPtr->TryRemoveVersion(consolidationProviderSPtr_->StateProviderId, sequenceNumberToBeDeleted, nextSequenceNumber, removeVersionResult);

               Diagnostics::Validate(status);

               // Move to snapsot container, if needed
               if (!removeVersionResult->CanBeRemoved)
               {
                  auto set = removeVersionResult->EnumerationSet;
                  auto enumerator = set->GetEnumerator();

                  STORE_ASSERT(enumerator != nullptr, "enumerator != nullptr");

                  while (enumerator->MoveNext())
                  {
                     LONG64 snapshotVisibilityLsn = enumerator->Current();

                     KSharedPtr<SnapshotComponent<TKey, TValue>> snapshotComponentSPtr = consolidationProviderSPtr_->SnapshotContainerSPtr->Read(snapshotVisibilityLsn);

                     if (snapshotComponentSPtr == nullptr)
                     {
                        status = SnapshotComponent<TKey, TValue>::Create(
                            *consolidationProviderSPtr_->SnapshotContainerSPtr, 
                            consolidationProviderSPtr_->IsValueAReferenceType, 
                            *consolidationProviderSPtr_->KeyComparerSPtr, 
                            *traceComponent_,
                            this->GetThisAllocator(), 
                            snapshotComponentSPtr);
                        Diagnostics::Validate(status);

                        snapshotComponentSPtr = consolidationProviderSPtr_->SnapshotContainerSPtr->GetOrAdd(snapshotVisibilityLsn, *snapshotComponentSPtr);
                     }

                     // Move file id for the versioned item, if it is not already present.
                     auto fileId = versionToBeDeleted.GetFileId();
                     if (fileId > 0)
                     {
                        FileMetadata::SPtr fileMetadata = nullptr;
                        if (metadataTable.Table->TryGetValue(fileId, fileMetadata))
                        {
                           STORE_ASSERT(fileMetadata != nullptr, "Failed to find the file metadata for a versioned item with file id {1}.", fileId);
                        }

                        // It is okay to not assert here that the lsn got added because duplicate adds could come in.
                        consolidationProviderSPtr_->SnapshotContainerSPtr->TryAddFileMetadata(snapshotVisibilityLsn, *fileMetadata);
                     }
                     else
                     {
                        // Should always be in memory, so safe to assert on value directly.
                        // This can happen only with previous version items from differential state.
                        // todo
                     }

                     STORE_ASSERT(snapshotComponentSPtr != nullptr, "snapshotComponentSPtr != nullptr");

                     // Add the key to the component
                     snapshotComponentSPtr->Add(key, versionToBeDeleted);
                  }

                  // Wait on notifications and remove the entry from the container.
                  if (removeVersionResult->EnumerationCompletionNotifications)
                  {
                     auto lEnumerator = removeVersionResult->EnumerationCompletionNotifications->GetEnumerator();

                     while (lEnumerator->MoveNext())
                     {
                        auto enumerationResult = lEnumerator->Current();
                        STORE_ASSERT(enumerationResult != nullptr, "(enumerationResult != nullptr");
                        ktl::Task task = RemoveFromContainer(*enumerationResult);
                        STORE_ASSERT(task.IsTaskStarted(), "Expected snapshot remove from container task to start");
                     }
                  }
               }
            }

            ULONG32 SweepConsolidatedState(__in ktl::CancellationToken const & cancellationToken)
            {
               cancellationToken.ThrowIfCancellationRequested();
               ULONG32 sweepCount = 0;
               auto cachedAggregatedComponentSPtr = aggregatedStoreComponentSPtr_.Get();
               STORE_ASSERT(cachedAggregatedComponentSPtr != nullptr, "cachedNewAggregatedComponentSPtr == nullptr");

               auto consolidatedState = cachedAggregatedComponentSPtr->GetConsolidatedState();
               auto consolidatedComponentEnumerator = consolidatedState->EnumerateEntries();

               // Iterate the differential list and consolidated list
               while (consolidatedComponentEnumerator->MoveNext())
               {
                  cancellationToken.ThrowIfCancellationRequested();
                  auto item = consolidatedComponentEnumerator->Current();
                  auto versionedItem = item.Value;

                  if (versionedItem->GetRecordKind() != RecordKind::DeletedVersion)
                  {
                     bool swept = false;
                     versionedItem->AcquireLock();

                     KFinally([&]
                     {
                        versionedItem->ReleaseLock(*traceComponent_);
                     });

                     if (versionedItem->IsInMemory() == true)
                     {
                        swept = SweepItem(*versionedItem);
                     }

                     if (swept)
                     {
                        sweepCount++;
                        consolidatedState->DecrementSize(versionedItem->GetValueSize());
                     }
                  }
               }

               // Iterate through delta differential states here
               KSharedPtr<SweepEnumerator<TKey, TValue>> valuesForSweepEnumeratorSPtr = nullptr;
               NTSTATUS status = SweepEnumerator<TKey, TValue>::Create(
                   *cachedAggregatedComponentSPtr->DeltaDifferentialStateList, 
                   cachedAggregatedComponentSPtr->Index, 
                   this->GetThisAllocator(), 
                   *traceComponent_,
                   valuesForSweepEnumeratorSPtr);
               Diagnostics::Validate(status);
               while (valuesForSweepEnumeratorSPtr->MoveNext())
               {
                  cancellationToken.ThrowIfCancellationRequested();
                  auto versionedItem = valuesForSweepEnumeratorSPtr->Current();
                  if (versionedItem->GetRecordKind() != RecordKind::DeletedVersion)
                  {
                     bool swept = false;
                     versionedItem->AcquireLock();

                     KFinally([&]
                     {
                        versionedItem->ReleaseLock(*traceComponent_);
                     });

                     if (versionedItem->IsInMemory() == true)
                     {
                        swept = SweepItem(*versionedItem);
                     }

                     if (swept)
                     {
                        sweepCount++;
                        auto diffComponentSPtr = valuesForSweepEnumeratorSPtr->CurrentComponentSPtr;
                        diffComponentSPtr->DecrementSize(versionedItem->GetValueSize());
                     }
                  }
               }

               return sweepCount;
            }

        private:
            ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> MergeAsync(
                __in MetadataTable & mergeTable,
                __in KSharedArray<ULONG32> & listOfFileIds, 
                __in ConsolidatedStoreComponent<TKey, TValue> & newConsolidatedState,
                __in StorePerformanceCountersSPtr & perfCounters,
                __in ktl::CancellationToken const cancellationToken)
            {
                StoreEventSource::Events->ConsolidationManagerMergeAsync(traceComponent_->PartitionId, traceComponent_->TraceTag, L"started");
                
                PostMergeMetadataTableInformation::SPtr mergeMetadataTableInformationSPtr = nullptr;
                KArray<KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>> enumerators(this->GetThisAllocator());
                SharedException::CSPtr exceptionCSPtr;


                ktl::io::KFileStream::SPtr keyFileStreamSPtr = nullptr;
                ktl::io::KFileStream::SPtr valueFileStreamSPtr = nullptr;
                KeyCheckpointFile::SPtr keyFileSPtr = nullptr;
                ValueCheckpointFile::SPtr valueFileSPtr = nullptr;
                bool isOpened = false;
                bool fileIsEmpty = true;

                ktl::CancellationToken snappedToken = cancellationToken; // To get around compiler bug
                
                try 
                {
                    MetadataTable::SPtr mergeTableSPtr = &mergeTable;
                    KSharedPtr<ConsolidatedStoreComponent<TKey, TValue>> newConsolidatedStateSPtr = &newConsolidatedState;
                    KSharedArray<ULONG32>::SPtr listOfFileIdsSPtr = &listOfFileIds;

                    // TODO: trace
                    FileMetadata::SPtr mergedFileMetadataSPtr = nullptr;
                    
                    KPriorityQueue<KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>> priorityQueue(
                        this->GetThisAllocator(),
                        KeyCheckpointFileAsyncEnumerator<TKey, TValue>::CompareEnumerators);

                    // Get Enumerators for each file from the MetadataTable
                    for (ULONG32 i = 0; i < listOfFileIdsSPtr->Count(); i++)
                    {
                        auto fileId = (*listOfFileIdsSPtr)[i];
                        FileMetadata::SPtr fileMetadataSPtr = nullptr;
                        bool found = mergeTableSPtr->Table->TryGetValue(fileId, fileMetadataSPtr);
                        STORE_ASSERT(found, "fileId {1} should be in merge table", fileId);

                        auto enumeratorSPtr = fileMetadataSPtr->CheckpointFileSPtr->GetAsyncEnumerator<TKey, TValue>(*consolidationProviderSPtr_->KeyConverterSPtr);
                        STORE_ASSERT(enumeratorSPtr != nullptr, "key checkpoint file enumerator should not be null");
                        enumeratorSPtr->KeyComparerSPtr = *consolidationProviderSPtr_->KeyComparerSPtr;

                        // Prime the enumerator so it can be compared
                        auto hasNext = co_await enumeratorSPtr->MoveNextAsync(cancellationToken);
                        STORE_ASSERT(hasNext, "enumerator should not be empty");

                        // Merge cannot be interrupted before enumerator is added here so races here can be ignored
                        auto status = enumerators.Append(enumeratorSPtr);
                        STORE_ASSERT(NT_SUCCESS(status), "unable to append enumerator to list of enumerators");
                        priorityQueue.Push(enumeratorSPtr);
                    }

                    // Start writing a new filename
                    auto logicalTimeStamp = consolidationProviderSPtr_->IncrementFileStamp();
                    
                    KString::SPtr fileNameSPtr = nullptr;

                    auto fileId = co_await CreateNewCheckpointFilesAsync(fileNameSPtr, keyFileSPtr, valueFileSPtr);

                    keyFileStreamSPtr = co_await keyFileSPtr->StreamPoolSPtr->AcquireStreamAsync();
                    valueFileStreamSPtr = co_await valueFileSPtr->StreamPoolSPtr->AcquireStreamAsync();
                    isOpened = true;

                    SharedBinaryWriter::SPtr keyMemoryBufferSPtr = nullptr;
                    NTSTATUS status = SharedBinaryWriter::Create(this->GetThisAllocator(), keyMemoryBufferSPtr);
                    Diagnostics::Validate(status);

                    SharedBinaryWriter::SPtr valueMemoryBufferSPtr = nullptr;
                    status = SharedBinaryWriter::Create(this->GetThisAllocator(), valueMemoryBufferSPtr);
                    Diagnostics::Validate(status);

                    KSharedPtr<BlockAlignedWriter<TKey, TValue>> blockAlignedWriterSPtr = nullptr;
                    status = BlockAlignedWriter<TKey, TValue>::Create(
                        *valueFileSPtr,
                        *keyFileSPtr,
                        *valueMemoryBufferSPtr,
                        *keyMemoryBufferSPtr,
                        *valueFileStreamSPtr,
                        *keyFileStreamSPtr,
                        logicalTimeStamp,
                        *consolidationProviderSPtr_->KeyConverterSPtr,
                        *consolidationProviderSPtr_->ValueConverterSPtr,
                        *traceComponent_,
                        this->GetThisAllocator(),
                        blockAlignedWriterSPtr);
                    Diagnostics::Validate(status);

                    CheckpointPerformanceCounterWriter checkpointPerfCounterWriter(perfCounters);

                    while (!priorityQueue.IsEmpty())
                    {
                        snappedToken.ThrowIfCancellationRequested();
                        KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> enumeratorToWriteSPtr = nullptr;
                        bool popped = priorityQueue.Pop(enumeratorToWriteSPtr);
                        STORE_ASSERT(popped, "priority queue should not be empty");
                        auto enumeratorSPtr = enumeratorToWriteSPtr;

                        auto keyToWrite = enumeratorSPtr->GetCurrent()->Key;
                        auto valueToWriteSPtr = enumeratorSPtr->GetCurrent()->Value;
                        LONG64 timestampForValueToWrite = enumeratorSPtr->GetCurrent()->LogicalTimeStamp;

                        // Ignore all duplicate keys with smaller LSNs
                        while (true)
                        {
                            if (priorityQueue.IsEmpty())
                            {
                                break;
                            }

                            KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> peekedEnumerator = nullptr;
                            bool peeked = priorityQueue.Peek(peekedEnumerator);
                            STORE_ASSERT(peeked, "priority queue should not be empty");
                            STORE_ASSERT(peekedEnumerator != nullptr, "peeked enumerator should not be null");
                            auto peekedKey = peekedEnumerator->GetCurrent()->Key;
                            if (consolidationProviderSPtr_->KeyComparerSPtr->Compare(peekedKey, keyToWrite) != 0)
                            {
                                break;
                            }

                            KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> poppedEnumerator = nullptr;
                            popped = priorityQueue.Pop(poppedEnumerator);
                            STORE_ASSERT(popped, "priority queue should not be empty");
                            STORE_ASSERT(poppedEnumerator != nullptr, "popped enumerator should not be null");

                            // Verify the item being skipped has an earlier LSN than the one being returned.
                            auto skipLsn = poppedEnumerator->GetCurrent()->Value->GetVersionSequenceNumber();
                            STORE_ASSERT(skipLsn <= valueToWriteSPtr->GetVersionSequenceNumber(), "Enumeration is returning a key with an earlier LSN. skipLsn={1} lsn={2}", skipLsn, valueToWriteSPtr->GetVersionSequenceNumber());

                            if (co_await poppedEnumerator->MoveNextAsync(snappedToken))
                            {
                                status = priorityQueue.Push(poppedEnumerator);
                                STORE_ASSERT(NT_SUCCESS(status), "unable to push enumerator to priority queue");
                            }
                            else
                            {
                                co_await poppedEnumerator->CloseAsync();
                            }
                        }

                        auto latestValueSPtr = newConsolidatedStateSPtr->Read(keyToWrite);
                        bool shouldKeyBeWritten = false;
                        bool shouldWriteSerializedValue = false;
                        KBuffer::SPtr serializedValueSPtr = nullptr;

                        if (latestValueSPtr == nullptr)
                        {
                            // Check if needs to be written by checking the metadata table to see if it contains
                            auto mergeTableEnumeratorSPtr = mergeTableSPtr->Table->GetEnumerator();
                            while (mergeTableEnumeratorSPtr->MoveNext())
                            {
                                auto currentItem = mergeTableEnumeratorSPtr->Current();
                                auto fMetadataSPtr = currentItem.Value;

                                if (fMetadataSPtr->LogicalTimeStamp < timestampForValueToWrite)
                                {
                                    // Assert that it is deleted item because non-zero logical time stamp mean deleted item only
                                    STORE_ASSERT(
                                        valueToWriteSPtr->GetRecordKind() == RecordKind::DeletedVersion,
                                        "should be deleted item. lsn={1} kind={2}",
                                        valueToWriteSPtr->GetVersionSequenceNumber(),
                                        static_cast<int>(valueToWriteSPtr->GetRecordKind()));

                                    // If fileid is part of the merge list, then skip writing the delete key onto the merged file
                                    if (!ListContainsId(*listOfFileIdsSPtr, fMetadataSPtr->FileId))
                                    {
                                        // If there is any file with a logical time stamp lesser than the time stamp of the delete record 
                                        // and it is not part of the merge list, it should be written
                                        shouldKeyBeWritten = true;
                                        break;
                                    }
                                }
                            }
                        }
                        else if (latestValueSPtr->GetFileId() == valueToWriteSPtr->GetFileId())
                        {
                            //assert that the lsns match
                            auto latestLSN = latestValueSPtr->GetVersionSequenceNumber();
                            auto valueLSN = valueToWriteSPtr->GetVersionSequenceNumber();
                            STORE_ASSERT(latestLSN == valueLSN, "Merged items lsn's do not match. Latest value lsn {1}, value to write lsn {2}", latestLSN, valueLSN);
                            shouldKeyBeWritten = true;

                            if (valueToWriteSPtr->GetRecordKind() != RecordKind::DeletedVersion)
                            {
                                // Read value for key
                                bool shouldLoadValue = false;

                                {
                                    latestValueSPtr->AcquireLock();
                                    KFinally([&] { latestValueSPtr->ReleaseLock(*traceComponent_); });

                                    if (latestValueSPtr->IsInMemory())
                                    {
                                        // Do not load from disk if you don't need to!
                                        valueToWriteSPtr->SetValue(latestValueSPtr->GetValue());
                                    }
                                    else
                                    {
                                        shouldLoadValue = true;
                                    }
                                }

                                if (shouldLoadValue)
                                {
                                    shouldWriteSerializedValue = true;
                                    // Load the serialized value into memory from disk (do not deserialize it)
                                    serializedValueSPtr = co_await MetadataManager::ReadValueAsync<TValue>(*mergeTableSPtr, *valueToWriteSPtr);
                                }

                            }
                        }
            
                        if (shouldKeyBeWritten)
                        {
                            fileIsEmpty = false;

                            KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> kvpToWrite(keyToWrite, valueToWriteSPtr);

                            checkpointPerfCounterWriter.StartMeasurement();

                            // Write the value
                            co_await blockAlignedWriterSPtr->BlockAlignedWriteItemAsync(
                                kvpToWrite,
                                serializedValueSPtr,
                                !shouldWriteSerializedValue);
                            
                            checkpointPerfCounterWriter.StopMeasurement();

                            if (kvpToWrite.Value->GetRecordKind() != RecordKind::DeletedVersion)
                            {
                                // Copy-on-write the versioned value in-memory into the next consolidated state, to avoid taking locks.
                                // TODO: check on perf testing for this allocation
                                newConsolidatedStateSPtr->Update(keyToWrite, *valueToWriteSPtr);
                            }
                        }

                        // Move the enumerator to the next key and add it back to the heap.
                        if (co_await enumeratorToWriteSPtr->MoveNextAsync(snappedToken))
                        {
                            priorityQueue.Push(enumeratorToWriteSPtr);
                        }
                        else
                        {
                            co_await enumeratorToWriteSPtr->CloseAsync();
                        }

                        ktl::AwaitableCompletionSource<bool>::SPtr testDelaySPtr = consolidationProviderSPtr_->TestDelayOnConsolidationSPtr;
                        if (testDelaySPtr != nullptr)
                        {
                            co_await testDelaySPtr->GetAwaitable();
                        }
                    }

                    if (!fileIsEmpty)
                    {
                       auto fullFileNameSPtr = CombinePaths(*consolidationProviderSPtr_->WorkingDirectoryCSPtr, *fileNameSPtr, L"");

                       StoreEventSource::Events->ConsolidationManagerMergeFile(traceComponent_->PartitionId, traceComponent_->TraceTag, ToStringLiteral(*fullFileNameSPtr));
                        
                       checkpointPerfCounterWriter.StartMeasurement();

                       // Flush both key and value checkpoints to disk
                       co_await blockAlignedWriterSPtr->FlushAsync();

                       checkpointPerfCounterWriter.StopMeasurement();

                       CheckpointFile::SPtr checkpointFileSPtr = nullptr;
                       status = CheckpointFile::Create(*fullFileNameSPtr, *keyFileSPtr, *valueFileSPtr, *traceComponent_, this->GetThisAllocator(), checkpointFileSPtr);
                       Diagnostics::Validate(status);

                       status = FileMetadata::Create(
                          fileId,
                          *fileNameSPtr,
                          checkpointFileSPtr->KeyCount,
                          checkpointFileSPtr->KeyCount,
                          logicalTimeStamp,
                          checkpointFileSPtr->DeletedKeyCount,
                          false,
                          this->GetThisAllocator(),
                          *traceComponent_,
                          mergedFileMetadataSPtr);

                       Diagnostics::Validate(status);

                       mergedFileMetadataSPtr->CheckpointFileSPtr = *checkpointFileSPtr;

                       ULONG64 writeBytes = co_await checkpointFileSPtr->GetTotalFileSizeAsync(this->GetThisAllocator());
                       ULONG64 writeBytesPerSecond = checkpointPerfCounterWriter.UpdatePerformanceCounter(writeBytes);

                       StoreEventSource::Events->CheckpointFileWriteBytesPerSec(
                           traceComponent_->PartitionId, traceComponent_->TraceTag,
                           writeBytesPerSecond);
                    }

                    KSharedArray<ULONG32>::SPtr deletedFileIds = _new(CONSOLIDATIONMANAGER_TAG, this->GetThisAllocator()) KSharedArray<ULONG32>();
                    Diagnostics::Validate(deletedFileIds);
                    for (ULONG32 i = 0; i < listOfFileIdsSPtr->Count(); i++)
                    {
                        status = deletedFileIds->Append((*listOfFileIdsSPtr)[i]);
                        STORE_ASSERT(NT_SUCCESS(status), "unable to append file id to deleted file ids list");
                    }

                    if (mergedFileMetadataSPtr != nullptr)
                    {
                        status = PostMergeMetadataTableInformation::Create(*deletedFileIds, mergedFileMetadataSPtr, this->GetThisAllocator(), mergeMetadataTableInformationSPtr);
                        Diagnostics::Validate(status);

                        MetadataTable::SPtr mergedMetadataTableSPtr;
                        MetadataTable::Create(this->GetThisAllocator(), mergedMetadataTableSPtr);
                        mergedMetadataTableSPtr->Table->Add(mergedFileMetadataSPtr->FileId, mergedFileMetadataSPtr);
                        consolidationProviderSPtr_->MergeMetadataTableSPtr = mergedMetadataTableSPtr;
                    }
                    else
                    {
                        // there could be deleted filed ids w/o a new merged file depending on invalid entries
                        status = PostMergeMetadataTableInformation::Create(*deletedFileIds, nullptr, this->GetThisAllocator(), mergeMetadataTableInformationSPtr);
                        Diagnostics::Validate(status);
                    }
                }
                catch (ktl::Exception const & e)
                {
                    exceptionCSPtr = SharedException::Create(e, this->GetThisAllocator());
                }

                for (ULONG32 i = 0; i < enumerators.Count(); i++)
                {
                    co_await enumerators[i]->CloseAsync();
                }

                if (isOpened)
                {
                   co_await keyFileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*keyFileStreamSPtr);
                   co_await valueFileSPtr->StreamPoolSPtr->ReleaseStreamAsync(*valueFileStreamSPtr);

                   if (fileIsEmpty || exceptionCSPtr != nullptr)
                   {
                       co_await keyFileSPtr->CloseAsync();
                       co_await valueFileSPtr->CloseAsync();
                   }

                   if (fileIsEmpty)
                   {
                       Common::File::Delete(keyFileSPtr->FileName->operator LPCWSTR(), true);
                       Common::File::Delete(valueFileSPtr->FileName->operator LPCWSTR(), true);
                   }
                }

                // TODO: trace
                if (exceptionCSPtr != nullptr)
                {
                    auto exec = exceptionCSPtr->Info;
                    throw exec;
                }

                STORE_ASSERT(mergeMetadataTableInformationSPtr != nullptr, "mergeMetadataTableInformationSPtr != nullptr");
                STORE_ASSERT(mergeMetadataTableInformationSPtr->DeletedFileIdsSPtr != nullptr, "mergeMetadataTableInformationSPtr->DeletedFileIdsSPtr != nullptr");

                StoreEventSource::Events->ConsolidationManagerMergeAsync(traceComponent_->PartitionId, traceComponent_->TraceTag, L"completed");

                co_return mergeMetadataTableInformationSPtr;
            }

           void MovePreviousVersionItemsToSnapshotContainerIfNeeded(__in ULONG32 highestIndex, __in MetadataTable& metadataTable)
           {
              KSharedPtr<DifferentialStoreComponent<TKey, TValue>> deltaDifferentialState = nullptr;

              auto cachedAggregratedStoreComponentSPtr = aggregatedStoreComponentSPtr_.Get();
              STORE_ASSERT(cachedAggregratedStoreComponentSPtr != nullptr, "cachedAggregratedStoreComponentSPtr != nullptr");

              bool result = cachedAggregratedStoreComponentSPtr->DeltaDifferentialStateList->TryGetValue(highestIndex, deltaDifferentialState);

              STORE_ASSERT(result == true, "differential state should be present at index {1}", highestIndex);
              STORE_ASSERT(deltaDifferentialState != nullptr, "deltaDifferentialState != nullptr");

              auto enumerator = deltaDifferentialState->GetKeys();

              // Preethas: TODO, move this under checkpoint file that enumerates over the key and value while writing into checkpoint file 
              // to avoid enumerating again.
              while (enumerator->MoveNext())
              {
                 TKey key = enumerator->Current();
                 KSharedPtr<DifferentialStateVersions<TValue>> diffStateVersions = deltaDifferentialState->ReadVersions(key);
                 if (diffStateVersions->PreviousVersionSPtr)
                 {
                    ProcessToBeRemovedVersions(key, *diffStateVersions->CurrentVersionSPtr, *diffStateVersions->PreviousVersionSPtr, metadataTable);
                    diffStateVersions->SetPreviousVersion(nullptr);
                 }
              }
           }

           void ProcessToBeRemovedVersions(
               __in TKey & key,
               __in KSharedArray<KSharedPtr<VersionedItem<TValue>>> & versionedItems,
               __in MetadataTable & metadataTable)
           {
               STORE_ASSERT(versionedItems.Count() >= 2, "versionedItems.Count -{1} >= 2", versionedItems.Count());
               for (ULONG32 idx = 0; idx < versionedItems.Count() - 1; idx++)
               {
                   auto versionToBeDeleted = versionedItems[idx];
                   auto nextVersion = versionedItems[idx + 1];

                   ProcessToBeRemovedVersions(key, *nextVersion, *versionToBeDeleted, metadataTable);
               }

               versionedItems.Clear();
           }

           ktl::Task RemoveFromContainer(__in TxnReplicator::EnumerationCompletionResult & completionResult)
           {
              KShared$ApiEntry();
              TxnReplicator::EnumerationCompletionResult::SPtr completionSPtr = &completionResult;

              try
              {
                  ktl::Awaitable<LONG64> notification = completionSPtr->Notification;
                  LONG64 visibilitySequenceNumber = co_await notification;
                  co_await consolidationProviderSPtr_->SnapshotContainerSPtr->RemoveAsync(visibilitySequenceNumber);

                  co_return;
              }
              catch (ktl::Exception const & e)
              {
                  AssertException(L"RemoveFromContainer", e);
              }
           }

           bool SweepItem(VersionedItem<TValue> & item)
           {
               STORE_ASSERT(item.GetRecordKind() != RecordKind::DeletedVersion, "A deleted item kind is not expected to be swept");
               STORE_ASSERT(item.GetFileId() > 0, "An item qualified to be swept should have a valid file id");

               // TODO: This assumes that sweep only applies to SPtr

               // TODO: Check if file ID is present in metadatatable (debug only)
               
               if (item.GetInUse())
               {
                   // Do not assert that value is not null because a reader sets the Inuse flag first before it loads the value, so the value can be null.
                   item.SetInUse(false);
                   return false;
               }
            
               item.UnSetValue();

               return true;
           }

           KString::SPtr CreateGUIDString()
           {
               KGuid guid;
               guid.CreateNew();
               KString::SPtr stringSPtr = nullptr;
               NTSTATUS status = KString::Create(stringSPtr, this->GetThisAllocator(), KStringView::MaxGuidString);
               Diagnostics::Validate(status);
               BOOLEAN result = stringSPtr->FromGUID(guid);
               STORE_ASSERT(result == TRUE, "failed to load GUID into string");
               stringSPtr->SetNullTerminator();
               return stringSPtr;
           }

           ktl::Awaitable<ULONG32> CreateNewCheckpointFilesAsync(
               __out KString::SPtr & fileNameSPtr,
               __out KeyCheckpointFile::SPtr & keyFileSPtr, 
               __out ValueCheckpointFile::SPtr & valueFileSPtr)
           {
               auto fileId = consolidationProviderSPtr_->IncrementFileId();

               fileNameSPtr = CreateGUIDString();
               auto keyFileNameSPtr = CombinePaths(*consolidationProviderSPtr_->WorkingDirectoryCSPtr, *fileNameSPtr, KeyCheckpointFile::GetFileExtension());
               auto valueFileNameSPtr = CombinePaths(*consolidationProviderSPtr_->WorkingDirectoryCSPtr, *fileNameSPtr, ValueCheckpointFile::GetFileExtension());

               keyFileSPtr = co_await KeyCheckpointFile::CreateAsync(*traceComponent_, *keyFileNameSPtr, consolidationProviderSPtr_->IsValueAReferenceType, fileId, this->GetThisAllocator());
               valueFileSPtr = co_await ValueCheckpointFile::CreateAsync(*traceComponent_, *valueFileNameSPtr, fileId, this->GetThisAllocator());

               co_return fileId;
           }

           KString::SPtr CombinePaths(__in KStringView const & directory, __in KStringView const & filename, __in KStringView const & extension)
           {
               KString::SPtr filePathSPtr;
               auto status = KString::Create(filePathSPtr, this->GetThisAllocator(), L"");
               Diagnostics::Validate(status);
               bool result = filePathSPtr->Concat(directory);
               STORE_ASSERT(result, "Unable to concat directory to path string");
               result = filePathSPtr->Concat(Common::Path::GetPathSeparatorWstr().c_str());
               STORE_ASSERT(result, "Unable to concat path separator to path string");
               result = filePathSPtr->Concat(filename);
               STORE_ASSERT(result, "Unable to concat filename to path string");
               result = filePathSPtr->Concat(extension);
               STORE_ASSERT(result, "Unable to concat extension to path string");

               return filePathSPtr;
           }

           bool ListContainsId(__in KSharedArray<ULONG32> & list, __in ULONG32 item)
           {
               for (ULONG32 i = 0; i < list.Count(); i++)
               {
                   if (list[i] == item)
                   {
                       return true;
                   }
               }

               return false;
           }

           void AssertException(__in KStringView const & methodName, __in ktl::Exception const & exception)
           {
               KDynStringA stackString(this->GetThisAllocator());
               Diagnostics::GetExceptionStackTrace(exception, stackString);
               STORE_ASSERT(
                   false,
                   "UnexpectedException: Message: {2} Code:{4}\nStack: {3}",
                   ToStringLiteral(methodName),
                   ToStringLiteral(stackString),
                   exception.GetStatus());
           }

           ConsolidationManager(
               __in IConsolidationProvider<TKey, TValue> & consolidationProvider, 
               __in StoreTraceComponent & traceComponent);

            KSharedPtr<IConsolidationProvider<TKey, TValue>> consolidationProviderSPtr_;
            ThreadSafeSPtrCache<AggregatedStoreComponent<TKey, TValue>> aggregatedStoreComponentSPtr_;
            KSharedPtr<AggregatedStoreComponent<TKey, TValue>> newAggregatedStoreComponentSPtr_;
            KSpinLock indexLock_;
            ULONG32 numberOfDeltasToBeConsolidated_;
            ULONG32 snapshotOfHighestIndexOnConsolidation_;

            StoreTraceComponent::SPtr traceComponent_;
        };

        template <typename TKey, typename TValue>
        ConsolidationManager<TKey, TValue>::ConsolidationManager(
            __in IConsolidationProvider<TKey, TValue> & consolidationProvider, 
            __in StoreTraceComponent & traceComponent) :
           traceComponent_(&traceComponent),
           consolidationProviderSPtr_(&consolidationProvider),
           aggregatedStoreComponentSPtr_(nullptr),
           newAggregatedStoreComponentSPtr_(nullptr),
           numberOfDeltasToBeConsolidated_(Constants::DefaultNumberOfDeltasTobeConsolidated)
        {
           KSharedPtr<AggregatedStoreComponent<TKey, TValue>> aggregatedStoreComponentSPtr = nullptr;
           NTSTATUS status = AggregatedStoreComponent<TKey, TValue>::Create(*consolidationProviderSPtr_->KeyComparerSPtr, traceComponent, this->GetThisAllocator(), aggregatedStoreComponentSPtr);
           if (!NT_SUCCESS(status))
           {
              this->SetConstructorStatus(status);
              return;
           }

           aggregatedStoreComponentSPtr_.Put(Ktl::Move(aggregatedStoreComponentSPtr));
        }

        template <typename TKey, typename TValue>
        ConsolidationManager<TKey, TValue>::~ConsolidationManager()
        {
        }
    }
}

