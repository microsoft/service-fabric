// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager 
    {
        // Metadata present state provider state.
        // SerializableMetadata is used for checkpoint files and copy state.
        class SerializableMetadata final :
            public KObject<SerializableMetadata>,
            public KShared<SerializableMetadata>
        {
            K_FORCE_SHARED(SerializableMetadata)

        public:
            // Name and typeString can not be null, thus we use copy by referance here. However initializationParameters can be null, use pointer
            // For type less than 8 bytes, use copy by value
            static NTSTATUS Create(
                __in KUri const & name,
                __in KString const & typeString,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in FABRIC_SEQUENCE_NUMBER createLSN,
                __in FABRIC_SEQUENCE_NUMBER deleteLSN,
                __in MetadataMode::Enum metadataMode,
                __in KAllocator & allocator,
                __in_opt Utilities::OperationData const * const initializationParameters,
                __out CSPtr & result) noexcept;

            static NTSTATUS Create(
                __in Metadata const & metadata,
                __in KAllocator & allocator,
                __out CSPtr & result) noexcept;

        public:
            // All property get with const keyword, making sure can't change member variables
            // We promise the object will not be changed, but don't care whether the pointer is changed or not
            __declspec(property(get = get_Name)) KUri::CSPtr Name;
            KUri::CSPtr SerializableMetadata::get_Name() const;

            __declspec(property(get = get_TypeString)) KString::CSPtr TypeString;
            KString::CSPtr SerializableMetadata::get_TypeString() const;

            __declspec(property(get = get_InitializationParameters)) Utilities::OperationData::CSPtr InitializationParameters;
            Utilities::OperationData::CSPtr SerializableMetadata::get_InitializationParameters() const;

            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID SerializableMetadata::get_StateProviderId() const;

            __declspec(property(get = get_ParentStateProviderId)) FABRIC_STATE_PROVIDER_ID ParentStateProviderId;
            FABRIC_STATE_PROVIDER_ID SerializableMetadata::get_ParentStateProviderId() const;

            __declspec(property(get = get_CreateLSN)) FABRIC_SEQUENCE_NUMBER CreateLSN;
            FABRIC_SEQUENCE_NUMBER SerializableMetadata::get_CreateLSN() const;

            __declspec(property(get = get_DeleteLSN)) FABRIC_SEQUENCE_NUMBER DeleteLSN;
            FABRIC_SEQUENCE_NUMBER SerializableMetadata::get_DeleteLSN() const;

            __declspec(property(get = get_MetadataMode)) MetadataMode::Enum MetadataMode;
            MetadataMode::Enum SerializableMetadata::get_MetadataMode() const;

        private: // Constructor
            SerializableMetadata(
                __in KUri const & name,
                __in KString const & typeString,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in FABRIC_SEQUENCE_NUMBER createLSN,
                __in FABRIC_SEQUENCE_NUMBER deleteLSN,
                __in MetadataMode::Enum metadataMode,
                __in_opt Utilities::OperationData const * const initializationParameters) noexcept;

        private: 
            // For SerializableMetadata, all members are initialized in the constructor and will not be changed later, so set all members to const
            // Order of member variables should be the same as order of initializers
            const KUri::CSPtr name_;
            const KString::CSPtr typeString_;
            const FABRIC_STATE_PROVIDER_ID stateProviderId_;
            const FABRIC_STATE_PROVIDER_ID parentStateProviderId_;
            const FABRIC_SEQUENCE_NUMBER createLSN_;
            const FABRIC_SEQUENCE_NUMBER deleteLSN_;
            const MetadataMode::Enum metadataMode_;
            const Utilities::OperationData::CSPtr initializationParameters_;
        };
    }
}
