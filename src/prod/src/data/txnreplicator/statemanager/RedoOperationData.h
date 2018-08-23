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
        /// RedoOperationData for StateManager operations like Add and Remove State Provider.
        /// </summary>
        /// <dataformat>
        /// Managed:
        /// OperationData:      [TypeName | BufferSize + Buffer | ParentStateProviderId]
        ///                     [String   | LONG32     + byte*  | LONG64]
        ///
        /// Native
        /// OperationData:      [Header][...][initializationParameters]
        /// Header:             [TypeName | BufferCount | ParentStateProviderId]
        ///                     [String   | LONG32      | LONG64]
        /// InitializationParameters: [Buffer0][Buffer1][...][BufferN]
        ///
        /// Note: The different between Managed code and Native code are the initialization parameter part.
        /// For the managed code, it will write the size of buffer then the actual byte array, if the initialization parameter
        /// is null, write -1. So for managed, the possible value are -1, 0, 1, 2...MAXLONG32. However, for the native, BufferCount
        /// is positive number(because include the header), to write the value, we take a complement of the origin size. So the possible 
        /// value for native is -2, -3...MINLONG32.
        /// In this way, after we read in the BufferCount, we can distinguish between managed and native format.
        /// </dataformat>
        class RedoOperationData final
            : public Utilities::OperationData
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(RedoOperationData);

        public:
            /// <summary>
            /// Creates an RedoOperationData::SPtr from inputs.
            /// Used in the IN path.
            /// </summary>
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KString const & typeName,
                __in OperationData const * const initializationParameters,
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in SerializationMode::Enum serailizationMode,
                __in KAllocator & allocator,
                __out CSPtr & result);

            /// <summary>
            /// Creates an RedoOperationData::SPtr by deserializing the OperationData from LoggingReplicator.
            /// Used in the OUT path.
            /// </summary>
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KAllocator & allocator,
                __in_opt OperationData const * const operationData,
                __out CSPtr & result);

        public:
            __declspec(property(get = get_TypeName)) KString::CSPtr TypeName;
            KString::CSPtr get_TypeName() const;

            __declspec(property(get = get_InitParams)) OperationData::CSPtr InitializationParameters;
            OperationData::CSPtr get_InitParams() const;

            __declspec(property(get = get_ParentId)) FABRIC_STATE_PROVIDER_ID ParentId;
            FABRIC_STATE_PROVIDER_ID get_ParentId() const;

        private: // Static header reader
            static __checkReturn NTSTATUS Deserialize(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KAllocator & allocator,
                __in_opt OperationData const * const operationData,
                __out LONG32 & bufferCount,
                __out KString::SPtr & typeName,
                __out OperationData::CSPtr & initializationParameters,
                __out FABRIC_STATE_PROVIDER_ID & parentId) noexcept;

            static __checkReturn NTSTATUS DeserializeNativeOperation(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryReader & reader,
                __in KAllocator & allocator,
                __in_opt OperationData const * const operationData,
                __out LONG32 & bufferCount,
                __out OperationData::CSPtr & initializationParameters,
                __out FABRIC_STATE_PROVIDER_ID & parentId) noexcept;

            static __checkReturn NTSTATUS DeserializeManagedOperation(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryReader & reader,
                __in KAllocator & allocator,
                __out LONG32 & bufferCount,
                __out OperationData::CSPtr & initializationParameters,
                __out FABRIC_STATE_PROVIDER_ID & parentId) noexcept;

        private: // Serialization methods.
            __checkReturn NTSTATUS Serialize(
                __in SerializationMode::Enum serailizationMode) noexcept;
            __checkReturn NTSTATUS SerializeNativeOperation() noexcept;
            __checkReturn NTSTATUS SerializeManagedOperation() noexcept;

        private: // Constructor
            FAILABLE RedoOperationData(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LONG32 headerBufferCount,
                __in KString const & typeName,
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in SerializationMode::Enum serailizationMode,
                __in_opt OperationData const * const initializationParameters) noexcept;

            FAILABLE RedoOperationData(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in LONG32 headerBufferCount,
                __in KString const & typeName,
                __in FABRIC_STATE_PROVIDER_ID parentStateProviderId,
                __in_opt OperationData const * const initializationParameters) noexcept;

        private:
            LONG32 bufferCountIncludeHeader_;
            KString::CSPtr typeNameSPtr_;
            OperationData::CSPtr initializationParametersCSPtr_;
            FABRIC_STATE_PROVIDER_ID parentStateProviderId_;
        };
    }
}
