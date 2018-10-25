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
        /// MetadataOperationData for StateManager operations like Add and Remove State Provider.
        /// </summary>
        /// <dataformat>
        /// OperationData:      [Header]...[n]
        /// Header:             [BufferCount | StateProvideId | StateProviderName | ApplyType]
        /// </dataformat>
        class MetadataOperationData final
            : public Utilities::OperationData
        {
            K_FORCE_SHARED(MetadataOperationData);

        public:
            /// <summary>
            /// Creates an MetadataOperationData::SPtr from inputs.
            /// Used in the IN path.
            /// </summary>
            static NTSTATUS Create(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in KUri const & stateProviderName,
                __in ApplyType::Enum applyType,
                __in KAllocator & allocator,
                __out CSPtr & result);

            /// <summary>
            /// Creates an MetadataOperationData::SPtr by deserializing the OperationData from LoggingReplicator.
            /// Used in the OUT path.
            /// </summary>
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __in_opt OperationData const * const operationData,
                __out CSPtr & result);

        public:
            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const;

            __declspec(property(get = get_StateProviderName))  KUri::CSPtr StateProviderName;
            KUri::CSPtr get_StateProviderName() const;

            __declspec(property(get = get_ApplyType)) ApplyType::Enum ApplyType;
            ApplyType::Enum get_ApplyType() const;

        private: // Static header reader
            static VOID Deserialize(
                __in KAllocator & allocator,
                __in_opt OperationData const * const operationData,
                __out ULONG32 & bufferCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out KUri::SPtr & stateProviderName,
                __out ApplyType::Enum & applyType);

        private:
            VOID Serialize();

        private:
            NOFAIL MetadataOperationData(
                __in ULONG32 headerBufferCount,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in KUri const & stateProviderName,
                __in ApplyType::Enum applyType,
                __in bool primeOperationData);

        private:
            static const ULONG32 WriteBufferCount;

        private:
            ULONG32 bufferCount_;
            FABRIC_STATE_PROVIDER_ID stateProviderId_;
            KUri::CSPtr stateProviderNameSPtr_;
            ApplyType::Enum applyType_;
        };
    }
}
