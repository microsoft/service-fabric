// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        // This class is used by fabrictest to mock a generated application package from a compose deployment. 
        // The test preprovisions some app types and registers them based on the app name.
        // This class stores that mapping, so it can be used when performing an operation on a compose deployment.
        class TestAppTypeNameVersionGenerator : public IApplicationDeploymentTypeNameVersionGenerator
        {
            DENY_COPY(TestAppTypeNameVersionGenerator)

        public:
            TestAppTypeNameVersionGenerator() { }
            virtual ~TestAppTypeNameVersionGenerator() { }

            virtual Common::ErrorCode GetTypeNameAndVersion(
                Store::StoreTransaction const &,
                Common::NamingUri const & appName,
                __out ServiceModelTypeName &typeName,
                __out ServiceModelVersion &typeVersion)
            {
                wstring appNameStr = appName.ToString();
                auto typeNameIter = mockTypeNames_.find(appNameStr);
                ASSERT_IF(typeNameIter == mockTypeNames_.end(), "Mock type name does not contain entry {0}", appName);

                auto typeVersionIter = mockTypeVersions_.find(appNameStr);
                ASSERT_IF(typeVersionIter == mockTypeVersions_.end(), "Mock type version does not contain entry {0}", appName);

                typeName = typeNameIter->second;
                typeVersion = typeVersionIter->second;

                return Common::ErrorCodeValue::Success;
            }

            virtual Common::ErrorCode GetNextVersion(
                Store::StoreTransaction const &,
                std::wstring const &deploymentName,
                ServiceModelVersion const &,
                __out ServiceModelVersion &typeVersion)
            {
                wstring appName = NamingUri(deploymentName).ToString();
                auto typeVersionIter = mockTypeVersions_.find(appName);
                ASSERT_IF(typeVersionIter == mockTypeVersions_.end(), "Mock type version does not container entry {0}", appName);

                typeVersion = typeVersionIter->second;
                return Common::ErrorCodeValue::Success;
            }

            bool SetMockType(
                std::wstring const & appName,
                ServiceModelTypeName && typeName,
                ServiceModelVersion && version)
            {
                return mockTypeNames_.insert({appName, move(typeName)}).second && mockTypeVersions_.insert({appName, move(version)}).second;
            }

            void EraseMockType(std::wstring const & appName)
            {
                mockTypeNames_.erase(appName);
                mockTypeVersions_.erase(appName);
            }

        private:
            std::unordered_map<std::wstring, ServiceModelTypeName> mockTypeNames_;
            std::unordered_map<std::wstring, ServiceModelVersion> mockTypeVersions_;
        };
    }
}
