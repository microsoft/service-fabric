// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        /// <summary>
        /// Metadata class is the class that encapsulate all information about a state provider being managed by State Manager.
        /// </summary>
        class Metadata final : 
            public KObject<Metadata>, 
            public KShared<Metadata>
        {
            K_FORCE_SHARED(Metadata)

        public:
            static NTSTATUS Create(
                __in KUri const & name,
                __in KString const & typeString,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __in MetadataMode::Enum metadataMode,
                __in bool transientCreate,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static NTSTATUS Create(
                __in KUri const & name,
                __in KString const & typeString,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __in FABRIC_SEQUENCE_NUMBER createLSN,
                __in FABRIC_SEQUENCE_NUMBER deleteLSN,
                __in MetadataMode::Enum metadataMode,
                __in bool transientCreate,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static NTSTATUS Create(
                __in KUri const & name,
                __in KString const & typeString,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __in FABRIC_SEQUENCE_NUMBER createLSN,
                __in FABRIC_SEQUENCE_NUMBER deleteLSN,
                __in MetadataMode::Enum metadataMode,
                __in bool transientCreate,
                __in KAllocator & allocator,
                __out CSPtr & result) noexcept;

            static NTSTATUS Create(
                __in SerializableMetadata const & metadata,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in bool transientCreate,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            __declspec(property(get = get_Name)) KUri::CSPtr Name;
            KUri::CSPtr Metadata::get_Name() const;

            __declspec(property(get = get_TypeString)) KString::CSPtr TypeString;
            KString::CSPtr Metadata::get_TypeString() const;

            __declspec(property(get = get_StateProvider)) TxnReplicator::IStateProvider2::SPtr StateProvider;
            TxnReplicator::IStateProvider2::SPtr Metadata::get_StateProvider() const;

            __declspec(property(get = get_InitializationParameters)) Data::Utilities::OperationData::CSPtr InitializationParameters;
            Data::Utilities::OperationData::CSPtr Metadata::get_InitializationParameters() const;

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID Metadata::get_StateProviderId() const;

            __declspec(property(get = get_TransactionId, put = put_TransactionId)) LONG64 TransactionId;
            LONG64 Metadata::get_TransactionId() const;
            void Metadata::put_TransactionId(__in LONG64 value);

            __declspec(property(get = get_ParentId)) FABRIC_STATE_PROVIDER_ID ParentId;
            FABRIC_STATE_PROVIDER_ID Metadata::get_ParentId() const;

            __declspec(property(get = get_CreateLSN, put = put_CreateLSN)) FABRIC_SEQUENCE_NUMBER CreateLSN;
            FABRIC_SEQUENCE_NUMBER Metadata::get_CreateLSN() const;
            void Metadata::put_CreateLSN(__in FABRIC_SEQUENCE_NUMBER value);

            __declspec(property(get = get_DeleteLSN, put = put_DeleteLSN)) FABRIC_SEQUENCE_NUMBER DeleteLSN;
            FABRIC_SEQUENCE_NUMBER Metadata::get_DeleteLSN() const;
            void Metadata::put_DeleteLSN(__in FABRIC_SEQUENCE_NUMBER deleteLSN);

            __declspec(property(get = get_MetadataMode, put = put_MetadataMode)) MetadataMode::Enum MetadataMode;
            MetadataMode::Enum  Metadata::get_MetadataMode() const;
            void Metadata::put_MetadataMode(__in MetadataMode::Enum metadataMode);

            __declspec(property(get = get_TransientCreate, put = put_TransientCreate)) bool TransientCreate;
            bool Metadata::get_TransientCreate() const;
            void Metadata::put_TransientCreate(__in bool value);

            __declspec(property(get = get_TransientDelete, put = put_TransientDelete)) bool TransientDelete;
            bool Metadata::get_TransientDelete() const;
            void Metadata::put_TransientDelete(__in bool value);

            // Following are not ported because they are currently not being used in the existing code.
            // They were constructs for feature: "Do not checkpoint a state provider that has not seen any operations"
            // void SetCheckpointFlag();
            // bool ShouldCheckpointAndResetIfNecessary();

        private: // Constructor.
            NOFAIL Metadata(
                __in KUri const & name,
                __in KString const & typeString,
                __in TxnReplicator::IStateProvider2 & stateProvider,
                __in_opt Data::Utilities::OperationData const * const initializationParameters,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_STATE_PROVIDER_ID parentId,
                __in FABRIC_SEQUENCE_NUMBER createLSN,
                __in FABRIC_SEQUENCE_NUMBER deleteLSN,
                __in MetadataMode::Enum metadataMode,
                __in bool transientCreate) noexcept;

        private: // Constant members
            const KUri::CSPtr name_;
            const KString::CSPtr typeString_;
            const TxnReplicator::IStateProvider2::SPtr stateProvider_;
            const Data::Utilities::OperationData::CSPtr initializationParameters_;
            const FABRIC_STATE_PROVIDER_ID stateProviderId_;
            const FABRIC_STATE_PROVIDER_ID parentId_;

        private: // Constructor initialized but not const.
            MetadataMode::Enum metadataMode_;

        private: // Members that are not constant.
            LONG64 transactionId_ = Constants::InvalidTransactionId;
            FABRIC_SEQUENCE_NUMBER createLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            FABRIC_SEQUENCE_NUMBER deleteLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
            bool transientCreate_ = false;
            bool transientDelete_ = false;
        };
    }
}
