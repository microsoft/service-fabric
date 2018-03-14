// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricHost
{
    class FabricActivatorService : public Common::ServiceBase
    {
        DENY_COPY(FabricActivatorService);

    public:
        explicit FabricActivatorService(bool runAsService, bool activateHidden = false, bool skipFabricSetup = false);
        void OnStart (Common::StringCollection const & args);
        void OnStop( bool shutdown );

#if !defined(PLATFORM_UNIX)
        void OnPreshutdown();
#endif
        void OnPause();
        void OnContinue();

    private:
        void OnOpenComplete(Common::AsyncOperationSPtr operation);
        void CreateAndOpen(Common::AutoResetEvent & openEvent);
        void StartActivationManager();
        void StopActivationManager();

#if !defined(PLATFORM_UNIX)
        void DisableNode();
#endif
        void Close(Common::AutoResetEvent & closeEvent);
        void OnCloseCompleted(Common::AsyncOperationSPtr operation);
        void ServiceModeStartWait();
        void ConsoleModeStartWait();
        void ServiceModeStopWait();
        void ConsoleModeStopWait();

    private:
        bool runAsService_;
        bool activateHidden_;
        bool skipFabricSetup_;
        bool stopping_;
        std::shared_ptr<Hosting2::FabricActivationManager> activationManager_;
    };
}
