// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class FabricActivationManager 
        : public Common::ComponentRoot
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FabricActivationManager)

    public:
        FabricActivationManager(
            bool activateHidden = false,
            bool skipFabricSetup = false);
        virtual ~FabricActivationManager();

        __declspec(property(get=get_TransportAddress)) std::wstring const & TransportAddress;
        inline std::wstring const & get_TransportAddress() const { return ipcServer_->TransportListenAddress; };

        __declspec(property(get=get_IpcServer)) Transport::IpcServer & IpcServerObj;
        inline Transport::IpcServer & get_IpcServer() const { return *ipcServer_; }

        _declspec(property(get=get_ProcessActivator)) ProcessActivatorUPtr const & ProcessActivatorObj;
        inline ProcessActivatorUPtr const & get_ProcessActivator() const { return processActivator_; }

        _declspec(property(get = get_ProcessActivationManager)) ProcessActivationManagerSPtr const & ProcessActivationManagerObj;
        inline ProcessActivationManagerSPtr const & get_ProcessActivationManager() const { return processActivationManager_; }

        _declspec(property(get = get_FabricRestartManager)) FabricRestartManagerUPtr const & FabricRestartManagerObj;
        inline FabricRestartManagerUPtr const & get_FabricRestartManager() const { return fabricRestartManager_; }

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &);

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & , 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &);

        virtual void OnAbort();

    private:
        Common::ErrorCode SetupPrivateObjectNamespace();
        Common::ErrorCode CleanupPrivateObjectNamespace();
        Common::ErrorCode Initialize();
        
    private:
        Transport::IpcServerUPtr ipcServer_;

        TraceSessionManagerUPtr traceSessionManager_;
        ProcessActivatorUPtr processActivator_;
        HostedServiceActivationManagerSPtr serviceActivator_;
        ProcessActivationManagerSPtr processActivationManager_;
        FabricRestartManagerUPtr fabricRestartManager_;        
        CertificateAclingManagerSPtr certificateAclingManager_;

        bool hostedServiceActivateHidden_;
        bool skipFabricSetup_;

        Common::TimerSPtr setupRetryTimer_;
        Common::RwLock timerLock_;

        static Common::GlobalWString SecurityGroupFileName;

        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class FabricSetupAsyncOperation;
    };
}
