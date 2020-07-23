// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    typedef std::function<void(DWORD exitCode)> EntryPointCallback;
        
    class ApplicationHostManager :
        public Common::RootedObject,
        public Common::AsyncFabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationHostManager)

    public:
        ApplicationHostManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~ApplicationHostManager();

        Common::ErrorCode IsRegistered(std::wstring const & hostId, __out bool & isRegistered);

       __declspec (property(get=Test_get_ApplicationHostRegistrationTable)) ApplicationHostRegistrationTableUPtr const & Test_ApplicationHostRegistrationTable;
        ApplicationHostRegistrationTableUPtr const & Test_get_ApplicationHostRegistrationTable() const { return registrationTable_; }

        __declspec (property(get = Test_get_ApplicationHostActivationTable)) ApplicationHostActivationTableUPtr const & Test_ApplicationHostActivationTable;
        ApplicationHostActivationTableUPtr const & Test_get_ApplicationHostActivationTable() const { return activationTable_; }

        __declspec (property(get=get_FabricActivator)) IFabricActivatorClientSPtr const & FabricActivator;
        IFabricActivatorClientSPtr const & get_FabricActivator() const { return hosting_.FabricActivatorClientObj; }

        Common::AsyncOperationSPtr BeginActivateCodePackage(
            CodePackageInstanceSPtr const & codePackageInstance,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndActivateCodePackage(
            Common::AsyncOperationSPtr const & operation,
            __out CodePackageActivationIdUPtr & activationId,
            __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation);

        Common::AsyncOperationSPtr BeginDeactivateCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeactivateCodePackage(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginUpdateCodePackageContext(
            CodePackageContext const & codePackageContext,
            CodePackageActivationId const & activationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndUpdateCodePackageContext(
            Common::AsyncOperationSPtr const & operation);
            
        Common::AsyncOperationSPtr BeginGetContainerInfo(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId,
            wstring const & containerInfoType,
            wstring const & containerInfoArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const & operation,
            __out wstring & containerInfo);

        void SendDependentCodePackageEvent(
            CodePackageEventDescription const & eventDescription,
            CodePackageActivationId const & codePackageActivationId);

        void TerminateCodePackage(CodePackageActivationId const & activationId);

        Common::ErrorCode TerminateCodePackageExternally(CodePackageActivationId const & activationId);

        void OnApplicationHostTerminated(
            Common::ActivityDescription const & activityDescription,
            std::wstring const & hostId,
            DWORD exitCode);

        Common::AsyncOperationSPtr BeginTerminateServiceHost(
            std::wstring const & hostId,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndTerminateServiceHost(
            Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginApplicationHostCodePackageOperation(
            ApplicationHostCodePackageOperationRequest const & request,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndApplicationHostCodePackageOperation(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode FindApplicationHost(std::wstring const & codePackageInstanceId, __out ApplicationHostProxySPtr & hostProxy);

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
        void RegisterIpcRequestHandler();
        void UnregisterIpcRequestHandler();

        void ProcessIpcMessage(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessApplicationHostCodePackageOperationRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr && context);

        void SendApplicationHostCodePackageOperationReply(
            ErrorCode error,
            __in Transport::IpcReceiverContextUPtr && context);

        void ProcessGetFabricProcessSidRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessStartRegisterApplicationHostRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void SendStartRegisterApplicationHostReply(
            StartRegisterApplicationHostRequest const & requestBody,
            Common::ErrorCode const error,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessFinishRegisterApplicationHostRequest(
            __in Transport::Message & message, 
            __in Transport::IpcReceiverContextUPtr & context);

        void SendFinishRegisterApplicationHostReply(
            FinishRegisterApplicationHostRequest const & requestBody,
            Common::ErrorCode const error,
            __in Transport::IpcReceiverContextUPtr & context);

        void OnApplicationHostRegistrationTimedout(
            std::wstring const & hostId, 
            Common::TimeSpan timeout, 
            Common::TimerSPtr const & timer);
       
        void ProcessUnregisterApplicationHostRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendUnregisterApplicationHostReply(
            UnregisterApplicationHostRequest const & requestBody,
            Common::ErrorCode const error,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessCodePackageTerminationNotification(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendCodePackageTerminationNotificationReply(
            CodePackageTerminationNotificationRequest const & requestBody,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessRegisterResourceMonitorService(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendRegisterResourceMonitorServiceReply(
            __in Transport::IpcReceiverContextUPtr & context);

        // this method is called when an application host is successfully registered
        void OnApplicationHostRegistered(ApplicationHostRegistrationSPtr const & registration);

        // this method is called when a registered application host is unregistered either by 
        // sending unregister request or as part of handling its termination
        void OnApplicationHostUnregistered(Common::ActivityDescription const & activityDescription, std::wstring const & hostId);
       
        void OnActivatedApplicationHostTerminated(Common::EventArgs const & eventArgs);

        // this method is called when a registered application host process terminates
        void OnRegisteredApplicationHostTerminated(
            Common::ActivityDescription const & activityDescription, 
            std::wstring const & hostId, 
            Common::ErrorCode const & waitResult);

        // this method is called when user is reporting health via IPC channel
        void ReportHealthReportMessageHandler(Transport::Message & message, Transport::IpcReceiverContextUPtr && context);

        void OnContainerHealthCheckStatusChanged(Common::EventArgs const & eventArgs);

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class ActivateCodePackageAsyncOperation;
        class DeactivateCodePackageAsyncOperation;
        class UpdateCodePackageContextAsyncOperation;
        class TerminateServiceHostAsyncOperation;
        class GetContainerInfoAsyncOperation;
        class ApplicationHostCodePackageAsyncOperation;

        friend class ApplicationHostProxy;

    private:
        HostingSubsystem & hosting_;
        ApplicationHostRegistrationTableUPtr registrationTable_;
        ApplicationHostActivationTableUPtr activationTable_;
        Common::RwLock updateContextlock_;

        DWORD processId_;
        std::wstring processSid_;
        std::unique_ptr<Common::ResourceHolder<Common::HHandler>> terminationNotificationHandler_;
        std::unique_ptr<Common::ResourceHolder<Common::HHandler>> healthStatusChangeNotificationHandler_;
    };
}