// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class FabricDeploymentSpecification
        {
        public:
            FabricDeploymentSpecification();
            FabricDeploymentSpecification(std::wstring const & dataRoot);
            FabricDeploymentSpecification(std::wstring const & dataRoot, std::wstring const & logRoot);
            FabricDeploymentSpecification(FabricDeploymentSpecification const & other);

            FabricDeploymentSpecification& operator = (FabricDeploymentSpecification const & other);

            std::wstring GetLogFolder() const;

            std::wstring GetTracesFolder() const;

            std::wstring GetCrashDumpsFolder() const;

            std::wstring GetApplicationCrashDumpsFolder() const;

            std::wstring GetAppInstanceDataFolder() const;

            std::wstring GetAppInstanceEtlDataFolder() const;

            std::wstring GetPerformanceCountersBinaryFolder() const;

            std::wstring GetTargetInformationFile() const;              

            std::wstring GetNodeFolder(std::wstring const & nodeName) const;

            std::wstring GetCurrentClusterManifestFile(std::wstring const & nodeName) const;

            std::wstring GetVersionedClusterManifestFile(std::wstring const & nodeName, std::wstring const & version) const;                          

            std::wstring GetInstallerScriptFile(std::wstring const & nodeName) const;

            std::wstring GetInstallerLogFile(std::wstring const & nodeName, std::wstring const & codeVersion) const;

#if defined(PLATFORM_UNIX)
            std::wstring GetUpgradeScriptFile(std::wstring const & nodeName) const;
#endif

            std::wstring GetInfrastructureManfiestFile(std::wstring const & nodeName);

            std::wstring GetConfigurationDeploymentFolder(std::wstring const & nodeName, std::wstring const & configVersion) const;

            std::wstring GetDataDeploymentFolder(std::wstring const & nodeName) const;

            std::wstring GetCodeDeploymentFolder(std::wstring const & nodeName, std::wstring const & service) const;

            std::wstring GetInstalledBinaryFolder(std::wstring const & installationDirectory, std::wstring const & service) const;

            std::wstring GetWorkFolder(std::wstring const & nodeName) const;

            std::wstring GetVersionedFabricPackageFile(std::wstring const & nodeName, std::wstring const & version) const;                          

            std::wstring GetCurrentFabricPackageFile(std::wstring const & nodeName) const;

            std::wstring GetFabricFolder(std::wstring const & nodeName) const;

            std::wstring GetQueryTraceFolder() const;

            _declspec(property(get=get_DataRoot, put=set_DataRoot)) std::wstring & DataRoot;
            std::wstring const & get_DataRoot() const { return dataRoot_; }
            void set_DataRoot(std::wstring const &);
            
            _declspec(property(get=get_LogRoot, put=set_LogRoot)) std::wstring & LogRoot;
            std::wstring const & get_LogRoot() const { return logRoot_; }
            void set_LogRoot(std::wstring const & value) { logRoot_.assign(value); }

            _declspec(property(get=get_EnableCircularTraceSession, put=set_EnableCircularTraceSession)) BOOLEAN & EnableCircularTraceSession;
            BOOLEAN const & get_EnableCircularTraceSession() const { return enableCircularTraceSession_; }
            void set_EnableCircularTraceSession(BOOLEAN const & value) { enableCircularTraceSession_ = value; }

        private:
            std::wstring dataRoot_;
            std::wstring logRoot_;
            BOOLEAN enableCircularTraceSession_;
        };
    }
}
