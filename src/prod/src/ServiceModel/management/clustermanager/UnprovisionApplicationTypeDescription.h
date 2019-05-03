// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {    
        class UnprovisionApplicationTypeDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_MOVE_AND_COPY(UnprovisionApplicationTypeDescription)
        public:
            UnprovisionApplicationTypeDescription();

            UnprovisionApplicationTypeDescription(
                std::wstring const & appTypeName,
                std::wstring const & appTypeVersion,
                bool isAsync);

            virtual ~UnprovisionApplicationTypeDescription();

            __declspec(property(get=get_AppTypeName)) std::wstring const & ApplicationTypeName;
            std::wstring const & get_AppTypeName() const { return appTypeName_; }

            __declspec(property(get=get_AppTypeVersion)) std::wstring const & ApplicationTypeVersion;
            std::wstring const & get_AppTypeVersion() const { return appTypeVersion_; }

            __declspec(property(get=get_IsAsync)) bool IsAsync;
            bool get_IsAsync() const { return isAsync_; }

            Common::ErrorCode FromPublicApi(FABRIC_UNPROVISION_APPLICATION_TYPE_DESCRIPTION const &);

            FABRIC_FIELDS_03(appTypeName_, appTypeVersion_, isAsync_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeName, appTypeName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ApplicationTypeVersion, appTypeVersion_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Async, isAsync_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring appTypeName_;
            std::wstring appTypeVersion_;
            bool isAsync_;
        };
    }
}
