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
        /// Collection of named operation data
        /// </summary>
        class NamedOperationDataStream final
            : public TxnReplicator::OperationDataStream
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(NamedOperationDataStream)

            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceType,
                __in SerializationMode::Enum serializationMode,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public:
            void Add(
                __in StateProviderIdentifier stateProviderInfo,
                __in OperationDataStream & operationDataStream);

        public:
            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out Utilities::OperationData::CSPtr & result) noexcept override;

        public:
            void Dispose() override;

        private:
            struct StateProviderDataStream
            {
                StateProviderIdentifier Info;
                OperationDataStream::SPtr OperationDataStream;

                StateProviderDataStream() :
                    Info(),
                    OperationDataStream(nullptr)
                {
                }

                StateProviderDataStream(
                    __in StateProviderIdentifier info,
                    __in TxnReplicator::OperationDataStream & operationDataStream):
                    Info(info),
                    OperationDataStream(&operationDataStream)
                {
                }

                StateProviderDataStream& operator=(StateProviderDataStream const & other)
                {
                    if (&other == this)
                    {
                        return *this;
                    }

                    Info = other.Info;
                    OperationDataStream = other.OperationDataStream;

                    return *this;
                }
            };

        private:
            NamedOperationDataStream(
                __in Utilities::PartitionedReplicaId const & traceType,
                __in SerializationMode::Enum serializationMode);

        private:
            SerializationMode::Enum const serializationMode_;

        private:
            /// <summary>
            /// Flag that is used to detect redundant calls to dispose.
            /// </summary>
            bool isDisposed_ = false;

            /// <summary>
            /// Current wire format version.
            /// </summary>
            int version_ = 1;

            ULONG currentIndex_ = 0;

        private:
            /// <summary>
            /// Collection of operation data.
            /// </summary>
            KArray<StateProviderDataStream> operationDataStreamCollection_;
        };
    }
}
