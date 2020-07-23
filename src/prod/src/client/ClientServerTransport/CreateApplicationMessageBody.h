// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class CreateApplicationMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            CreateApplicationMessageBody()
            {
            }

            CreateApplicationMessageBody(
                Common::NamingUri const & applicationName,
                std::wstring const & applicationType,
                std::wstring const & applicationVersion,
                std::map<std::wstring, std::wstring> const & applicationParameters,
                Reliability::ApplicationCapacityDescription const & applicationCapacity)
                : applicationName_(applicationName),
                applicationType_(applicationType),
                applicationVersion_(applicationVersion),
                applicationParameters_(applicationParameters),
                applicationCapacity_(applicationCapacity)
            {
            }

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;

            std::wstring const & get_ApplicationType() const { return applicationType_; }
            __declspec(property(get=get_ApplicationType)) std::wstring const & ApplicationType;

            std::wstring const & get_ApplicationVersion() const { return applicationVersion_; }
            __declspec(property(get=get_ApplicationVersion)) std::wstring const & ApplicationVersion;

            std::map<std::wstring, std::wstring> const & get_ApplicationParameters() const { return applicationParameters_; }
            __declspec(property(get=get_ApplicationParameters)) std::map<std::wstring, std::wstring> const & ApplicationParameters;

            __declspec(property(get=get_ApplicationCapacity)) Reliability::ApplicationCapacityDescription const & ApplicationCapacity;

            Reliability::ApplicationCapacityDescription const & get_ApplicationCapacity() const { return applicationCapacity_; }

            FABRIC_FIELDS_05(applicationName_,
                applicationType_, 
                applicationVersion_, 
                applicationParameters_,
                applicationCapacity_);

        private:
            Common::NamingUri applicationName_;
            std::wstring applicationType_;
            std::wstring applicationVersion_;
            std::map<std::wstring, std::wstring> applicationParameters_;
            Reliability::ApplicationCapacityDescription applicationCapacity_;
        };
    }
}
