// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class FabricUpgradeSpecification : public Serialization::FabricSerializable
    {
    public:
        FabricUpgradeSpecification();
        FabricUpgradeSpecification(
            Common::FabricVersion const &, 
            uint64 instanceId, 
            UpgradeType::Enum,
            bool isMonitored,
            bool isManual);
        FabricUpgradeSpecification(FabricUpgradeSpecification const & other);
        FabricUpgradeSpecification(FabricUpgradeSpecification && other);

        __declspec(property(get=get_FabricVersion, put=put_FabricVersion)) Common::FabricVersion const & Version;
        Common::FabricVersion const & get_FabricVersion() const { return fabricVersion_; }
        void put_FabricVersion(Common::FabricVersion const & value) { fabricVersion_ = value; }

        __declspec(property(get=get_InstanceId, put=put_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }
        void put_InstanceId(uint64 value) { instanceId_ = value; }

        __declspec(property(get=get_UpgradeType)) UpgradeType::Enum UpgradeType;
        UpgradeType::Enum get_UpgradeType() const { return upgradeType_; }
        void SetUpgradeType(UpgradeType::Enum value) { upgradeType_ = value; }

        __declspec(property(get=get_IsMonitored)) bool IsMonitored;
        bool get_IsMonitored() const { return isMonitored_; }
        void SetIsMonitored(bool value) { isMonitored_ = value; }

        __declspec(property(get=get_IsManual)) bool IsManual;
        bool get_IsManual() const { return isManual_; }
        void SetIsManual(bool value) { isManual_ = value; }

        FabricUpgradeSpecification const & operator = (FabricUpgradeSpecification const & other);
        FabricUpgradeSpecification const & operator = (FabricUpgradeSpecification && other);

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(fabricVersion_, instanceId_, upgradeType_, isMonitored_, isManual_);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;


    private:
        Common::FabricVersion fabricVersion_;
        uint64 instanceId_;
        UpgradeType::Enum upgradeType_;
        bool isMonitored_;
        bool isManual_;
    };
}

