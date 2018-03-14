// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class UpgradeDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(UpgradeDescription);
        DEFAULT_COPY_ASSIGNMENT(UpgradeDescription);

    public:
        UpgradeDescription()
        {
        }

        UpgradeDescription(ServiceModel::ApplicationUpgradeSpecification && specification);

        UpgradeDescription(UpgradeDescription && other);

        UpgradeDescription & operator = (UpgradeDescription && other);

        __declspec (property(get=get_Specification)) ServiceModel::ApplicationUpgradeSpecification & Specification;
        ServiceModel::ApplicationUpgradeSpecification const & get_Specification() const { return specification_; }
        ServiceModel::ApplicationUpgradeSpecification & get_Specification() { return specification_; }

        __declspec (property(get=get_UpgradeType)) ServiceModel::UpgradeType::Enum UpgradeType;
        ServiceModel::UpgradeType::Enum get_UpgradeType() const { return specification_.UpgradeType; }

        __declspec(property(get=get_ApplicationId, put=put_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return specification_.ApplicationId; }

        __declspec(property(get=get_InstanceId, put=set_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return specification_.InstanceId; }
        void set_InstanceId(uint64 value) { specification_.InstanceId = value; }

        __declspec(property(get=get_IsMonitored)) bool IsMonitored;
        bool get_IsMonitored() const { return specification_.IsMonitored; }

        __declspec(property(get = get_IsManual)) bool IsManual;
        bool get_IsManual() const { return specification_.IsManual; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_01(specification_);

    private:
        ServiceModel::ApplicationUpgradeSpecification specification_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::UpgradeDescription);
