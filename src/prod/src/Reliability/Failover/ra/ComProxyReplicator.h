// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class ComProxyReplicator
        {
            DENY_COPY(ComProxyReplicator);

        public:
            ComProxyReplicator(Common::ComPointer<::IFabricReplicator> const & replicator) :
                replicator_(replicator),
                primary_(replicator, ::IID_IFabricPrimaryReplicator),
                internalReplicator_(replicator, ::IID_IFabricInternalReplicator)
            {
                ASSERT_IFNOT(replicator_.GetRawPointer() != nullptr, "Replicator didn't implement IFabricReplicator");
                ASSERT_IFNOT(primary_.GetRawPointer() != nullptr, "Replicator didn't implement IFabricPrimaryReplicator");
                
                doesReplicatorSupportCatchupSpecificQuorum_ = CheckIfInterfaceIsImplemented<::IFabricReplicatorCatchupSpecificQuorum>(replicator, ::IID_IFabricReplicatorCatchupSpecificQuorum);
            }

            __declspec(property(get = get_DoesReplicatorSupportQueries)) bool const DoesReplicatorSupportQueries;
            bool const get_DoesReplicatorSupportQueries() const {
                return (internalReplicator_.GetRawPointer() != nullptr);
            }

            __declspec(property(get = get_DoesReplicatorSupportCatchupSpecificQuorum)) bool DoesReplicatorSupportCatchupSpecificQuorum;
            bool get_DoesReplicatorSupportCatchupSpecificQuorum() const
            {
                return doesReplicatorSupportCatchupSpecificQuorum_;
            }

            // IFabricReplicator

            Common::AsyncOperationSPtr BeginOpen(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & replicationEndpoint);

            Common::AsyncOperationSPtr BeginChangeRole(
                ::FABRIC_EPOCH const & epoch,
                ::FABRIC_REPLICA_ROLE newRole,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndChangeRole(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginUpdateEpoch(
                ::FABRIC_EPOCH const & epoch,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndUpdateEpoch(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const & asyncOperation);

            void Abort();

            Common::ErrorCode GetStatus(__out FABRIC_SEQUENCE_NUMBER & firstLsn, __out FABRIC_SEQUENCE_NUMBER & lastLsn);

            // IFabricPrimaryReplicator

            Common::ErrorCode ComProxyReplicator::UpdateCatchUpReplicaSetConfiguration(
                ::FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration,
                ::FABRIC_REPLICA_SET_CONFIGURATION const * previousConfiguration);

            Common::AsyncOperationSPtr BeginWaitForCatchUpQuorum(
                ::FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndWaitForCatchUpQuorum(Common::AsyncOperationSPtr const & asyncOperation);

            Common::ErrorCode ComProxyReplicator::UpdateCurrentReplicaSetConfiguration(
                ::FABRIC_REPLICA_SET_CONFIGURATION const * currentConfiguration);

            Common::AsyncOperationSPtr BeginBuildIdleReplica(
                Reliability::ReplicaDescription const & idleReplicaDescription,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndBuildIdleReplica(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginOnDataLoss(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndOnDataLoss(Common::AsyncOperationSPtr const & asyncOperation, __out bool & isStateChanged);

            Common::ErrorCode RemoveIdleReplica(FABRIC_REPLICA_ID replicaId);

            // IFabricInternalReplicator

            Common::ErrorCode GetReplicatorQueryResult(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result);

        private:
            template<typename T>
            static bool CheckIfInterfaceIsImplemented(Common::ComPointer<::IFabricReplicator> const & replicator, IID const & iid)
            {
                auto casted = Common::ComPointer<T>(replicator, iid);
                return casted.GetRawPointer() != nullptr;
            }

            class OpenAsyncOperation;
            class ChangeRoleAsyncOperation;
            class BuildIdleReplicaAsyncOperation;
            class CatchupReplicaSetAsyncOperation;
            class CloseAsyncOperation;
            class OnDataLossAsyncOperation;
            class UpdateEpochAsyncOperation;

            Common::ComPointer<::IFabricReplicator> replicator_;
            Common::ComPointer<::IFabricPrimaryReplicator> primary_;
            Common::ComPointer<::IFabricInternalReplicator> internalReplicator_;
            bool doesReplicatorSupportCatchupSpecificQuorum_;
        };
    }
}
