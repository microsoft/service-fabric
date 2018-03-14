// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CreateServiceFromTemplateMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            CreateServiceFromTemplateMessageBody()
                : servicePackageActivationMode_(ServiceModel::ServicePackageActivationMode::SharedProcess)
            {
            }

			CreateServiceFromTemplateMessageBody(
				ServiceModel::ServiceFromTemplateDescription const & serviceFromTemplateDesc)
				: applicationName_(serviceFromTemplateDesc.ApplicationName),
				serviceName_(serviceFromTemplateDesc.ServiceName),
				serviceDnsName_(serviceFromTemplateDesc.ServiceDnsName),
				serviceType_(serviceFromTemplateDesc.ServiceTypeName),
				initializationData_(serviceFromTemplateDesc.InitializationData),
				servicePackageActivationMode_(serviceFromTemplateDesc.ServicePackageActivationMode)
			{
			}

            CreateServiceFromTemplateMessageBody(
                Common::NamingUri const & applicationName,
                Common::NamingUri const & serviceName,
				std::wstring const & serviceDnsName,
                std::wstring const & serviceType,
                ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode,
                std::vector<byte> const & initializationData)
                : applicationName_(applicationName),
                serviceName_(serviceName),
				serviceDnsName_(serviceDnsName),
                serviceType_(serviceType),
                initializationData_(initializationData),
                servicePackageActivationMode_(servicePackageActivationMode)
            {
            }

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;

            Common::NamingUri const & get_ServiceName() const { return serviceName_; }
            __declspec(property(get=get_ServiceName)) Common::NamingUri const & ServiceName;

			std::wstring const & get_ServiceType() const { return serviceType_; }
			__declspec(property(get = get_ServiceType)) std::wstring const & ServiceType;

            std::wstring const & get_ServiceDnsName() const { return serviceDnsName_; }
            __declspec(property(get=get_ServiceDnsName)) std::wstring const & ServiceDnsName;

            std::vector<byte> const & get_InitializationData() const { return initializationData_; }
            __declspec(property(get=get_InitializationData)) std::vector<byte> const & InitializationData;

            __declspec (property(get = get_PackageActivationMode)) ServiceModel::ServicePackageActivationMode::Enum PackageActivationMode;
            ServiceModel::ServicePackageActivationMode::Enum get_PackageActivationMode() const { return servicePackageActivationMode_; }

            FABRIC_FIELDS_06(applicationName_, serviceName_, serviceType_, initializationData_, servicePackageActivationMode_, serviceDnsName_);

        private:
            Common::NamingUri applicationName_;
            Common::NamingUri serviceName_;
			std::wstring serviceDnsName_;
            std::wstring serviceType_;
            std::vector<byte> initializationData_;
            ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode_;
        };
    }
}
