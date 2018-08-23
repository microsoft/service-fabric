// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestHelper
    {
    public:
        enum FileSource
        {
            Managed,
            Native,
        };

    public :
        static KtlSystem* CreateKtlSystem();

        static Data::Utilities::PartitionedReplicaId::SPtr CreatePartitionedReplicaId(
            __in KAllocator & allocator);

        static Data::StateManager::MetadataManager::SPtr CreateMetadataManager(
            __in KAllocator & allocator);

        static Data::StateManager::Metadata::SPtr CreateMetadata(
            __in KUri const & name, 
            __in KAllocator & allocator);

        static Data::StateManager::Metadata::SPtr CreateMetadata(
            __in KUri const & name, 
            __in bool transientCreate, 
            __in KAllocator & allocator);

        static Data::StateManager::Metadata::SPtr CreateMetadata(
            __in KUri const & name,
            __in bool transientCreate,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in KAllocator & allocator);

        static Data::StateManager::NamedOperationData::CSPtr CreateNamedOperationData(
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in Data::StateManager::SerializationMode::Enum mode,
            __in KAllocator & allocator,
            __in_opt Data::Utilities::OperationData const * const operationDataPtr);

        static Data::StateManager::NamedOperationData::CSPtr CreateNamedOperationData(
            __in KAllocator & allocator,
            __in_opt Data::Utilities::OperationData const * const operationDataPtr);

        static Data::StateManager::MetadataOperationData::CSPtr CreateMetadataOperationData(
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in KUri const & stateProviderName,
            __in Data::StateManager::ApplyType::Enum applyType,
            __in KAllocator & allocator);

        static Data::StateManager::MetadataOperationData::CSPtr CreateMetadataOperationData(
            __in KAllocator & allocator,
            __in_opt Data::Utilities::OperationData const * const operationDataPtr);

        static Data::StateManager::RedoOperationData::CSPtr CreateRedoOperationData(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in KString const & typeName,
            __in Data::Utilities::OperationData const * const initializationParameters,
            __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
            __in KAllocator & allocator,
            __in_opt Data::StateManager::SerializationMode::Enum serializationMode = Data::StateManager::SerializationMode::Native);

        static Data::StateManager::RedoOperationData::CSPtr CreateRedoOperationData(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in KAllocator & allocator,
            __in_opt Data::Utilities::OperationData const * const operationDataPtr);

        static Data::StateManager::StateManagerLockContext::SPtr CreateLockContext(
            __in Data::StateManager::MetadataManager & metadataManager, 
            __in KAllocator & allocator);

        static TxnReplicator::IRuntimeFolders::SPtr CreateRuntimeFolders(__in KAllocator& allocator);

        static Data::Utilities::IStatefulPartition::SPtr CreateStatefulServicePartition(
            __in KAllocator & allocator);

        static Data::Utilities::IStatefulPartition::SPtr CreateStatefulServicePartition(
            __in Common::Guid partitionId, 
            __in KAllocator & allocator,
            __in FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED,
            __in FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

        static BOOL Equals(
            __in Data::Utilities::OperationData const * const operationData,
            __in Data::Utilities::OperationData const * const otherOperationData);

        static KSharedArray<Data::StateManager::SerializableMetadata::CSPtr>::SPtr CreateSerializableMetadataArray(
            __in KAllocator & allocator);

        static KSharedArray<ULONG32>::SPtr CreateRecordBlockSizesArray(
            __in KAllocator & allocator);

        static KSharedArray<Data::StateManager::Metadata::CSPtr>::SPtr CreateMetadataArray(
            __in KAllocator & allocator);

        static KSharedArray<KUri::CSPtr>::SPtr CreateStateProviderNameArray(
            __in KAllocator & allocator);

        static void VerifyMetadata(
            __in_opt Data::StateManager::Metadata const * const metadata1, 
            __in_opt Data::StateManager::Metadata const * const metadata2);

        static Data::Utilities::OperationData::SPtr CreateInitParameters(
            __in KAllocator & allocator,
            __in_opt ULONG32 bufferCount = 2,
            __in_opt ULONG32 bufferSize = 16);

        static void VerifyInitParameters(
            __in Data::Utilities::OperationData const & initParameters,
            __in_opt ULONG32 bufferCount = 2,
            __in_opt ULONG32 bufferSize = 16);

        static KWString CreateFileName(
            __in KStringView const & name,
            __in KAllocator & allocator,
            __in_opt FileSource fileSource = FileSource::Native);

        // If the input name is empty, should set the concatName to false
        static KString::CSPtr CreateFileString(
            __in KAllocator & allocator,
            __in KStringView const & name);

        static Data::StateManager::SerializableMetadata::CSPtr CreateSerializableMetadata(
            __in LONG64 expectedId,
            __in KUriView & expectedNameView,
            __in LONG64 expectedParentId,
            __in LONG64 expectedCreateLSN,
            __in LONG64 expectedDeleteLSN,
            __in KAllocator & allocator);

        static KUri::CSPtr CreateStateProviderName(
            __in LONG64 stateProviderId, 
            __in KAllocator & allocator);

        static Data::Utilities::OperationData::SPtr Read(
            __in Data::Utilities::BinaryReader & reader,
            __in KAllocator & allocator);

        static void ReadOperationDatas(
            __in Data::Utilities::BinaryReader & reader,
            __in KAllocator & allocator,
            __out Data::Utilities::OperationData::CSPtr & metadata,
            __out Data::Utilities::OperationData::CSPtr & redo,
            __out Data::Utilities::OperationData::CSPtr & undo);

        static KString::SPtr GetStateManagerWorkDirectory(
            __in LPCWSTR runtimeFolders,
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in KAllocator & allocator);

    private:
        static Common::Random random;
    };
}
