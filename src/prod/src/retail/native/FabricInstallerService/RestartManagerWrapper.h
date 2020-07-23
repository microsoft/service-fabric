// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RestartManager.h"

#define INVALID_RM_SESSION_HANDLE 0xFFFFFFFF

namespace FabricInstallerService
{
    class RestartManagerWrapper : 
        public Common::TextTraceComponent<Common::TraceTaskCodes::FabricInstallerService>
    {
        DENY_COPY(RestartManagerWrapper);

    public:
        RestartManagerWrapper();
       
        ~RestartManagerWrapper();

        __declspec(property(get = get_IsSessionValid)) bool const IsSessionValid;
        bool const get_IsSessionValid() const { return (this->sessionHandle_ != INVALID_RM_SESSION_HANDLE); }

        _declspec(property(get = get_SessionHandle)) DWORD const SessionHandle;
        DWORD const get_SessionHandle() const { return this->sessionHandle_; }

        _declspec(property(get = get_SessionKey)) const WCHAR* SessionKey;
        const WCHAR* get_SessionKey() const { return this->szSessionKey; }

        void EnumerateAndRegisterFiles(std::vector<std::wstring> const & dirs);
        Common::ErrorCode ShutdownProcesses();
        Common::ErrorCode RestartProcesses();
        Common::ErrorCode ForceCloseNonCriticalProcesses();
    
    private:
        Common::ErrorCode CreateNewSessionAndRegisterFiles();
        static void ProcessShutdownProgressCallback(UINT progress);
        static void ProcessRestartProgressCallback(UINT progress);

    private:
        DWORD sessionHandle_;
        WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1];
        std::vector<std::wstring> registeredFiles_;
        std::vector<RM_PROCESS_INFO> resilientProcesses_;
        std::wstring traceId_;
        bool isRestartable_;
    };
}
