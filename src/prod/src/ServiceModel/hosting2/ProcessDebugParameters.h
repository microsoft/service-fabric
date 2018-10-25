// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ProcessDebugParameters : public Serialization::FabricSerializable
    {

    public:
        ProcessDebugParameters();
        ProcessDebugParameters(
            std::wstring const & ProgramExePath,
            std::wstring const & arguments,
            std::wstring const & lockFile,
            std::wstring const & workingFolder,
            std::wstring const & debugParametersFile,
            Common::EnvironmentMap const & envVars,
            std::vector<std::wstring> const & containerEntryPoints,
            std::vector<std::wstring> const & containerMountedVolumes,
            std::vector<std::wstring> const & containerEnvironmentBlock,
            std::vector<std::wstring> const & containerLabels);

        ProcessDebugParameters(ProcessDebugParameters const & other);

        ProcessDebugParameters(ProcessDebugParameters && other);

        ProcessDebugParameters const & operator = (ProcessDebugParameters const & other);
        ProcessDebugParameters const & operator = (ProcessDebugParameters && other);
        
        __declspec(property(get=get_ExePath)) std::wstring const & ExePath;
        inline std::wstring const & get_ExePath() const { return exePath_; };
        inline void set_ExePath(std::wstring const & exePath) { exePath_ = std::wstring(exePath); };

        __declspec(property(get=get_Arguments)) std::wstring const & Arguments;
        inline std::wstring const & get_Arguments() const { return arguments_; };
        inline void set_Arguments(std::wstring const & arguments) { arguments_ = std::wstring(arguments); };

        __declspec(property(get = get_LockFile)) std::wstring const & LockFile;
        inline std::wstring const & get_LockFile() const { return lockFile_; };
        
        __declspec(property(get = get_WorkingFolder)) std::wstring const & WorkingFolder;
        inline std::wstring const & get_WorkingFolder() const { return workingFolder_; };
        
        __declspec(property(get = get_DebugParametersFile)) std::wstring const & DebugParametersFile;
        inline std::wstring const & get_DebugParametersFile() const { return debugParametersFile_; };

        __declspec(property(get=get_EnvironmentBlock)) std::vector<wchar_t> const & EnvironmentBlock;
        inline std::vector<wchar_t> const & get_EnvironmentBlock() const { return environmentBlock_; };

        __declspec(property(get = get_ContainerEntryPoints)) std::vector<std::wstring> const & ContainerEntryPoints;
        inline std::vector<std::wstring> const & get_ContainerEntryPoints() const { return containerEntryPoints_; };

        __declspec(property(get = get_ContainerMountedVolumes)) std::vector<std::wstring> const & ContainerMountedVolumes;
        inline std::vector<std::wstring> const & get_ContainerMountedVolumes() const { return containerMountedVolumes_; };

        __declspec(property(get = get_ContainerEnvironmentBlock)) std::vector<std::wstring> const & ContainerEnvironmentBlock;
        inline std::vector<std::wstring> const & get_ContainerEnvironmentBlock() const { return containerEnvironmentBlock_; };

        __declspec(property(get = get_ContainerLabels)) std::vector<std::wstring> const & ContainerLabels;
        inline std::vector<std::wstring> const & get_ContainerLabels() const { return containerLabels_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        Common::ErrorCode ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_PROCESS_DEBUG_PARAMETERS & publicDeployedCodePackageQueryResult) const;

        FABRIC_FIELDS_10(exePath_, arguments_, environmentBlock_,
            lockFile_, workingFolder_, debugParametersFile_,
            containerEntryPoints_,
            containerMountedVolumes_,
            containerEnvironmentBlock_,
            containerLabels_);

    private:
        std::wstring exePath_;
        std::wstring arguments_;
        std::wstring lockFile_;
        std::wstring workingFolder_;
        std::wstring debugParametersFile_;
        std::vector<wchar_t> environmentBlock_;
        std::vector<std::wstring> containerEntryPoints_;
        std::vector<std::wstring> containerMountedVolumes_;
        std::vector<std::wstring> containerEnvironmentBlock_;
        std::vector<std::wstring> containerLabels_;
    };

    class ProcessDebugParametersJson
        : public Common::IFabricJsonSerializable
    {
    public:
        ProcessDebugParametersJson();

        ~ProcessDebugParametersJson();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ProcessDebugParametersJson::ExePathParameter, ExePath)
            SERIALIZABLE_PROPERTY(ProcessDebugParametersJson::ArgumentsParameter, Arguments)
            END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::wstring ExePath;
        std::wstring Arguments;
 
        static Common::WStringLiteral const ExePathParameter;
        static Common::WStringLiteral const ArgumentsParameter;
    };
}
