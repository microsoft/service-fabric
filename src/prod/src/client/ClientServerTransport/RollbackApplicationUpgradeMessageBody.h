// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class RollbackApplicationUpgradeMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            RollbackApplicationUpgradeMessageBody()
            {
            }

            RollbackApplicationUpgradeMessageBody(
                Common::NamingUri const & applicationName)
                : applicationName_(applicationName)
            {
            }

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;

            FABRIC_FIELDS_01(applicationName_);

        private:
            Common::NamingUri applicationName_;
        };
    }
}
