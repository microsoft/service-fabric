// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace Hosting2;
using namespace ServiceModel;

namespace
{
    // Pick an error that is unlikely to be returned from hosting
    // This way any End*** function can determine whether the async op was created
    // by the test hosting subsystem fault injection
    ErrorCodeValue::Enum const DefaultError = ErrorCodeValue::GlobalLeaseLost;
    wstring const DefaultTag = L"hosting_fault_injected_error";
    StringLiteral const TraceType("TestHosting");

    // For now assume that there is only one application and fault injection does no 
    // application name based filtering
    wstring const DefaultName = L"*";

    // Supports cancellation
    class BlockingAsyncOperation : public Common::AsyncOperation
    {
    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {
            waitEvent_.Reset();

            Threadpool::Post([thisSPtr, this]
            {
                TestSession::WriteInfo(TraceType, "Waiting for cancel");
                if (!waitEvent_.WaitOne(TimeSpan::FromMinutes(0.5)))
                {
                    Assert::CodingError("Cancel not received");
                }

                TryComplete(thisSPtr, ErrorCodeValue::Success);
            });
        }

        void OnCancel()
        {
            TestSession::WriteInfo(TraceType, "Cancel");
            waitEvent_.Set();
        }

    private:
        ManualResetEvent waitEvent_;
    };
}

IHostingSubsystemSPtr TestHostingSubsystem::Create(HostingSubsystemConstructorParameters & parameters)
{
    return IHostingSubsystemSPtr(new TestHostingSubsystem(parameters));
}

// Initialize the real hosting subsystem using the standard factory provided by hosting
TestHostingSubsystem::TestHostingSubsystem(
    HostingSubsystemConstructorParameters & parameters)
    : hosting_(HostingSubsystemFactory(parameters)),
    nodeId_(parameters.Federation->Id)
{
}

AsyncOperationSPtr TestHostingSubsystem::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & operation)
{
    return hosting_->BeginOpen(timeout, callback, operation);
}

ErrorCode TestHostingSubsystem::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return hosting_->EndOpen(operation);
}

AsyncOperationSPtr TestHostingSubsystem::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & operation)
{
    return hosting_->BeginClose(timeout, callback, operation);
}

ErrorCode TestHostingSubsystem::OnEndClose(AsyncOperationSPtr const & operation)
{
    return hosting_->EndClose(operation);
}

void TestHostingSubsystem::OnAbort()
{
    return hosting_->Abort();
}

IFabricActivatorClientSPtr const & TestHostingSubsystem::get_FabricActivatorClient() const
{
    return hosting_->FabricActivatorClientObj;
}

wstring const  TestHostingSubsystem::get_NodeName() const
{
    return hosting_->NodeName;
}

ErrorCode TestHostingSubsystem::FindServiceTypeRegistration(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    Reliability::ServiceDescription const & serviceDescription,
	ServiceModel::ServicePackageActivationContext const &isolationContext,
    __out uint64 & sequenceNumber,
    __out ServiceTypeRegistrationSPtr & registration)
{
    return WrapSyncApiWithFaultInjection(
        DefaultName,
        ApiFaultHelper::FindServiceTypeRegistration,
        [&]() {return hosting_->FindServiceTypeRegistration(versionedServiceTypeId, serviceDescription, isolationContext, sequenceNumber, registration); });
}

ErrorCode TestHostingSubsystem::GetHostId(
    ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    std::wstring const & applicationName,
    __out std::wstring & hostId)
{
    return WrapSyncApiWithFaultInjection(
        DefaultName,
        ApiFaultHelper::GetHostId,
        [&]() {return hosting_->GetHostId(versionedServiceTypeId, applicationName, hostId); });
}

ErrorCode TestHostingSubsystem::GetHostId(
    ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    std::wstring const & applicationName,
    ServiceModel::ServicePackageActivationContext const & activationContext,
    __out std::wstring & hostId)
{
    return WrapSyncApiWithFaultInjection(
        DefaultName,
        ApiFaultHelper::GetHostId,
        [&]() {return hosting_->GetHostId(versionedServiceTypeId, applicationName, activationContext, hostId); });
}

AsyncOperationSPtr TestHostingSubsystem::BeginDownloadApplication(
    ApplicationDownloadSpecification const & appDownloadSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return WrapBeginWithFaultInjection(
        DefaultName,
        ApiFaultHelper::DownloadApplication,
        callback,
        parent,
        [&]() { return hosting_->BeginDownloadApplication(appDownloadSpec, callback, parent); });
}

ErrorCode TestHostingSubsystem::EndDownloadApplication(
    AsyncOperationSPtr const & operation)
{
    return WrapEndWithFaultInjection(
        operation,
        [&]() { return hosting_->EndDownloadApplication(operation); });
}

ErrorCode TestHostingSubsystem::AnalyzeApplicationUpgrade(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    __out AffectedRuntimesSet & affectedRuntimeIds)
{
    return WrapSyncApiWithFaultInjection(
        DefaultName,
        ApiFaultHelper::AnalyzeApplicationUpgrade,
        [&]() { return hosting_->AnalyzeApplicationUpgrade(appUpgradeSpec, affectedRuntimeIds); });
}

AsyncOperationSPtr TestHostingSubsystem::BeginUpgradeApplication(
    ApplicationUpgradeSpecification const & appUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return WrapBeginWithFaultInjection(
        DefaultName,
        ApiFaultHelper::UpgradeApplication,
        callback,
        parent,
        [&]() { return hosting_->BeginUpgradeApplication(appUpgradeSpec, callback, parent); });
}

ErrorCode TestHostingSubsystem::EndUpgradeApplication(
    AsyncOperationSPtr const & operation)
{
    return WrapEndWithFaultInjection(
        operation,
        [&]() { return hosting_->EndUpgradeApplication(operation); });
}

AsyncOperationSPtr TestHostingSubsystem::BeginDownloadFabric(
    FabricVersion const & fabricVersion,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return WrapBeginWithFaultInjection(
        DefaultName,
        ApiFaultHelper::DownloadFabric,
        callback,
        parent,
        [&]() { return hosting_->BeginDownloadFabric(fabricVersion, callback, parent); });
}

ErrorCode TestHostingSubsystem::EndDownloadFabric(
    AsyncOperationSPtr const & operation)
{
    return WrapEndWithFaultInjection(
        operation,
        [&]() { return hosting_->EndDownloadFabric(operation); });
}

AsyncOperationSPtr TestHostingSubsystem::BeginValidateFabricUpgrade(
    FabricUpgradeSpecification const & fabricUpgradeSpec,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return WrapBeginWithFaultInjection(
        DefaultName,
        ApiFaultHelper::ValidateFabricUpgrade,
        callback,
        parent,
        [&]() { return hosting_->BeginValidateFabricUpgrade(fabricUpgradeSpec, callback, parent); });
}

ErrorCode TestHostingSubsystem::EndValidateFabricUpgrade(
    __out bool & shouldRestartReplica,
    AsyncOperationSPtr const & operation)
{
    return WrapEndWithFaultInjection(
        operation,
        [&]() { return hosting_->EndValidateFabricUpgrade(shouldRestartReplica, operation); });
}

AsyncOperationSPtr TestHostingSubsystem::BeginFabricUpgrade(
    FabricUpgradeSpecification const & fabricUpgradeSpec,
    bool const shouldRestartReplica,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return WrapBeginWithFaultInjection(
        DefaultName,
        ApiFaultHelper::FabricUpgrade,
        callback,
        parent,
        [&]() { return hosting_->BeginFabricUpgrade(fabricUpgradeSpec, shouldRestartReplica, callback, parent); });
}

ErrorCode TestHostingSubsystem::EndFabricUpgrade(
    AsyncOperationSPtr const & operation)
{
    return WrapEndWithFaultInjection(
        operation,
        [&]() { return hosting_->EndFabricUpgrade(operation); });
}

Common::AsyncOperationSPtr TestHostingSubsystem::BeginTerminateServiceHost(
    std::wstring const & hostId,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return hosting_->BeginTerminateServiceHost(
        hostId,
        callback,
        parent);
}

void TestHostingSubsystem::EndTerminateServiceHost(
    Common::AsyncOperationSPtr const & operation)
{
    hosting_->EndTerminateServiceHost(operation);
}

ErrorCode TestHostingSubsystem::IncrementUsageCount(ServiceTypeIdentifier const & serviceTypeId, ServicePackageActivationContext const & activationContext)
{
    return hosting_->IncrementUsageCount(serviceTypeId, activationContext);
}

void TestHostingSubsystem::DecrementUsageCount(ServiceTypeIdentifier const & serviceTypeId, ServicePackageActivationContext const & activationContext)
{
    return hosting_->DecrementUsageCount(serviceTypeId, activationContext);
}

void TestHostingSubsystem::InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient)
{
    return hosting_->InitializeHealthReportingComponent(healthClient);
}

void TestHostingSubsystem::InitializePassThroughClientFactory(Api::IClientFactoryPtr const &clientFactory)
{
    return hosting_->InitializePassThroughClientFactory(clientFactory);
}

void TestHostingSubsystem::InitializeLegacyTestabilityRequestHandler(
    Hosting2::LegacyTestabilityRequestHandlerSPtr const & legacyTestabilityRequestHandler)
{
    hosting_->InitializeLegacyTestabilityRequestHandler(legacyTestabilityRequestHandler);
}

Common::AsyncOperationSPtr TestHostingSubsystem::BeginRestartDeployedPackage(
    int64 codePackageInstanceId,
    std::wstring applicationNameArgument,
    std::wstring serviceManifestNameArgument,
    std::wstring codePackageNameArgument,
    Common::ActivityId const activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent) 
{
    return hosting_->BeginRestartDeployedPackage(
        codePackageInstanceId,
        applicationNameArgument,
        serviceManifestNameArgument,
        codePackageNameArgument,
        activityId,
        timeout,
        callback,
        parent);
}

Common::AsyncOperationSPtr TestHostingSubsystem::BeginRestartDeployedPackage(
    int64 codePackageInstanceId,
    std::wstring const & applicationNameArgument,
    std::wstring const & serviceManifestNameArgument,
    std::wstring const & servicePackageActivationId,
    std::wstring const & codePackageNameArgument,
    Common::ActivityId const activityId,
    Common::TimeSpan timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return hosting_->BeginRestartDeployedPackage(
        codePackageInstanceId,
        applicationNameArgument,
        serviceManifestNameArgument,
        servicePackageActivationId,
        codePackageNameArgument,
        activityId,
        timeout,
        callback,
        parent);
}

Common::ErrorCode TestHostingSubsystem::EndRestartDeployedPackage(
    Common::AsyncOperationSPtr const &operation,
    __out Transport::MessageUPtr& reply)
{
    return hosting_->EndRestartDeployedPackage(operation, reply);
}

ServiceTypeRegisteredEventHHandler TestHostingSubsystem::RegisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHandler const & handler)
{
    return hosting_->RegisterServiceTypeRegisteredEventHandler(handler);
}

bool TestHostingSubsystem::UnregisterServiceTypeRegisteredEventHandler(ServiceTypeRegisteredEventHHandler const & hHandler)
{
    return hosting_->UnregisterServiceTypeRegisteredEventHandler(hHandler);
}

ServiceTypeDisabledEventHHandler TestHostingSubsystem::RegisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHandler const & handler)
{
    return hosting_->RegisterServiceTypeDisabledEventHandler(handler);
}

bool TestHostingSubsystem::UnregisterServiceTypeDisabledEventHandler(ServiceTypeDisabledEventHHandler const & hHandler)
{
    return hosting_->UnregisterServiceTypeDisabledEventHandler(hHandler);
}

ServiceTypeEnabledEventHHandler TestHostingSubsystem::RegisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHandler const & handler)
{
    return hosting_->RegisterServiceTypeEnabledEventHandler(handler);
}

bool TestHostingSubsystem::UnregisterServiceTypeEnabledEventHandler(ServiceTypeEnabledEventHHandler const & hHandler)
{
    return hosting_->UnregisterServiceTypeEnabledEventHandler(hHandler);
}

RuntimeClosedEventHHandler TestHostingSubsystem::RegisterRuntimeClosedEventHandler(RuntimeClosedEventHandler const & handler)
{
    return hosting_->RegisterRuntimeClosedEventHandler(handler);
}

bool TestHostingSubsystem::UnregisterRuntimeClosedEventHandler(RuntimeClosedEventHHandler const & hHandler)
{
    return hosting_->UnregisterRuntimeClosedEventHandler(hHandler);
}

ApplicationHostClosedEventHHandler TestHostingSubsystem::RegisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHandler const & handler)
{
    return hosting_->RegisterApplicationHostClosedEventHandler(handler);
}

bool TestHostingSubsystem::UnregisterApplicationHostClosedEventHandler(ApplicationHostClosedEventHHandler const & hHandler)
{
    return hosting_->UnregisterApplicationHostClosedEventHandler(hHandler);
}

AvailableContainerImagesEventHHandler TestHostingSubsystem::RegisterSendAvailableContainerImagesEventHandler(
    AvailableContainerImagesEventHandler const & handler)
{
    return hosting_->RegisterSendAvailableContainerImagesEventHandler(handler);
}

bool TestHostingSubsystem::UnregisterSendAvailableContainerImagesEventHandler(AvailableContainerImagesEventHHandler const & hHandler)
{
    return hosting_->UnregisterSendAvailableContainerImagesEventHandler(hHandler);
}

namespace
{
    AsyncOperationSPtr CreateAndStartFaultInjectedAsyncOperation(
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        wstring copy = DefaultTag;
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCode(DefaultError, move(copy)),
            callback,
            parent);
    }

    bool IsBlockingAsyncOperation(AsyncOperationSPtr const & operation)
    {
        return dynamic_pointer_cast<BlockingAsyncOperation>(operation) != nullptr;
    }

    bool IsCompletedFaultInjectionAsyncOperation(AsyncOperationSPtr const & operation)
    {
        auto casted = dynamic_pointer_cast<CompletedAsyncOperation>(operation);
        if (casted == nullptr)
        {
            return false;
        }

        if (casted->Error.ReadValue() != DefaultError ||
            casted->Error.Message != DefaultTag)
        {
            return false;
        }

        return true;
    }

    bool IsAsyncOperationAFaultInjected(AsyncOperationSPtr const & operation)
    {
        if (IsBlockingAsyncOperation(operation))
        {
            return true;
        }

        if (IsCompletedFaultInjectionAsyncOperation(operation))
        {
            return true;
        }

        return false;
    }
}

AsyncOperationSPtr TestHostingSubsystem::WrapBeginWithFaultInjection(
    wstring const & name,
    ApiFaultHelper::ApiName api,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    function<AsyncOperationSPtr()> beginFunc)
{
    if (ApiFaultHelper::Get().ShouldFailOn(nodeId_, name, ApiFaultHelper::Hosting, api))
    {
        TestSession::WriteInfo(TraceType, wformatString(nodeId_), "Injected fault for {0} at {1} on node {2}", name, static_cast<int>(api), nodeId_);
        return CreateAndStartFaultInjectedAsyncOperation(callback, parent);
    }
    else if (ApiFaultHelper::Get().ShouldFailOn(nodeId_, name, ApiFaultHelper::Hosting, api, ApiFaultHelper::Block))
    {
        TestSession::WriteInfo(TraceType, wformatString(nodeId_), "Injected block for {0} at {1} on node {2}", name, static_cast<int>(api), nodeId_);
        return AsyncOperation::CreateAndStart<BlockingAsyncOperation>();
    }

    return beginFunc();
}

ErrorCode TestHostingSubsystem::WrapEndWithFaultInjection(
    AsyncOperationSPtr const & operation,
    function<ErrorCode()> endFunc)
{
    if (IsAsyncOperationAFaultInjected(operation))
    {
        return operation->Error;
    }

    return endFunc();
}

ErrorCode TestHostingSubsystem::WrapSyncApiWithFaultInjection(
    wstring const & name,
    ApiFaultHelper::ApiName api,
    function<ErrorCode()> syncFunc)
{
    if (ApiFaultHelper::Get().ShouldFailOn(nodeId_, name, ApiFaultHelper::Hosting, api))
    {
        TestSession::WriteInfo(TraceType, wformatString(nodeId_), "Injected fault for {0} at {1}", name, static_cast<int>(api));
        return DefaultError;
    }

    return syncFunc();
}

uint64 TestHostingSubsystem::GetResourceNodeCapacity(std::wstring const& resourceName) const
{
    return hosting_->GetResourceNodeCapacity(resourceName);
}
