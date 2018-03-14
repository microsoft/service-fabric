// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class RestartDeployedCodePackageDescriptionUsingNodeName
            : public Serialization::FabricSerializable
        {
        public:

            RestartDeployedCodePackageDescriptionUsingNodeName();
			RestartDeployedCodePackageDescriptionUsingNodeName(
				std::wstring const & nodeName,
				Common::NamingUri const & applicationName,
				std::wstring const & serviceManifestName,
				std::wstring const & servicePackageActivationId,
				std::wstring const & codePackageName,
				FABRIC_INSTANCE_ID codePackageInstanceId);

            RestartDeployedCodePackageDescriptionUsingNodeName(RestartDeployedCodePackageDescriptionUsingNodeName const &);
            RestartDeployedCodePackageDescriptionUsingNodeName(RestartDeployedCodePackageDescriptionUsingNodeName &&);

            __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get = get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get = get_ServiceManifestName)) std::wstring const & ServiceManifestName;
            __declspec(property(get = get_ServicePackageActivationId)) std::wstring const & ServicePackageActivationId;
            __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
            __declspec(property(get = get_CodePackageInstanceId)) FABRIC_INSTANCE_ID CodePackageInstanceId;

            std::wstring const & get_NodeName() const { return nodeName_; }
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            std::wstring const & get_ServiceManifestName() const { return serviceManifestName_; }
            std::wstring const & get_ServicePackageActivationId() const { return servicePackageActivationId_; }
            std::wstring const & get_CodePackageName() const { return codePackageName_; }
            FABRIC_INSTANCE_ID get_CodePackageInstanceId() const { return codePackageInstanceId_; }

            Common::ErrorCode FromPublicApi(FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_RESTART_DEPLOYED_CODE_PACKAGE_DESCRIPTION_USING_NODE_NAME &) const;

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
