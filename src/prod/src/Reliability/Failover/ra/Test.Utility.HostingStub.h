// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "RATestPointers.h"

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class FindServiceTypeRegistrationError
            {
            public:
                class CategoryName
                {
                public:
                    enum Enum
                    {
                        Success = 0,

                        // Something like activation in progress
                        RetryableErrorForOpen = 1,

                        // Fatal error for Open (ServiceTypeDisabled)
                        FatalErrorForOpen = 2,

                        // Unknown error for Open (GlobalLeaseLost)
                        UnknownErrorForOpen = 3,

                        // Something like activation in progress
                        RetryableErrorForReopen = 4,

                        // AppINstanceDeleted
                        FatalErrorForReopen = 5,

                        // Unknown error for Reopen (GlobalLEaseLost)
                        UnknownErrorForReopen = 6,

                        // Retryable error for both open and reopen (DeploymentInProgress)
                        RetryableErrorForOpenAndReopen = 7,

                        // Retryable Error for drop
                        RetryableErrorForDrop = 8,

                        // Fatal error for Drop 
                        FatalErrorForDrop = 9,

                        UnknownErrorForDrop = 10,

                        RetryableErrorForOpenAndReopenAndDrop = 11,
                    };
                };

                FindServiceTypeRegistrationError(Common::ErrorCodeValue::Enum error, CategoryName::Enum name)
                    : error_(error),
                    name_(name)
                {
                }

                __declspec(property(get = get_Name)) CategoryName::Enum Name;
                CategoryName::Enum get_Name() const { return name_; }

                __declspec(property(get = get_Error)) Common::ErrorCode Error;
                Common::ErrorCode get_Error() const { return Common::ErrorCode(error_); }

                static FindServiceTypeRegistrationError Create(CategoryName::Enum category)
                {
                    switch (category)
                    {
                    case CategoryName::Success:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::Success, category);

                    case CategoryName::RetryableErrorForOpen:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::HostingDeploymentInProgress, category);

                    case CategoryName::FatalErrorForOpen:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::HostingServiceTypeDisabled, category);

                    case CategoryName::UnknownErrorForOpen:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::GlobalLeaseLost, category);

                    case CategoryName::RetryableErrorForDrop:
                    case CategoryName::RetryableErrorForReopen:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::HostingDeploymentInProgress, category);

                    case CategoryName::FatalErrorForDrop:
                    case CategoryName::FatalErrorForReopen:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::ApplicationInstanceDeleted, category);

                    case CategoryName::UnknownErrorForDrop:
                    case CategoryName::UnknownErrorForReopen:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::GlobalLeaseLost, category);

                    case CategoryName::RetryableErrorForOpenAndReopen:
                    case CategoryName::RetryableErrorForOpenAndReopenAndDrop:
                        return FindServiceTypeRegistrationError(Common::ErrorCodeValue::HostingDeploymentInProgress, category);

                    default:
                        Common::Assert::CodingError("Incorrect definition {0}", static_cast<int>(category));
                    };
                }

            private:
                CategoryName::Enum name_;
                Common::ErrorCodeValue::Enum error_;
            };

            class HostingStub : public Hosting2::IHostingSubsystem
            {
            public:
                typedef AsyncApiStub<Common::FabricVersion, void*> DownloadFabricAsyncApiStub;
                DownloadFabricAsyncApiStub DownloadFabricAsyncApi;

                typedef AsyncApiStub<std::tuple<ServiceModel::FabricUpgradeSpecification, bool>, void*> UpgradeFabricAsyncApiStub;
                UpgradeFabricAsyncApiStub UpgradeFabricAsyncApi;

                typedef AsyncApiStub<ServiceModel::FabricUpgradeSpecification, bool> ValidateFabricAsyncApiStub;
                ValidateFabricAsyncApiStub ValidateFabricAsyncApi;

                typedef AsyncApiStub<Hosting2::ApplicationDownloadSpecification, void*> DownloadApplicationAsyncApiStub;
                DownloadApplicationAsyncApiStub DownloadApplicationAsyncApi;                

                typedef AsyncApiStub<ServiceModel::ApplicationUpgradeSpecification, void*> UpgradeApplicationAsyncApiStub;
                UpgradeApplicationAsyncApiStub UpgradeApplicationAsyncApi;

                std::vector<ServiceModel::ApplicationUpgradeSpecification> AnalyzeApplicationUpgradeCalls;
                
                Common::ErrorCodeValue::Enum ErrorToReturnFromOperation;

                struct FindServiceTypeParameters
                {
                    ServiceModel::VersionedServiceTypeIdentifier ServiceType;
                    ServiceModel::ServicePackageActivationContext ActivationContext;
                    Reliability::ServiceDescription Description;
                };
                typedef AsyncApiStub<FindServiceTypeParameters, void*> FindServiceTypeApiStub;
                FindServiceTypeApiStub FindServiceTypeApi;

                AffectedRuntimesSet AffectedRuntimes;

                struct IncrementDecrementParameters
                {
                    ServiceModel::ServiceTypeIdentifier ServiceTypeId;
                    ServiceModel::ServicePackageActivationContext ActivationContext;
                };

                typedef AsyncApiStub<IncrementDecrementParameters, void*> IncrementDecrementApiStub;
                IncrementDecrementApiStub IncrementApi;
                IncrementDecrementApiStub DecrementApi;

                typedef AsyncApiStub<std::wstring, void*> TerminateServiceHostApiStub;
                TerminateServiceHostApiStub TerminateServiceHostApi;

                HostingStub() :
                    DownloadFabricAsyncApi(L"DownloadFabric"),
                    UpgradeFabricAsyncApi(L"UpgradeFabric"),
                    ValidateFabricAsyncApi(L"ValidateFabric"),
                    DownloadApplicationAsyncApi(L"DownloadApplication"),
                    UpgradeApplicationAsyncApi(L"UpgradeApplication"),
                    TerminateServiceHostApi(L"TerminateServiceHost"),
                    FindServiceTypeApi(L"FindServiceTypeApi"),
                    IncrementApi(L"IncrementServiceTypeRegistration"),
                    DecrementApi(L"DecrementServiceTypeRegistration")
                {
                    ErrorToReturnFromOperation = Common::ErrorCodeValue::Success;
                    TerminateServiceHostApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    FindServiceTypeApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    IncrementApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    DecrementApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                }

                void Reset()
                {
                    registrationExpectations_.clear();
                    FindServiceTypeApi.Clear();
                    IncrementApi.Clear();
                    DecrementApi.Clear();

                    DownloadFabricAsyncApi.Clear();
                    UpgradeFabricAsyncApi.Clear();
                    ValidateFabricAsyncApi.Clear();
                    DownloadApplicationAsyncApi.Clear();
                    UpgradeApplicationAsyncApi.Clear();
                    TerminateServiceHostApi.Clear();
                }

                void SetAllAsyncApisToCompleteSynchronouslyWithSuccess()
                {
                    DownloadFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    UpgradeFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    ValidateFabricAsyncApi.SetCompleteSynchronouslyWithSuccess(true);
                    DownloadApplicationAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    UpgradeApplicationAsyncApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                    FindServiceTypeApi.SetCompleteSynchronouslyWithSuccess(nullptr);
                }

                void SetAllAsyncApisToCompleteAsynchronously()
                {
                    DownloadFabricAsyncApi.SetCompleteAsynchronously();
                    UpgradeFabricAsyncApi.SetCompleteAsynchronously();
                    ValidateFabricAsyncApi.SetCompleteAsynchronously();
                    DownloadApplicationAsyncApi.SetCompleteAsynchronously();
                    UpgradeApplicationAsyncApi.SetCompleteAsynchronously();
                }

                static Hosting2::ServiceTypeRegistrationSPtr CreateRegistrationHelper(StateManagement::ServiceTypeReadContext const & readContext)
                {
                    return readContext.CreateServiceTypeRegistration();
                }

                static Hosting2::ServiceTypeRegistrationSPtr CreateRegistrationHelper(StateManagement::ServiceTypeReadContext const & readContext, ServiceModel::ServicePackageActivationContext const & activationContext)
                {
                    return readContext.CreateServiceTypeRegistration(activationContext);
                }

                Common::ErrorCode FindServiceTypeRegistration(
                    ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
                    Reliability::ServiceDescription const & serviceDescription,
                    ServiceModel::ServicePackageActivationContext const & activationContext,
                    __out uint64 & sequenceNumber,
                    __out Hosting2::ServiceTypeRegistrationSPtr & registration) override
                {
                    sequenceNumber = 0;

                    FindServiceTypeParameters parameters;
                    parameters.Description = serviceDescription;
                    parameters.ActivationContext = activationContext;
                    parameters.ServiceType = versionedServiceTypeId;

                    registration = Hosting2::ServiceTypeRegistrationSPtr();

                    auto error = Common::ErrorCodeValue::Success;

                    auto it = registrationExpectations_.find(versionedServiceTypeId.Id.ServiceTypeName);
                    if (it == registrationExpectations_.end())
                    {
                        error = Common::ErrorCodeValue::HostingDeploymentInProgress;
                    }
                    else
                    {
                        if (it->second.ErrorToReturn == Common::ErrorCodeValue::Success ||
                            it->second.ErrorToReturn == Common::ErrorCodeValue::HostingServiceTypeRegistrationVersionMismatch)
                        {
                            registration = CreateRegistrationHelper(it->second.ReadContext, activationContext);
                        }

                        error = it->second.ErrorToReturn;
                    }
                    
                    FindServiceTypeApi.SetCompleteSynchronouslyWithError(error);

                    return FindServiceTypeApi.OnCallAsSynchronousApi(parameters);
                }

                Common::ErrorCode GetHostId(
                    ServiceModel::VersionedServiceTypeIdentifier const &,
                    std::wstring const &,
                    __out std::wstring &)
                {
                    return Common::ErrorCodeValue::NotImplemented;
                }

                Common::ErrorCode GetHostId(
                    ServiceModel::VersionedServiceTypeIdentifier const &,
                    std::wstring const &,
                    ServiceModel::ServicePackageActivationContext const &,
                    __out std::wstring &)
                {
                    return Common::ErrorCodeValue::NotImplemented;
                }

                Common::AsyncOperationSPtr BeginDownloadApplication(
                    Hosting2::ApplicationDownloadSpecification const & appDownloadSpec,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return DownloadApplicationAsyncApi.OnCall(appDownloadSpec, callback, parent);
                }

                Common::ErrorCode EndDownloadApplication(
                    Common::AsyncOperationSPtr const & operation) override 
                {
                    return DownloadApplicationAsyncApiStub::End(operation);
                }

                Common::ErrorCode AnalyzeApplicationUpgrade(
                    ServiceModel::ApplicationUpgradeSpecification const & upgradeSpec,
                    __out AffectedRuntimesSet & affectedRuntimeIds)
                {
                    AnalyzeApplicationUpgradeCalls.push_back(upgradeSpec);
                    affectedRuntimeIds = AffectedRuntimes;
                    return ErrorToReturnFromOperation;
                }

                Common::AsyncOperationSPtr BeginUpgradeApplication(
                    ServiceModel::ApplicationUpgradeSpecification const & appUpgradeSpec,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return UpgradeApplicationAsyncApi.OnCall(appUpgradeSpec, callback, parent);
                }

                Common::ErrorCode EndUpgradeApplication(
                    Common::AsyncOperationSPtr const & operation) override
                {
                    return UpgradeApplicationAsyncApiStub::End(operation);
                }

                Common::ErrorCode IncrementUsageCount(
                    ServiceModel::ServiceTypeIdentifier const & id,
                    ServiceModel::ServicePackageActivationContext const & activationContext) override
                {
                    IncrementDecrementParameters parameters;
                    parameters.ServiceTypeId = id;
                    parameters.ActivationContext = activationContext;

                    return IncrementApi.OnCallAsSynchronousApi(parameters);
                }

                void DecrementUsageCount(
                    ServiceModel::ServiceTypeIdentifier const & id,
                    ServiceModel::ServicePackageActivationContext const & activationContext) override
                {
                    IncrementDecrementParameters parameters;
                    parameters.ServiceTypeId = id;
                    parameters.ActivationContext = activationContext;

                    DecrementApi.OnCallAsSynchronousApi(parameters).ReadValue();
                }

                Common::AsyncOperationSPtr BeginDownloadFabric(
                    Common::FabricVersion const & fabricVersion,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return DownloadFabricAsyncApi.OnCall(fabricVersion, callback, parent);
                }

                Common::ErrorCode EndDownloadFabric(
                    Common::AsyncOperationSPtr const & operation) override
                {
                    return DownloadFabricAsyncApiStub::End(operation);
                }

                Common::AsyncOperationSPtr BeginFabricUpgrade(
                    ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
                    bool const shouldRestartReplica,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return UpgradeFabricAsyncApi.OnCall(std::make_tuple(fabricUpgradeSpec, shouldRestartReplica), callback, parent);
                }

                Common::ErrorCode EndFabricUpgrade(
                    Common::AsyncOperationSPtr const & operation) override
                {
                    return UpgradeFabricAsyncApiStub::End(operation);
                }

                Common::AsyncOperationSPtr BeginValidateFabricUpgrade(
                    ServiceModel::FabricUpgradeSpecification const & fabricUpgradeSpec,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return ValidateFabricAsyncApi.OnCall(fabricUpgradeSpec, callback, parent);
                }

                Common::ErrorCode EndValidateFabricUpgrade(
                    __out bool & shouldRestartReplica,
                    Common::AsyncOperationSPtr const & operation) override
                {
                    return ValidateFabricAsyncApiStub::End(operation, shouldRestartReplica);
                }

                Common::AsyncOperationSPtr BeginTerminateServiceHost(
                    std::wstring const & hostId,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return TerminateServiceHostApi.OnCall(hostId, callback, parent);
                }

                void EndTerminateServiceHost(
                    Common::AsyncOperationSPtr const & operation) override
                {
                    TerminateServiceHostApiStub::End(operation);
                }

                void AddFindServiceTypeExpectation(StateManagement::ServiceTypeReadContext const & context, FindServiceTypeRegistrationError::CategoryName::Enum errorCategory)
                {
                    auto fsre = FindServiceTypeRegistrationError::Create(errorCategory);
                    AddFindServiceTypeExpectation(context, fsre.Error.ReadValue());
                }

                void AddFindServiceTypeExpectation(StateManagement::ServiceTypeReadContext const & context, Common::ErrorCodeValue::Enum error)
                {
                    ServiceTypeRegistrationExpectation exp;
                    exp.ReadContext = context;
                    exp.ErrorToReturn = error;

                    registrationExpectations_[context.ServiceTypeId.ServiceTypeName] = exp;
                }

                void AddAffectedRuntime(StateManagement::ServiceTypeReadContext const & context)
                {
                    AffectedRuntimes.insert(context.RuntimeId);
                }

                Hosting2::ServiceTypeRegisteredEventHHandler RegisterServiceTypeRegisteredEventHandler(Hosting2::ServiceTypeRegisteredEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterServiceTypeRegisteredEventHandler(Hosting2::ServiceTypeRegisteredEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::ServiceTypeDisabledEventHHandler RegisterServiceTypeDisabledEventHandler(Hosting2::ServiceTypeDisabledEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterServiceTypeDisabledEventHandler(Hosting2::ServiceTypeDisabledEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::ServiceTypeEnabledEventHHandler RegisterServiceTypeEnabledEventHandler(Hosting2::ServiceTypeEnabledEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterServiceTypeEnabledEventHandler(Hosting2::ServiceTypeEnabledEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::RuntimeClosedEventHHandler RegisterRuntimeClosedEventHandler(Hosting2::RuntimeClosedEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterRuntimeClosedEventHandler(Hosting2::RuntimeClosedEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::ApplicationHostClosedEventHHandler RegisterApplicationHostClosedEventHandler(Hosting2::ApplicationHostClosedEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterApplicationHostClosedEventHandler(Hosting2::ApplicationHostClosedEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::AvailableContainerImagesEventHHandler RegisterSendAvailableContainerImagesEventHandler(
                    Hosting2::AvailableContainerImagesEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterSendAvailableContainerImagesEventHandler(Hosting2::AvailableContainerImagesEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Common::AsyncOperationSPtr OnBeginOpen(
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::AsyncOperationSPtr OnBeginClose(
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void OnAbort()
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void InitializePassThroughClientFactory(Api::IClientFactoryPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void InitializeLegacyTestabilityRequestHandler(
                    Hosting2::LegacyTestabilityRequestHandlerSPtr const &) override
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::AsyncOperationSPtr BeginRestartDeployedPackage(
                    int64,
                    std::wstring,
                    std::wstring,
                    std::wstring,
                    Common::ActivityId const,
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &) override
                {
                    Common::Assert::CodingError("NotImpl");
                }
                
                Common::AsyncOperationSPtr BeginRestartDeployedPackage(
                    int64,
                    std::wstring const &,
                    std::wstring const &,
					std::wstring const &,
                    std::wstring const &,
                    Common::ActivityId const,
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &) override
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::ErrorCode EndRestartDeployedPackage(
                    Common::AsyncOperationSPtr const &,
                    __out Transport::MessageUPtr&) override
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Hosting2::IFabricActivatorClientSPtr const & get_FabricActivatorClient() const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                std::wstring const get_NodeName() const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                uint64 GetResourceNodeCapacity(std::wstring const&) const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                __declspec(property(get = get_NetworkInventoryAgent, put = set_NetworkInventoryAgent)) Hosting2::NetworkInventoryAgentSPtr NetworkInventoryAgent;
                Hosting2::NetworkInventoryAgentSPtr const get_NetworkInventoryAgent() const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void set_NetworkInventoryAgent(Hosting2::NetworkInventoryAgentSPtr value)
                {
                    Common::Assert::CodingError("NotImpl");
                }                          

            private:
                struct ServiceTypeRegistrationExpectation
                {
                    StateManagement::ServiceTypeReadContext ReadContext;
                    Common::ErrorCodeValue::Enum ErrorToReturn;
                };

                std::map<std::wstring, ServiceTypeRegistrationExpectation> registrationExpectations_;
            };

        class PerfTestHostingStub : public Hosting2::IHostingSubsystem
            {
            public:
                Common::ErrorCode FindServiceTypeRegistration(
                    ServiceModel::VersionedServiceTypeIdentifier const & ,
                    Reliability::ServiceDescription const & ,
                    ServiceModel::ServicePackageActivationContext const & ,
                    __out uint64 & sequenceNumber,
                    __out Hosting2::ServiceTypeRegistrationSPtr & registration) override
                {
                    sequenceNumber = 0;
                    registration = Hosting2::ServiceTypeRegistrationSPtr();
                    return Common::ErrorCodeValue::HostingDeploymentInProgress;
                }

                Common::ErrorCode GetHostId(
                    ServiceModel::VersionedServiceTypeIdentifier const &,
                    std::wstring const &,
                    __out std::wstring &)
                {
                    return Common::ErrorCodeValue::NotImplemented;
                }

                Common::ErrorCode GetHostId(
                    ServiceModel::VersionedServiceTypeIdentifier const &,
                    std::wstring const &,
                    ServiceModel::ServicePackageActivationContext const &,
                    __out std::wstring &)
                {
                    return Common::ErrorCodeValue::NotImplemented;
                }

                Common::AsyncOperationSPtr BeginDownloadApplication(
                    Hosting2::ApplicationDownloadSpecification const & ,
                    Common::AsyncCallback const & ,
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode EndDownloadApplication(
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode AnalyzeApplicationUpgrade(
                    ServiceModel::ApplicationUpgradeSpecification const & ,
                    __out AffectedRuntimesSet & )
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::AsyncOperationSPtr BeginUpgradeApplication(
                    ServiceModel::ApplicationUpgradeSpecification const & ,
                    Common::AsyncCallback const & ,
                    Common::AsyncOperationSPtr const &) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode EndUpgradeApplication(
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode IncrementUsageCount(
                    ServiceModel::ServiceTypeIdentifier const &,
                    ServiceModel::ServicePackageActivationContext const & ) override
                {
                    return Common::ErrorCode::Success();
                }

                void DecrementUsageCount(
                    ServiceModel::ServiceTypeIdentifier const &,
                    ServiceModel::ServicePackageActivationContext const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::AsyncOperationSPtr BeginDownloadFabric(
                    Common::FabricVersion const & ,
                    Common::AsyncCallback const & ,
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode EndDownloadFabric(
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::AsyncOperationSPtr BeginFabricUpgrade(
                    ServiceModel::FabricUpgradeSpecification const & ,
                    bool const ,
                    Common::AsyncCallback const & ,
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode EndFabricUpgrade(
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::AsyncOperationSPtr BeginValidateFabricUpgrade(
                    ServiceModel::FabricUpgradeSpecification const & ,
                    Common::AsyncCallback const & ,
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::ErrorCode EndValidateFabricUpgrade(
                    __out bool & ,
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Common::AsyncOperationSPtr BeginTerminateServiceHost(
                    std::wstring const & ,
                    Common::AsyncCallback const & ,
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                void EndTerminateServiceHost(
                    Common::AsyncOperationSPtr const & ) override
                {
                    Common::Assert::CodingError("not impl");
                }

                Hosting2::ServiceTypeRegisteredEventHHandler RegisterServiceTypeRegisteredEventHandler(Hosting2::ServiceTypeRegisteredEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterServiceTypeRegisteredEventHandler(Hosting2::ServiceTypeRegisteredEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::ServiceTypeDisabledEventHHandler RegisterServiceTypeDisabledEventHandler(Hosting2::ServiceTypeDisabledEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterServiceTypeDisabledEventHandler(Hosting2::ServiceTypeDisabledEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::ServiceTypeEnabledEventHHandler RegisterServiceTypeEnabledEventHandler(Hosting2::ServiceTypeEnabledEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterServiceTypeEnabledEventHandler(Hosting2::ServiceTypeEnabledEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::RuntimeClosedEventHHandler RegisterRuntimeClosedEventHandler(Hosting2::RuntimeClosedEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterRuntimeClosedEventHandler(Hosting2::RuntimeClosedEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::ApplicationHostClosedEventHHandler RegisterApplicationHostClosedEventHandler(Hosting2::ApplicationHostClosedEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterApplicationHostClosedEventHandler(Hosting2::ApplicationHostClosedEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Hosting2::AvailableContainerImagesEventHHandler RegisterSendAvailableContainerImagesEventHandler(
                    Hosting2::AvailableContainerImagesEventHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                bool UnregisterSendAvailableContainerImagesEventHandler(Hosting2::AvailableContainerImagesEventHHandler const &)
                {
                    Common::Assert::CodingError("unsupported");
                }

                Common::AsyncOperationSPtr OnBeginOpen(
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::AsyncOperationSPtr OnBeginClose(
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void OnAbort()
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void InitializePassThroughClientFactory(Api::IClientFactoryPtr const &)
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void InitializeLegacyTestabilityRequestHandler(
                    Hosting2::LegacyTestabilityRequestHandlerSPtr const &) override
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::AsyncOperationSPtr BeginRestartDeployedPackage(
                    int64,
                    std::wstring,
                    std::wstring,
                    std::wstring,
                    Common::ActivityId const,
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &) override
                {
                    Common::Assert::CodingError("NotImpl");
                }
                
                Common::AsyncOperationSPtr BeginRestartDeployedPackage(
                    int64,
                    std::wstring const &,
                    std::wstring const &,
                    std::wstring const &,
                    std::wstring const &,
                    Common::ActivityId const,
                    Common::TimeSpan,
                    Common::AsyncCallback const &,
                    Common::AsyncOperationSPtr const &) override
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Common::ErrorCode EndRestartDeployedPackage(
                    Common::AsyncOperationSPtr const &,
                    __out Transport::MessageUPtr&) override
                {
                    Common::Assert::CodingError("NotImpl");
                }

                Hosting2::IFabricActivatorClientSPtr const & get_FabricActivatorClient() const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                std::wstring const get_NodeName() const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                uint64 GetResourceNodeCapacity(std::wstring const&) const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                __declspec(property(get = get_NetworkInventoryAgent, put = set_NetworkInventoryAgent)) Hosting2::NetworkInventoryAgentSPtr NetworkInventoryAgent;
                Hosting2::NetworkInventoryAgentSPtr const get_NetworkInventoryAgent() const
                {
                    Common::Assert::CodingError("NotImpl");
                }

                void set_NetworkInventoryAgent(Hosting2::NetworkInventoryAgentSPtr value)
                {
                    Common::Assert::CodingError("NotImpl");
                }          
            };
        }
    }
}
