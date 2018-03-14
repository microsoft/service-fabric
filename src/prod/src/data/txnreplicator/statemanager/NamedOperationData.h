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
        /// Named Operation Data encapsulates multiplexing information to indicate which 
        /// component a given operation data belongs to : SM or SP2.
        /// </summary>
        /// <remarks>
        /// NamedOperationData is used in copy and replication.
        /// </remarks>
        /// <dataformat>
        /// NATIVE:
        ///                     |UserOperationData||Context             |
        /// OperationData:      [0].............[n][HvX]...[Hv1][Header0]
        /// HeaderVersion0:     [ContextOperationDataCount|UserOperationDataCount|StateProviderID]
        ///                     [ULONG32                  |ULONG32               |LONG64         ]
        ///                     = 4 + 4 + 8 = 16 bytes
        /// Max number of context header segments:  4,294,967,295
        /// Max number of user segments:            4,294,967,295
        /// Version one header segment count:       1.
        /// Value add of this format is version-ability.
        /// </dataformat>
        /// <dataformat>
        /// MANAGED: 
        ///                     |UserOperationData||Context|
        /// OperationData:      [0].............[n][Header ]
        /// Header:             [Version|StateProviderID|IsStateProviderDataNull]
        ///                     [LONG32 |LONG64         |bool                   ]
        ///                     = 4 + 8 + 1 = 13 bytes
        /// Possible Version Number Range:          [1, 1]
        /// Max number of context header segments:  1
        /// Max number of user segments:            Not limited by protocol (2,147,483,647)
        /// </dataformat>
        /// <devnote>
        /// We use size of Header0 to determine whether the operation is formatted as managed or native.
        /// </devnote>
        class NamedOperationData final 
            : public Utilities::OperationData
        {
            K_FORCE_SHARED(NamedOperationData)

        public: // static Creators.
            /// <summary>
            /// Creates an NamedOperationData::SPtr from inputs.
            /// Used in the IN path.
            /// </summary>
            static NTSTATUS Create(
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in SerializationMode::Enum serailizationMode,
                __in KAllocator & allocator,
                __in_opt OperationData const * const userOperationData,
                __out CSPtr & result) noexcept;

            /// <summary>
            /// Creates an NamedOperationData::SPtr by de-serializing the OperationData from LoggingReplicator.
            /// Used in the OUT path.
            /// </summary>
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __in_opt Utilities::OperationData const * const operationData,
                __out CSPtr & result) noexcept;

        public: // Properties
            __declspec(property(get = get_StateProviderId)) FABRIC_STATE_PROVIDER_ID StateProviderId;
            FABRIC_STATE_PROVIDER_ID get_StateProviderId() const noexcept;

            __declspec(property(get = get_UserOperationData)) OperationData::CSPtr UserOperationDataCSPtr;
            OperationData::CSPtr get_UserOperationData() const noexcept;

        private: // Static header reader
            static NTSTATUS DeserializeContext(
                __in_opt Utilities::OperationData const * const operationData,
                __in KAllocator & allocator,
                __out ULONG32 & metadataOperationDataCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out OperationData::CSPtr & userOperationData) noexcept;

            static NTSTATUS DeserializeNativeContext(
                __in_opt Utilities::OperationData const & operationData,
                __in KAllocator & allocator,
                __out ULONG32 & metadataOperationDataCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out OperationData::CSPtr & userOperationData) noexcept;

            static NTSTATUS DeserializeManagedContext(
                __in_opt Utilities::OperationData const & operationData,
                __in KAllocator & allocator,
                __out ULONG32 & metadataOperationDataCount,
                __out FABRIC_STATE_PROVIDER_ID & stateProviderId,
                __out OperationData::CSPtr & userOperationData) noexcept;

        private:
            __checkReturn NTSTATUS Serialize(
                __in SerializationMode::Enum mode) noexcept;

            __checkReturn NTSTATUS CreateNativeContext(
                __out OperationData::CSPtr & result) noexcept;
            
            // Managed Header for backwards compatibility.
            __checkReturn NTSTATUS CreateManagedContext(
                __out OperationData::CSPtr & result) noexcept;

        private: // Constructor
            /// <summary>
            /// Initializes a new instance of the <see cref="NamedOperationData"/> class.
            /// </summary>
            FAILABLE NamedOperationData(
                __in ULONG32 metadataCount,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in_opt Utilities::OperationData const * const userOperationData) noexcept;

            /// <summary>
            /// Initializes a new instance of the <see cref="NamedOperationData"/> class.
            /// </summary>
            FAILABLE NamedOperationData(
                __in ULONG32 metadataCount,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in SerializationMode::Enum mode,
                __in_opt Utilities::OperationData const * const userOperationData) noexcept;

        private: // Statics
            static const LONG32 NullUserOperationData;
            static const ULONG32 NativeContextHeaderSize;
            static const ULONG32 ManagedContextHeaderSize;
            static const LONG32 ManagedVersion;
            static const ULONG32 CurrentWriteMetadataOperationDataCount;

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
