// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceGroupFromTemplateDescription :
        public Serialization::FabricSerializable,
        public Common::IFabricJsonSerializable
    {
    public:
        ServiceGroupFromTemplateDescription();

        ServiceGroupFromTemplateDescription(
            Common::NamingUri const & applicationName,
            Common::NamingUri const & serviceName,
            std::wstring const & serviceTypeName,
            ServicePackageActivationMode::Enum servicePackageActivationMode,
            std::vector<byte> const & initializationData);

        ServiceGroupFromTemplateDescription(
            ServiceGroupFromTemplateDescription const & other);

        ServiceGroupFromTemplateDescription operator=(
            ServiceGroupFromTemplateDescription const & other);

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_GROUP_FROM_TEMPLATE_DESCRIPTION const & svcFromTemplateDesc);
        
        Common::ErrorCode FromPublicApi(
            FABRIC_URI applicationName,
            FABRIC_URI serviceName,
            LPCWSTR serviceTypeName,
            ULONG initializationDataSize,
            BYTE *initializationData);

        __declspec(property(get = get_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

        __declspec(property(get = get_ServiceName)) Common::NamingUri const & ServiceName;
        Common::NamingUri const & get_ServiceName() const { return serviceName_; }

        __declspec(property(get = get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        std::wstring const & get_ServiceTypeName() const { return serviceTypeName_; }

        __declspec(property(get = get_ServicePackageActivationMode)) ServicePackageActivationMode::Enum ServicePackageActivationMode;
        ServicePackageActivationMode::Enum get_ServicePackageActivationMode() const { return servicePackageActivationMode_; }

        __declspec(property(get = get_InitializationData)) std::vector<byte> const & InitializationData;
        std::vector<byte> const & get_InitializationData() const { return initializationData_; }

        void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(applicationName_, serviceName_, serviceTypeName_, servicePackageActivationMode_, initializationData_);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationName, applicationName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceName, serviceName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ServiceTypeName, serviceTypeName_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::InitializationData, initializationData_)
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ServicePackageActivationMode, servicePackageActivationMode_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        Common::ErrorCode FromPublicApiInternal(
            FABRIC_URI applicationName,
            FABRIC_URI serviceName,
            LPCWSTR serviceTypeName,
            FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE servicePackageActivationMode,
            ULONG initializationDataSize,
            BYTE *initializationData);

        Common::NamingUri applicationName_;
        Common::NamingUri serviceName_;
        std::wstring serviceTypeName_;
        ServicePackageActivationMode::Enum servicePackageActivationMode_;
        std::vector<byte> initializationData_;
    };
}
