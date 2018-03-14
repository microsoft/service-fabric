// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ReconfigurationInformation
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ReconfigurationInformation();
        ReconfigurationInformation(
            FABRIC_REPLICA_ROLE previousConfigurationRole,
            FABRIC_RECONFIGURATION_PHASE reconfigurationPhase,
            FABRIC_RECONFIGURATION_TYPE reconfigurationType,
            Common::DateTime reconfigurationStartTimeUtc);

        __declspec(property(get = get_PreviousConfigurationRole)) FABRIC_REPLICA_ROLE PrevioudConfigurationRole;
        FABRIC_REPLICA_ROLE get_PreviousConfigurationRole() const { return previousConfigurationRole_; }

        __declspec(property(get = get_ReconfigurationPhase)) FABRIC_RECONFIGURATION_PHASE ReconfigurationPhase;
        FABRIC_RECONFIGURATION_PHASE get_ReconfigurationPhase() const { return reconfigurationPhase_; }

        __declspec(property(get = get_ReconfigurationType)) FABRIC_RECONFIGURATION_TYPE ReconfigurationType;
        FABRIC_RECONFIGURATION_TYPE get_ReconfigurationType() const { return reconfigurationType_; }

        __declspec(property(get = get_ReconfigurationStartTimeUtc)) Common::DateTime ReconfigurationStartTimeUtc;
        Common::DateTime get_ReconfigurationStartTimeUtc() const { return reconfigurationStartTimeUtc_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_RECONFIGURATION_INFORMATION_QUERY_RESULT & reconfigurationInformationQueryResult) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(Constants::PreviousConfigurationRole, previousConfigurationRole_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ReconfigurationPhase, reconfigurationPhase_)
            SERIALIZABLE_PROPERTY_ENUM(Constants::ReconfigurationType, reconfigurationType_)
            SERIALIZABLE_PROPERTY(Constants::ReconfigurationStartTimeUtc, reconfigurationStartTimeUtc_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_04(
                previousConfigurationRole_,
                reconfigurationPhase_,
                reconfigurationType_,
                reconfigurationStartTimeUtc_);

    private:
        FABRIC_REPLICA_ROLE previousConfigurationRole_;
        FABRIC_RECONFIGURATION_PHASE reconfigurationPhase_;
        FABRIC_RECONFIGURATION_TYPE reconfigurationType_;
        Common::DateTime reconfigurationStartTimeUtc_;
    };
}
