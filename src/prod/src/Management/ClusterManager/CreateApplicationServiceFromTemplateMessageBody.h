// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CreateApplicationServiceFromTemplateMessageBody : public Serialization::FabricSerializable
        {
        public:
            CreateApplicationServiceFromTemplateMessageBody()
            {
            }

            CreateApplicationServiceFromTemplateMessageBody(
                Common::NamingUri const & applicationName,
                Common::NamingUri const & serviceName,
                std::wstring const & serviceType,
                std::vector<byte> const & initializationData)
                : applicationName_(applicationName),
                serviceName_(serviceName),
                serviceType_(serviceType),
                initializationData_(initializationData)
            {
            }

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;

            Common::NamingUri const & get_ServiceName() const { return serviceName_; }
            __declspec(property(get=get_ServiceName)) Common::NamingUri const & ServiceName;

            std::wstring const & get_ServiceType() const { return serviceType_; }
            __declspec(property(get=get_ServiceType)) std::wstring const & ServiceType;

            std::vector<byte> const & get_InitializationData() const { return initializationData_; }
            __declspec(property(get=get_InitializationData)) std::vector<byte> const & InitializationData;

            FABRIC_FIELDS_04(applicationName_, serviceName_, serviceType_, initializationData_);

        private:
            Common::NamingUri applicationName_;
            Common::NamingUri serviceName_;
            std::wstring serviceType_;
            std::vector<byte> initializationData_;
        };
    }
}
