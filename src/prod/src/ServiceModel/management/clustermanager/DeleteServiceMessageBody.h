// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class DeleteServiceMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            DeleteServiceMessageBody()
                : isForce_(false)
                , applicationName_()
                , serviceName_()
            {
            }

            DeleteServiceMessageBody(
                Common::NamingUri const & applicationName,
                Common::NamingUri const & serviceName,
                bool const isForce)
                : applicationName_(applicationName)
                , serviceName_(serviceName)
                , isForce_(isForce)
            {
            }

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_ServiceName)) Common::NamingUri const & ServiceName;
            __declspec(property(get=get_IsForce)) bool IsForce;

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            Common::NamingUri const & get_ServiceName() const { return serviceName_; }
            bool get_IsForce() const { return isForce_; }

            FABRIC_FIELDS_03(applicationName_, serviceName_, isForce_);

        private:
            Common::NamingUri applicationName_;
            Common::NamingUri serviceName_;
            bool isForce_;
        };
    }
}

