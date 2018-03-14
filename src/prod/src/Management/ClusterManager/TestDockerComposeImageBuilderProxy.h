// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class TestDockerComposeImageBuilderProxy 
            : public ImageBuilderProxy
        {
            DENY_COPY(TestDockerComposeImageBuilderProxy)
        public:
            virtual ~TestDockerComposeImageBuilderProxy() {}

            static std::shared_ptr<TestDockerComposeImageBuilderProxy> Create(
                std::wstring const & imageBuilderExeDirectory,
                std::wstring const & schemaOutputPath,
                std::wstring const & nodeName,
                Federation::NodeInstance const &);

            bool SetMockAppBuildPath(std::wstring const & appName, std::wstring && value)
            {
                return mockAppBuildPaths_.insert({appName, move(value)}).second;
            }

            void EraseMockAppBuildPath(std::wstring const & appName)
            {
                mockAppBuildPaths_.erase(appName);
            }

        private:

            TestDockerComposeImageBuilderProxy(
                std::wstring const & imageBuilderExeDirectory,
                std::wstring const & schemaOutputPath,
                std::wstring const & nodeName,
                Federation::NodeInstance const &);

            virtual Common::ErrorCode ValidateComposeFile(
                Common::ByteBuffer const &,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const &) override;

            virtual Common::AsyncOperationSPtr BeginBuildComposeDeploymentType(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &,
                Common::ByteBufferSPtr const &,
                std::wstring const &,
                std::wstring const &,
                bool,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &callback,
                Common::AsyncOperationSPtr const &parent) override;

            virtual Common::ErrorCode EndBuildComposeDeploymentType(
                Common::AsyncOperationSPtr const &operation,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring &,
                __out std::wstring &,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &) override;

            virtual Common::AsyncOperationSPtr BeginBuildComposeApplicationTypeForUpgrade(
                Common::ActivityId const &,
                Common::ByteBufferSPtr const &composeFile,
                Common::ByteBufferSPtr const &overridesFile,
                std::wstring const &registryUserName,
                std::wstring const &registryPassword,
                bool isPasswordEncrypted,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                ServiceModelVersion const &,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &) override;

            virtual Common::ErrorCode EndBuildComposeApplicationTypeForUpgrade(
                Common::AsyncOperationSPtr const &,
                __out std::vector<ServiceModelServiceManifestDescription> &,
                __out std::wstring & applicationManifestId,
                __out std::wstring & applicationManifestContent,
                __out ServiceModel::ApplicationHealthPolicy &,
                __out std::map<std::wstring, std::wstring> &,
                __out std::wstring &mergedComposeFile) override;

        private:
            std::unordered_map<std::wstring, std::wstring> mockAppBuildPaths_; // <appname, appBuildPath>
        };
    }
}
