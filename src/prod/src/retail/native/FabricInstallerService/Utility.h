// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricInstallerService
{
    class Utility : 
        public Common::TextTraceComponent<Common::TraceTaskCodes::FabricInstallerService>
    {
        DENY_COPY(Utility);

    private:
        struct FileInfo;

    public:
        static Common::ErrorCode MoveFileAtReboot(std::wstring const & src, std::wstring const & dest);

        static Common::ErrorCode DeleteFileAtReboot(std::wstring const & filepath);

        static Common::ErrorCode DeleteDirectoryAtReboot(std::wstring const & directoryPath);

        static Common::ErrorCode ReplaceDirectoryAtReboot(std::wstring const & srcDir, std::wstring const & destDir);

        static Common::ErrorCode RebootMachine();

        static Common::ErrorCode WriteUpgradeValidationFile(std::wstring const & validationFilePath,
            std::vector<std::wstring> const & versionTargetFiles, 
            std::wstring const & rootDirectory);

        static Common::ErrorCode ValidateUpgrade(std::wstring const & validationFilePath, std::wstring const & rootDirectory);

    private:
        static Common::ErrorCode WriteValidationFile(std::wstring const & outFilePath, const std::vector<FileInfo> & files);

        static Common::ErrorCode ReadUpgradeValidationFile(std::wstring const & readFilePath, __out std::vector<FileInfo> & resultFiles);
    };
}
