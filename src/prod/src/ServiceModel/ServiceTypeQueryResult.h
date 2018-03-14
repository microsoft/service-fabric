// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServiceTypeQueryResult 
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceTypeQueryResult();

        ServiceTypeQueryResult(
            ServiceModel::ServiceTypeDescription const & serviceTypeDescription, 
            std::wstring const & serviceManifestVersion,
            std::wstring const & serviceManifestName);
        
        __declspec(property(get=get_ServiceManifestVersion)) std::wstring const & ServiceManifestVersion;
        std::wstring const & get_ServiceManifestVersion() { return serviceManifestVersion_; }

        __declspec(property(get=get_ServiceManifestName)) std::wstring const & ServiceManifestName;
        std::wstring const & get_ServiceManifestName() { return serviceManifestName_; }

        __declspec(property(get=get_ServiceTypeDescriptionObj)) ServiceModel::ServiceTypeDescription const & ServiceTypeDescriptionObj;
        ServiceModel::ServiceTypeDescription const & get_ServiceTypeDescriptionObj() { return serviceTypeDescription_; }

        __declspec(property(get = get_IsServiceGroup, put = put_IsServiceGroup)) bool IsServiceGroup;
        bool get_IsServiceGroup() const { return isServiceGroup_; }
        void put_IsServiceGroup(bool isServiceGroup) { isServiceGroup_ = isServiceGroup; }

        void ToPublicApi(
            __in Common::ScopedHeap & heap, 
            __out FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM & publicServiceTypeQueryResult) const ;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        std::wstring ToString() const;

        Common::ErrorCode FromPublicApi(__in FABRIC_SERVICE_TYPE_QUERY_RESULT_ITEM const& publicServiceTypeQueryResult);

        FABRIC_FIELDS_04(serviceTypeDescription_, serviceManifestVersion_, serviceManifestName_, isServiceGroup_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ServiceTypeDescription, serviceTypeDescription_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestVersion, serviceManifestVersion_)
            SERIALIZABLE_PROPERTY(Constants::ServiceManifestName, serviceManifestName_)
            SERIALIZABLE_PROPERTY(Constants::IsServiceGroup, isServiceGroup_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        ServiceModel::ServiceTypeDescription serviceTypeDescription_;
        std::wstring serviceManifestVersion_;
        std::wstring serviceManifestName_;
        bool isServiceGroup_;
    };

}
