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

        static Common::ErrorCode Equals(CServiceGroupMemberLoadMetricDescription const & first, CServiceGroupMemberLoadMetricDescription const & second)
        {
            if (first.Name != second.Name)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberLoadMetricDescription_Name_Changed), first.Name, second.Name));
            }

            if (first.Weight != second.Weight)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberLoadMetricDescription_Weight_Changed), first.Weight, second.Weight));
            }

            if (first.PrimaryDefaultLoad != second.PrimaryDefaultLoad)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberLoadMetricDescription_PrimaryDefaultLoad_Changed), first.PrimaryDefaultLoad, second.PrimaryDefaultLoad));
            }

            if (first.SecondaryDefaultLoad != second.SecondaryDefaultLoad)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberLoadMetricDescription_SecondaryDefaultLoad_Changed), first.SecondaryDefaultLoad, second.SecondaryDefaultLoad));
            }

            return Common::ErrorCode::Success();
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

        static Common::ErrorCode Equals(CServiceGroupMemberDescription const & first, CServiceGroupMemberDescription const & second, BOOLEAN ignoreMemberIdentifier)
        {
            if (!ignoreMemberIdentifier)
            {
                if (first.Identifier != second.Identifier)
                {
                    return Common::ErrorCode(
                        Common::ErrorCodeValue::InvalidArgument,
                        GET_FM_RC(ServiceGroupMemberDescription_Member_Identifier_Changed));
                }
            }

            if (first.ServiceDescriptionType != second.ServiceDescriptionType)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberDescription_ServiceDescriptionType_Changed), first.ServiceDescriptionType, second.ServiceDescriptionType));
            }

            if (first.ServiceType != second.ServiceType)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberDescription_ServiceType_Changed), first.ServiceType, second.ServiceType));
            }

            if (first.ServiceName != second.ServiceName)
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberDescription_ServiceName_Changed), first.ServiceName, second.ServiceName));
            }

            if (first.Metrics.size() != second.Metrics.size())
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupMemberDescription_Load_Metric_Count_Changed), first.Metrics.size(), second.Metrics.size()));
            }

            for (size_t i = 0; i < first.Metrics.size(); ++i)
            {
                auto error = CServiceGroupMemberLoadMetricDescription::Equals(first.Metrics[i], second.Metrics[i]);
                if (!error.IsSuccess()) { return error; }
            }

            if (first.ServiceGroupMemberInitializationData.size() != second.ServiceGroupMemberInitializationData.size())
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupDescription_ServiceGroupInitializationData_Size_Changed), first.ServiceGroupMemberInitializationData.size(), second.ServiceGroupMemberInitializationData.size()));
            }

            for (size_t i = 0; i < first.ServiceGroupMemberInitializationData.size(); ++i)
            {
                if (first.ServiceGroupMemberInitializationData[i] != second.ServiceGroupMemberInitializationData[i])
                {
                    return Common::ErrorCode(
                        Common::ErrorCodeValue::InvalidArgument,
                        Common::wformatString(
                            GET_FM_RC(ServiceGroupDescription_ServiceGroupInitializationData_Changed), i, first.ServiceGroupMemberInitializationData[i], second.ServiceGroupMemberInitializationData[i]));
                }
            }

            return Common::ErrorCode::Success();
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

        static Common::ErrorCode Equals(CServiceGroupDescription const & first, CServiceGroupDescription const & second, BOOLEAN ignoreMemberIdentifier)
        {
            if (first.HasPersistedState != second.HasPersistedState)
            {
                if (first.HasPersistedState)
                {
                    return Common::ErrorCode(
                        Common::ErrorCodeValue::InvalidArgument,
                        GET_FM_RC(ServiceGroupDescription_Changed_To_NonPersisted));
                }
                else
                {
                    return Common::ErrorCode(
                        Common::ErrorCodeValue::InvalidArgument,
                        GET_FM_RC(ServiceGroupDescription_Changed_To_Persisted));
                }
            }

            if (first.ServiceGroupMemberData.size() != second.ServiceGroupMemberData.size())
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupDescription_ServiceGroupMemberData_Size_Changed), first.ServiceGroupMemberData.size(), second.ServiceGroupMemberData.size()));
            }

            for (size_t i = 0; i < first.ServiceGroupMemberData.size(); ++i)
            {
                auto error = CServiceGroupMemberDescription::Equals(first.ServiceGroupMemberData[i], second.ServiceGroupMemberData[i], ignoreMemberIdentifier);
                if (!error.IsSuccess())
                {
                    return error;
                }
            }

            if (first.ServiceGroupInitializationData.size() != second.ServiceGroupInitializationData.size())
            {
                return Common::ErrorCode(
                    Common::ErrorCodeValue::InvalidArgument,
                    Common::wformatString(
                        GET_FM_RC(ServiceGroupDescription_ServiceGroupInitializationData_Size_Changed), first.ServiceGroupInitializationData.size(), second.ServiceGroupInitializationData.size()));
            }

            for (size_t i = 0; i < first.ServiceGroupInitializationData.size(); ++i)
            {
                if (first.ServiceGroupInitializationData[i] != second.ServiceGroupInitializationData[i])
                {
                    return Common::ErrorCode(
                        Common::ErrorCodeValue::InvalidArgument,
                        Common::wformatString(
                            GET_FM_RC(ServiceGroupDescription_ServiceGroupInitializationData_Changed), i, first.ServiceGroupInitializationData[i], second.ServiceGroupInitializationData[i]));
                }
            }

            return Common::ErrorCode::Success();
        }

        static Common::ErrorCode Equals(std::vector<byte> const & first, std::vector<byte> const & second, BOOLEAN ignoreMemberIdentifier)
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
