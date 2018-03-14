// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceGroupMemberTypeQueryResult 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceGroupMemberTypeQueryResult();

        ServiceGroupMemberTypeQueryResult(
            std::vector<ServiceModel::ServiceGroupTypeMemberDescription> const & serviceGroupMemberTypeDescription, 
            std::wstring const & serviceManifestVersion,
            std::wstring const & serviceManifestName);
        
        __declspec(property(get=get_ServiceManifestVersion)) std::wstring const & ServiceManifestVersion;
        std::wstring const & get_ServiceManifestVersion() { return serviceManifestVersion_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() { return serviceManifestName_; }

        __declspec(property(get = get_ServiceGroupMemberTypeDescriptionObj)) std::vector<ServiceModel::ServiceGroupTypeMemberDescription> const & ServiceGroupMemberTypeDescriptionObj;
        std::vector<ServiceModel::ServiceGroupTypeMemberDescription> const & get_ServiceGroupMemberTypeDescriptionObj() { return serviceGroupMembers_; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_GROUP_MEMBER_TYPE_QUERY_RESULT_ITEM & publicServiceGroupMemberTypeQueryResult) const;

        FABRIC_FIELDS_03(serviceGroupMembers_, serviceManifestVersion_, serviceManifestName_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceGroupTypeMemberDescription, serviceGroupMembers_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestVersion, serviceManifestVersion_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::vector<ServiceModel::ServiceGroupTypeMemberDescription> serviceGroupMembers_;
        std::wstring serviceManifestVersion_;
        std::wstring serviceManifestName_;
    };
}
