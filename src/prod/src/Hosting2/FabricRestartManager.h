// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricRestartManager
		: public Common::RootedObject
        , public Common::AsyncFabricComponent
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FabricRestartManager)

    public:
		FabricRestartManager(
			Common::ComponentRoot const & root,
			FabricActivationManager const & fabricHost);
        virtual ~FabricRestartManager();

        Common::AsyncOperationSPtr BeginNodeDisable(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndNodeDisable(Common::AsyncOperationSPtr const &);

        std::wstring callbackAddress_;
    
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
		void StartPollingAsync();
		void StopPolling();
		void Run();
		bool IfRebootIsPending();
        bool IfNodeNeedToBeEnabled();

        void SendNodeDisableRequest();
        void SendNodeEnableRequest();
		void RegisterIpcRequestHandler();
		void UnregisterIpcRequestHandler();
		void ProcessIpcMessage(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessNodeEnabledNotification(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);
        void ProcessNodeDisabledNotification(__in Transport::Message & message, __in Transport::IpcReceiverContextUPtr & context);

        void FinishSendDisableNodeRequestMessage(Common::AsyncOperationSPtr operation, bool expectedCompletedAsynchronously);
        void FinishSendEnableNodeRequestMessage(Common::AsyncOperationSPtr operation, bool expectedCompletedAsynchronously);

		FabricActivationManager const & fabricHost_;
		Common::ManualResetEvent runStarted_;
		Common::ManualResetEvent runCompleted_;
		Common::ManualResetEvent runShouldExit_;
        Common::ManualResetEvent nodeDisabled_;
        Common::ManualResetEvent nodeEnabled_;
        Common::ExclusiveLock lock_;

        // rebootPending flag is used when system detects a pending windows update and needs to reboot the system.
        bool rebootPending_;

        // externalRebootPending flag is used when fabrichostsvc is about to be shut down as a result node is getting disabled. 
        bool externalRebootPending_;

		class OpenAsyncOperation;
		class CloseAsyncOperation;
        class DisableNodeAsyncOperation;
    };
}
