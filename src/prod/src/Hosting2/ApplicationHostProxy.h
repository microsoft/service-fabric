// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ApplicationHostProxy :
        public Common::ComponentRoot,
        public Common::AsyncFabricComponent,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ApplicationHostProxy)

    public:
        __declspec(property(get = get_Context)) ApplicationHostContext const & Context;
        inline ApplicationHostContext const & get_Context() const { return appHostContext_; }

        __declspec(property(get = get_Id)) std::wstring const & HostId;
        inline std::wstring const & get_Id() const { return appHostContext_.HostId; }

        __declspec(property(get = get_RunAsId)) std::wstring const & RunAsId;
        inline std::wstring const & get_RunAsId() const { return runAsId_; }

        __declspec(property(get = get_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        inline ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return servicePackageInstanceId_.ApplicationId; }

        __declspec(property(get = get_ServicePackageInstanceId)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        inline ServicePackageInstanceIdentifier const & get_ServicePackageInstanceId() const { return servicePackageInstanceId_; }

        __declspec(property(get = get_HostType)) ApplicationHostType::Enum HostType;
        inline ApplicationHostType::Enum get_HostType() const { return appHostContext_.HostType; }

        __declspec(property(get = get_EntryPointType)) ServiceModel::EntryPointType::Enum EntryPointType;
        inline ServiceModel::EntryPointType::Enum get_EntryPointType() const { return entryPointType_; }

        __declspec(property(get = get_HostingHolder)) HostingSubsystemHolder const & HostingHolder;
        HostingSubsystemHolder const & get_HostingHolder() { return this->hostingHolder_; }

        __declspec(property(get = get_Hosting)) HostingSubsystem & Hosting;
        HostingSubsystem & get_Hosting() { return this->hostingHolder_.RootedObject; }

        __declspec(property(get = get_AppHostIsolationContext)) ApplicationHostIsolationContext const & AppHostIsolationContext;
        ApplicationHostIsolationContext const & get_AppHostIsolationContext() { return this->appHostIsolationContext_; }

        __declspec(property(get = get_IsUpdateContextPending, put = set_IsUpdateContextPending)) bool IsUpdateContextPending;
        bool get_IsUpdateContextPending() const { return this->isUpdateContextPending_; }
        void set_IsUpdateContextPending(bool isPending) { this->isUpdateContextPending_ = isPending; }

        static Common::ErrorCode Create(
            HostingSubsystemHolder const & hostingHolder,
            ApplicationHostIsolationContext const & isolationContext,
            CodePackageInstanceSPtr const & codePackageInstance,
            __out ApplicationHostProxySPtr & applicationHostProxy);

        virtual Common::AsyncOperationSPtr BeginActivateCodePackage(
            CodePackageInstanceSPtr const & codePackage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndActivateCodePackage(
            Common::AsyncOperationSPtr const & operation,
            __out CodePackageActivationId & activationId,
            __out CodePackageRuntimeInformationSPtr & codePackageRuntimeInformation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeactivateCodePackage(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDeactivateCodePackage(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateCodePackageContext(
            CodePackageContext const & codePackageContext,
            CodePackageActivationId const & codePackageActivationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndUpdateCodePackageContext(
            Common::AsyncOperationSPtr const & operation);

        virtual Common::AsyncOperationSPtr BeginGetContainerInfo(
            wstring const & containerInfoType,
            wstring const & containerInfoArgs,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetContainerInfo(
            Common::AsyncOperationSPtr const & operation,
            __out wstring & containerInfo) = 0;

        virtual Common::AsyncOperationSPtr BeginApplicationHostCodePackageOperation(
            ApplicationHostCodePackageOperationRequest const & requestBody,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndApplicationHostCodePackageOperation(
            Common::AsyncOperationSPtr const & operation);

        virtual void SendDependentCodePackageEvent(
            CodePackageEventDescription const & eventDescription);

        virtual bool HasHostedCodePackages() = 0;

        virtual void OnApplicationHostRegistered() = 0;
        virtual void OnApplicationHostTerminated(DWORD exitCode) = 0;

        virtual void OnContainerHealthCheckStatusChanged(ContainerHealthStatusInfo const & healthStatusInfo) = 0;

        virtual void OnCodePackageTerminated(
            CodePackageInstanceIdentifier const & codePackageInstanceId, 
            CodePackageActivationId const & activationId) = 0;

        virtual void TerminateExternally();

        virtual bool GetLinuxContainerIsolation();

    protected:
        ApplicationHostProxy(
            HostingSubsystemHolder const & hostingHolder,
            ApplicationHostContext const & context,
            ApplicationHostIsolationContext const & isolationContext,
            std::wstring const & runAsId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            ServiceModel::EntryPointType::Enum entryPointType,
            bool removeServiceFabricRuntimeAccess);

        virtual ~ApplicationHostProxy();

        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) = 0;

        virtual void OnAbort() = 0;

    protected:
        ErrorCode AddHostContextAndRuntimeConnection(Common::EnvironmentMap & envMap);

        Common::ErrorCode GetCurrentProcessEnvironmentVariables(
            __in Common::EnvironmentMap &  envMap,
            std::wstring const & tempDirectory,
            std::wstring const & fabricBinFolder);

        void AddSFRuntimeEnvironmentVariable(Common::EnvironmentMap & envMap, std::wstring const &name, std::wstring const &value);
        void AddEnvironmentVariable(Common::EnvironmentMap & envMap, std::wstring const &name, std::wstring const &value);

        bool removeServiceFabricRuntimeAccess_;
    
    private:
        ErrorCode ApplicationHostProxy::AddTlsConnectionInformation(__inout Common::EnvironmentMap & envMap);

        std::wstring ApplicationHostProxy::GetWellKnownValue(std::wstring const &value);

        class UpdateCodePackageContextAsyncOperation;

    private:
        HostingSubsystemHolder hostingHolder_;
        ApplicationHostContext const appHostContext_;
        ApplicationHostIsolationContext const appHostIsolationContext_;
        std::wstring const runAsId_;
        ServicePackageInstanceIdentifier const servicePackageInstanceId_;
        ServiceModel::EntryPointType::Enum entryPointType_;
        bool isUpdateContextPending_;
        RwLock lock_;
    };
}
