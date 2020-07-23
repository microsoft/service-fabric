// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

StringLiteral const TraceType("ServiceTypeStateManager");

GlobalWString ServiceTypeStateManager::AdhocServiceType = make_global<wstring>(L"AdhocServiceType");

// ********************************************************************************************************************
// ServiceTypeStateManager::Entry Implementation
//

struct ServiceTypeStateManager::Entry
{
    DENY_COPY(Entry)

public:
    Entry(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, wstring const & applicationName)
        : ServiceTypeInstanceId(serviceTypeInstanceId),
        ApplicationName(applicationName),
        State(),
        DisableTimer(),
        DisabledRecoveryTimer(),
        RegisteredComponents()
    {
        ASSERT_IF(
            applicationName.empty() && !ServiceTypeInstanceId.ApplicationId.IsAdhoc(), 
            "ApplicationName required for NonAdhoc applications. ServiceTypeInstanceId={0}", 
            serviceTypeInstanceId);
    }

    Entry(
        ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, 
        wstring const & applicationName, 
        ServiceTypeStatus::Enum initialStatus, 
        uint64 lastSequenceNumber)
        : ServiceTypeInstanceId(serviceTypeInstanceId),
        ApplicationName(applicationName),
        State(initialStatus, lastSequenceNumber),
        DisableTimer(),
        DisabledRecoveryTimer(),
        RegisteredComponents()
    {
        ASSERT_IF(
            applicationName.empty() && !ServiceTypeInstanceId.ApplicationId.IsAdhoc(), 
            "ApplicationName required for NonAdhoc applications. ServiceTypeInstanceId={0}", serviceTypeInstanceId);
    }

    void WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write("{ ");
        w.Write("ServiceTypeInstanceId={0},", ServiceTypeInstanceId);
        w.Write("ApplicationName={0},", ApplicationName);
        w.Write("State={0},", State);
        w.Write("DisableTimer={0},", static_cast<void*>(DisableTimer.get()));
        w.Write("DisabledRecoveryTimer={0},", static_cast<void*>(DisabledRecoveryTimer.get()));        
        
        w.Write("RegisteredComponents={");
        for(auto iter = RegisteredComponents.begin(); iter != RegisteredComponents.end(); ++iter)
        {
            w.Write("{0},", *iter);
        }
        w.Write("}");

        w.Write(" }");
    }

public:
    ServiceTypeInstanceIdentifier const ServiceTypeInstanceId;
    wstring const ApplicationName;
    ServiceTypeState State;
    TimerSPtr DisableTimer;
    TimerSPtr DisabledRecoveryTimer;  
    set<wstring> RegisteredComponents;
};

// ********************************************************************************************************************
// ServiceTypeStateManager Implementation
//

ServiceTypeStateManager::ServiceTypeStateManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    hosting_(hosting),
    sequenceNumber_(0),
    isClosed_(false),
    map_(),
    registeredFailuresMap_(),
    isBlocklistingEnabled_(true)
{
}

ServiceTypeStateManager::~ServiceTypeStateManager()
{
}

ErrorCode ServiceTypeStateManager::Test_GetStateIfPresent(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
    __out ServiceTypeState & state)
{
    EntrySPtr entry;
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(serviceTypeInstanceId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::NotFound);
        }

        entry = iter->second;
        state = entry->State;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServiceTypeStateManager::GetState(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, __out ServiceTypeState & state)
{            
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(serviceTypeInstanceId);
        if (iter != map_.end())
        {
            EntrySPtr entry = iter->second;
            state = entry->State;
            
            WriteNoise(
                TraceType,
                Root.TraceId,
                "GetState: serviceTypeInstanceId={0}, State={1}",
                serviceTypeInstanceId,
                state);            
        }
        else
        {
            state = ServiceTypeState(ServiceTypeStatus::NotRegistered_Enabled, GetNextSequenceNumber_CallerHoldsLock());
        }      
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ServiceTypeStateManager::CreateNewServiceTypeEntry_CallerHoldsLock(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, 
    wstring const & applicationName, 
    __out EntrySPtr & entry)
{    
    WriteNoise(                
        TraceType,
        Root.TraceId,
        "CreateNewServiceTypeEntry_CallerHoldsLock: ServiceTypeInstanceId:{0}, ApplicationName:{1}",
        serviceTypeInstanceId,
        applicationName);

    // No health reports for Adhoc application
    if (!serviceTypeInstanceId.ApplicationId.IsAdhoc())
    {        
        auto error = hosting_.HealthManagerObj->RegisterSource(
            serviceTypeInstanceId.ServicePackageInstanceId,
            applicationName, 
            GetHealthPropertyId(serviceTypeInstanceId));
        if(!error.IsSuccess())
        {
            WriteWarning(                
                TraceType,
                Root.TraceId,
                "CreateNewServiceTypeEntry_CallerHoldsLock: Failed to register with HealthManger. serviceTypeInstanceId:{0}, ApplicationName:{1}, Error:{2}",
                serviceTypeInstanceId,
                applicationName,
                error);
            return error;
        }
    }    

    entry = make_shared<Entry>(serviceTypeInstanceId, applicationName, ServiceTypeStatus::NotRegistered_Enabled, GetNextSequenceNumber_CallerHoldsLock());
    map_.insert(make_pair(serviceTypeInstanceId, entry));

    if (serviceTypeInstanceId.ApplicationId.IsAdhoc())
    {        
        this->RegisterSource_CallerHoldsLock(entry, AdhocServiceType);
    }    

    return ErrorCodeValue::Success;
}

void ServiceTypeStateManager::SetFailureId_CallerHoldsLock(EntrySPtr const & entry, wstring const & failureId)
{
    // FailureId is empty for adhoc ServiceType
    if(!failureId.empty())
    {                
        entry->State.FailureId = failureId;        
        
        auto iter = registeredFailuresMap_.find(failureId);
        ASSERT_IF(iter == registeredFailuresMap_.end(), "FailureId:{0} should be registered", failureId);
        ASSERT_IF(iter->second.find(entry->ServiceTypeInstanceId) != iter->second.end(), "serviceTypeInstanceId={0} is already in the RegisteredFailureMap", entry->ServiceTypeInstanceId);

        iter->second.insert(entry->ServiceTypeInstanceId);        

        this->RegisterSource_CallerHoldsLock(entry, failureId);
    }
}

void ServiceTypeStateManager::ResetFailureId_CallerHoldsLock(EntrySPtr const & entry)
{
    wstring failureId = entry->State.FailureId;
    // FailureId is empty for adhoc ServiceType
    if(!failureId.empty())
    {
        entry->State.FailureId.clear();

        auto registeredFailureIter = registeredFailuresMap_.find(failureId);
        ASSERT_IF(registeredFailureIter == registeredFailuresMap_.end(), "FailureId:{0} should be registered", failureId);

        auto disabledTypeIter = registeredFailureIter->second.find(entry->ServiceTypeInstanceId);
        ASSERT_IF(disabledTypeIter == registeredFailureIter->second.end(), "serviceTypeInstanceId={0} should be in the RegisteredFailureMap", entry->ServiceTypeInstanceId);

        registeredFailureIter->second.erase(disabledTypeIter);

        this->UnregisterSource_CallerHoldsLock(entry, failureId);
    }    
}

void ServiceTypeStateManager::RegisterSource_CallerHoldsLock(EntrySPtr const & entry, wstring const & componentId)
{           
    ASSERT_IF(componentId.empty(), "ComponentId cannot be empty. Entry: {0}", *entry);
    ASSERT_IF(entry->RegisteredComponents.find(componentId) != entry->RegisteredComponents.end(), "ComponentId {0} is already registered", componentId);       
          
    entry->RegisteredComponents.insert(componentId);

    WriteNoise(        
        TraceType,
        Root.TraceId,
        "RegisterSource: ComponentId:{0}. Entry:{1}",
        componentId,
        *entry);    
}

void ServiceTypeStateManager::UnregisterSource_CallerHoldsLock(EntrySPtr const & entry, wstring const & componentId)
{
    ASSERT_IF(componentId.empty(), "ComponentId cannot be empty. Entry: {0}", *entry);

    auto componentIter = entry->RegisteredComponents.find(componentId);
    if(componentIter == entry->RegisteredComponents.end())
    {
        return;
    }

    entry->RegisteredComponents.erase(componentIter);
    
    if(entry->RegisteredComponents.empty())
    {        
        switch(entry->State.Status)
        {
        case ServiceTypeStatus::NotRegistered_Enabled:
        case ServiceTypeStatus::InProgress_Enabled:        
        case ServiceTypeStatus::Registered_Enabled:
            ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer must be null for entry {0}.", *entry);
            ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer must be null for entry {0}.", *entry);
            ASSERT_IFNOT(entry->State.FailureId.empty(), "FailureId must be empty for entry {0}.", *entry);

            entry->State.Status =  ServiceTypeStatus::Closed;
            entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
            break;

        default:
            Assert::CodingError("Entry: {0} is in an invalid state and cannot be deleted", *entry);

        }

        auto iter = map_.find(entry->ServiceTypeInstanceId);
        ASSERT_IF(iter == map_.end(), "ServiceType entry should be found. Entry:{0}", *entry);
        map_.erase(iter);

        WriteNoise(        
            TraceType,
            Root.TraceId,
            "UnregisterSource: Entry={0} is deleted.",
            *entry);    

        // No health reports for Adhoc application
        if(!entry->ServiceTypeInstanceId.ApplicationId.IsAdhoc())
        {
            hosting_.HealthManagerObj->UnregisterSource(
                entry->ServiceTypeInstanceId.ServicePackageInstanceId, 
                GetHealthPropertyId(entry->ServiceTypeInstanceId));
        }
    }
}

void ServiceTypeStateManager::ReportServiceTypeHealth(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, 
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    // No health reports for Adhoc application
    if(serviceTypeInstanceId.ApplicationId.IsAdhoc())
    {
        return;
    }

    hosting_.HealthManagerObj->ReportHealth(
        serviceTypeInstanceId.ServicePackageInstanceId,
        GetHealthPropertyId(serviceTypeInstanceId),
        reportCode,
        healthDescription,
        sequenceNumber);
}

vector<DeployedServiceTypeQueryResult> ServiceTypeStateManager::GetDeployedServiceTypeQueryResult(
    ApplicationIdentifier const & applicationId, 
    wstring const & filterServiceManifestName,
    wstring const & filterServiceTypeName)
{
    vector<DeployedServiceTypeQueryResult> serviceTypeQueryResult;
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return serviceTypeQueryResult; }

        //
        // ServiceTypeStateManager::EntryMap uses ServiceTypeInstanceIdentifier as key. Entries in it are ordered in
        // following order of comparison: ApplicationId -> ServicePackageName -> ActivationContext -> SericeTypeName
        //
        bool canBreak = false;
        for (auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            if (iter->first.ApplicationId == applicationId)
            {
                // We have found first entry with matching ApplicationId. All the entries for a given ApplicationId
                // are ordered contiguously in EntryMap. Break the loop at next item with different ApplicationId.
                canBreak = true;

                if(filterServiceManifestName.empty() || 
                   StringUtility::AreEqualCaseInsensitive(iter->first.ServicePackageInstanceId.ServicePackageName, filterServiceManifestName))
                {                                        
                    if(filterServiceTypeName.empty() || StringUtility::AreEqualCaseInsensitive(iter->first.ServiceTypeName, filterServiceTypeName))
                    {
                        auto serviceTypeEntry = iter->second;
                        if (applicationId.IsAdhoc() && !serviceTypeEntry->ServiceTypeInstanceId.ServiceTypeId.IsAdhocServiceType())
                        {
                            continue;
                        }

                        DeployedServiceTypeQueryResult queryResult(
                            serviceTypeEntry->ServiceTypeInstanceId.ServiceTypeName,
                            serviceTypeEntry->State.CodePackageName,
                            serviceTypeEntry->ServiceTypeInstanceId.ServicePackageInstanceId.ServicePackageName,
                            serviceTypeEntry->ServiceTypeInstanceId.ServicePackageInstanceId.PublicActivationId,
                            ServiceTypeStatus::ToPublicRegistrationStatus(serviceTypeEntry->State.Status));

                        serviceTypeQueryResult.push_back(move(queryResult));                       
                    }
                }                
            }
            else
            {
                if (canBreak)
                {
                    break;
                }
            }
        }        
    }

    return serviceTypeQueryResult;
}


void ServiceTypeStateManager::Test_GetServiceTypeEntryInStoredOrder(vector<ServiceTypeInstanceIdentifier> & svcTypeEntries)
{
    for (auto iter = map_.begin(); iter != map_.end(); ++iter)
    {
        svcTypeEntries.push_back(iter->second->ServiceTypeInstanceId);
    }
}

ErrorCode ServiceTypeStateManager::Test_GetHostAndRuntime(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
    __out std::wstring & hostId, 
    __out std::wstring & runtimeId)
{
    ServiceTypeState state;
    auto error = GetState(serviceTypeInstanceId, state);
    if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::NotFound)) { return error; }

    if (!error.IsError(ErrorCodeValue::NotFound) && ServiceTypeStatus::IsRegistered(state.Status))
    {
        hostId = state.HostId;
        runtimeId = state.RuntimeId;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
    }
}

bool ServiceTypeStateManager::Test_IsDisabled(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId)
{
    ServiceTypeState state;
    auto error = GetState(serviceTypeInstanceId, state);
    if (error.IsSuccess())
    {
        return ServiceTypeStatus::IsDisabled(state.Status);
    }
    else
    {
        return false;
    }
}

ErrorCode ServiceTypeStateManager::StartRegister(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, wstring const & applicationName, wstring const & runtimeId,  wstring const & codePackageName, wstring const & hostId)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "StartRegister: serviceTypeInstanceId={0}, RuntimeId={1}, CodePackageName={2}, HostId={3}",
        serviceTypeInstanceId,
        runtimeId,
        codePackageName,
        hostId);

    EntrySPtr entry;    
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(serviceTypeInstanceId);
        if (iter == map_.end())
        {
            if(serviceTypeInstanceId.ApplicationId.IsAdhoc())
            {
                // no existing entry found, add new entry for adhoc service type
                auto error = this->CreateNewServiceTypeEntry_CallerHoldsLock(serviceTypeInstanceId, applicationName, entry);
                ASSERT_IF(!error.IsSuccess(), "Creation of new Adhoc ServiceType should always succeed. serviceTypeInstanceId:{0}, Error:{1}", serviceTypeInstanceId, error);                
            }
            else
            {                
                WriteWarning(
                    TraceType,
                    Root.TraceId,
                    "StartRegister: serviceTypeInstanceId={0} is invalid and cannot be registered. Only ServiceTypes specified in the ServiceManifest can register.",
                    serviceTypeInstanceId,
                    runtimeId,
                    codePackageName,
                    hostId);
                
                return ErrorCodeValue::InvalidServiceType;
            }
        }
        else
        {
            entry = iter->second;
        }

        auto error = ProcessStartRegister(entry, runtimeId, codePackageName, hostId);           

        ASSERT_IF(entry->RegisteredComponents.empty(), "ServiceType entry should have one or more registered components. Entry={0}", *entry);

        return error;
    } // end of lock
}

ErrorCode ServiceTypeStateManager::ProcessStartRegister(EntrySPtr const & entry, wstring const & runtimeId, wstring const & codePackageName, wstring const & hostId)
{
    ErrorCode retval;

    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessStartRegister: Before={0}",
        *entry);

    // check the current status
    switch(entry->State.Status)
    {
    case ServiceTypeStatus::NotRegistered_Enabled:
        {
            ProcessStartRegister_NotRegistered_Enabled(entry, runtimeId, codePackageName, hostId);
            retval = ErrorCode(ErrorCodeValue::Success);
            break;
        }
    case ServiceTypeStatus::NotRegistered_Disabled:
        {
            ProcessStartRegister_NotRegistered_Disabled(entry, runtimeId, codePackageName, hostId);
            retval = ErrorCode(ErrorCodeValue::Success);
            break;
        }
    case ServiceTypeStatus::NotRegistered_DisableScheduled:
        {
            ProcessStartRegister_NotRegistered_DisableScheduled(entry, runtimeId, codePackageName, hostId);
            retval = ErrorCode(ErrorCodeValue::Success);
            break;
        }
    case ServiceTypeStatus::Registered_Enabled:
        {
            if (StringUtility::AreEqualCaseInsensitive(entry->State.RuntimeId, runtimeId))
            {
                retval = ErrorCode(ErrorCodeValue::HostingServiceTypeAlreadyRegisteredToSameRuntime);
            }
            else
            {
                retval = ErrorCode(ErrorCodeValue::HostingServiceTypeAlreadyRegistered);
            }
            break;
        }
    case ServiceTypeStatus::InProgress_Disabled:
    case ServiceTypeStatus::InProgress_Enabled:
        {
            retval = ErrorCode(ErrorCodeValue::InvalidState);
            break;
        }
    default:
        Assert::CodingError("Invalid state for entry {0}", entry);
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessStartRegister: After={0}, Error={1}",
        *entry,
        retval);

    return retval;
}

void ServiceTypeStateManager::ProcessStartRegister_NotRegistered_Enabled(
    EntrySPtr const & entry, 
    wstring const & runtimeId, 
    wstring const & codePackageName,
    wstring const & hostId)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.FailureId.empty(), "FailureId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::InProgress_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    entry->State.RuntimeId = runtimeId;
    entry->State.CodePackageName = codePackageName;
    entry->State.HostId = hostId;
}

void ServiceTypeStateManager::ProcessStartRegister_NotRegistered_Disabled(
    EntrySPtr const & entry, 
    wstring const & runtimeId, 
    wstring const & codePackageName,
    wstring const & hostId)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::InProgress_Disabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    entry->State.RuntimeId = runtimeId;
    entry->State.CodePackageName = codePackageName;
    entry->State.HostId = hostId;
    if (entry->DisabledRecoveryTimer != nullptr)
    {
        entry->DisabledRecoveryTimer->Cancel();
        entry->DisabledRecoveryTimer = nullptr;
    }
}

void ServiceTypeStateManager::ProcessStartRegister_NotRegistered_DisableScheduled(
    EntrySPtr const & entry, 
    wstring const & runtimeId, 
    wstring const & codePackageName,
    wstring const & hostId)
{
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);
    ASSERT_IF(entry->DisableTimer == nullptr, "DisableTimer for must not be null for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::InProgress_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    entry->State.RuntimeId = runtimeId;
    entry->State.CodePackageName = codePackageName;
    entry->State.HostId = hostId;     
    entry->DisableTimer->Cancel();
    entry->DisableTimer = nullptr;
    this->ResetFailureId_CallerHoldsLock(entry);
}

ErrorCode ServiceTypeStateManager::FinishRegister(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, 
    bool isValid, 
    FabricRuntimeContext const & runtimeContext)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "FinishRegister: serviceTypeInstanceId={0}, IsValid={1}, RuntimeContext={2}",
        serviceTypeInstanceId,
        isValid,
        runtimeContext);

    ErrorCode error;
    FABRIC_SEQUENCE_NUMBER healthSequence;
    EntrySPtr entry;    

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(serviceTypeInstanceId);
        if (iter == map_.end())
        {
            // No existing entry found. This would happen if the ServicePackage gets deactivated during the 
            // ServiceType registration process
            WriteInfo(
                TraceType,
                Root.TraceId,
                "FinishRegister: serviceTypeInstanceId {0} is not present in the map.",
                serviceTypeInstanceId);
            return ErrorCodeValue::NotFound;            
        }
        else
        {
            entry = iter->second;
        }                
        
        error = ProcessFinishRegister(entry, isValid, runtimeContext);                        

        healthSequence = SequenceNumber::GetNext();
    } // end of lock

    if(error.IsSuccess())
    {            
        if(isValid)
        {            
            this->ReportServiceTypeHealth(
                entry->ServiceTypeInstanceId,
                SystemHealthReportCode::Hosting_ServiceTypeRegistered,
                L"" /*extraDescription*/,
                healthSequence);
        }

        if(!isValid && serviceTypeInstanceId.ApplicationId.IsAdhoc())
        {
            this->Disable(serviceTypeInstanceId);
        }                
    }    

    return error;
}

ErrorCode ServiceTypeStateManager::ProcessFinishRegister(EntrySPtr const & entry, bool isValid, FabricRuntimeContext const & runtimeContext)
{
     WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessFinishRegister: Before={0}",
        *entry);

    ErrorCode retval;
    // check the current status
    switch(entry->State.Status)
    {
    case ServiceTypeStatus::InProgress_Enabled:
        {
            if (isValid)
            {
                ProcessFinishRegisterSuccess_InProgress_Enabled(entry, runtimeContext);
                retval = ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                ProcessFinishRegisterFailure_InProgress_Enabled(entry);
                retval = ErrorCode(ErrorCodeValue::Success);
            }
            break;
        }
    case ServiceTypeStatus::InProgress_Disabled:
        {
            if (isValid)
            {
                ProcessFinishRegisterSuccess_InProgress_Disabled(entry, runtimeContext);
                retval = ErrorCode(ErrorCodeValue::Success);
            }
            else
            {
                ProcessFinishRegisterFailure_InProgress_Disabled(entry);
                retval = ErrorCode(ErrorCodeValue::Success);
            }
            break;
        }
        
    case ServiceTypeStatus::Registered_Enabled:
        {
            // no-op
            retval = ErrorCode(ErrorCodeValue::Success);
            break;
        }
        
    case ServiceTypeStatus::NotRegistered_Enabled:
    case ServiceTypeStatus::NotRegistered_DisableScheduled:
    case ServiceTypeStatus::NotRegistered_Disabled:
        {
            retval = ErrorCode(ErrorCodeValue::InvalidState);
            break;
        }
    default:
        Assert::CodingError("Invalid state for entry {0}", *entry);
    }    

     WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessFinishRegister: After={0}, Error={1}",
        *entry,
        retval);

    return retval;
}

void ServiceTypeStateManager::ProcessFinishRegisterSuccess_InProgress_Enabled(EntrySPtr const & entry, FabricRuntimeContext const & runtimeContext)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.FailureId.empty(), "FailureId for must be empty for entry {0}.", *entry);
    ASSERT_IF(entry->State.RuntimeId.empty(), "RuntimeId for must not be empty for entry {0}.", *entry);
    ASSERT_IF(entry->State.HostId.empty(), "HostId for must not be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::Registered_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();

    this->RaiseServiceTypeRegisteredEvent_CallerHoldsLock(entry, runtimeContext);
}

void ServiceTypeStateManager::ProcessFinishRegisterFailure_InProgress_Enabled(EntrySPtr const & entry)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.FailureId.empty(), "FailureId for must be empty for entry {0}.", *entry);
    ASSERT_IF(entry->State.RuntimeId.empty(), "RuntimeId for must not be empty for entry {0}.", *entry);
    ASSERT_IF(entry->State.HostId.empty(), "HostId for must not be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::NotRegistered_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    entry->State.RuntimeId.clear();
    entry->State.CodePackageName.clear();
    entry->State.HostId.clear();
}

void ServiceTypeStateManager::ProcessFinishRegisterSuccess_InProgress_Disabled(EntrySPtr const & entry, FabricRuntimeContext const & runtimeContext)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisabledRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IF(entry->State.RuntimeId.empty(), "RuntimeId for must not be empty for entry {0}.", *entry);
    ASSERT_IF(entry->State.HostId.empty(), "HostId for must not be empty for entry {0}.", *entry);
    
    entry->State.Status = ServiceTypeStatus::Registered_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    this->ResetFailureId_CallerHoldsLock(entry);

    this->RaiseServiceTypeRegisteredEvent_CallerHoldsLock(entry, runtimeContext);
    this->RaiseServiceTypeEnabledEvent_CallerHoldsLock(entry);
}

void ServiceTypeStateManager::ProcessFinishRegisterFailure_InProgress_Disabled(EntrySPtr const & entry)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisabledRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IF(entry->State.RuntimeId.empty(), "RuntimeId for must not be empty for entry {0}.", *entry);
    ASSERT_IF(entry->State.HostId.empty(), "HostId for must not be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::NotRegistered_Disabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    entry->State.RuntimeId.clear();
    entry->State.CodePackageName.clear();
    entry->State.HostId.clear();
    CreateAndStartRecoveryTimerIfNeeded(entry);
}

ErrorCode ServiceTypeStateManager::OnServicePackageActivated(vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds, wstring const & applicationName, wstring const & componentId)
{
    ASSERT_IF(serviceTypeInstanceIds.empty(), "OnServicePackageActivating: Atleast one serviceTypeInstanceId is required");

    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnServicePackageActivating: ServicePackageId={0}",
        serviceTypeInstanceIds[0].ServicePackageInstanceId);
    
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        DateTime activationTime = DateTime::Now();

        for(auto serviceTypeInstanceIdIter = serviceTypeInstanceIds.begin(); serviceTypeInstanceIdIter != serviceTypeInstanceIds.end(); ++serviceTypeInstanceIdIter)
        {            
            EntrySPtr entry;
            auto iter = map_.find(*serviceTypeInstanceIdIter);
            if (iter == map_.end())
            {
                // no existing entry found, add new entry
                auto error = this->CreateNewServiceTypeEntry_CallerHoldsLock(*serviceTypeInstanceIdIter, applicationName, entry);
                if(!error.IsSuccess())
                {
                    return error;
                }
            }
            else
            {
                entry = iter->second;
                ASSERT_IFNOT(entry->State.HostId.empty(), "HostId should be empty when OnServicePackageActivated is called.");
                ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId should be empty when OnServicePackageActivated is called.");                
            }

            this->RegisterSource_CallerHoldsLock(entry, componentId);

            ASSERT_IF(entry->RegisteredComponents.empty(), "ServiceType entry should have one or more registered componenets. Entry={0}", *entry);
        }               
    } // end of lock

    return ErrorCodeValue::Success;
}

void ServiceTypeStateManager::OnServicePackageDeactivated(vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds, wstring const & componentId)
{        
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) return;

        for(auto serviceTypeIter = serviceTypeInstanceIds.begin(); serviceTypeIter != serviceTypeInstanceIds.end(); ++serviceTypeIter)
        {
            auto iter = map_.find(*serviceTypeIter);
            if(iter == map_.end()) { continue; }

            EntrySPtr entry = iter->second;

            WriteNoise(
                TraceType,
                Root.TraceId,
                "OnServicePackageDeactivated: {0}",
                entry->ServiceTypeInstanceId);                

            this->UnregisterSource_CallerHoldsLock(entry, componentId);

        }        
    }
}

void ServiceTypeStateManager::OnServiceTypeRegistrationNotFoundAfterTimeout(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, uint64 const sequenceNumber)
{
    FABRIC_SEQUENCE_NUMBER healthSequence = 0;
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) return;

        auto iter = map_.find(serviceTypeInstanceId);
        if(iter == map_.end())
        {
            return;
        }

        if(iter->second->State.Status != ServiceTypeStatus::NotRegistered_Enabled || iter->second->State.LastSequenceNumber != sequenceNumber)
        {
            return;
        }        

        healthSequence = SequenceNumber::GetNext();
    }

    this->ReportServiceTypeHealth(
        serviceTypeInstanceId,        
        SystemHealthReportCode::Hosting_ServiceTypeRegistrationTimeout,
        L"" /*extraDescription*/,
        healthSequence);
}

void ServiceTypeStateManager::ReportHealth(
    vector<ServiceTypeInstanceIdentifier> && serviceTypeInstanceIds,
    FABRIC_SEQUENCE_NUMBER healthSequence)
{
    for (auto const& svcTypeInstanceId : serviceTypeInstanceIds)
    {
        auto const& servicePackageInstanceId = svcTypeInstanceId.ServicePackageInstanceId;

        auto shouldReportHealth = hosting_.ApplicationManagerObj->ShouldReportHealth(servicePackageInstanceId);

        if (shouldReportHealth)
        {
            this->ReportServiceTypeHealth(
                svcTypeInstanceId,
                SystemHealthReportCode::Hosting_ServiceTypeUnregistered,
                L"" /*healthDescription*/,
                healthSequence);
        }
    }
}

void ServiceTypeStateManager::DisableAdhocTypes(vector<ServiceTypeInstanceIdentifier> && adhocDisableList)
{
    for (auto const & adhocType : adhocDisableList)
    {
        this->Disable(adhocType);
    }
}

void ServiceTypeStateManager::ProcessServiceTypeUnregistered_CallerHoldsLock(
    bool isRuntimeClosed,
    wstring const & runtimeOrHostId,
    uint64 sequenceNumber,
    _Out_ vector<ServiceTypeInstanceIdentifier> & adhocDisableList,
    _Out_ vector<ServiceTypeInstanceIdentifier> & serviceTypeInstanceIds,
    _Out_ vector<ServiceTypeIdentifier> & serviceTypeIds,
    _Out_ ServicePackageInstanceIdentifier & servicePackageInstanceId,
    _Out_ ServicePackageActivationContext & activationContext,
    _Out_ wstring & servicePackageActivationId)
{
    for (auto const & kvPair : map_)
    {
        auto const & entry = kvPair.second;
        auto const & filterProperty = isRuntimeClosed ? entry->State.RuntimeId : entry->State.HostId;

        if (StringUtility::AreEqualCaseInsensitive(filterProperty, runtimeOrHostId))
        {
            ProcessApplicationHostOrRuntimeClosed(entry, sequenceNumber, adhocDisableList);

            serviceTypeInstanceIds.push_back(entry->ServiceTypeInstanceId);
            serviceTypeIds.push_back(entry->ServiceTypeInstanceId.ServiceTypeId);
        }
    }

    if (serviceTypeInstanceIds.size() > 0)
    {
        auto const & serviceTypeInstanceId = serviceTypeInstanceIds.front();

        servicePackageInstanceId = serviceTypeInstanceId.ServicePackageInstanceId;
        activationContext = serviceTypeInstanceId.ActivationContext;
        servicePackageActivationId = serviceTypeInstanceId.PublicActivationId;
    }
}

void ServiceTypeStateManager::OnRuntimeClosed(wstring const & runtimeId, wstring const & hostId)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnRuntimeClosed: RuntimeId={0}, HostId={1}",
        runtimeId,
        hostId);

     vector<ServiceTypeInstanceIdentifier> adhocDisableList;
     vector<ServiceTypeInstanceIdentifier> serviceTypeInstanceIds;
     vector<ServiceTypeIdentifier> serviceTypeIds;
     FABRIC_SEQUENCE_NUMBER healthSequence;
     uint64 sequenceNumber = 0;
     ApplicationIdentifier applicationId;
     ServicePackageInstanceIdentifier servicePackageInstanceId;
     ServicePackageActivationContext activationContext;
     wstring servicePackageActivationId;

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) return;

        sequenceNumber = GetNextSequenceNumber_CallerHoldsLock();

        this->ProcessServiceTypeUnregistered_CallerHoldsLock(
            true /* isRuntimeClosed */,
            runtimeId,
            sequenceNumber,
            adhocDisableList,
            serviceTypeInstanceIds,
            serviceTypeIds,
            servicePackageInstanceId,
            activationContext,
            servicePackageActivationId);

        //
        // Notify service package corresponding to these service types
        // to update ServiceType registration timeout tracking information.
        //
        hosting_.ApplicationManagerObj->OnServiceTypesUnregistered(
            servicePackageInstanceId, serviceTypeInstanceIds);

        //
        // Raise event for RA and other interested components. 
        //
        this->RaiseRuntimeClosedEvent_CallerHoldsLock(
            runtimeId, hostId, serviceTypeIds, sequenceNumber, activationContext, servicePackageActivationId);

        healthSequence = SequenceNumber::GetNext();
    }

    this->ReportHealth(move(serviceTypeInstanceIds), healthSequence);

    this->DisableAdhocTypes(move(adhocDisableList));
}

void ServiceTypeStateManager::OnApplicationHostClosed(
    ActivityDescription const & activityDescription, 
    wstring const & hostId)
{
    WriteInfo(
        TraceType,
        Root.TraceId,
        "OnApplicationHostClosed: HostId={0} activityDescription={1}",
        hostId,
        activityDescription);

    vector<ServiceTypeInstanceIdentifier> adhocDisableList;
    vector<ServiceTypeInstanceIdentifier> serviceTypeInstanceIds;
    vector<ServiceTypeIdentifier> serviceTypeIds;
    FABRIC_SEQUENCE_NUMBER healthSequence;
    uint64 sequenceNumber = 0;
    ApplicationIdentifier applicationId;
    ServicePackageInstanceIdentifier servicePackageInstanceId;
    ServicePackageActivationContext activationContext;
    wstring servicePackageActivationId;

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) return;

        sequenceNumber = GetNextSequenceNumber_CallerHoldsLock();

        this->ProcessServiceTypeUnregistered_CallerHoldsLock(
            false /* isRuntimeClosed */,
            hostId,
            sequenceNumber,
            adhocDisableList,
            serviceTypeInstanceIds,
            serviceTypeIds,
            servicePackageInstanceId,
            activationContext,
            servicePackageActivationId);

        hosting_.ApplicationManagerObj->OnServiceTypesUnregistered(
            servicePackageInstanceId, serviceTypeInstanceIds);

        //
        // Raise event for RA and other interested components. 
        //
        this->RaiseApplicationHostClosedEvent_CallerHoldsLock(
            activityDescription, hostId, serviceTypeIds, sequenceNumber, activationContext, servicePackageActivationId);

        healthSequence = SequenceNumber::GetNext();
    }

    this->ReportHealth(move(serviceTypeInstanceIds), healthSequence);

    this->DisableAdhocTypes(move(adhocDisableList));
}

void ServiceTypeStateManager::ProcessApplicationHostOrRuntimeClosed(
    EntrySPtr const & entry, 
    uint64 sequenceNumber, 
    __out vector<ServiceTypeInstanceIdentifier> & adhocDisableList)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessHostOrRuntimeClosed: Before={0}, SequenceNumber={1}",
        *entry,
        sequenceNumber);

    switch(entry->State.Status)
    {
    case ServiceTypeStatus::InProgress_Enabled:
    case ServiceTypeStatus::Registered_Enabled:
        {
            ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
            ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
            ASSERT_IFNOT(entry->State.FailureId.empty(), "FailureId for must be empty for entry {0}.", *entry);
            ASSERT_IF(entry->State.RuntimeId.empty(), "RuntimeId for must not be empty for entry {0}.", *entry);
            ASSERT_IF(entry->State.HostId.empty(), "HostId for must not be empty for entry {0}.", *entry);

            entry->State.Status = ServiceTypeStatus::NotRegistered_Enabled;
            entry->State.LastSequenceNumber = sequenceNumber;
            entry->State.RuntimeId.clear();
            entry->State.CodePackageName.clear();
            entry->State.HostId.clear();
            if (entry->ServiceTypeInstanceId.ApplicationId.IsAdhoc())
            {
                adhocDisableList.push_back(entry->ServiceTypeInstanceId);
            }
            break;
        }

    case ServiceTypeStatus::InProgress_Disabled:
        {
            ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
            ASSERT_IF(entry->State.RuntimeId.empty(), "RuntimeId for must not be empty for entry {0}.", *entry);
            ASSERT_IF(entry->State.HostId.empty(), "HostId for must not be empty for entry {0}.", *entry);

            entry->State.Status = ServiceTypeStatus::NotRegistered_Disabled;
            entry->State.LastSequenceNumber = sequenceNumber;
            entry->State.RuntimeId.clear();
            entry->State.CodePackageName.clear();
            entry->State.HostId.clear();
            CreateAndStartRecoveryTimerIfNeeded(entry);
            break;
        }

    case ServiceTypeStatus::NotRegistered_Enabled:
    case ServiceTypeStatus::NotRegistered_DisableScheduled:
    case ServiceTypeStatus::NotRegistered_Disabled:
        Assert::CodingError("Invalid state for entry {0}. It must not be associated with an ApplicationHost or a Runtime.", *entry);
        break;
    default:
        Assert::CodingError("Invalid state for entry {0}", *entry);
        break;
    }
   
    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessHostOrRuntimeClosed: After={0}",
        *entry);
}

void ServiceTypeStateManager::Disable(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId)
{
    this->Disable(serviceTypeInstanceId, L"", L"");
}

void ServiceTypeStateManager::Disable(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, wstring const & applicationName, wstring const & failureId)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Disable: serviceTypeInstanceId={0}, FailureId={1}",
        serviceTypeInstanceId, 
        failureId);
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return; }
        if (!isBlocklistingEnabled_) { return; }

        ASSERT_IF(failureId.empty() && !serviceTypeInstanceId.ApplicationId.IsAdhoc(), "Only AdhocServiceType can have empty FailureId. serviceTypeInstanceId: {0}", serviceTypeInstanceId);

        if(!serviceTypeInstanceId.ApplicationId.IsAdhoc() && registeredFailuresMap_.find(failureId) == registeredFailuresMap_.end()) 
        { 
            WriteNoise(
                TraceType,
                Root.TraceId,
                "Disable: The FailureId={0} is not registered. serviceTypeInstanceId={1}",
                failureId,
                serviceTypeInstanceId);

            return; 
        }
                
        EntrySPtr entry;
        auto iter = map_.find(serviceTypeInstanceId);
        if (iter == map_.end())
        {
            auto error = this->CreateNewServiceTypeEntry_CallerHoldsLock(serviceTypeInstanceId, applicationName, entry);
            if(!error.IsSuccess())
            {
                return;
            }
        }
        else
        {
            entry = iter->second;
        }
        
        ProcessDisable(entry, failureId, HostingConfig::GetConfig().ServiceTypeDisableGraceInterval);        

        ASSERT_IF(entry->RegisteredComponents.empty(), "ServiceType entry should have one or more registered components. Entry={0}", *entry);
    }
}

void ServiceTypeStateManager::ProcessDisable(EntrySPtr const & entry, wstring const & failureId, Common::TimeSpan const graceInterval)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessDisable: Before={0}, FailureId={1}, GraceInterval={2}",
        *entry, failureId, graceInterval);

    if (entry->State.Status != ServiceTypeStatus::NotRegistered_Enabled)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Ignoring Disabled(FailureId={0}) received for entry {1}.",
            failureId,
            *entry);        

        return;
    }

    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.FailureId.empty(), "FailureId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);
    
    entry->State.Status = ServiceTypeStatus::NotRegistered_DisableScheduled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    this->SetFailureId_CallerHoldsLock(entry, failureId);    

    CreateAndStartDisableTimer(entry, graceInterval);        

    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessDisable: After={0}",
        *entry);
}

void ServiceTypeStateManager::OnDisableTimerCallback(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, uint64 sequenceNumber)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnDisableTimerCallback: serviceTypeInstanceId={0}, SequenceNumber={1}",
        serviceTypeInstanceId,
        sequenceNumber);
    
    EntrySPtr entry;
    FABRIC_SEQUENCE_NUMBER healthSequence;
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return; }

        auto iter = map_.find(serviceTypeInstanceId);
        if(iter == map_.end())
        {
            WriteInfo(
                TraceType,
                Root.TraceId,
                "OnDisableTimerCallback: serviceTypeInstanceId {0} is not present in the map.",
                serviceTypeInstanceId);
            return;
        }
        
        entry = iter->second;

        WriteNoise(
            TraceType,
            Root.TraceId,
            "ProcessDisableTimerCallback: Before={0}",
            *entry);
        
        if ((entry->State.Status != ServiceTypeStatus::NotRegistered_DisableScheduled) || (entry->State.LastSequenceNumber != sequenceNumber))
        {
            WriteInfo(
                TraceType,
                Root.TraceId,
                "Ignoring stale scheduled disable (SequenceNumber={0}) for entry {2}.",
                sequenceNumber,
                *entry);
            return;
        }

        ASSERT_IF(entry->DisableTimer == nullptr, "DisableTimer for must not be null for entry {0}.", *entry);
        ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
        ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
        ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
        ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);
        
        entry->State.Status = ServiceTypeStatus::NotRegistered_Disabled;
        entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
        entry->DisableTimer->Cancel();
        entry->DisableTimer = nullptr;
        CreateAndStartRecoveryTimerIfNeeded(entry);

        this->RaiseServiceTypeDisabledEvent_CallerHoldsLock(entry);

        WriteInfo(
            TraceType,
            Root.TraceId,
            "HostingServiceTypeDisabled ServiceTypeInstanceId: {0}.",
            serviceTypeInstanceId);

        healthSequence = SequenceNumber::GetNext();
    }    

    this->ReportServiceTypeHealth(
        entry->ServiceTypeInstanceId,
        SystemHealthReportCode::Hosting_ServiceTypeDisabled,
        L"" /*extraDescription*/,
        healthSequence);

    WriteNoise(
        TraceType,
        Root.TraceId,
        "ProcessDisableTimerCallback: After={0}",
        *entry);
}

void ServiceTypeStateManager::OnRecoveryTimerCallback(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId, uint64 sequenceNumber)
{
    ASSERT_IFNOT(serviceTypeInstanceId.ApplicationId.IsAdhoc(), "OnRecoveryTimerCallback should be called only for adhoc ServiceTypes. serviceTypeInstanceId:{0}", serviceTypeInstanceId);

    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnRecoveryTimerCallback: serviceTypeInstanceId={0}, SequenceNumber={1}",
        serviceTypeInstanceId,
        sequenceNumber);

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return; }

        auto iter = map_.find(serviceTypeInstanceId);        
        ASSERT_IF(iter == map_.end(), "Entry for serviceTypeInstanceId={0} should be found", serviceTypeInstanceId);        

        auto entry = iter->second;

        WriteNoise(
            TraceType,
            Root.TraceId,
            "ProcessRecoveryTimerCallback: Before={0}",
            *entry);

        if ((entry->State.Status != ServiceTypeStatus::NotRegistered_Disabled) || (entry->State.LastSequenceNumber != sequenceNumber))
        {
            WriteInfo(
                TraceType,
                Root.TraceId,
                "Ignoring stale scheduled recovery (SequenceNumber={0}) for entry {2}.",
                sequenceNumber,
                *entry);
            return;
        }

        ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
        ASSERT_IF(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must not be null for entry {0}.", *entry);
        ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
        ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
        ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);

        entry->State.Status = ServiceTypeStatus::NotRegistered_Enabled;
        entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();        
        entry->DisabledRecoveryTimer->Cancel();
        entry->DisabledRecoveryTimer = nullptr;        

        this->RaiseServiceTypeEnabledEvent_CallerHoldsLock(entry);

        this->UnregisterSource_CallerHoldsLock(entry, AdhocServiceType);
    }
}

void ServiceTypeStateManager::CreateAndStartDisableTimer(EntrySPtr const & entry, TimeSpan const disableInterval)
{
    auto root = Root.CreateComponentRoot();
    auto sequenceNumber = entry->State.LastSequenceNumber;
    auto serviceTypeInstanceId = entry->ServiceTypeInstanceId;

    entry->DisableTimer = Timer::Create(TimerTagDefault, [root, this, serviceTypeInstanceId, sequenceNumber](TimerSPtr const & timer)
    { 
        timer->Cancel(); 
        this->OnDisableTimerCallback(serviceTypeInstanceId, sequenceNumber);
    });
    entry->DisableTimer->Change(disableInterval);
}

void ServiceTypeStateManager::CreateAndStartRecoveryTimerIfNeeded(EntrySPtr const & entry)
{
    if (entry->ServiceTypeInstanceId.ApplicationId.IsAdhoc() && HostingConfig::GetConfig().DisabledServiceTypeRecoveryInterval != TimeSpan::MaxValue)
    {
        auto recoveryInterval = HostingConfig::GetConfig().DisabledServiceTypeRecoveryInterval;        
        auto root = Root.CreateComponentRoot();
        auto sequenceNumber = entry->State.LastSequenceNumber;
        auto serviceTypeInstanceId = entry->ServiceTypeInstanceId;

        entry->DisabledRecoveryTimer = Timer::Create(TimerTagDefault, [root, this, serviceTypeInstanceId, sequenceNumber](TimerSPtr const & timer)
        { 
            timer->Cancel(); 
            this->OnRecoveryTimerCallback(serviceTypeInstanceId, sequenceNumber);
        });
        entry->DisabledRecoveryTimer->Change(recoveryInterval);        
    }
}

void ServiceTypeStateManager::RegisterFailure(wstring const & failureId)
{
    WriteNoise(TraceType, Root.TraceId, "RegisterFailure: FailureId={0}", failureId);

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return; }

        auto iter = registeredFailuresMap_.find(failureId);
        if (iter == registeredFailuresMap_.end())
        {
            registeredFailuresMap_.insert(make_pair(failureId, move(set<ServiceTypeInstanceIdentifier>())));
        }
    }
}

void ServiceTypeStateManager::UnregisterFailure(wstring const & failureId)
{
    WriteNoise(TraceType, Root.TraceId, "UnregisterFailure: FailureId={0}", failureId);
    
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) return;
                        
        auto registeredFailureIter = registeredFailuresMap_.find(failureId);
        if(registeredFailureIter != registeredFailuresMap_.end())
        {      
            set<ServiceTypeInstanceIdentifier> disabledServiceTypes = registeredFailureIter->second;
            for(auto disabledTypeIter = disabledServiceTypes.begin(); disabledTypeIter != disabledServiceTypes.end(); ++disabledTypeIter)
            {
                auto iter = map_.find(*disabledTypeIter);
                ASSERT_IF(iter == map_.end(), "An entry for serviceTypeInstanceId={0} should be found", *disabledTypeIter);

                EntrySPtr entry = iter->second;
                ASSERT_IFNOT(StringUtility::AreEqualCaseInsensitive(entry->State.FailureId, failureId), "The FailueId={0} should match the FailureId on the Entry={1}", failureId, *entry);
                
                ProcessUnregisterFailure(entry);                                
            }

            ASSERT_IFNOT(registeredFailureIter->second.empty(), "The failure on all ServiceTypes should have been removed");
            registeredFailuresMap_.erase(registeredFailureIter);
        }
    }
}

void ServiceTypeStateManager::ProcessUnregisterFailure(EntrySPtr const & entry)
{    
    WriteNoise(
        TraceType, 
        Root.TraceId, 
        "ProcessUnregisterFailure: Before={0}",
        *entry);

    switch(entry->State.Status)
    {
    case ServiceTypeStatus::NotRegistered_DisableScheduled:
        this->ProcessUnregisterFailure_NotRegistered_DisableScheduled(entry);
        break;

    case ServiceTypeStatus::NotRegistered_Disabled:
        this->ProcessUnregisterFailure_NotRegistered_Disabled(entry);
        break;

    case ServiceTypeStatus::InProgress_Disabled:
        this->ProcessUnregisterFailure_InProgress_Disabled(entry);
        break;

    case ServiceTypeStatus::NotRegistered_Enabled:
    case ServiceTypeStatus::InProgress_Enabled:
    case ServiceTypeStatus::Registered_Enabled:
        Assert::CodingError("Invalid state for entry {0}. It must not be associated with a Failure.", *entry);
        break;
    default:
        Assert::CodingError("Invalid state for entry {0}", *entry);
        break;
    }    

    WriteNoise(
        TraceType, 
        Root.TraceId, 
        "ProcessUnregisterFailure: After={0}",
        *entry);
}

void ServiceTypeStateManager::ProcessUnregisterFailure_NotRegistered_DisableScheduled(EntrySPtr const & entry)
{
    ASSERT_IF(entry->DisableTimer == nullptr, "DisableTimer for must not be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->DisabledRecoveryTimer == nullptr, "DisableRecoveryTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::NotRegistered_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();        
    entry->DisableTimer->Cancel();
    entry->DisableTimer = nullptr;    
    this->ResetFailureId_CallerHoldsLock(entry);
}

void ServiceTypeStateManager::ProcessUnregisterFailure_NotRegistered_Disabled(EntrySPtr const & entry)
{
    ASSERT_IFNOT(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.RuntimeId.empty(), "RuntimeId for must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.CodePackageName.empty(), "CodePackageName must be empty for entry {0}.", *entry);
    ASSERT_IFNOT(entry->State.HostId.empty(), "HostId for must be empty for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::NotRegistered_Enabled;
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();       
    if (entry->DisabledRecoveryTimer != nullptr)
    {
        entry->DisabledRecoveryTimer->Cancel();
        entry->DisabledRecoveryTimer = nullptr;
    }    
    this->ResetFailureId_CallerHoldsLock(entry);

    this->RaiseServiceTypeEnabledEvent_CallerHoldsLock(entry);
}

void ServiceTypeStateManager::ProcessUnregisterFailure_InProgress_Disabled(EntrySPtr const & entry)
{
    ASSERT_IF(entry->DisableTimer == nullptr, "DisableTimer for must be null for entry {0}.", *entry);
    ASSERT_IF(entry->DisabledRecoveryTimer == nullptr, "DisabledRecoveryTimer for must be null for entry {0}.", *entry);

    entry->State.Status = ServiceTypeStatus::InProgress_Enabled;    
    entry->State.LastSequenceNumber = GetNextSequenceNumber_CallerHoldsLock();
    this->ResetFailureId_CallerHoldsLock(entry);

    this->RaiseServiceTypeEnabledEvent_CallerHoldsLock(entry);
}

void ServiceTypeStateManager::Close()
{
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return; }
        isClosed_ = true;

        for(auto iter = map_.begin(); iter != map_.end(); ++iter)
        {
            CloseEntry(iter->second);
        }

        map_.clear();
        registeredFailuresMap_.clear();
    }

}

void ServiceTypeStateManager::CloseEntry(EntrySPtr const & entry)
{
    if (entry->DisabledRecoveryTimer)
    {
        entry->DisabledRecoveryTimer->Cancel();
        entry->DisabledRecoveryTimer = nullptr;
    }

    if (entry->DisableTimer)
    {
        entry->DisableTimer->Cancel();
        entry->DisableTimer = nullptr;
    }
}

uint64 ServiceTypeStateManager::GetNextSequenceNumber_CallerHoldsLock()
{
    return ++sequenceNumber_;
}

bool ServiceTypeStateManager::get_BlocklistingEnabled() const
{
    {
        AcquireReadLock lock(lock_);
        return this->isBlocklistingEnabled_;
    }
}

void ServiceTypeStateManager::set_BlocklistingEnabled(bool value)
{
    {
        AcquireWriteLock lock(lock_);
        this->isBlocklistingEnabled_ = value;
    }
}

void ServiceTypeStateManager::RaiseServiceTypeRegisteredEvent_CallerHoldsLock(EntrySPtr const & entry, FabricRuntimeContext const & runtimeContext)
{
    auto registration = CreateServiceTypeRegistration(
        entry->ServiceTypeInstanceId.ServiceTypeId, 
        entry->ServiceTypeInstanceId.ActivationContext,
		entry->ServiceTypeInstanceId.ServicePackageInstanceId.PublicActivationId,
        runtimeContext);
    
    ServiceTypeRegistrationEventArgs eventArgs(entry->State.LastSequenceNumber, registration);
    hosting_.EventDispatcherObj->EnqueueServiceTypeRegisteredEvent(move(eventArgs));
}

void ServiceTypeStateManager::RaiseServiceTypeEnabledEvent_CallerHoldsLock(EntrySPtr const & entry)
{
    ServiceTypeStatusEventArgs eventArgs(
        entry->State.LastSequenceNumber,
        entry->ServiceTypeInstanceId.ServiceTypeId,
        entry->ServiceTypeInstanceId.ActivationContext,
        entry->ServiceTypeInstanceId.PublicActivationId);

    hosting_.EventDispatcherObj->EnqueueServiceTypeEnabledEvent(move(eventArgs));
}

void ServiceTypeStateManager::RaiseServiceTypeDisabledEvent_CallerHoldsLock(EntrySPtr const & entry)
{
    ServiceTypeStatusEventArgs eventArgs(
        entry->State.LastSequenceNumber, 
        entry->ServiceTypeInstanceId.ServiceTypeId, 
        entry->ServiceTypeInstanceId.ActivationContext,
        entry->ServiceTypeInstanceId.PublicActivationId);

    hosting_.EventDispatcherObj->EnqueueServiceTypeDisabledEvent(move(eventArgs));
}

void ServiceTypeStateManager::RaiseApplicationHostClosedEvent_CallerHoldsLock(
    Common::ActivityDescription const & activityDescription,
    std::wstring const & hostId, 
    vector<ServiceTypeIdentifier> const & serviceTypes, 
    uint64 sequenceNumber,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackageActivationId)
{
    ApplicationHostClosedEventArgs eventArgs(sequenceNumber, hostId, serviceTypes, activationContext, servicePackageActivationId, activityDescription);
    hosting_.EventDispatcherObj->EnqueueApplicationHostClosedEvent(move(eventArgs));
}

void ServiceTypeStateManager::RaiseRuntimeClosedEvent_CallerHoldsLock(
    std::wstring const & runtimeId, 
    std::wstring const & hostId, 
    vector<ServiceTypeIdentifier> const & serviceTypes, 
    uint64 sequenceNumber,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackageActivationId)
{
    RuntimeClosedEventArgs eventArgs(sequenceNumber, hostId, runtimeId, serviceTypes, activationContext, servicePackageActivationId);
    hosting_.EventDispatcherObj->EnqueueRuntimeClosedEvent(move(eventArgs));
}

wstring ServiceTypeStateManager::GetHealthPropertyId(ServiceTypeInstanceIdentifier const & serviceTypeInstanceId)
{
    return wformatString("ServiceTypeRegistration:{0}", serviceTypeInstanceId.ServiceTypeName);
}
