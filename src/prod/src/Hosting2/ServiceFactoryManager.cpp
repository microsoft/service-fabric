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

StringLiteral const TraceType("ServiceFactoryManager");

// ********************************************************************************************************************
// ServiceFactoryManager::RegisterServiceFactoryAsyncOperation Implementation
//

class ServiceFactoryManager::RegisterServiceFactoryAsyncOperation : 
    public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(RegisterServiceFactoryAsyncOperation)

public:
    RegisterServiceFactoryAsyncOperation(
        ServiceFactoryManager & owner,
        ServiceFactoryRegistrationSPtr const & registration,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        registration_(registration),
        timeoutHelper_(timeout)
    {
    }

    virtual ~RegisterServiceFactoryAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<RegisterServiceFactoryAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType,
            owner_.runtime_.TraceId,
            "Registering ServiceFactory: Registration={0}", 
            *registration_);

        // create local registration entry
        auto error = owner_.table_->Add(registration_);
        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.runtime_.TraceId,
            "Local Registration: ErrorCode={0}, Registration={1}",
            error,
            *registration_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // register service type on fabric node
        RegisterServiceType(thisSPtr);
    }

private:
    void RegisterServiceType(AsyncOperationSPtr const & thisSPtr)
    {
        TimeSpan timeout = HostingConfig::GetConfig().RequestTimeout;
        if (timeout > timeoutHelper_.GetRemainingTime())
        {
            timeout = timeoutHelper_.GetRemainingTime();
        }
        
        MessageUPtr request = CreateRegisterServiceTypeRequestMessage();
        WriteInfo(
            TraceType, 
            owner_.runtime_.TraceId, 
            "Begin(RegisterServiceTypeRequest): Timeout={0}, Request={1}", 
            timeout,
            *request);
        auto registerServiceTypeRequest = owner_.runtime_.Host.Client.BeginRequest(
            move(request),
            timeout,
            [this](AsyncOperationSPtr const & operation){ OnRegisterServiceTypeReplyReceived(operation); },
            thisSPtr);
        if (registerServiceTypeRequest->CompletedSynchronously)
        {
            FinishRegisterServiceType(registerServiceTypeRequest);
        }
    }

    void OnRegisterServiceTypeReplyReceived(AsyncOperationSPtr const & registerServiceTypeRequest)
    {
        if (!registerServiceTypeRequest->CompletedSynchronously)
        {
            FinishRegisterServiceType(registerServiceTypeRequest);
        }
    }

    void FinishRegisterServiceType(AsyncOperationSPtr const & registerServiceTypeRequest)
    {
        MessageUPtr reply;
        auto error = owner_.runtime_.Host.Client.EndRequest(registerServiceTypeRequest, reply);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType, 
            owner_.runtime_.TraceId, 
            "End(RegisterServiceTypeRequest): ErrorCode={0}", 
            error);
        if (!error.IsSuccess())
        {            
            RetryServiceTypeRegistration(registerServiceTypeRequest->Parent, error);
            return;
        }

        // process reply
        RegisterServiceTypeReply replyBody;
        if (!reply->GetBody<RegisterServiceTypeReply>(replyBody))
        {
            error = ErrorCode::FromNtStatus(reply->Status);
            WriteWarning(
                TraceType,
                owner_.runtime_.TraceId,
                "GetBody<RegisterServiceTypeReply> failed: Message={0}, ErrorCode={1}",
                *reply,
                error);
            CleanupAndComplete(registerServiceTypeRequest->Parent, error);
            return;
        }

        WriteTrace(
            error.ToLogLevel(),
            TraceType,
            owner_.runtime_.TraceId,
            "RegisterServiceTypeReply: Message={0}, Body={1}",
            *reply,
            replyBody);
        error = replyBody.Error;
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::HostingServiceTypeAlreadyRegisteredToSameRuntime))
        {
            CleanupAndComplete(registerServiceTypeRequest->Parent, error);
            return;
        }

        // validate local entry
        ValidateLocalRegistration(registerServiceTypeRequest->Parent);
    }

    void ValidateLocalRegistration(AsyncOperationSPtr const & thisSPtr)
    {
        auto error = owner_.table_->Validate(registration_->ServiceTypeId);
        WriteTrace(
            error.IsSuccess() ? LogLevel::Info : LogLevel::Warning,
            TraceType,
            owner_.runtime_.TraceId,
            "Validate Local Registration: ErrorCode={0}, Registration={1}",
            error,
            *registration_);

        TryComplete(thisSPtr, error);
    }
    
    void RetryServiceTypeRegistration(AsyncOperationSPtr const & thisSPtr, ErrorCode ipcError)
    {
        auto timeout = timeoutHelper_.GetRemainingTime();
        if ((timeoutHelper_.GetRemainingTime() > TimeSpan::Zero) && ipcError.IsError(ErrorCodeValue::Timeout))
        {
            // retry service type registration
            RegisterServiceType(thisSPtr);
            return;
        }
        else
        {
            CleanupAndComplete(thisSPtr, ipcError);
            return;
        }
    }

    void CleanupAndComplete(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
    {
        auto removeError = owner_.table_->Remove(registration_->ServiceTypeId, true);
        removeError.ReadValue();

        TryComplete(thisSPtr, error);
        return;
    }

    MessageUPtr CreateRegisterServiceTypeRequestMessage()
    {
		auto runtimeContext = owner_.runtime_.GetRuntimeContext();

		ServiceTypeInstanceIdentifier svcTypeInstanceId(
			registration_->ServiceTypeId,
			runtimeContext.CodeContext.CodePackageInstanceId.ServicePackageInstanceId.ActivationContext,
			runtimeContext.CodeContext.CodePackageInstanceId.ServicePackageInstanceId.PublicActivationId);

        RegisterServiceTypeRequest requestBody(runtimeContext, svcTypeInstanceId);
        
        MessageUPtr request = make_unique<Message>(requestBody);
        request->Headers.Add(Transport::ActorHeader(Actor::FabricRuntimeManager));
        request->Headers.Add(Transport::ActionHeader(Hosting2::Protocol::Actions::RegisterServiceTypeRequest));

        WriteNoise(
            TraceType, 
            owner_.runtime_.TraceId, 
            "CreateRegisterServiceTypeRequestMessage: Message={0}, Body={1}", 
            *request, 
            requestBody);

        return move(request);
    }

private:
    ServiceFactoryManager & owner_;
    ServiceFactoryRegistrationSPtr registration_;
    TimeoutHelper timeoutHelper_;
};

// ********************************************************************************************************************
// ServiceFactoryManager Implementation
//
ServiceFactoryManager::ServiceFactoryManager(
    ComponentRoot & root,
    FabricRuntimeImpl & runtime)
    : RootedObject(root),
    runtime_(runtime),
    table_()
{
    table_ = make_unique<ServiceFactoryRegistrationTable>();
}

AsyncOperationSPtr ServiceFactoryManager::BeginRegisterStatelessServiceFactory(
    wstring const & originalServiceTypeName, 
    ComPointer<IFabricStatelessServiceFactory> const & statelessFactory,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ServiceFactoryRegistrationSPtr registration = make_shared<ServiceFactoryRegistration>(
        originalServiceTypeName,
        GetMappedServiceType(originalServiceTypeName),
        make_shared<ComProxyStatelessServiceFactory>(statelessFactory));

    return AsyncOperation::CreateAndStart<RegisterServiceFactoryAsyncOperation>(
            *this,
            registration,
            timeout,
            callback,
            parent);
}

ErrorCode ServiceFactoryManager::EndRegisterStatelessServiceFactory(
    AsyncOperationSPtr const & asyncOperation)
{
    return RegisterServiceFactoryAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ServiceFactoryManager::BeginRegisterStatefulServiceFactory(
    wstring const & originalServiceTypeName, 
    ComPointer<IFabricStatefulServiceFactory> const & statefulFactory,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
   ServiceFactoryRegistrationSPtr registration = make_shared<ServiceFactoryRegistration>(
        originalServiceTypeName,
        GetMappedServiceType(originalServiceTypeName),
        make_shared<ComProxyStatefulServiceFactory>(statefulFactory));

    return AsyncOperation::CreateAndStart<RegisterServiceFactoryAsyncOperation>(
            *this,
            registration,
            timeout,
            callback,
            parent);
}

ErrorCode ServiceFactoryManager::EndRegisterStatefulServiceFactory(
    AsyncOperationSPtr const & asyncOperation)
{
    return RegisterServiceFactoryAsyncOperation::End(asyncOperation);
}

AsyncOperationSPtr ServiceFactoryManager::BeginGetStatelessServiceFactory(
    ServiceTypeIdentifier const & serviceTypeId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return table_->BeginFind(serviceTypeId, callback, parent);
}

ErrorCode ServiceFactoryManager::EndGetStatelessServiceFactory(
    AsyncOperationSPtr const & operation,
    __out wstring & originalServiceTypeName,
    __out ComProxyStatelessServiceFactorySPtr & factory)
{
    ServiceFactoryRegistrationSPtr registration;
    ServiceTypeIdentifier serviceTypeId;
    auto error = table_->EndFind(operation, registration, serviceTypeId);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            runtime_.TraceId,
            "EndGetStatelessServiceFactory failed to find factory registration: ErrorCode={0}, ServiceTypeInstanceId={1}",
            error,
            serviceTypeId);
        return error;
    }

    if (!registration->StatelessFactory)
    {
        WriteWarning(
            TraceType,
            runtime_.TraceId,
            "EndGetStatelessServiceFactory found an entry but it does not have valid stateless service factory: ServiceTypeInstanceId={0}",
            serviceTypeId);
        return ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
    }

    originalServiceTypeName = registration->OriginalServiceTypeName;
    factory = registration->StatelessFactory;

    return ErrorCode(ErrorCodeValue::Success);
}

AsyncOperationSPtr ServiceFactoryManager::BeginGetStatefulServiceFactory(
    ServiceTypeIdentifier const & serviceTypeId,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return table_->BeginFind(serviceTypeId, callback, parent);
}

ErrorCode ServiceFactoryManager::EndGetStatefulServiceFactory(
    AsyncOperationSPtr const & operation,
    __out wstring & originalServiceTypeName,
    __out ComProxyStatefulServiceFactorySPtr & factory)
{
    ServiceFactoryRegistrationSPtr registration;
    ServiceTypeIdentifier serviceTypeId;
    auto error = table_->EndFind(operation, registration, serviceTypeId);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            runtime_.TraceId,
            "EndGetStatefulServiceFactory failed to find factory registration: ErrorCode={0}, ServiceTypeInstanceId={1}",
            error,
            serviceTypeId);
        return error;
    }

    if (!registration->StatefulFactory)
    {
        WriteWarning(
            TraceType,
            runtime_.TraceId,
            "EndGetStatefulServiceFactory found an entry but it does not have valid stateful service factory: ServiceTypeInstanceId={0}",
            serviceTypeId);
        return ErrorCode(ErrorCodeValue::HostingServiceTypeNotRegistered);
    }

    originalServiceTypeName = registration->OriginalServiceTypeName;
    factory = registration->StatefulFactory;

    return ErrorCode(ErrorCodeValue::Success);
}

ServiceTypeIdentifier ServiceFactoryManager::GetMappedServiceType(wstring const & originalServiceTypeName)
{
    return ServiceTypeIdentifier(
        runtime_.GetRuntimeContext().CodeContext.CodePackageInstanceId.ServicePackageInstanceId.ServicePackageId, 
		originalServiceTypeName);
}

bool ServiceFactoryManager::Test_IsServiceTypeInvalid(ServiceTypeIdentifier const & serviceTypeId) const
{
    return table_->Test_IsServiceTypeInvalid(serviceTypeId);
}
