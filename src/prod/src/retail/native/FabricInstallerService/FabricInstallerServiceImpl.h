// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricInstallerService
{
    class FabricInstallerServiceImpl : public Common::ServiceBase
    {
        DENY_COPY(FabricInstallerServiceImpl);

    public:
        explicit FabricInstallerServiceImpl(bool runAsService, bool autoclean);
        void OnStart (Common::StringCollection const & args);
        void OnStop( bool shutdown );
        void OnPreshutdown();
        void OnPause();
        void OnContinue();

    private:
        void OnOpenComplete(Common::AsyncOperationSPtr operation);
        void CreateAndOpen(Common::AutoResetEvent & openEvent);
        void StartFabricUpgradeManager();
        void StopFabricUpgradeManager();
        void Close(Common::AutoResetEvent & closeEvent);
        void OnCloseCompleted(Common::AsyncOperationSPtr operation);
        void ServiceModeStartWait();
        void ConsoleModeStartWait();
        void ServiceModeStopWait();
        void ConsoleModeStopWait();

    private:
        bool runAsService_;
        bool stopping_;
        bool autoclean_;
        std::shared_ptr<FabricInstallerService::FabricUpgradeManager> fabricUpgradeManager_;
    };
}
