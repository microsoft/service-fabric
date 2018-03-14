// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricUpgradeManager :
        public Common::RootedObject,        
        public Common::StateMachine,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FabricUpgradeManager)

        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_07(Inactive, Starting, Idle, Validating, Upgrading, Closing, Closed);
        STATEMACHINE_TRANSITION(Starting, Inactive);
        STATEMACHINE_TRANSITION(Idle, Starting|Validating|Upgrading);
        STATEMACHINE_TRANSITION(Validating, Idle|Validating);
        STATEMACHINE_TRANSITION(Upgrading, Idle);
        STATEMACHINE_TRANSITION(Closing, Idle);
        STATEMACHINE_TRANSITION(Closed, Closing);
        STATEMACHINE_ABORTED_TRANSITION(Idle|Validating|Upgrading);
        STATEMACHINE_TERMINAL_STATES(Aborted|Closed);

    public:
        FabricUpgradeManager(
            Common::ComponentRoot const & root, 
            __in HostingSubsystem & hosting);
        ~FabricUpgradeManager();

        Common::ErrorCode Open();
        Common::ErrorCode Close();

        Common::AsyncOperationSPtr BeginDownloadFabric(
            Common::FabricVersion const & fabricVersion,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDownloadFabric(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginValidateFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndValidateFabricUpgrade(  
            __out bool & shouldRestartReplica,
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginFabricUpgrade(
            ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
            bool const shouldRestart,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);        
        Common::ErrorCode EndFabricUpgrade(Common::AsyncOperationSPtr const & operation);

        bool Test_SetFabricUpgradeImpl(IFabricUpgradeImplSPtr const & testFabricUpgradeImp);

    protected:        
        virtual void OnAbort();

    private:
        class FabricUpgradeAsyncOperation;
        friend class FabricUpgradeAsyncOperation;

        class ValidateFabricUpgradeAsyncOperation;
        friend class ValidateFabricUpgradeAsyncOperation;

        class DownloadForFabricUpgradeAsyncOperation;
        friend class DownloadForFabricUpgradeAsyncOperation;

        HostingSubsystem & hosting_;        
        IFabricUpgradeImplSPtr fabricUpgradeImpl_;
        PendingOperationMapUPtr pendingOperations_;        
        Common::ExclusiveLock lock_;

        IFabricUpgradeImplSPtr testFabricUpgradeImpl_;
    };
}
