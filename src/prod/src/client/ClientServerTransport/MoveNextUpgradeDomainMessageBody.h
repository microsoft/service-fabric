// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class MoveNextUpgradeDomainMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            MoveNextUpgradeDomainMessageBody()
            {
            }

            explicit MoveNextUpgradeDomainMessageBody(
                Common::NamingUri const & applicationName,
                std::wstring const & nextUpgradeDomain)
                : applicationName_(applicationName) 
                , nextUpgradeDomain_(nextUpgradeDomain)
            {
            }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }

            __declspec(property(get=get_NextUpgradeDomain)) std::wstring const & NextUpgradeDomain;
            std::wstring const & get_NextUpgradeDomain() { return nextUpgradeDomain_; }

            FABRIC_FIELDS_02(applicationName_, nextUpgradeDomain_)

        private:
            Common::NamingUri applicationName_;
            std::wstring nextUpgradeDomain_;
        };
    }
}
