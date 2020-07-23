// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Hosting2;
using namespace Reliability;

StringLiteral const TraceType("FabricRuntimeManager");

// ********************************************************************************************************************
// FabricRuntimeManager Implementation
//
FabricRuntimeManager::FabricRuntimeManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting) :
    RootedObject(root),
    hosting_(hosting),
    runtimeRegistrationTable_(),
    serviceTypeStateManager_()
{
    auto runtimeRegistrationTable = make_unique<RuntimeRegistrationTable>(root);
    auto serviceTypeStateManager = make_unique<ServiceTypeStateManager>(root, hosting_);

    runtimeRegistrationTable_ = move(runtimeRegistrationTable);
    serviceTypeStateManager_ = move(serviceTypeStateManager);
}

FabricRuntimeManager::~FabricRuntimeManager()
{
}

ErrorCode FabricRuntimeManager::OnOpen()
{
    this->RegisterIpcRequestHandler();

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode FabricRuntimeManager::OnClose()
{
    this->UnregisterIpcRequestHandler();
    this->runtimeRegistrationTable_->Close();
    this->serviceTypeStateManager_->Close();

    return ErrorCode(ErrorCodeValue::Success);
}

void FabricRuntimeManager::OnAbort()
{
    this->UnregisterIpcRequestHandler();
    this->runtimeRegistrationTable_->Close();
    this->serviceTypeStateManager_->Close();
}

ErrorCode FabricRuntimeManager::FindServiceTypeRegistration(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    Reliability::ServiceDescription const & serviceDescription,
    ServiceModel::ServicePackageActivationContext const & activationContext,
    __out uint64 & sequenceNumber,
    __out ServiceTypeRegistrationSPtr & serviceTypeRegistration)
{
    return FindServiceTypeRegistration(
	versionedServiceTypeId,
	serviceDescription,
        activationContext,
	true, // shouldActivate,
	sequenceNumber,
        serviceTypeRegistration);
}

ErrorCode FabricRuntimeManager::GetHostId(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    wstring const & applicationName,
    __out wstring & hostId)
{
    return GetHostId(
      	versionedServiceTypeId,
        applicationName,
        ServicePackageActivationContext(),
        hostId);
}

ErrorCode FabricRuntimeManager::GetHostId(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    wstring const & applicationName,
    ServicePackageActivationContext const & activationContext,
    __out wstring & hostId)
{
    UNREFERENCED_PARAMETER(applicationName);
    
    uint64 sequenceNumber;
    ServiceTypeRegistrationSPtr registrationSPtr;

    auto error = FindServiceTypeRegistration(
        versionedServiceTypeId,
        ServiceDescription(), // Not needed when shouldActivate=false.
        activationContext,
        false, // shouldActivate,
        sequenceNumber,
        registrationSPtr);

    if (error.IsSuccess())
    {
        hostId = registrationSPtr->HostId;
    }

    return error;
}

ErrorCode FabricRuntimeManager::FindServiceTypeRegistration(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    Reliability::ServiceDescription const & serviceDescription,
    ServiceModel::ServicePackageActivationContext const & activationContext,
    bool shouldActivate,
    __out uint64 & sequenceNumber,
    __out ServiceTypeRegistrationSPtr & serviceTypeRegistration)
{
    serviceTypeRegistration = ServiceTypeRegistrationSPtr();
    ServiceTypeState serviceTypeState;

    wstring svcPkgPublicActivationId;

    if (shouldActivate)
    {
        svcPkgPublicActivationId = hosting_.GetOrAddServicePackagePublicActivationId(
            versionedServiceTypeId.Id.ServicePackageId,
            activationContext,
            serviceDescription.Name,
            serviceDescription.ApplicationName);
    }
    else
    {
        auto res = hosting_.TryGetServicePackagePublicActivationId(
            versionedServiceTypeId.Id.ServicePackageId,
            activationContext,
            svcPkgPublicActivationId);

        if (!res)
        {
            return ErrorCodeValue::HostingServiceTypeNotRegistered;
        }
    }

    ServiceTypeInstanceIdentifier serviceTypeInstanceId(versionedServiceTypeId.Id, activationContext, svcPkgPublicActivationId);

    auto error = serviceTypeStateManager_->GetState(serviceTypeInstanceId, serviceTypeState);
    if (!error.IsSuccess())
    {
        return error;
    }

    // check the state of the service type
    sequenceNumber = serviceTypeState.LastSequenceNumber;

    switch (serviceTypeState.Status)
    {
    case ServiceTypeStatus::Registered_Enabled:
    {
        // create registration information and return
        error = CreateServiceTypeRegistration(
            serviceTypeState.RuntimeId, 
            versionedServiceTypeId.Id,
            activationContext,
            svcPkgPublicActivationId,
            serviceTypeRegistration);
        
        if (error.IsSuccess())
        {
            error = EnsureReturnedServiceTypeVersion(versionedServiceTypeId.servicePackageVersionInstance, serviceTypeRegistration);
        }
        break;
    }

    case ServiceTypeStatus::NotRegistered_Disabled:
    {
        error = ErrorCode(ErrorCodeValue::HostingServiceTypeDisabled);
        break;
    }

    case ServiceTypeStatus::NotRegistered_Enabled:
    {
        if (serviceTypeInstanceId.ApplicationId.IsAdhoc())
        {
            // disable the servicetype as we do not know how to activate adhoc services
            serviceTypeStateManager_->Disable(serviceTypeInstanceId);
        }
        else if (shouldActivate)
        {
            // try to download and activate managed service types
            error = DownloadAndActivate(
                sequenceNumber, 
                versionedServiceTypeId, 
                activationContext,
                svcPkgPublicActivationId,
                serviceDescription.ApplicationName);
        }

        if (error.IsSuccess())
        {
            error = ErrorCodeValue::HostingServiceTypeNotRegistered;
        }

        break;
    }

    case ServiceTypeStatus::NotRegistered_DisableScheduled:
    {
        // try to download and activate managed services types
        // adhoc service type is already scheduled for disable
        if (!serviceTypeInstanceId.ApplicationId.IsAdhoc() && shouldActivate)
        {
            // try to download and activate managed service types
            error = DownloadAndActivate(
                sequenceNumber, 
                versionedServiceTypeId, 
                activationContext,
                svcPkgPublicActivationId,
                serviceDescription.ApplicationName);
        }

        if (error.IsSuccess())
        {
            error = ErrorCodeValue::HostingServiceTypeNotRegistered;
        }

        break;
    }
    case ServiceTypeStatus::InProgress_Enabled:
    case ServiceTypeStatus::InProgress_Disabled:
    {
        // registration is in progress, no action ncessary
        error = ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
        break;
    }

    default:
        Assert::CodingError("Invalid ServiceTypeState: {0}", serviceTypeState);
    }

    return error;
}

ErrorCode FabricRuntimeManager::GetRuntimeIds(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    __out CaseInsensitiveStringSet & runtimeIds)
{
    vector<RuntimeRegistrationSPtr> registrations;
    auto error = this->runtimeRegistrationTable_->Find(servicePackageInstanceId, registrations);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto iter = registrations.begin(); iter != registrations.end(); ++iter)
    {
        wstring runtimeId = (*iter)->RuntimeContext.RuntimeId;
        if (runtimeIds.find(runtimeId) == runtimeIds.end())
        {
            runtimeIds.insert(runtimeId);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FabricRuntimeManager::GetRuntimeIds(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    __out CaseInsensitiveStringSet & runtimeIds)
{
    vector<RuntimeRegistrationSPtr> registrations;
    auto error = this->runtimeRegistrationTable_->Find(codePackageInstanceId, registrations);
    if (!error.IsSuccess())
    {
        return error;
    }

    for (auto iter = registrations.begin(); iter != registrations.end(); ++iter)
    {
        wstring runtimeId = (*iter)->RuntimeContext.RuntimeId;
        if (runtimeIds.find(runtimeId) == runtimeIds.end())
        {
            runtimeIds.insert(runtimeId);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode FabricRuntimeManager::DownloadAndActivate(
    uint64 const sequenceNumber,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackagePublicActivationId,
    wstring const & applicationName)
{
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Begin(BeginDownloadAndActivate): VersionedServiceTypeId={0}, ActivationContext={1}, ServicePackagePublicActivationId={2}, SequenceNumber={3}",
        versionedServiceTypeId,
        activationContext,
        servicePackagePublicActivationId,
        sequenceNumber);

    auto operation = hosting_.ApplicationManagerObj->BeginDownloadAndActivate(
        sequenceNumber,
        versionedServiceTypeId,
        activationContext,
        servicePackagePublicActivationId,
        applicationName,
        [this, sequenceNumber, versionedServiceTypeId, activationContext, servicePackagePublicActivationId](AsyncOperationSPtr const & operation)
        {
            this->OnDownloadAndActivateCompleted(
                operation, 
                sequenceNumber, 
                versionedServiceTypeId, 
                activationContext, 
                servicePackagePublicActivationId);
        },
        Root.CreateAsyncOperationRoot());

    if (operation->CompletedSynchronously)
    {
        return FinishDownloadAndActivate(
            operation, 
            sequenceNumber, 
            versionedServiceTypeId, 
            activationContext, 
            servicePackagePublicActivationId);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::HostingDeploymentInProgress);
    }
}

void FabricRuntimeManager::OnDownloadAndActivateCompleted(
    AsyncOperationSPtr const & operation,
    uint64 const sequenceNumber,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackagePublicActivationId)
{
    if (!operation->CompletedSynchronously)
    {
        FinishDownloadAndActivate(
            operation, 
            sequenceNumber, 
            versionedServiceTypeId, 
            activationContext,
            servicePackagePublicActivationId).ReadValue();
    }
}

ErrorCode FabricRuntimeManager::FinishDownloadAndActivate(
    AsyncOperationSPtr const & operation,
    uint64 const sequenceNumber,
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackagePublicActivationId)
{
    auto error = hosting_.ApplicationManagerObj->EndDownloadAndActivate(operation);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "End(BeginDownloadAndActivate): Error={0}, VersionedServiceTypeId={1}, ActivationContext={2}, ServicePackagePublicActivationId={3}, SequenceNumber={4}",
        error,
        versionedServiceTypeId,
        activationContext,
        servicePackagePublicActivationId,
        sequenceNumber);
    return error;
}

ErrorCode FabricRuntimeManager::EnsureReturnedServiceTypeVersion(
    ServicePackageVersionInstance const & requestedVersion,
    ServiceTypeRegistrationSPtr const & serviceTypeRegistration)
{
    if (requestedVersion != serviceTypeRegistration->servicePackageVersionInstance)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "EnsureReturnedServiceTypeVersion: ServiceTypeRegistration {0} found with a different version than requested version {1}.",
            *serviceTypeRegistration,
            requestedVersion);

        WriteNoise(
            TraceType,
            Root.TraceId,
            "EnsureReturnedServiceTypeVersion: Deactivating ServicePackage Id={0},Version={1},InstanceId={2}, ActivationContext={3} if it is not used.",
            serviceTypeRegistration->ServiceTypeId.ServicePackageId,
            serviceTypeRegistration->servicePackageVersionInstance,
            serviceTypeRegistration->ServicePackageInstanceId,
            serviceTypeRegistration->ActivationContext);

        bool hasReplica = false;
        ServicePackageInstanceIdentifier servicePackageIntanceId(
            serviceTypeRegistration->ServiceTypeId.ServicePackageId,
            serviceTypeRegistration->ActivationContext,
            serviceTypeRegistration->ServicePackagePublicActivationId);

        this->hosting_.ApplicationManagerObj->ScheduleForDeactivationIfNotUsed(servicePackageIntanceId, hasReplica);

        if (hasReplica)
        {
            // Replicas were found
            return ErrorCode(ErrorCodeValue::HostingServiceTypeRegistrationVersionMismatch);
        }
        else
        {
            // No replicas were found and the ServicePackage has been scheduled for deactivation or is already deactivated.
            return ErrorCode(ErrorCodeValue::NotFound);
        }

    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode FabricRuntimeManager::CreateServiceTypeRegistration(
    wstring const & runtimeId,
    ServiceTypeIdentifier const & serviceTypeId,
    ServicePackageActivationContext const & activationContext,
    wstring const & servicePackagePublicActivationId,
    __out ServiceTypeRegistrationSPtr & serviceTypeRegistration)
{
    RuntimeRegistrationSPtr runtimeRegistration;
    auto error = runtimeRegistrationTable_->Find(runtimeId, runtimeRegistration);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "RuntimeRegistrationTable.Find({0}) completed with ErrorCode={1}",
        runtimeId,
        error);

    if (error.IsSuccess())
    {
        serviceTypeRegistration = Hosting2::CreateServiceTypeRegistration(
            serviceTypeId,
            activationContext,
            servicePackagePublicActivationId,
            runtimeRegistration->RuntimeContext);
    }

    return error;
}

void FabricRuntimeManager::RegisterIpcRequestHandler()
{
    auto root = Root.CreateComponentRoot();
    hosting_.IpcServerObj.RegisterMessageHandler(
        Actor::FabricRuntimeManager,
        [this, root](MessageUPtr & message, IpcReceiverContextUPtr & context) { this->ProcessIpcMessage(*message, context); },
        true/*dispatchOnTransportThread*/);
}

void FabricRuntimeManager::UnregisterIpcRequestHandler()
{
    hosting_.IpcServerObj.UnregisterMessageHandler(Actor::FabricRuntimeManager);
}

void FabricRuntimeManager::ProcessIpcMessage(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    wstring const & action = message.Action;
    if (action == Hosting2::Protocol::Actions::RegisterServiceTypeRequest)
    {
        this->ProcessRegisterServiceTypeRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::RegisterFabricRuntimeRequest)
    {
        this->ProcessRegisterFabricRuntimeRequest(message, context);
    }
    else if (action == Hosting2::Protocol::Actions::UnregisterFabricRuntimeRequest)
    {
        this->ProcessUnregisterFabricRuntimeRequest(message, context);
    }
    else
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "Dropping unsupported message: {0}",
            message);
    }
}

void FabricRuntimeManager::ProcessRegisterFabricRuntimeRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    RegisterFabricRuntimeRequest requestBody;
    if (!message.GetBody<RegisterFabricRuntimeRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<RegisterFabricRuntimeRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing RegisterFabricRuntimeRequest: {0}",
        requestBody);

    // create a tenative entry in runtime registration table
    auto error = this->AddRuntimeRegistrationEntry(requestBody.RuntimeContext);
    if (!error.IsSuccess())
    {
        this->SendRegisterFabricRuntimeReply(requestBody, error, context);
        return;
    }

    // verify that the host is still valid in the host registration table
    error = this->EnsureValidApplicationHostRegistration(requestBody.RuntimeContext.HostContext.HostId);
    if (!error.IsSuccess())
    {
        this->CleanRuntimeRegistrationEntry(requestBody.RuntimeContext.RuntimeId);
        this->SendRegisterFabricRuntimeReply(requestBody, error, context);
        return;
    }

    // validate the entry, if exists. If the entry does not exist, the host went down 
    // in the meanwhile, so fail this registration
    RuntimeRegistrationSPtr registration;
    error = this->ValidateRuntimeRegistrationEntry(requestBody.RuntimeContext.RuntimeId, registration);
    if (!error.IsSuccess())
    {
        this->CleanRuntimeRegistrationEntry(requestBody.RuntimeContext.RuntimeId);
        this->SendRegisterFabricRuntimeReply(requestBody, error, context);
        return;
    }

    this->SendRegisterFabricRuntimeReply(requestBody, ErrorCode(ErrorCodeValue::Success), context);
}


void FabricRuntimeManager::SendRegisterFabricRuntimeReply(
    RegisterFabricRuntimeRequest const & requestBody,
    ErrorCode const error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    RegisterFabricRuntimeReply replybody(error);
    MessageUPtr reply = make_unique<Message>(replybody);
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent RegisterFabricRuntimeReply: RequestBody={0}, ReplyBody={1}",
        requestBody,
        replybody);
}

ErrorCode FabricRuntimeManager::AddRuntimeRegistrationEntry(FabricRuntimeContext const & runtimeContext)
{
    auto error = this->runtimeRegistrationTable_->Add(runtimeContext);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "AddRuntimeRegistrationEntry: ErrorCode={0}, RuntimeContext={1}",
        error,
        runtimeContext);

    return error;
}

ErrorCode FabricRuntimeManager::EnsureValidApplicationHostRegistration(wstring const & hostId)
{
    bool isHostRegistered = false;
    auto error = hosting_.ApplicationHostManagerObj->IsRegistered(hostId, isHostRegistered);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "Application Host Registration Status Check: ErrorCode={0}, HostId={1}, IsRegistered={2}",
        error,
        hostId,
        isHostRegistered);
    if (!error.IsSuccess()) { return error; }

    if (!isHostRegistered) { return ErrorCode(ErrorCodeValue::HostingApplicationHostNotRegistered); }

    return ErrorCode(ErrorCodeValue::Success);
}

void FabricRuntimeManager::CleanRuntimeRegistrationEntry(wstring const & runtimeId)
{
    RuntimeRegistrationSPtr registration;
    auto error = this->runtimeRegistrationTable_->Remove(runtimeId, registration);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "CleanRuntimeRegistrationEntry: ErrorCode={0}, RuntimeId={1}",
        error,
        runtimeId);
    error.ReadValue();
}

ErrorCode FabricRuntimeManager::ValidateRuntimeRegistrationEntry(
    wstring const & runtimeId,
    __out RuntimeRegistrationSPtr & registration)
{
    auto error = this->runtimeRegistrationTable_->Validate(runtimeId, registration);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "ValidateRuntimeRegistrationEntry: ErrorCode={0}, RuntimeId={1}",
        error,
        runtimeId);
    return error;
}

void FabricRuntimeManager::ProcessUnregisterFabricRuntimeRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    UnregisterFabricRuntimeRequest requestBody;
    if (!message.GetBody<UnregisterFabricRuntimeRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<UnregisterFabricRuntimeRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing UnregisterFabricRuntimeRequest: {0}",
        requestBody);

    RuntimeRegistrationSPtr registration;
    auto error = this->RemoveRuntimeRegistrationEntry(requestBody.Id, registration);
    this->SendUnregisterFabricRuntimeReply(requestBody, error, context);
    if (error.IsSuccess())
    {
        this->OnRuntimeUnregistered(registration);
    }
}

ErrorCode FabricRuntimeManager::RemoveRuntimeRegistrationEntry(
    wstring const & runtimeId,
    __out RuntimeRegistrationSPtr & registration)
{
    // remove entry from the runtime registration table
    auto error = this->runtimeRegistrationTable_->Remove(runtimeId, registration);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "RemoveRuntimeRegistrationEntry: ErrorCode={0}, RuntimeId={1}",
        error,
        runtimeId);
    return error;
}

void FabricRuntimeManager::SendUnregisterFabricRuntimeReply(
    UnregisterFabricRuntimeRequest const & requestBody,
    ErrorCode const error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    UnregisterFabricRuntimeReply replybody(error);
    MessageUPtr reply = make_unique<Message>(replybody);
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent UnregisterFabricRuntimeReply: RequestBody={0}, ReplyBody={1}",
        requestBody,
        replybody);
}

void FabricRuntimeManager::ProcessRegisterServiceTypeRequest(
    __in Transport::Message & message,
    __in Transport::IpcReceiverContextUPtr & context)
{
    RegisterServiceTypeRequest requestBody;
    if (!message.GetBody<RegisterServiceTypeRequest>(requestBody))
    {
        auto error = ErrorCode::FromNtStatus(message.Status);
        WriteWarning(
            TraceType,
            Root.TraceId,
            "GetBody<RegisterServiceTypeRequest> failed: Message={0}, ErrorCode={1}",
            message,
            error);
        return;
    }

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Processing RegisterServiceTypeRequest: {0}",
        requestBody);

    // create tentative entry in service type registration table
    auto error = this->AddServiceTypeRegistrationEntry(
        requestBody.ServiceTypeInstanceId,
        requestBody.RuntimeContext);
    if (!error.IsSuccess())
    {
        this->SendRegisterServiceTypeReply(requestBody, error, context);
        return;
    }

    // ensure that a valid runtime entry is present in the runtime table
    error = this->EnsureValidRuntimeRegistration(requestBody.RuntimeContext);
    if (!error.IsSuccess())
    {
        this->CleanServiceTypeRegistrationEntry(requestBody.ServiceTypeInstanceId, requestBody.RuntimeContext);
        this->SendRegisterServiceTypeReply(requestBody, error, context);
        return;
    }

    // ensure that a valid host entry in present in the host table
    error = this->EnsureValidApplicationHostRegistration(requestBody.RuntimeContext.HostContext.HostId);
    if (!error.IsSuccess())
    {
        this->CleanServiceTypeRegistrationEntry(requestBody.ServiceTypeInstanceId, requestBody.RuntimeContext);
        this->SendRegisterServiceTypeReply(requestBody, error, context);
        return;
    }

    // validate the entry in the service type registration table
    error = this->ValidateServiceTypeRegistrationEntry(requestBody.ServiceTypeInstanceId, requestBody.RuntimeContext);
    this->SendRegisterServiceTypeReply(requestBody, error, context);
}

void FabricRuntimeManager::SendRegisterServiceTypeReply(
    RegisterServiceTypeRequest const & requestBody,
    ErrorCode const error,
    __in Transport::IpcReceiverContextUPtr & context)
{
    RegisterServiceTypeReply replybody(error);
    MessageUPtr reply = make_unique<Message>(replybody);
    reply->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::RegisterServiceTypeReply));
    context->Reply(move(reply));
    WriteNoise(
        TraceType,
        Root.TraceId,
        "Sent RegisterServiceTypeReply: RequestBody={0}, ReplyBody={1}",
        requestBody,
        replybody);
}

ErrorCode FabricRuntimeManager::AddServiceTypeRegistrationEntry(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
    FabricRuntimeContext const & runtimeContext)
{
    CODING_ERROR_ASSERT(serviceTypeInstanceId.ActivationContext == runtimeContext.ServicePackageActivationId.ActivationContext);

    auto error = this->serviceTypeStateManager_->StartRegister(
        serviceTypeInstanceId, 
        runtimeContext.CodeContext.ApplicationName, 
        runtimeContext.RuntimeId, 
        runtimeContext.CodeContext.CodePackageInstanceId.CodePackageName,
        runtimeContext.HostContext.HostId);
    
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "AddServiceTypeRegistrationEntry: ErrorCode={0}, ServiceTypeInstanceId={1}, RuntimeContext={2}",
        error,
        serviceTypeInstanceId,
        runtimeContext);

    return error;
}

ErrorCode FabricRuntimeManager::EnsureValidRuntimeRegistration(
    FabricRuntimeContext const & runtimeContext)
{
    RuntimeRegistrationSPtr registration;
    auto error = this->runtimeRegistrationTable_->Find(runtimeContext.RuntimeId, registration);
    if (error.IsSuccess())
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "EnsureValidRuntimeRegistration: ErrorCode={0}, RuntimeId={1}, RuntimeRegistration={2}",
            error,
            runtimeContext.RuntimeId,
            *registration);

        if (!StringUtility::AreEqualCaseInsensitive(registration->RuntimeContext.RuntimeId, runtimeContext.RuntimeId) ||
            !StringUtility::AreEqualCaseInsensitive(registration->RuntimeContext.HostContext.HostId, runtimeContext.HostContext.HostId) ||
            registration->RuntimeContext.ServicePackageActivationId.ActivationContext != runtimeContext.ServicePackageActivationId.ActivationContext)
        {
            WriteInfo(
                TraceType,
                Root.TraceId,
                "EnsureValidRuntimeRegistration failed because context did not match. Actual {0}, Expected {1}",
                registration->RuntimeContext,
                runtimeContext);

            error = ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
        }
    }
    else
    {
        WriteWarning(
            TraceType,
            Root.TraceId,
            "EnsureValidRuntimeRegistration: ErrorCode={0}, RuntimeId={1}",
            error,
            runtimeContext.RuntimeId);
    }

    return error;
}

void FabricRuntimeManager::CleanServiceTypeRegistrationEntry(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
    FabricRuntimeContext const & runtimeContext)
{
    auto error = this->serviceTypeStateManager_->FinishRegister(serviceTypeInstanceId, false, runtimeContext);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "CleanServiceTypeRegistrationEntry: ErrorCode={0}, ServiceTypeInstanceId={1}",
        error,
        serviceTypeInstanceId);

    error.ReadValue();
}

ErrorCode FabricRuntimeManager::ValidateServiceTypeRegistrationEntry(
    ServiceTypeInstanceIdentifier const & serviceTypeInstanceId,
    FabricRuntimeContext const & runtimeContext)
{
    CODING_ERROR_ASSERT(serviceTypeInstanceId.ActivationContext == runtimeContext.ServicePackageActivationId.ActivationContext);

    auto error = this->serviceTypeStateManager_->FinishRegister(serviceTypeInstanceId, true, runtimeContext);
    WriteTrace(
        error.ToLogLevel(),
        TraceType,
        Root.TraceId,
        "ValidateServiceTypeRegistrationEntry: ErrorCode={0}, ServiceTypeId={1}",
        error,
        serviceTypeInstanceId);

    return error;
}

// this method is called when an application host is unregistered
void FabricRuntimeManager::OnApplicationHostUnregistered(ActivityDescription const & activityDescription, wstring const & hostId)
{
    // remove runtime registrations associated with the unregistered host
    vector<RuntimeRegistrationSPtr> runtimeRegistrationRemoved;

    auto error = runtimeRegistrationTable_->RemoveRegistrationsFromHost(
        hostId,
        runtimeRegistrationRemoved);

    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnApplicationHostUnregistered: RemoveRuntimeRegistrationsFromHost: ErrorCode={0}, HostId={1}, Removed-Size={2} ActivityDescription={3}",
        error,
        hostId,
        runtimeRegistrationRemoved.size(),
        activityDescription);
    error.ReadValue();

    serviceTypeStateManager_->OnApplicationHostClosed(activityDescription, hostId);
}

// this method is called when runtime is unregistered successfully
void FabricRuntimeManager::OnRuntimeUnregistered(RuntimeRegistrationSPtr const & runtimeRegistration)
{
    serviceTypeStateManager_->OnRuntimeClosed(
        runtimeRegistration->RuntimeContext.RuntimeId,
        runtimeRegistration->RuntimeContext.HostContext.HostId);
}

// this method is called when a service package is closed
void FabricRuntimeManager::OnServicePackageClosed(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    int64 const instanceId)
{
    // remove runtime registrations from this service package
    vector<RuntimeRegistrationSPtr> runtimeRegistrationsRemoved;

    auto error = runtimeRegistrationTable_->RemoveRegistrationsFromServicePackage(
        servicePackageInstanceId,
        instanceId,
        runtimeRegistrationsRemoved);
    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnServicePackageClosed: RemoveRegistrationsFromServicePackage: ErrorCode={0}, ServicePackageInstanceId={1}, InstanceId={2}, Removed-Size={3}",
        error,
        servicePackageInstanceId,
        instanceId,
        runtimeRegistrationsRemoved.size());

    if (!error.IsSuccess()) { return; }

    // remove registrations from the runtime
    for (auto iter = runtimeRegistrationsRemoved.cbegin(); iter != runtimeRegistrationsRemoved.cend(); ++iter)
    {
        OnRuntimeUnregistered(*iter);
    }
}

// this method is called when a code package is closed
void FabricRuntimeManager::OnCodePackageClosed(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    int64 const instanceId)
{
    // remove runtime registrations from this service package
    vector<RuntimeRegistrationSPtr> runtimeRegistrationsRemoved;

    auto error = runtimeRegistrationTable_->RemoveRegistrationsFromCodePackage(
        codePackageInstanceId,
        instanceId,
        runtimeRegistrationsRemoved);
    WriteNoise(
        TraceType,
        Root.TraceId,
        "OnCodePackageClosed: RemoveRegistrationsFromCodePackage: ErrorCode={0}, CodePackageInstanceId={1}, InstanceId={2}, Removed-Size={3}",
        error,
        codePackageInstanceId,
        instanceId,
        runtimeRegistrationsRemoved.size());

    if (!error.IsSuccess()) { return; }

    // remove registrations from the runtime
    for (auto iter = runtimeRegistrationsRemoved.cbegin(); iter != runtimeRegistrationsRemoved.cend(); ++iter)
    {
        OnRuntimeUnregistered(*iter);
    }
}

ErrorCode FabricRuntimeManager::UpdateRegistrationsFromCodePackage(CodePackageContext const & existingContext, CodePackageContext const & updatedContext)
{
	return this->runtimeRegistrationTable_->UpdateRegistrationsFromCodePackage(existingContext, updatedContext);
}
