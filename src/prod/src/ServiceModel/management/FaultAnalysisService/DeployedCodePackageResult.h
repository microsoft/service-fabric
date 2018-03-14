// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class DeployedCodePackageResult : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(DeployedCodePackageResult)
            
        public:

            DeployedCodePackageResult();
            DeployedCodePackageResult(
                std::wstring const & nodeName, 
                Common::NamingUri const & applicationName, 
                std::wstring const & serviceManifestName, 
                std::wstring const & servicePackageActivationId,
                std::wstring const & codePackageName, 
                FABRIC_INSTANCE_ID const instanceId)
                : nodeName_(nodeName)
                , applicationName_(applicationName)
                , serviceManifestName_(serviceManifestName)
                , servicePackageActivationId_(servicePackageActivationId)
                , codePackageName_(codePackageName)
                , codePackageInstanceId_(instanceId)
            {};

            DeployedCodePackageResult(DeployedCodePackageResult &&);

            __declspec(property(get = get_NodeName)) std::wstring const& NodeName;
            std::wstring const& get_NodeName() const { return nodeName_; }
            __declspec(property(get = get_ApplicationName)) Common::NamingUri const& ApplicationName;
            Common::NamingUri const& get_ApplicationName() const { return applicationName_; }
            __declspec(property(get = get_ServiceManifestName)) std::wstring const& ServiceManifestName;
            std::wstring const& get_ServiceManifestName() const { return serviceManifestName_; }
            __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
            std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }
            __declspec(property(get = get_CodePackageName)) std::wstring const& CodePackageName;
            std::wstring const& get_CodePackageName() const { return codePackageName_; }
            __declspec(property(get = get_CodePackageInstanceId)) FABRIC_INSTANCE_ID CodePackageInstanceId;
            FABRIC_INSTANCE_ID get_CodePackageInstanceId() const { return codePackageInstanceId_; }

            Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_CODE_PACKAGE_RESULT const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_DEPLOYED_CODE_PACKAGE_RESULT &) const;

            FABRIC_FIELDS_06(
                nodeName_, 
                applicationName_,
                serviceManifestName_,
                codePackageName_,
                codePackageInstanceId_,
                servicePackageActivationId_);

        private:
            std::wstring nodeName_;
            Common::NamingUri applicationName_;
            std::wstring serviceManifestName_;
            std::wstring servicePackageActivationId_;
            std::wstring codePackageName_;
            FABRIC_INSTANCE_ID codePackageInstanceId_;
        };
    }
}
