// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ProcessDescription : public Serialization::FabricSerializable
    {

    public:
        ProcessDescription();
        ProcessDescription(
            std::wstring const & exePath,
            std::wstring const & arguments,
            std::wstring const & startInDirectory,
            Common::EnvironmentMap const & envVars,
            std::wstring const & appDirectory,
            std::wstring const & tempDirectory,
            std::wstring const & workDirectory,
            std::wstring const & logDirectory,
            bool showNoWindow,
            bool redirectConsole,
            bool allowChildProcessDetach,
            bool notAttachedToJob,
            std::wstring const & redirectedConsoleFileNamePrefix = L"",
            LONG consoleRedirectionFileRetentionCount = 0,
            LONG consoleRedirectionFileMaxSizeInKb = 0,
            ProcessDebugParameters const & debugParameters = ProcessDebugParameters(),
            ServiceModel::ResourceGovernancePolicyDescription const & resourceGovernance = ServiceModel::ResourceGovernancePolicyDescription(),
            ServiceModel::ServicePackageResourceGovernanceDescription const & spResourceGovernance = ServiceModel::ServicePackageResourceGovernanceDescription(),
            std::wstring const & cgroupOrJobObjectName = L"",
            bool isHostedServiceProcess = false,
            std::map<std::wstring, std::wstring> const& encryptedSettings = std::map<std::wstring, std::wstring>());

        ProcessDescription(ProcessDescription const & other);

        ProcessDescription(ProcessDescription && other);

        ProcessDescription & operator = (ProcessDescription const & other) = default;
        ProcessDescription & operator = (ProcessDescription && other) = default;
        
        __declspec(property(get=get_ExePath)) std::wstring const & ExePath;
        inline std::wstring const & get_ExePath() const { return exePath_; };

        __declspec(property(get=get_Arguments)) std::wstring const & Arguments;
        inline std::wstring const & get_Arguments() const { return arguments_; };

        __declspec(property(get=get_WorkingDirectory)) std::wstring const & StartInDirectory;
        inline std::wstring const & get_WorkingDirectory() const { return startInDirectory_; };

        __declspec(property(get=get_EnvironmentBlock)) std::vector<wchar_t> const & EnvironmentBlock;
        inline std::vector<wchar_t> const & get_EnvironmentBlock() const { return environmentBlock_; };

        __declspec(property(get = get_EnvironmentVariables)) Common::EnvironmentMap const & EnvironmentVariables;
        inline Common::EnvironmentMap const & get_EnvironmentVariables() const { return envVars_; };

        __declspec(property(get=get_AppDirectory)) std::wstring const & AppDirectory;
        inline std::wstring const & get_AppDirectory() const { return appDirectory_; };

        __declspec(property(get=get_TempDirectory)) std::wstring const & TempDirectory;
        inline std::wstring const & get_TempDirectory() const { return tempDirectory_; };

        __declspec(property(get=get_WorkDirectory)) std::wstring const & WorkDirectory;
        inline std::wstring const & get_WorkDirectory() const { return workDirectory_; };

        __declspec(property(get=get_LogDirectory)) std::wstring const & LogDirectory;
        inline std::wstring const & get_LogDirectory() const { return logDirectory_; };

        __declspec(property(get=get_redirectConsole)) bool RedirectConsole;
        inline bool get_redirectConsole() const { return redirectConsole_; };

        __declspec(property(get=get_redirectedConsoleFileNamePrefix)) std::wstring const & RedirectedConsoleFileNamePrefix;
        inline std::wstring const & get_redirectedConsoleFileNamePrefix() const { return redirectedConsoleFileNamePrefix_; };

        __declspec(property(get=get_consoleRedirectionFileRetentionCount)) int const ConsoleRedirectionFileRetentionCount;
        inline int const get_consoleRedirectionFileRetentionCount() const { return consoleRedirectionFileRetentionCount_; }

        __declspec(property(get=get_consoleRedirectionFileMaxSizeInKb)) LONG const ConsoleRedirectionFileMaxSizeInKb;
        inline const LONG get_consoleRedirectionFileMaxSizeInKb() const { return consoleRedirectionFileMaxSizeInKb_; }
        
        __declspec(property(get=get_ShowNoWindow)) bool const & ShowNoWindow;
        inline bool const & get_ShowNoWindow() const { return showNoWindow_; };

        __declspec(property(get=get_AllowChildProcessDetach)) bool const & AllowChildProcessDetach;
        inline bool const & get_AllowChildProcessDetach() const { return allowChildProcessDetach_; };

        __declspec(property(get=get_NotAttachedToJob)) bool const & NotAttachedToJob;
        inline bool const & get_NotAttachedToJob() const { return notAttachedToJob_; };

        __declspec(property(get = get_ProcessDebugParameters)) ProcessDebugParameters const & DebugParameters;
        inline ProcessDebugParameters const & get_ProcessDebugParameters() const { return debugParameters_; };

        __declspec(property(get = get_ResourceGovernancePolicy, put = put_ResourceGovernancePolicy)) ServiceModel::ResourceGovernancePolicyDescription const & ResourceGovernancePolicy;
        inline ServiceModel::ResourceGovernancePolicyDescription const & get_ResourceGovernancePolicy() const { return resourceGovernancePolicy_; };
        void put_ResourceGovernancePolicy(ServiceModel::ResourceGovernancePolicyDescription const & value) {resourceGovernancePolicy_ = value; };

        __declspec(property(get = get_SPResourceGovernancePolicy)) ServiceModel::ServicePackageResourceGovernanceDescription const & ServicePackageResourceGovernance;
        inline ServiceModel::ServicePackageResourceGovernanceDescription const & get_SPResourceGovernancePolicy() const { return spResourceGovernance_; };

        __declspec(property(get=get_CgroupName)) std::wstring const & CgroupName;
        inline std::wstring const & get_CgroupName() const { return cgroupOrJobObjectName_; };

        __declspec(property(get = get_JobObjectName)) std::wstring const & JobObjectName;
        inline std::wstring const & get_JobObjectName() const { return cgroupOrJobObjectName_; };

        __declspec(property(get = get_IsHostedServiceProcess)) bool IsHostedServiceProcess;
        inline bool get_IsHostedServiceProcess() const { return isHostedServiceProcess_; };

        __declspec(property(get = get_EncryptedEnvironmentVariables)) std::map<std::wstring, std::wstring> const & EncryptedEnvironmentVariables;
        inline std::map<std::wstring, std::wstring> const & get_EncryptedEnvironmentVariables() const { return encryptedEnvironmentVariables_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_PROCESS_DESCRIPTION & fabricProcessDesc) const;

        FABRIC_FIELDS_21(exePath_, arguments_, startInDirectory_, appDirectory_, tempDirectory_, workDirectory_, logDirectory_,
            redirectConsole_, redirectedConsoleFileNamePrefix_,    consoleRedirectionFileRetentionCount_, consoleRedirectionFileMaxSizeInKb_,
            showNoWindow_, allowChildProcessDetach_,  notAttachedToJob_, environmentBlock_, debugParameters_, resourceGovernancePolicy_,
            spResourceGovernance_, cgroupOrJobObjectName_, isHostedServiceProcess_, encryptedEnvironmentVariables_);

    private:
        std::wstring exePath_;
        std::wstring arguments_;
        std::wstring startInDirectory_;
        Common::EnvironmentMap envVars_;
        std::wstring appDirectory_;
        std::wstring tempDirectory_;
        std::wstring workDirectory_;
        std::wstring logDirectory_;
        bool redirectConsole_;
        std::wstring redirectedConsoleFileNamePrefix_;
        LONG consoleRedirectionFileRetentionCount_;
        LONG consoleRedirectionFileMaxSizeInKb_;
        bool showNoWindow_;
        bool allowChildProcessDetach_;
        bool notAttachedToJob_;
        std::vector<wchar_t> environmentBlock_;
        ProcessDebugParameters debugParameters_;
        ServiceModel::ResourceGovernancePolicyDescription resourceGovernancePolicy_;
        ServiceModel::ServicePackageResourceGovernanceDescription spResourceGovernance_;
        // CGroup/JobObject name of the process
        // (for Linux container groups this is the name of parent CGroup docker should use)
        std::wstring cgroupOrJobObjectName_;
        bool isHostedServiceProcess_;
        std::map<std::wstring, std::wstring> encryptedEnvironmentVariables_;
    };
}
DEFINE_AS_BLITTABLE(wchar_t);
