// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CrashDumpConfigurationManager : 
        public Common::RootedObject,
        public Common::FabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(CrashDumpConfigurationManager)

    public:
        explicit CrashDumpConfigurationManager(
            Common::ComponentRoot const & root);
        virtual ~CrashDumpConfigurationManager();

        void ProcessConfigureCrashDumpRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        static bool IsCrashDumpCollectionEnabled(std::wstring const & exeName);

    protected:
        // ****************************************************
        // FabricComponent specific methods
        // ****************************************************
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
        Common::ErrorCode CreateCrashDumpsBaseFolder();

        Common::ErrorCode CleanupServiceCrashDumpCollectionLocked(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::wstring const & servicePackageKey,
            bool modifyNodeToServicePackagesMap);

        Common::ErrorCode CleanupNodeCrashDumpCollectionLocked(
            std::wstring const & nodeId,
            bool modifyNodeToServicePackagesMap);

        Common::ErrorCode CleanupCrashDumpCollection();

        Common::ErrorCode EnableCrashDumpCollection(
            std::wstring const & exeName);

        Common::ErrorCode DisableCrashDumpCollection(
            std::wstring const & exeName);

        Common::ErrorCode SetupServiceCrashDumpCollection(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<std::wstring> const & exeNames);

        Common::ErrorCode CleanupServiceCrashDumpCollection(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId);

        Common::ErrorCode CleanupNodeCrashDumpCollection(
            std::wstring const & nodeId);

        void SendConfigureCrashDumpReply(
            Common::ErrorCode const & error,
            __in Transport::IpcReceiverContextUPtr & context);

    private:
        bool crashDumpCollectionAccessDenied_;
        std::wstring crashDumpsBaseFolder_;
        std::map<std::wstring, std::set<std::wstring>, Common::IsLessCaseInsensitiveComparer<std::wstring>> exeToServicePackages_;
        std::map<std::wstring, std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>>> servicePackageToExes_;
        std::map<std::wstring, std::set<std::wstring>> nodeToServicePackages_;
        Common::RwLock mapLock_;
        bool mapIsClosed_;

        static const std::wstring CrashDumpsKeyName;
        static const std::wstring FabricCrashDumpsKeyName;
        static const std::wstring ApplicationCrashDumpsFolderName;
        static const std::wstring DumpFolderValueName;
        static const std::wstring DumpTypeValueName;
        static const DWORD DumpTypeFullDump = 2;
    };
}
