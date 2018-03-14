// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverManagerService;
        typedef std::shared_ptr<FailoverManagerService> FailoverManagerServiceSPtr;

        typedef std::function<void(FailoverManagerServiceSPtr const &)> OpenReplicaCallback;
        typedef std::function<void(::FABRIC_REPLICA_ROLE, Common::ComPointer<IFabricStatefulServicePartition> const &)> ChangeRoleCallback;
        typedef std::function<void()> CloseReplicaCallback;

        class FailoverManagerService :
            public Store::KeyValueStoreReplica,
            private Common::RootedObject
        {
            DENY_COPY(FailoverManagerService)

        public:

            FailoverManagerService(
                Common::Guid const & partitionId,
                FABRIC_REPLICA_ID replicaId,
                Federation::NodeInstance const &,
                Common::ComponentRoot const &);

            ~FailoverManagerService();

            ChangeRoleCallback OnChangeRoleCallback;
            CloseReplicaCallback OnCloseReplicaCallback;
            OpenReplicaCallback OnOpenReplicaCallback;

        protected:

            virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition) override;
            virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation) override;
            virtual Common::ErrorCode OnClose() override;
            virtual void OnAbort() override;

        private:

            Federation::NodeInstance nodeInstance_;
            ComPointer<IFabricStatefulServicePartition> servicePartition_;
        };
    }
}
