// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define RECOVERY_COMPONENT_TAG 'RSCT'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class RecoveryStoreComponent
            : public KObject<RecoveryStoreComponent<TKey, TValue>>
            , public KShared<RecoveryStoreComponent<TKey, TValue>>
        {
            K_FORCE_SHARED(RecoveryStoreComponent)

        public:
            static NTSTATUS
                Create(
                    __in MetadataTable& metadataTable,
                    __in KStringView const& workDirectory,
                    __in StoreTraceComponent & traceComponent,
                    __in IComparer<TKey>& keyComparer,
                    __in Data::StateManager::IStateSerializer<TKey>& keyConverter,
                    __in bool isValueReferenceType,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(RECOVERY_COMPONENT_TAG, allocator) RecoveryStoreComponent(metadataTable, workDirectory, traceComponent, keyComparer, keyConverter, isValueReferenceType);

                if (!output)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }
                
                result = Ktl::Move(output);
                return status;
            }

            __declspec(property(get = get_FileId)) ULONG32 FileId;
            ULONG32 get_FileId() const
            {
                return fileId_;
            }

            __declspec(property(get = get_TotalKeyCount)) LONG64 TotalKeyCount;
            LONG64 get_TotalKeyCount() const
            {
                return totalKeyCount_;
            }

            __declspec(property(get = get_TotalKeySize)) LONG64 TotalKeySize;
            LONG64 get_TotalKeySize() const
            {
                return totalKeySize_;
            }

            __declspec(property(get = get_LogicalCheckpointFileTimeStamp)) LONG64 LogicalCheckpointFileTimeStamp;
            LONG64 get_LogicalCheckpointFileTimeStamp() const
            {
               STORE_ASSERT(logicalCheckpointFileTimeStamp_ >= 0, "LogicalCheckpointFileTimeStamp must be greater than or equal to 0. timestamp={1}", logicalCheckpointFileTimeStamp_);
               return logicalCheckpointFileTimeStamp_;
            }

            KSharedPtr<RecoveryStoreEnumerator<TKey, TValue>> GetEnumerable()
            {
                KSharedPtr<RecoveryStoreEnumerator<TKey, TValue>> enumeratorSPtr;
                RecoveryStoreEnumerator<TKey, TValue>::Create(componentSPtr_, this->GetThisAllocator(), enumeratorSPtr);

                return enumeratorSPtr;
            }

            ktl::Awaitable<void> RecoverAsync(__in ktl::CancellationToken const & cancellationToken)
            {
               STORE_ASSERT(metadataTableSPtr_ != nullptr, "metadataTable cannot be null");
               STORE_ASSERT(metadataTableSPtr_->Table != nullptr, "metadataTable.Table cannot be null");

               IDictionary<ULONG32, FileMetadata::SPtr>::SPtr table = metadataTableSPtr_->Table;
               KSharedPtr<KSharedArray<KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>>> keyCheckpointFileListSPtr =
                  _new(RECOVERY_COMPONENT_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>>();
               keyCheckpointFileListSPtr->Reserve(table->Count);

               KSharedPtr<IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>> enumeratorSPtr = table->GetEnumerator();
               SharedException::CSPtr exception = nullptr;

               try
               {
                   while (enumeratorSPtr->MoveNext())
                   {
                       FileMetadata::SPtr fileMetadataSPtr = enumeratorSPtr->Current().Value;

                       if (fileMetadataSPtr->FileId > fileId_)
                       {
                           fileId_ = fileMetadataSPtr->FileId;
                       }

                       if (fileMetadataSPtr->LogicalTimeStamp > logicalCheckpointFileTimeStamp_)
                       {
                           logicalCheckpointFileTimeStamp_ = fileMetadataSPtr->LogicalTimeStamp;
                       }

                       KString::SPtr checkpointFileName;
                       auto status = KString::Create(checkpointFileName, this->GetThisAllocator(), L"");
                       Diagnostics::Validate(status);
                       bool result = checkpointFileName->Concat(*workDirectorySPtr_);
                       STORE_ASSERT(result, "Unable to concat path string");
                       result = checkpointFileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
                       STORE_ASSERT(result, "Unable to concat path string");
                       result = checkpointFileName->Concat(*fileMetadataSPtr->FileName);
                       STORE_ASSERT(result, "Unable to concat path string");

                       CheckpointFile::SPtr checkpointFileSPtr = co_await CheckpointFile::OpenAsync(*checkpointFileName, *traceComponent_, this->GetThisAllocator(), isValueReferenceType_);
                       fileMetadataSPtr->CheckpointFileSPtr = *checkpointFileSPtr;
                       keyCheckpointFileListSPtr->Append(fileMetadataSPtr->CheckpointFileSPtr->GetAsyncEnumerator<TKey, TValue>(*keySerializerSPtr_));
                   }

                   co_await MergeAsync(keyCheckpointFileListSPtr, cancellationToken);
               }
               catch (ktl::Exception const& e)
               {
                   exception = SharedException::Create(e, this->GetThisAllocator());
               }

               if (exception != nullptr)
               {
                   for (ULONG i = 0; i < keyCheckpointFileListSPtr->Count(); i++)
                   {
                       co_await (*keyCheckpointFileListSPtr)[i]->CloseAsync();
                   }

                   //clang compiler error, needs to assign before throw.
                   auto ex = exception->Info;
                   throw ex;
               }
            }

        private:
            void VerifyStoreComponent()
            {
                if (componentSPtr_->Count() == 0) return;

                auto previousItem = (*componentSPtr_)[0];
                for (ULONG i = 1; i < componentSPtr_->Count(); i++)
                {
                    auto currentItem = (*componentSPtr_)[i];
                    int comparison = comparerSPtr_->Compare(previousItem.Key, currentItem.Key);
                    STORE_ASSERT(comparison < 0, "previous item must be larger than current item");
                }
            }
            ktl::Awaitable<void> MergeAsync(
                __in KSharedPtr<KSharedArray<KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>>>& keyCheckpointFileListSPtr,
                __in ktl::CancellationToken const & cancellationToken)
            {
                KPriorityQueue<KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>> priorityQueue(
                    this->GetThisAllocator(), 
                    KeyCheckpointFileAsyncEnumerator<TKey, TValue>::CompareEnumerators);

                StoreEventSource::Events->RecoveryStoreComponentMergeKeyCheckpointFilesAsync(traceComponent_->PartitionId, traceComponent_->TraceTag, L"starting", -1);
                LONG64 count = 0;

                // Get Enumerators for each file from the MetadataTable
                for (ULONG i = 0; i < keyCheckpointFileListSPtr->Count(); i++)
                {
                    KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> keyCheckpointEnumeratorSPtr = (*keyCheckpointFileListSPtr)[i];
                    
                    keyCheckpointEnumeratorSPtr->KeyComparerSPtr = *comparerSPtr_;

                    // Move the enumerator once to make it point at the first item
                    co_await keyCheckpointEnumeratorSPtr->MoveNextAsync(cancellationToken);

                    priorityQueue.Push(keyCheckpointEnumeratorSPtr);
                }

                while (!priorityQueue.IsEmpty())
                {
                    KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> currentEnumeratorSPtr;
                    bool result = priorityQueue.Pop(currentEnumeratorSPtr);
                    STORE_ASSERT(result, "priority queue should not be empty");

                    KSharedPtr<KeyData<TKey, TValue>> row = currentEnumeratorSPtr->GetCurrent();

                    count++;

                    totalKeyCount_++;
                    totalKeySize_ += row->KeySize;

                    AddOrUpdate(row->Key, row->Value);

                    result = co_await currentEnumeratorSPtr->MoveNextAsync(cancellationToken);
                    if (result)
                    {
                        priorityQueue.Push(currentEnumeratorSPtr);
                    }
                }

                for (ULONG i = 0; i < keyCheckpointFileListSPtr->Count(); i++)
                {
                    KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> keyCheckpointEnumeratorSPtr = (*keyCheckpointFileListSPtr)[i];
                    co_await keyCheckpointEnumeratorSPtr->CloseAsync();
                }

                StoreEventSource::Events->RecoveryStoreComponentMergeKeyCheckpointFilesAsync(traceComponent_->PartitionId, traceComponent_->TraceTag, L"completed", count);

#ifdef DBG
                VerifyStoreComponent();
#endif
                co_return;
            }


            void AddOrUpdate(TKey inputKey, KSharedPtr<VersionedItem<TValue>> const & inputValue)
            {
                STORE_ASSERT(inputValue != nullptr, "value should not be null");

                KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> inputVersion(inputKey, inputValue);

                if (componentSPtr_->Count() == 0)
                {
                    componentSPtr_->Append(inputVersion);
                    return;
                }

                KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> lastItem = (*componentSPtr_)[componentSPtr_->Count() - 1];

                LONG32 keyComparison = comparerSPtr_->Compare(lastItem.Key, inputKey);
                STORE_ASSERT(keyComparison <= 0, "input key must be greater than or equal to last key processed");
                
                // If this is a new key, append to the end of the list
                if (keyComparison < 0)
                {
                    componentSPtr_->Append(inputVersion);
                    return;
                }

                auto lastItemVersionSequenceNumber = lastItem.Value->GetVersionSequenceNumber();
                auto inputValueVersionSequenceNumber = inputValue->GetVersionSequenceNumber();
                
                // Existing value is older than new version, accept the new version
                if (lastItemVersionSequenceNumber < inputValueVersionSequenceNumber)
                {
                    // List maintains count
                    (*componentSPtr_)[componentSPtr_->Count() - 1] = inputVersion;
                }
                else if (lastItemVersionSequenceNumber == inputValueVersionSequenceNumber)
                {
                    // This case only happen in copy and recovery cases where the log replays the operations which gets checkpointed.
                    // To strengthen this assert, we could not taken in replayed items to differential in future.
                    STORE_ASSERT(lastItem.Value->GetRecordKind() == RecordKind::DeletedVersion, "Existing must be delete");
                    STORE_ASSERT(inputValue->GetRecordKind() == RecordKind::DeletedVersion, "Input must be delete");
                }
            }

            RecoveryStoreComponent(
                __in MetadataTable& metadataTableSPtr,
                __in KStringView const & workDirectory,
                __in StoreTraceComponent & traceComponent,
                __in IComparer<TKey> & keyComparer,
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                __in bool isValueReferenceType);

            ULONG32 fileId_;
            LONG64 logicalCheckpointFileTimeStamp_;
            bool isValueReferenceType_;
            MetadataTable::SPtr metadataTableSPtr_;
            KString::SPtr workDirectorySPtr_;
            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr_;
            KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>> componentSPtr_;
            KSharedPtr<IComparer<TKey>> comparerSPtr_;

            LONG64 totalKeyCount_;
            LONG64 totalKeySize_;
            
            StoreTraceComponent::SPtr traceComponent_;
        };
        
        template<typename TKey, typename TValue>
        RecoveryStoreComponent<TKey, TValue>::RecoveryStoreComponent(
            __in MetadataTable& metadataTable,
            __in KStringView const& workDirectory,
            __in StoreTraceComponent & traceComponent,
            __in IComparer<TKey> & keyComparer,
            __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
            __in bool isValueReferenceType)
            : metadataTableSPtr_(&metadataTable),
            workDirectorySPtr_(nullptr),
            keySerializerSPtr_(&keySerializer),
            isValueReferenceType_(isValueReferenceType),
            comparerSPtr_(&keyComparer),
            totalKeyCount_(0),
            totalKeySize_(0),
            traceComponent_(&traceComponent)
        {
            auto status = KString::Create(workDirectorySPtr_, this->GetThisAllocator(), workDirectory);
            Diagnostics::Validate(status);
            
            componentSPtr_ = _new(RECOVERY_COMPONENT_TAG, this->GetThisAllocator()) KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>();
            componentSPtr_->Reserve(Constants::InitialRecoveryComponentSize);

            ULONG32 invalidFileId = 0;
            fileId_ = invalidFileId;

            LONG64 invalidTimeStamp = 0;
            logicalCheckpointFileTimeStamp_ = invalidTimeStamp;
        }

        template<typename TKey, typename TValue>
        RecoveryStoreComponent<TKey, TValue>::~RecoveryStoreComponent()
        {
            
        }

    }
}
