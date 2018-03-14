// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeUpgradeReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        NodeUpgradeReplyMessageBody()
        {
        }

        NodeUpgradeReplyMessageBody(
            ServiceModel::ApplicationUpgradeSpecification const & upgradeSpecification,
            std::vector<Reliability::FailoverUnitInfo> && replicaList,
            std::vector<Reliability::FailoverUnitInfo> && droppedReplicaList)
            :   applicationId_(upgradeSpecification.ApplicationId),
                instanceId_(upgradeSpecification.InstanceId),
                replicas_(std::move(replicaList), std::move(droppedReplicaList), false)
        {
        }

        __declspec(property(get=get_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return applicationId_; }

        __declspec(property(get=get_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }

        __declspec(property(get=get_Replicas)) ReplicaUpMessageBody & Replicas;
        ReplicaUpMessageBody & get_Replicas() { return replicas_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("ApplicationId: {0} InstanceId: {1}\r\n{2}", applicationId_, instanceId_, replicas_);
        }

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_03(applicationId_, instanceId_, replicas_);

    private:
        ServiceModel::ApplicationIdentifier applicationId_;
        uint64 instanceId_;

        ReplicaUpMessageBody replicas_;
    };
}
