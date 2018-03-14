// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class CServiceGroupMemberLoadMetricDescription : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
    {
    public:
        std::wstring Name;
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT Weight;
        uint PrimaryDefaultLoad;
        uint SecondaryDefaultLoad;

        FABRIC_FIELDS_04(
            Name,
            Weight,
            PrimaryDefaultLoad,
            SecondaryDefaultLoad);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, Name)
            SERIALIZABLE_PROPERTY_ENUM(Constants::Weight, Weight)
            SERIALIZABLE_PROPERTY(Constants::PrimaryDefaultLoad, PrimaryDefaultLoad)
            SERIALIZABLE_PROPERTY(Constants::SecondaryDefaultLoad, SecondaryDefaultLoad)
        END_JSON_SERIALIZABLE_PROPERTIES()

        static BOOLEAN Equals(CServiceGroupMemberLoadMetricDescription const & first, CServiceGroupMemberLoadMetricDescription const & second)
        {
            BOOLEAN isEqual = TRUE;

            isEqual = (first.Name == second.Name);
            if (!isEqual) { return isEqual; }

            isEqual = (first.Weight == second.Weight);
            if (!isEqual) { return isEqual; }

            isEqual = (first.PrimaryDefaultLoad == second.PrimaryDefaultLoad);
            if (!isEqual) { return isEqual; }

            isEqual = (first.SecondaryDefaultLoad == second.SecondaryDefaultLoad);
            if (!isEqual) { return isEqual; }

            return isEqual;
        }
    };

    class CServiceGroupMemberDescription : public Serialization::FabricSerializable, public Common::IFabricJsonSerializable
    { 
    public:
        FABRIC_PARTITION_ID Identifier;
        FABRIC_SERVICE_DESCRIPTION_KIND ServiceDescriptionType;
        std::wstring ServiceType; 
        std::wstring ServiceName; 
        std::vector<byte> ServiceGroupMemberInitializationData;
        std::vector<CServiceGroupMemberLoadMetricDescription> Metrics;
        uint DefaultMoveCost;
    
        FABRIC_FIELDS_07(
            Identifier,
            ServiceDescriptionType, 
            ServiceType, 
            ServiceName, 
            ServiceGroupMemberInitializationData,
            Metrics,
            DefaultMoveCost);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServiceKind, ServiceDescriptionType)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceTypeName, ServiceType)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, ServiceName)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::InitializationData, ServiceGroupMemberInitializationData)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceLoadMetrics, Metrics)
        END_JSON_SERIALIZABLE_PROPERTIES()

        static BOOLEAN Equals(CServiceGroupMemberDescription const & first, CServiceGroupMemberDescription const & second, BOOLEAN ignoreMemberIdentifier)
        {
            BOOLEAN isEqual = TRUE;

            if (!ignoreMemberIdentifier)
            {
                isEqual = (first.Identifier == second.Identifier);
                if (!isEqual) { return isEqual; }
            }

            isEqual = (first.ServiceDescriptionType == second.ServiceDescriptionType);
            if (!isEqual) { return isEqual; }

            isEqual = (first.ServiceType == second.ServiceType);
            if (!isEqual) { return isEqual; }

            isEqual = (first.ServiceName == second.ServiceName);
            if (!isEqual) { return isEqual; }

            isEqual = (first.Metrics.size() == second.Metrics.size());
            if (!isEqual) { return isEqual; }

            for (size_t i = 0; i < first.Metrics.size(); ++i)
            {
                isEqual = CServiceGroupMemberLoadMetricDescription::Equals(first.Metrics[i], second.Metrics[i]);
                if (!isEqual) { return isEqual; }
            }

            isEqual = (first.ServiceGroupMemberInitializationData.size() == second.ServiceGroupMemberInitializationData.size());
            if (!isEqual) { return isEqual; }

            for (size_t i = 0; i < first.ServiceGroupMemberInitializationData.size(); ++i)
            {
                isEqual = (first.ServiceGroupMemberInitializationData[i] == second.ServiceGroupMemberInitializationData[i]);
                if (!isEqual) { return isEqual; }
            }

            return isEqual;
        }
    };

    class CServiceGroupDescription : public Serialization::FabricSerializable
    {
    public:
        BOOLEAN HasPersistedState;
        std::vector<byte> ServiceGroupInitializationData;
        std::vector<CServiceGroupMemberDescription> ServiceGroupMemberData;
    
        FABRIC_FIELDS_03(
            HasPersistedState,
            ServiceGroupInitializationData, 
            ServiceGroupMemberData);

        static BOOLEAN Equals(CServiceGroupDescription const & first, CServiceGroupDescription const & second, BOOLEAN ignoreMemberIdentifier)
        {
            BOOLEAN isEqual = TRUE;
            isEqual = (first.HasPersistedState == second.HasPersistedState);
            if (!isEqual) { return isEqual; }

            isEqual = (first.ServiceGroupMemberData.size() == second.ServiceGroupMemberData.size());
            if (!isEqual) { return isEqual; }

            for (size_t i = 0; i < first.ServiceGroupMemberData.size(); ++i)
            {
                isEqual = CServiceGroupMemberDescription::Equals(first.ServiceGroupMemberData[i], second.ServiceGroupMemberData[i], ignoreMemberIdentifier);
                if (!isEqual) { return isEqual; }
            }

            isEqual = (first.ServiceGroupInitializationData.size() == second.ServiceGroupInitializationData.size());
            if (!isEqual) { return isEqual; }

            for (size_t i = 0; i < first.ServiceGroupInitializationData.size(); ++i)
            {
                isEqual = (first.ServiceGroupInitializationData[i] == second.ServiceGroupInitializationData[i]);
                if (!isEqual) { return isEqual; }
            }

            return isEqual;
        }

        static BOOLEAN Equals(std::vector<byte> const & first, std::vector<byte> const & second, BOOLEAN ignoreMemberIdentifier)
        {
            CServiceGroupDescription deserializedFirst;
            CServiceGroupDescription deserializedSecond;

            std::vector<byte> copyFirst(first);
            std::vector<byte> copySecond(second);

            Common::ErrorCode error = Common::FabricSerializer::Deserialize(deserializedFirst, copyFirst);
            ASSERT_IFNOT(error.IsSuccess(), "Could not deserialize first due to {0}.", error);

            error = Common::FabricSerializer::Deserialize(deserializedSecond, copySecond);
            ASSERT_IFNOT(error.IsSuccess(), "Could not deserialize second due to {0}.", error);

            return Equals(deserializedFirst, deserializedSecond, ignoreMemberIdentifier);
        }
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::CServiceGroupMemberLoadMetricDescription);
DEFINE_USER_ARRAY_UTILITY(ServiceModel::CServiceGroupMemberDescription);
