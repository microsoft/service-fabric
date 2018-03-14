// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class Replicator::ReplicateAsyncOperation : 
            public Common::AsyncOperation
        {
            DENY_COPY(ReplicateAsyncOperation)

        public:
            ReplicateAsyncOperation(
                __in Replicator & parent,
                Common::ComPointer<IFabricOperationData> && comOperationPointer,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & state);

            virtual ~ReplicateAsyncOperation() {}

            __declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return sequenceNumber_; }

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

        private:

            void ReplicateCallback(
                Common::AsyncOperationSPtr const & asyncOperation,
                PrimaryReplicatorSPtr const & primary);

            void FinishReplicate(
                Common::AsyncOperationSPtr const & asyncOperation,
                PrimaryReplicatorSPtr const & primary);

            Replicator & parent_;
            Common::ComPointer<IFabricOperationData> comOperationPointer_;
            FABRIC_SEQUENCE_NUMBER sequenceNumber_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
