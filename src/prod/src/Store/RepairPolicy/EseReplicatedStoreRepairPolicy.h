// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseReplicatedStoreRepairPolicy 
        : public IRepairPolicy
        , public Common::ComponentRoot
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

    public:

        static Common::ErrorCode Create(
            Api::IClientFactoryPtr const &, 
            Common::NamingUri const &,
            bool allowRepairUpToQuorum,
            PartitionedReplicaTraceComponent const &,
            __out std::shared_ptr<IRepairPolicy> &);

    public:

        __declspec(property(get=get_ServiceName)) Common::NamingUri const & ServiceName;
        Common::NamingUri const & get_ServiceName() const { return serviceName_; }

        virtual Common::AsyncOperationSPtr BeginGetRepairActionOnOpen(
            RepairDescription const &,
            Common::TimeSpan const &,
            __in Common::AsyncCallback const &,
            __in Common::AsyncOperationSPtr const &) override;

        virtual Common::ErrorCode EndGetRepairActionOnOpen(
            __in Common::AsyncOperationSPtr const & operation,
            __out RepairAction::Enum &) override;

    private:

        class GetRepairActionOnOpenAsyncOperation;

        EseReplicatedStoreRepairPolicy(
            __in Api::IQueryClientPtr &,
            Common::NamingUri const &,
            bool allowRepairUpToQuorum,
            PartitionedReplicaTraceComponent const &);

    private:

        Api::IQueryClientPtr queryClientPtr_;
        Common::NamingUri serviceName_;
        bool allowRepairUpToQuorum_;
    };

    typedef std::shared_ptr<EseReplicatedStoreRepairPolicy> EseReplicatedStoreRepairPolicySPtr;
}
