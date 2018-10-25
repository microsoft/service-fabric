// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricRuntimeManager :
        public Common::RootedObject,
        public Common::FabricComponent,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(FabricRuntimeManager)

    public:
        FabricRuntimeManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~FabricRuntimeManager();

        __declspec (property(get = Test_get_RuntimeRegistrationTable)) RuntimeRegistrationTableUPtr const & Test_RuntimeRegistrationTable;
        RuntimeRegistrationTableUPtr const & Test_get_RuntimeRegistrationTable() const { return runtimeRegistrationTable_; }

        __declspec (property(get = get_ServiceTypeStateManager)) ServiceTypeStateManagerUPtr const & ServiceTypeStateManagerObj;
        ServiceTypeStateManagerUPtr const & get_ServiceTypeStateManager() const { return serviceTypeStateManager_; }

        Common::ErrorCode FindServiceTypeRegistration(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            Reliability::ServiceDescription const & serviceDescription,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out uint64 & sequenceNumber,
            __out ServiceTypeRegistrationSPtr & serviceTypeRegistration);

        Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            __out std::wstring & hostId);

        Common::ErrorCode GetHostId(
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            std::wstring const & applicationName,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            __out std::wstring & hostId);

        Common::ErrorCode GetRuntimeIds(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId,
            __out CaseInsensitiveStringSet & runtimeIds);

        Common::ErrorCode GetRuntimeIds(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            __out CaseInsensitiveStringSet & runtimeIds);

        void OnApplicationHostUnregistered(Common::ActivityDescription const & activityDescription, std::wstring const & hostId);

        void OnRuntimeUnregistered(RuntimeRegistrationSPtr const & runtimeRegistration);

        void OnServicePackageClosed(ServicePackageInstanceIdentifier const & servicePackageInstanceId, int64 const instanceId);

        void OnCodePackageClosed(CodePackageInstanceIdentifier const & codePackageInstanceId, int64 const instanceId);

        Common::ErrorCode UpdateRegistrationsFromCodePackage(CodePackageContext const & existingContext, CodePackageContext const & updatedContext);

    protected:
        virtual Common::ErrorCode OnOpen();
        virtual Common::ErrorCode OnClose();
        virtual void OnAbort();

    private:
    Common::ErrorCode FindServiceTypeRegistration(
      ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
      Reliability::ServiceDescription const & serviceDescription,
      ServiceModel::ServicePackageActivationContext const & activationContext,
      bool shouldActivate,
      __out uint64 & sequenceNumber,
      __out ServiceTypeRegistrationSPtr & serviceTypeRegistration);

        Common::ErrorCode CreateServiceTypeRegistration(
            std::wstring const & runtimeId,
            ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackagePublicActivationId,
            __out ServiceTypeRegistrationSPtr & serviceTypeRegistration);

        Common::ErrorCode DownloadAndActivate(
            uint64 const sequenceNumber,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackagePublicActivationId,
            std::wstring const & applicationName);

        void OnDownloadAndActivateCompleted(
            Common::AsyncOperationSPtr const & operation,
            uint64 const sequenceNumber,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackagePublicActivationId);

        Common::ErrorCode FinishDownloadAndActivate(Common::AsyncOperationSPtr const & operation,
            uint64 const sequenceNumber,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackagePublicActivationId);

        Common::ErrorCode EnsureReturnedServiceTypeVersion(
            ServiceModel::ServicePackageVersionInstance const & requestedVersion,
            ServiceTypeRegistrationSPtr const & serviceTypeRegistration);

        void RegisterIpcRequestHandler();
        void UnregisterIpcRequestHandler();

        void ProcessIpcMessage(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void ProcessRegisterFabricRuntimeRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendRegisterFabricRuntimeReply(
            RegisterFabricRuntimeRequest const & requestBody,
            Common::ErrorCode const error,
            __in Transport::IpcReceiverContextUPtr & context);

        Common::ErrorCode AddRuntimeRegistrationEntry(
            FabricRuntimeContext const & runtimeContext);

        Common::ErrorCode EnsureValidApplicationHostRegistration(
            std::wstring const & hostId);

        void CleanRuntimeRegistrationEntry(
            std::wstring const & runtimeId);

        Common::ErrorCode ValidateRuntimeRegistrationEntry(
            std::wstring const & runtimeId,
            __out RuntimeRegistrationSPtr & registration);

        void ProcessUnregisterFabricRuntimeRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendUnregisterFabricRuntimeReply(
            UnregisterFabricRuntimeRequest const & requestBody,
            Common::ErrorCode const error,
            __in Transport::IpcReceiverContextUPtr & context);

        Common::ErrorCode RemoveRuntimeRegistrationEntry(
            std::wstring const & runtimeId,
            __out RuntimeRegistrationSPtr & registration);

        void ProcessRegisterServiceTypeRequest(
            __in Transport::Message & message,
            __in Transport::IpcReceiverContextUPtr & context);

        void SendRegisterServiceTypeReply(
            RegisterServiceTypeRequest const & requestBody,
            Common::ErrorCode const error,
            __in Transport::IpcReceiverContextUPtr & context);

        Common::ErrorCode AddServiceTypeRegistrationEntry(
            ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
            FabricRuntimeContext const & runtimeContext);

        Common::ErrorCode EnsureValidRuntimeRegistration(
            FabricRuntimeContext const & runtimeContext);

        void CleanServiceTypeRegistrationEntry(
            ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
            FabricRuntimeContext const & runtimeContext);

        Common::ErrorCode ValidateServiceTypeRegistrationEntry(
            ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
            FabricRuntimeContext const & runtimeContext);

    private:
        class FindServiceTypeRegistrationAsyncOperation;
        friend class FindServiceTypeRegistrationAsyncOperation;

        HostingSubsystem & hosting_;
        RuntimeRegistrationTableUPtr runtimeRegistrationTable_;
        ServiceTypeStateManagerUPtr serviceTypeStateManager_;
    };
}
