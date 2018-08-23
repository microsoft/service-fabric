// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceTypeStateManager :
        public Common::RootedObject,
        Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(ServiceTypeStateManager)

    public:
        ServiceTypeStateManager(
            Common::ComponentRoot const & root,
            __in HostingSubsystem & hosting);
        virtual ~ServiceTypeStateManager();

    public:
         __declspec(property(get=get_BlocklistingEnabled, put=set_BlocklistingEnabled)) bool ServiceTypeBlocklistingEnabled;
        bool get_BlocklistingEnabled() const;
        void set_BlocklistingEnabled(bool value);

    public:
        Common::ErrorCode GetState(ServiceTypeInstanceIdentifier const & serviceTypeId, __out ServiceTypeState & state);

        std::vector<ServiceModel::DeployedServiceTypeQueryResult> GetDeployedServiceTypeQueryResult(
            ServiceModel::ApplicationIdentifier const & applicationId, 
            std::wstring const & filterServiceManifestName = L"",
            std::wstring const & filterServiceTypeName = L"");
        
        Common::ErrorCode StartRegister(ServiceTypeInstanceIdentifier const & serviceTypeId, std::wstring const & applicationName, std::wstring const & runtimeId, std::wstring const & codePackageName, std::wstring const & hostId);
        Common::ErrorCode FinishRegister(ServiceTypeInstanceIdentifier const & serviceTypeId, bool isValid, FabricRuntimeContext const & runtimeContext);
        
        void OnApplicationHostClosed(Common::ActivityDescription const & activityDescription, std::wstring const & hostId);
        void OnRuntimeClosed(std::wstring const & runtimeId, std::wstring const & hostId);
        
        void OnServiceTypeRegistrationNotFoundAfterTimeout(ServiceTypeInstanceIdentifier const & serviceTypeId, uint64 const sequenceNumber);
        
        Common::ErrorCode OnServicePackageActivated(std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeIds, std::wstring const & applicationName, std::wstring const & componentId);
        void OnServicePackageDeactivated(std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeIds, std::wstring const & componentId);
 
        void Disable(ServiceTypeInstanceIdentifier const & serviceTypeId);
        void Disable(ServiceTypeInstanceIdentifier const & serviceTypeId, std::wstring const & applicationName, std::wstring const & failureId);

        void RegisterFailure(std::wstring const & failureId);

        void UnregisterFailure(std::wstring const & failureId);

        void Close();
        
        bool Test_IsDisabled(ServiceTypeInstanceIdentifier const & serviceTypeId);

        Common::ErrorCode Test_GetHostAndRuntime(
            ServiceTypeInstanceIdentifier const & serviceTypeIntanceId,
            __out std::wstring & hostId, 
            __out std::wstring & runtimeId);

        Common::ErrorCode Test_GetStateIfPresent(
            ServiceTypeInstanceIdentifier const & serviceTypeIntanceId,
            __out ServiceTypeState & state);

        void Test_GetServiceTypeEntryInStoredOrder(std::vector<ServiceTypeInstanceIdentifier> & svcTypeEntries);

    private:
        struct Entry;
        typedef std::shared_ptr<Entry> EntrySPtr;
        typedef std::map<ServiceTypeInstanceIdentifier, EntrySPtr> EntryMap;

    private:
        Common::ErrorCode CreateNewServiceTypeEntry_CallerHoldsLock(ServiceTypeInstanceIdentifier const & serviceTypeId, std::wstring const & applicationName, __out EntrySPtr & entry);
        void RegisterSource_CallerHoldsLock(EntrySPtr const & entry, std::wstring const & componentId);
        void UnregisterSource_CallerHoldsLock(EntrySPtr const & entry, std::wstring const & componentId);
        void ReportServiceTypeHealth(
            ServiceTypeInstanceIdentifier const & serviceTypeId, 
            Common::SystemHealthReportCode::Enum reportCode,
            std::wstring const & healthDescription,
            FABRIC_SEQUENCE_NUMBER sequenceNumber);

        void SetFailureId_CallerHoldsLock(EntrySPtr const & entry, std::wstring const & failureId);
        void ResetFailureId_CallerHoldsLock(EntrySPtr const & entry);

        void OnDisableTimerCallback(ServiceTypeInstanceIdentifier const & serviceTypeId, uint64 sequenceNumber);
        void OnRecoveryTimerCallback(ServiceTypeInstanceIdentifier const & serviceTypeId, uint64 sequenceNumber);
        uint64 GetNextSequenceNumber_CallerHoldsLock();
        void CreateAndStartDisableTimer(EntrySPtr const & entry, Common::TimeSpan const disableInterval);
        void CreateAndStartRecoveryTimerIfNeeded(EntrySPtr const & entry);
        void CloseEntry(EntrySPtr const & entry);

        void ReportHealth(
            std::vector<ServiceTypeInstanceIdentifier> && serviceTypeInstanceIds,
            ::FABRIC_SEQUENCE_NUMBER healthSequence);

        void DisableAdhocTypes(std::vector<ServiceTypeInstanceIdentifier> && adhocDisableList);

        void ProcessServiceTypeUnregistered_CallerHoldsLock(
            bool isRuntimeClosed,
            std::wstring const & runtimeOrHostId,
            uint64 sequenceNumber,
            _Out_ std::vector<ServiceTypeInstanceIdentifier> & adhocDisableList,
            _Out_ std::vector<ServiceTypeInstanceIdentifier> & serviceTypeInstanceIds,
            _Out_ std::vector<ServiceModel::ServiceTypeIdentifier> & serviceTypeIds,
            _Out_ ServicePackageInstanceIdentifier & servicePackageInstanceId,
            _Out_ ServiceModel::ServicePackageActivationContext & activationContext,
            _Out_ std::wstring & servicePackageActivationId);

    private:
        // all process method are performed under the lock
        Common::ErrorCode ProcessStartRegister(EntrySPtr const & entry, std::wstring const & runtimeId, std::wstring const & codePackageName, std::wstring const & hostId);
        void ProcessStartRegister_NotRegistered_Enabled(EntrySPtr const & entry, std::wstring const & runtimeId, std::wstring const & codePackageName, std::wstring const & hostId);
        void ProcessStartRegister_NotRegistered_Disabled(EntrySPtr const & entry, std::wstring const & runtimeId, std::wstring const & codePackageName, std::wstring const & hostId);
        void ProcessStartRegister_NotRegistered_DisableScheduled(EntrySPtr const & entry, std::wstring const & runtimeId, std::wstring const & codePackageName, std::wstring const & hostId);
        
        Common::ErrorCode ProcessFinishRegister(EntrySPtr const & entry, bool isValid, FabricRuntimeContext const & runtimeContext);
        void ProcessFinishRegisterSuccess_InProgress_Disabled(EntrySPtr const & entry, FabricRuntimeContext const & runtimeContext);
        void ProcessFinishRegisterFailure_InProgress_Disabled(EntrySPtr const & entry);
        void ProcessFinishRegisterSuccess_InProgress_Enabled(EntrySPtr const & entry, FabricRuntimeContext const & runtimeContext);
        void ProcessFinishRegisterFailure_InProgress_Enabled(EntrySPtr const & entry);

        void ProcessApplicationHostOrRuntimeClosed(EntrySPtr const & entry, uint64 sequenceNumber, __out std::vector<ServiceTypeInstanceIdentifier> & adhocDisableList);

        void ProcessUnregisterFailure(EntrySPtr const & entry);
        void ProcessUnregisterFailure_NotRegistered_DisableScheduled(EntrySPtr const & entry);
        void ProcessUnregisterFailure_NotRegistered_Disabled(EntrySPtr const & entry);
        void ProcessUnregisterFailure_InProgress_Disabled(EntrySPtr const & entry);

        void ProcessDisable(EntrySPtr const & entry, std::wstring const & failureId, Common::TimeSpan const graceInterval);
       
        
    private:
        void RaiseServiceTypeRegisteredEvent_CallerHoldsLock(EntrySPtr const & entry, FabricRuntimeContext const & runtimeContext);
        void RaiseServiceTypeEnabledEvent_CallerHoldsLock(EntrySPtr const & entry);
        void RaiseServiceTypeDisabledEvent_CallerHoldsLock(EntrySPtr const & entry);
        void RaiseApplicationHostClosedEvent_CallerHoldsLock(
            Common::ActivityDescription const & activityDescription,
            std::wstring const & hostId, 
            std::vector<ServiceModel::ServiceTypeIdentifier> const & serviceTypes,
            uint64 sequenceNumber,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackageActivationId);

        void RaiseRuntimeClosedEvent_CallerHoldsLock(
            std::wstring const & runtimeId, 
            std::wstring const & hostId, 
            std::vector<ServiceModel::ServiceTypeIdentifier> const & serviceTypes, 
            uint64 sequenceNumber,
            ServiceModel::ServicePackageActivationContext const & activationContext,
            std::wstring const & servicePackageActivationId);

        static std::wstring GetHealthPropertyId(ServiceTypeInstanceIdentifier const & serviceTypeId);

    private:
        HostingSubsystem & hosting_;
        uint64 sequenceNumber_;
        bool isClosed_;
        mutable Common::RwLock lock_;
        EntryMap map_;
        std::map<std::wstring, std::set<ServiceTypeInstanceIdentifier>, Common::IsLessCaseInsensitiveComparer<std::wstring>> registeredFailuresMap_;
        bool isBlocklistingEnabled_;  

        static Common::GlobalWString AdhocServiceType;
    };
}
