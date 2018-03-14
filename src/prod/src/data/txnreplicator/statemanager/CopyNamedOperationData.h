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
        /// CopyNamedOperationData encapsulates multiplexing information to indicate which 
        /// component a given operation data belongs to : SM or SP2.
        /// </summary>
        /// <remarks>
        /// NamedOperationData is used in copy.
        /// </remarks>
        /// <dataformat>
        /// NATIVE:
        ///                     |UserOperationData||Context             |
        /// OperationData:      [HeaderVersion0][HeaderVersion1]...[HeaderVersionX][0].............[n]
        /// HeaderVersion0:     [HeaderOperationDataCount|StateProviderID]
        ///                     [ULONG32                 |LONG64         ]
        ///                     = 4 + 8 = 12 bytes
        /// Max number of context header segments:  4,294,967,295
        /// Max number of user segments:            4,294,967,295
        /// Version one header segment count:       1.
        /// Value add of this format is version-ability.
        /// </dataformat>
        /// <dataformat>
        /// MANAGED: 
        ///                     |UserOperationData||Context|
        /// OperationData:      [H0: Version][H1: StateProviderID][0].............[n]
        /// Header:             [Version][StateProviderID]
        ///                     [LONG32 ][LONG64         ]
        ///                     = 4 + 8 = 12 bytes
        /// Possible Version Number Range:          [1, 1]
        /// Max number of context header segments:  1
        /// Max number of user segments:            Not limited by protocol (2,147,483,647)
        /// </dataformat>
        /// <devnote>
        /// We use size of Header0 to determine whether the operation is formatted as managed or native.
        /// </devnote>
        class CopyNamedOperationData final 
            : public Utilities::OperationData
        {
            K_FORCE_SHARED(CopyNamedOperationData)

        public: // static Creators.
            // Creates an CopyNamedOperationData::SPtr from inputs.
            // Used in the IN path.
            static NTSTATUS Create(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in SerializationMode::Enum serailizationMode,
                __in KAllocator & allocator,
                __in OperationData const & userOperationData,
                __out CSPtr & result) noexcept;

            // Creates an CopyNamedOperationData::SPtr by de-serializing the OperationData from LoggingReplicator.
            // Used in the OUT path.
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __in Utilities::OperationData const & inputOperationData,
                __out CSPtr & result) noexcept;

        public: // Properties
            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const noexcept;

            __declspec(property(get = get_UserOperationData)) OperationData::CSPtr UserOperationDataCSPtr;
            OperationData::CSPtr get_UserOperationData() const noexcept;

        private: // Static header reader
            static NTSTATUS DeserializeContext(
                __in Utilities::OperationData const & operationData,
                __in KAllocator & allocator,
                __out ULONG32 & metadataOperationDataCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out OperationData::CSPtr & userOperationData) noexcept;

            static NTSTATUS DeserializeNativeContext(
                __in Utilities::OperationData const & operationData,
                __in KAllocator & allocator,
                __out ULONG32 & metadataOperationDataCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out OperationData::CSPtr & userOperationData) noexcept;

            static NTSTATUS DeserializeManagedContext(
                __in Utilities::OperationData const & operationData,
                __in KAllocator & allocator,
                __out ULONG32 & metadataOperationDataCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out OperationData::CSPtr & userOperationData) noexcept;

        private:
            __checkReturn NTSTATUS Serialize(
                __in SerializationMode::Enum mode) noexcept;

            __checkReturn NTSTATUS CreateNativeContext() noexcept;

            // Managed Header for backwards compatibility.
            __checkReturn NTSTATUS CreateManagedContext() noexcept;

        private: // Constructor
            // Constructor for read path (deserialization)
            FAILABLE CopyNamedOperationData(
                __in ULONG32 metadataCount,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Utilities::OperationData const & inputOperationData) noexcept;

            // Constructor for write path (serialization)
            FAILABLE CopyNamedOperationData(
                __in ULONG32 metadataCount,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in SerializationMode::Enum mode,
                __in Utilities::OperationData const & userOperationData) noexcept;

        private: // Statics
            static const ULONG32 NativeContextHeaderSize;
            static const ULONG32 CurrentWriteMetadataOperationDataCount;
            static const ULONG32 NativeHeaderCount;

            static const LONG32 ManagedVersion;
            static const LONG32 ManagedVersionBufferSize;
            static const LONG32 ManagedStateProviderBufferSize;
            static const ULONG32 ManagedHeaderCount;

        private: // Member variables
            // Number of operationData used as metadata.
            // Since this number can only increase, this number is also used as version number.
            ULONG32 const metadataCount_;

            // State Provider ID that the operation data belongs to.
            FABRIC_STATE_PROVIDER_ID const stateProviderId_;

            // Operation data of the SP2 or SM.
            Utilities::OperationData::CSPtr userOperationDataCSPtr_;
        };
    }
}
