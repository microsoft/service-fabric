// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "Hosting2/FabricNodeHost.Test.h"
#include "Management/ImageModel/ImageModel.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;
using namespace Query;
using namespace Management::ImageModel;
using namespace Reliability;

const StringLiteral TraceType("OnDemandCodePackageActivationTest");

class OnDemandCodePackageActivationTestClass
{
protected:
    OnDemandCodePackageActivationTestClass()
        : fabricNodeHost_(make_shared<TestFabricNodeHost>())
        , servicePackageNames_()
        , serviceDescription_()
    {
        applicationName_ = *OnDemandCodePackageActivationAppName;
        applicationId_ = *OnDemandCodePackageActivationAppId;

        servicePackageNames_.push_back(L"OnDemandCodePackageActivationTestService");
        servicePackageNames_.push_back(L"OnDemandCodePackageActivationTestService1");

        serviceDescription_.ApplicationName = std::move(wstring(applicationName_));
        serviceDescription_.Name = L"fabric:/OnDemandCodePackageActivationApp/OnDemandCodePackageActivationService";

        BOOST_REQUIRE(Setup());
    }

    TEST_CLASS_SETUP(Setup);
    ~OnDemandCodePackageActivationTestClass() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    class OpenGuestServiceInstanceAsyncOperation;
    class CloseGuestServiceInstanceAsyncOperation;

    HostingSubsystem & GetHosting();
    FabricRuntimeManagerUPtr const & GetFabricRuntimeManager();
    HostingQueryManagerUPtr & GetHostingQueryManager();
    GuestServiceTypeHostManagerUPtr const & GetTypeHostManager();
    ApplicationHostActivationTableUPtr const & GetApplicationHostActivationTable();

    void VerifyRegistrations();
    void VerifyDeployedCodePackages(bool checkPresense);
    void VerifyGuestServiceTypeHostCount(size_t count);

    void GetDeployedCodePackagesResult(
        wstring const & codePackageName,
        vector<DeployedCodePackageQueryResult> & codePackageResult);

    void VerifyDeployedCodePackagesResult(
        vector<DeployedCodePackageQueryResult> const & codePackageResult, 
        wstring const & codePackageName);

    void VerifyRegistrationsRemoved();

    void ActivatePackage(
        VersionedServiceTypeIdentifier const & versionedServiceTypeId,
        ServicePackageActivationContext const & activationContext);

    void VerifyOnDemandActivationWithStatelessServiceInstance(GuestServiceTypeHostSPtr & guestTypeHost);
    void VerifyOnDemandActivationWithStatefulServiceReplica(GuestServiceTypeHostSPtr & guestTypeHost);
    void VerifyActivatorCodePackageTermination(GuestServiceTypeHostSPtr & guestTypeHost);

    void VerifyOnDemandActivation();
    void WaitforDeployedCodePackageCount(
        int resultCount, 
        int retryCount,
        vector<DeployedCodePackageQueryResult> & result);

    void OpenServiceInstance(ComPointer<ComGuestServiceInstance> & serviceInstance);
    void CloseServiceInstance(ComPointer<ComGuestServiceInstance> & serviceInstance);

    void OpenServiceReplica(ComPointer<ComGuestServiceReplica> & serviceReplica);
    void CloseServiceReplica(ComPointer<ComGuestServiceReplica> & serviceReplica);
    void ChangeServiceReplicaRole(
        ComPointer<ComGuestServiceReplica> & serviceReplica,
        FABRIC_REPLICA_ROLE newRole);

    void VerifyCodePackageTracking(GuestServiceTypeHostSPtr & guestTypeHost, bool active);
    void TerminateRandomCodePackage(bool activatorCodePackage);
    void GetServiceTypeHost(GuestServiceTypeHostSPtr & guestTypeHost);

    wstring GetProgramPath(
        StoreLayoutSpecification const & storeLayout,
        wstring const & serviceManifestName,
        wstring const & codePackageName,
        ExeEntryPointDescription const & entryPoint);

    shared_ptr<TestFabricNodeHost> fabricNodeHost_;

    wstring applicationName_;
    wstring applicationId_;
    vector<wstring> servicePackageNames_;
    vector<ServiceTypeDescription> typesPerPackage_;
    vector<CodePackageDescription> codePackagesPerServicePackage_;
    ServiceDescription serviceDescription_;

    ServicePackageInstanceIdentifier lastActivatedPackage_;
    vector<pair<VersionedServiceTypeIdentifier, ServicePackageActivationContext>> activatedServiceTypeIds_;
    vector<pair<VersionedServiceTypeIdentifier, ServicePackageInstanceIdentifier>> expectedRegistrations_;
    set<wstring, IsLessCaseInsensitiveComparer<wstring>> expectedRuntimeIds_;

    static int PackageActivationCount;
    static GlobalWString OnDemandCodePackageActivationAppTypeName;
    static GlobalWString OnDemandCodePackageActivationAppId;
    static GlobalWString OnDemandCodePackageActivationAppName;
    static GlobalWString ActivatorServiceTypeName;
    static GlobalWString ManifestVersion;

    static ServicePackageVersionInstance PackageVersion;
};

int OnDemandCodePackageActivationTestClass::PackageActivationCount = 1;
GlobalWString OnDemandCodePackageActivationTestClass::OnDemandCodePackageActivationAppTypeName = make_global<wstring>(L"OnDemandCodePackageActivationTestApp");
GlobalWString OnDemandCodePackageActivationTestClass::OnDemandCodePackageActivationAppId = make_global<wstring>(L"OnDemandCodePackageActivationTestApp_App0");
GlobalWString OnDemandCodePackageActivationTestClass::OnDemandCodePackageActivationAppName = make_global<wstring>(L"fabric:/OnDemandCodePackageActivationApp");
GlobalWString OnDemandCodePackageActivationTestClass::ActivatorServiceTypeName = make_global<wstring>(L"TestImplicitServiceType");
GlobalWString OnDemandCodePackageActivationTestClass::ManifestVersion = make_global<wstring>(L"1.0");

ServicePackageVersionInstance OnDemandCodePackageActivationTestClass::PackageVersion(
    ServicePackageVersion(ApplicationVersion(RolloutVersion(1, 0)), RolloutVersion(1, 0)), 1);

class ComDummyStatelessServicePartition 
    : public IFabricStatelessServicePartition2
    , public ComUnknownBase
{
    DENY_COPY(ComDummyStatelessServicePartition);

    BEGIN_COM_INTERFACE_LIST(ComDummyStatelessServicePartition)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServicePartition)
        COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition, IFabricStatelessServicePartition)
        COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition1, IFabricStatelessServicePartition1)
        COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition2, IFabricStatelessServicePartition2)
    END_COM_INTERFACE_LIST()

public:
    ComDummyStatelessServicePartition()
        : IFabricStatelessServicePartition2()
        , ComUnknownBase()
    {
    }

    ~ComDummyStatelessServicePartition()
    {
    }

public:
    HRESULT STDMETHODCALLTYPE GetPartitionInfo(
        /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue) override
    {
        UNREFERENCED_PARAMETER(bufferedValue);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportLoad(
        /* [in] */ ULONG metricCount,
        /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics) override
    {
        UNREFERENCED_PARAMETER(metricCount);
        UNREFERENCED_PARAMETER(metrics);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportFault(
        /* [in] */ FABRIC_FAULT_TYPE faultType) override
    {
        UNREFERENCED_PARAMETER(faultType);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportMoveCost(
        /* [in] */ FABRIC_MOVE_COST moveCost) override
    {
        UNREFERENCED_PARAMETER(moveCost);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportInstanceHealth(
        /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) override
    {
        UNREFERENCED_PARAMETER(healthInfo);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportPartitionHealth(
        /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo)override
    {
        UNREFERENCED_PARAMETER(healthInfo);
        return S_OK;
    }

private:
    std::wstring supportedServiceType_;
};

class ComDummyStatefulServicePartition
    : public IFabricStatefulServicePartition2
    , public ComUnknownBase
{
    DENY_COPY(ComDummyStatefulServicePartition);

    BEGIN_COM_INTERFACE_LIST(ComDummyStatefulServicePartition)
        COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServicePartition)
        COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition, IFabricStatefulServicePartition)
        COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition1, IFabricStatefulServicePartition1)
        COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition2, IFabricStatefulServicePartition2)
        END_COM_INTERFACE_LIST()

public:
    ComDummyStatefulServicePartition()
        : IFabricStatefulServicePartition2()
        , ComUnknownBase()
    {
    }

    ~ComDummyStatefulServicePartition()
    {
    }

public:
    HRESULT STDMETHODCALLTYPE GetPartitionInfo(
        /* [retval][out] */ const FABRIC_SERVICE_PARTITION_INFORMATION **bufferedValue) override
    {
        UNREFERENCED_PARAMETER(bufferedValue);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetReadStatus(
        /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus) override
    {
        UNREFERENCED_PARAMETER(readStatus);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetWriteStatus(
        /* [retval][out] */ FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus) override
    {
        UNREFERENCED_PARAMETER(writeStatus);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CreateReplicator(
        /* [in] */ IFabricStateProvider *stateProvider,
        /* [in] */ const FABRIC_REPLICATOR_SETTINGS *replicatorSettings,
        /* [out] */ IFabricReplicator **replicator,
        /* [retval][out] */ IFabricStateReplicator **stateReplicator) override
    {
        UNREFERENCED_PARAMETER(stateProvider);
        UNREFERENCED_PARAMETER(replicatorSettings);
        UNREFERENCED_PARAMETER(replicator);
        UNREFERENCED_PARAMETER(stateReplicator);

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportLoad(
        /* [in] */ ULONG metricCount,
        /* [size_is][in] */ const FABRIC_LOAD_METRIC *metrics) override
    {
        UNREFERENCED_PARAMETER(metricCount);
        UNREFERENCED_PARAMETER(metrics);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportFault(
        /* [in] */ FABRIC_FAULT_TYPE faultType) override
    {
        UNREFERENCED_PARAMETER(faultType);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportMoveCost(
        /* [in] */ FABRIC_MOVE_COST moveCost) override
    {
        UNREFERENCED_PARAMETER(moveCost);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportReplicaHealth(
        /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo) override
    {
        UNREFERENCED_PARAMETER(healthInfo);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ReportPartitionHealth(
        /* [in] */ const FABRIC_HEALTH_INFORMATION *healthInfo)override
    {
        UNREFERENCED_PARAMETER(healthInfo);
        return S_OK;
    }

private:
    std::wstring supportedServiceType_;
};

class OpenInstanceAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(OpenInstanceAsyncOperation)

public:
    OpenInstanceAsyncOperation(
        ComPointer<ComGuestServiceInstance> & serviceInstance,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , serviceInstance_(serviceInstance)
    {
    }

    virtual ~OpenInstanceAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        auto partition = make_com<ComDummyStatelessServicePartition, IFabricStatelessServicePartition2>();
        return serviceInstance_->BeginOpen(partition.GetRawPointer(), callback, context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricStringResult> result;
        return serviceInstance_->EndOpen(context, result.InitializationAddress());
    }

private:
    ComPointer<ComGuestServiceInstance> & serviceInstance_;
};

class CloseInstanceAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(CloseInstanceAsyncOperation)

public:
    CloseInstanceAsyncOperation(
        ComPointer<ComGuestServiceInstance> & serviceInstance,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , serviceInstance_(serviceInstance)
    {
    }

    virtual ~CloseInstanceAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseInstanceAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        return serviceInstance_->BeginClose(callback, context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return serviceInstance_->EndClose(context);
    }

private:
    ComPointer<ComGuestServiceInstance> & serviceInstance_;
};

class OpenReplicaAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(OpenReplicaAsyncOperation)

public:
    OpenReplicaAsyncOperation(
        ComPointer<ComGuestServiceReplica> & serviceReplica,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , serviceReplica_(serviceReplica)
    {
    }

    virtual ~OpenReplicaAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenReplicaAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        auto partition = make_com<ComDummyStatefulServicePartition, IFabricStatefulServicePartition2>();
        return serviceReplica_->BeginOpen(
            FABRIC_REPLICA_OPEN_MODE_NEW,
            partition.GetRawPointer(), 
            callback, 
            context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricReplicator> result;
        return serviceReplica_->EndOpen(context, result.InitializationAddress());
    }

private:
    ComPointer<ComGuestServiceReplica> & serviceReplica_;
};

class CloseReplicaAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(CloseReplicaAsyncOperation)

public:
    CloseReplicaAsyncOperation(
        ComPointer<ComGuestServiceReplica> & serviceReplica,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , serviceReplica_(serviceReplica)
    {
    }

    virtual ~CloseReplicaAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseReplicaAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        return serviceReplica_->BeginClose(callback, context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        return serviceReplica_->EndClose(context);
    }

private:
    ComPointer<ComGuestServiceReplica> & serviceReplica_;
};

class ChangeReplicaRoleAsyncOperation
    : public ComProxyAsyncOperation
{
    DENY_COPY(ChangeReplicaRoleAsyncOperation)

public:
    ChangeReplicaRoleAsyncOperation(
        ComPointer<ComGuestServiceReplica> & serviceReplica,
        FABRIC_REPLICA_ROLE newRole,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : ComProxyAsyncOperation(callback, parent)
        , serviceReplica_(serviceReplica)
        , newRole_(newRole)
    {
    }

    virtual ~ChangeReplicaRoleAsyncOperation() { }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<ChangeReplicaRoleAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    HRESULT BeginComAsyncOperation(
        IFabricAsyncOperationCallback * callback,
        IFabricAsyncOperationContext ** context)
    {
        return serviceReplica_->BeginChangeRole(newRole_, callback, context);
    }

    HRESULT EndComAsyncOperation(IFabricAsyncOperationContext * context)
    {
        ComPointer<IFabricStringResult> result;
        return serviceReplica_->EndChangeRole(context, result.InitializationAddress());
    }

private:
    ComPointer<ComGuestServiceReplica> & serviceReplica_;
    FABRIC_REPLICA_ROLE newRole_;
};

BOOST_FIXTURE_TEST_SUITE(OnDemandCodePackageActivationTestClassSuite,OnDemandCodePackageActivationTestClass)

// LINUXTODO: disable this test due to the CodePackage Name issue.
#if !defined(PLATFORM_UNIX)
BOOST_AUTO_TEST_CASE(BasicTest)
{
    for (auto const & servicePackageName : servicePackageNames_)
    {
        ServicePackageIdentifier servicePackageId(applicationId_, servicePackageName);

        VersionedServiceTypeIdentifier versionedServiceTypeId(
            ServiceTypeIdentifier(servicePackageId, *ActivatorServiceTypeName), PackageVersion);

        auto activationContext = ServicePackageActivationContext(Guid::NewGuid());
        activatedServiceTypeIds_.push_back(make_pair(versionedServiceTypeId, activationContext));

        this->ActivatePackage(versionedServiceTypeId, activationContext);

        wstring servicePackageActivationId;
        if (this->GetHosting().TryGetServicePackagePublicActivationId(
            servicePackageId,
            activationContext,
            servicePackageActivationId))
        {
            ASSERT("this->GetHosting().TryGetServicePackagePublicActivationId() returned false.");
        }

        lastActivatedPackage_ = ServicePackageInstanceIdentifier(
            servicePackageId,
            activationContext,
            servicePackageActivationId);

        this->VerifyRegistrations();

        if (servicePackageName == L"OnDemandCodePackageActivationTestService")
        {
            this->VerifyGuestServiceTypeHostCount(1);
            this->VerifyDeployedCodePackages(false);

            //
            // Guest app
            //
            GuestServiceTypeHostSPtr guestTypeHost;
            this->GetServiceTypeHost(guestTypeHost);

            this->VerifyOnDemandActivationWithStatelessServiceInstance(guestTypeHost);
            this->VerifyOnDemandActivationWithStatefulServiceReplica(guestTypeHost);
            this->VerifyActivatorCodePackageTermination(guestTypeHost);
        }
        else
        {
            // External activator code pacakge
            this->VerifyGuestServiceTypeHostCount(0);
            this->VerifyOnDemandActivation();
            this->TerminateRandomCodePackage(true);
            this->VerifyOnDemandActivation();
        }

        for (auto const & item : activatedServiceTypeIds_)
        {
            Trace.WriteNoise(TraceType, "Decrementing usage count for ServiceType {0}.", item);
            this->GetHosting().DecrementUsageCount(item.first.Id, item.second);
        }

        this->VerifyRegistrationsRemoved();
        this->VerifyGuestServiceTypeHostCount(0);
    }
}
#endif

BOOST_AUTO_TEST_SUITE_END()

void OnDemandCodePackageActivationTestClass::ActivatePackage(
    VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    ServicePackageActivationContext const & activationContext)
{
    bool done = false;
    int tries = 0;
    uint64 sequenceNumber;
    ServiceTypeRegistrationSPtr registration;
    bool success = false;
    while (!done)
    {
        auto error = this->GetHosting().FindServiceTypeRegistration(
            versionedServiceTypeId, 
            serviceDescription_,
            activationContext,
            sequenceNumber,
            registration);

        if (!error.IsSuccess())
        {
            tries++;
            if (tries == 10)
            {
                Trace.WriteError(TraceType, "Failed to activate ServiceType {0}. ErrorCode={1}", versionedServiceTypeId, error);
                success = false;
                done = false;
            }
            else
            {
                Trace.WriteNoise(TraceType, "Waiting for ServiceType {0} get registered: ErrorCode={1}", versionedServiceTypeId, error);
                ::Sleep(5000);
            }
        }
        else
        {
            ::Sleep(5000); // ensure that all types are registered

            success = true;
            done = true;

            Trace.WriteNoise(TraceType, "Incrementing usage count for ServiceType {0}.", versionedServiceTypeId);

            error = this->GetHosting().IncrementUsageCount(versionedServiceTypeId.Id, activationContext);

            VERIFY_IS_TRUE(error.IsSuccess(), L"IncrementUsageCount");
        }
    } // Each Package activation loop.
}

void OnDemandCodePackageActivationTestClass::VerifyOnDemandActivation()
{
    auto activatorCodePackage = L"Code1";
    vector<DeployedCodePackageQueryResult> codePackageResult;

    this->WaitforDeployedCodePackageCount(1, 5, codePackageResult);
    this->VerifyDeployedCodePackagesResult(codePackageResult, activatorCodePackage);

    this->WaitforDeployedCodePackageCount(5, 20, codePackageResult);
    this->VerifyDeployedCodePackages(true);

    this->WaitforDeployedCodePackageCount(1, 20, codePackageResult);
    this->VerifyDeployedCodePackagesResult(codePackageResult, activatorCodePackage);
}

void OnDemandCodePackageActivationTestClass::WaitforDeployedCodePackageCount(
    int resultCount,
    int retryCount,
    vector<DeployedCodePackageQueryResult> & result)
{
    int retry = 0;
    while (true)
    {
        this->GetDeployedCodePackagesResult(L"", result);
        if (result.size() == resultCount)
        {
            return;
        }

        if (retry == retryCount)
        {
            TRACE_LEVEL_AND_ASSERT(
                Trace.WriteError,
                TraceType,
                "WaitforDeployedCodePackageCount(): Failed. ResultCount={0}, RetryCount={1}, ResultSize={2}.",
                resultCount,
                retryCount,
                result.size());
        }

        retry++;
        ::Sleep(2000);
    }
}

void OnDemandCodePackageActivationTestClass::VerifyOnDemandActivationWithStatelessServiceInstance(
    GuestServiceTypeHostSPtr & guestTypeHost)
{
    //
    // Open -> Close
    //
    auto serviceInstance = make_com<ComGuestServiceInstance>(
        *(guestTypeHost),
        serviceDescription_.Name,
        *ActivatorServiceTypeName,
        Guid::NewGuid(),
        123456);

    this->OpenServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    this->CloseServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(false);
    ::Sleep(5000);
    this->VerifyCodePackageTracking(guestTypeHost, false);

    //
    // Open -> fault -> Close
    //
    serviceInstance = make_com<ComGuestServiceInstance>(
        *(guestTypeHost),
        serviceDescription_.Name,
        *ActivatorServiceTypeName,
        Guid::NewGuid(),
        123456);

    this->OpenServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    this->TerminateRandomCodePackage(false);
    ::Sleep(10000);
    auto isFaulted = serviceInstance->Test_IsInstanceFaulted();
    ASSERT_IFNOT(isFaulted, "ServiceInstance did not fault on CodePackage termination.");

    this->CloseServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    //
    // Open -> Close
    //
    serviceInstance = make_com<ComGuestServiceInstance>(
        *(guestTypeHost),
        serviceDescription_.Name,
        *ActivatorServiceTypeName,
        Guid::NewGuid(),
        123456);

    this->OpenServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    this->CloseServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(false);
    ::Sleep(5000);
    this->VerifyCodePackageTracking(guestTypeHost, false);
}

void OnDemandCodePackageActivationTestClass::VerifyOnDemandActivationWithStatefulServiceReplica(
    GuestServiceTypeHostSPtr & guestTypeHost)
{
    auto serviceReplica = make_com<ComGuestServiceReplica>(
        *(guestTypeHost),
        serviceDescription_.Name,
        *ActivatorServiceTypeName,
        Guid::NewGuid(),
        123456);

    this->OpenServiceReplica(serviceReplica);

    //
    // Should not activate code packages
    //
    this->ChangeServiceReplicaRole(serviceReplica, FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
    this->VerifyDeployedCodePackages(false);
    this->VerifyCodePackageTracking(guestTypeHost, false);

    //
    // Should activate code packages
    //
    this->ChangeServiceReplicaRole(serviceReplica, FABRIC_REPLICA_ROLE_PRIMARY);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    //
    // Should deactivate code packages
    //
    this->ChangeServiceReplicaRole(serviceReplica, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
    this->VerifyDeployedCodePackages(false);
    ::Sleep(5000);
    this->VerifyCodePackageTracking(guestTypeHost, false);

    //
    // Should activate code packages
    //
    this->ChangeServiceReplicaRole(serviceReplica, FABRIC_REPLICA_ROLE_PRIMARY);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    this->TerminateRandomCodePackage(false);
    ::Sleep(10000);
    auto isFaulted = serviceReplica->Test_IsReplicaFaulted();
    ASSERT_IFNOT(isFaulted, "ServiceInstance did not fault on CodePackage termination.");

    //
    // Should skip deactivating code packages
    //
    this->CloseServiceReplica(serviceReplica);
    this->VerifyDeployedCodePackages(true);
    ::Sleep(5000);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    //
    // Create new replica
    //
    serviceReplica = make_com<ComGuestServiceReplica>(
        *(guestTypeHost),
        serviceDescription_.Name,
        *ActivatorServiceTypeName,
        Guid::NewGuid(),
        123456);

    this->OpenServiceReplica(serviceReplica);

    //
    // Activate would be no-op
    //
    this->ChangeServiceReplicaRole(serviceReplica, FABRIC_REPLICA_ROLE_PRIMARY);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    //
    // Should deactivate code packages
    //
    this->ChangeServiceReplicaRole(serviceReplica, FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
    this->VerifyDeployedCodePackages(false);
    ::Sleep(5000);
    this->VerifyCodePackageTracking(guestTypeHost, false);
}

void OnDemandCodePackageActivationTestClass::VerifyActivatorCodePackageTermination(
    GuestServiceTypeHostSPtr & guestTypeHost)
{
    auto serviceInstance = make_com<ComGuestServiceInstance>(
        *(guestTypeHost),
        serviceDescription_.Name,
        *ActivatorServiceTypeName,
        Guid::NewGuid(),
        123456);

    this->OpenServiceInstance(serviceInstance);
    this->VerifyDeployedCodePackages(true);
    this->VerifyCodePackageTracking(guestTypeHost, true);

    this->TerminateRandomCodePackage(true);
    ::Sleep(10000);
    this->VerifyDeployedCodePackages(false);

    GuestServiceTypeHostSPtr newGuestTypeHost;
    this->GetServiceTypeHost(newGuestTypeHost);
    CODING_ERROR_ASSERT(guestTypeHost->HostContext.HostId != newGuestTypeHost->HostContext.HostId);
}

void OnDemandCodePackageActivationTestClass::TerminateRandomCodePackage(
    bool activatorCodePackage)
{
    auto const & activationTable = this->GetApplicationHostActivationTable();

    HostProxyMap proxyMap;
    activationTable->Test_GetHostProxyMap(proxyMap);

    for (auto const & kvPair : proxyMap)
    {
        if ((kvPair.second->Context.IsCodePackageActivatorHost && activatorCodePackage) ||
            (!kvPair.second->Context.IsCodePackageActivatorHost && !activatorCodePackage))
        {
            kvPair.second->TerminateExternally();
            return;
        }
    }

    ASSERT_IF(true, "No dependent ApplicationHostProxy found in ActivationTable");
}

void OnDemandCodePackageActivationTestClass::VerifyCodePackageTracking(
    GuestServiceTypeHostSPtr & guestTypeHost,
    bool active)
{
    CodePackageEventTracker eventTracker;
    guestTypeHost->ActivationManager->Test_GetCodePackageEventTracker(eventTracker);

    for (auto const & kvPair : eventTracker)
    {
        ASSERT_IFNOT(
            kvPair.second.first == active,
            "CodePackageEventTracker has CP={0} tracked as {1}. Expected={2}.",
            kvPair.first,
            kvPair.second.first,
            active);
    }
}

void OnDemandCodePackageActivationTestClass::OpenServiceInstance(
    ComPointer<ComGuestServiceInstance> & serviceInstance)
{
    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<OpenInstanceAsyncOperation>(
        serviceInstance,
        [this, waiter](AsyncOperationSPtr const & operation)
        {
            auto error = OpenInstanceAsyncOperation::End(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();

    Trace.WriteNoise(
        TraceType, 
        "OpenServiceInstance: ErrorCode={0}", error);

    ASSERT_IFNOT(error.IsSuccess(), "OpenServiceInstance()");
}

void OnDemandCodePackageActivationTestClass::CloseServiceInstance(
    ComPointer<ComGuestServiceInstance> & serviceInstance)
{
    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<CloseInstanceAsyncOperation>(
        serviceInstance,
        [this, waiter](AsyncOperationSPtr const & operation)
        {
            auto error = CloseInstanceAsyncOperation::End(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();

    Trace.WriteNoise(
        TraceType,
        "CloseServiceInstance: ErrorCode={0}", error);

    ASSERT_IFNOT(error.IsSuccess(), "CloseServiceInstance()");
}

void OnDemandCodePackageActivationTestClass::OpenServiceReplica(
    ComPointer<ComGuestServiceReplica> & serviceReplica)
{
    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<OpenReplicaAsyncOperation>(
        serviceReplica,
        [this, waiter](AsyncOperationSPtr const & operation)
        {
            auto error = OpenReplicaAsyncOperation::End(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();

    Trace.WriteNoise(
        TraceType,
        "OpenServiceReplica: ErrorCode={0}", error);

    ASSERT_IFNOT(error.IsSuccess(), "OpenServiceReplica()");
}

void OnDemandCodePackageActivationTestClass::CloseServiceReplica(
    ComPointer<ComGuestServiceReplica> & serviceReplica)
{
    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<CloseReplicaAsyncOperation>(
        serviceReplica,
        [this, waiter](AsyncOperationSPtr const & operation)
        {
            auto error = CloseReplicaAsyncOperation::End(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();

    Trace.WriteNoise(
        TraceType,
        "CloseServiceReplica: ErrorCode={0}", error);

    ASSERT_IFNOT(error.IsSuccess(), "CloseServiceReplica()");
}

void OnDemandCodePackageActivationTestClass::ChangeServiceReplicaRole(
    ComPointer<ComGuestServiceReplica> & serviceReplica,
    FABRIC_REPLICA_ROLE newRole)
{
    auto waiter = make_shared<AsyncOperationWaiter>();

    auto operation = AsyncOperation::CreateAndStart<ChangeReplicaRoleAsyncOperation>(
        serviceReplica,
        newRole,
        [this, waiter](AsyncOperationSPtr const & operation)
        {
            auto error = ChangeReplicaRoleAsyncOperation::End(operation);
            waiter->SetError(error);
            waiter->Set();
        },
        AsyncOperationSPtr());

    waiter->WaitOne();
    auto error = waiter->GetError();

    Trace.WriteNoise(
        TraceType,
        "ChangeServiceReplicaRole: ErrorCode={0}", error);

    ASSERT_IFNOT(error.IsSuccess(), "ChangeServiceReplicaRole()");
}

void OnDemandCodePackageActivationTestClass::VerifyRegistrations()
{
    expectedRegistrations_.clear();

    for (auto const & serviceType : typesPerPackage_)
    {
        ServiceTypeIdentifier serviceTypeId(lastActivatedPackage_.ServicePackageId, serviceType.ServiceTypeName);

        expectedRegistrations_.push_back(
            make_pair(VersionedServiceTypeIdentifier(serviceTypeId, PackageVersion), lastActivatedPackage_));
    }

    auto const & runtimeManager = GetFabricRuntimeManager();

    int tries = 0;

    for(auto iter = expectedRegistrations_.begin(); iter != expectedRegistrations_.end(); ++iter)
    {
        ServiceTypeInstanceIdentifier serviceTypeInstanceId(
            iter->first.Id,
            iter->second.ActivationContext,
            iter->second.PublicActivationId);

        ServiceTypeState state;
        auto error = runtimeManager->ServiceTypeStateManagerObj->Test_GetStateIfPresent(serviceTypeInstanceId, state);

        VERIFY_IS_TRUE(error.IsSuccess(), L"VerifyRegistrations() -- ErrorCode");

        if (state.Status != ServiceTypeStatus::Registered_Enabled)
        {
            Trace.WriteNoise(TraceType, "VerifyRegistrations(): WAITING => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);

            iter--;
            tries++;

            if (tries == 10)
            {
                Trace.WriteError(TraceType, "VerifyRegistrations(): WAITING FAILED => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);
                ASSERT_IF(true, "VerifyRegistrations(): WAITING FAILED => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);
            }

            ::Sleep(5000);
        }
        else
        {
            Trace.WriteNoise(TraceType, "VerifyRegistrations(): SUCCESS => ServiceTypeInstanceId=[{0}], State=[{1}]", serviceTypeInstanceId, state);

            expectedRuntimeIds_.insert(state.RuntimeId);
        }
    }
}

void OnDemandCodePackageActivationTestClass::VerifyGuestServiceTypeHostCount(size_t expectedCount)
{
    size_t typeHostCount;
    auto error = this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCount);

    ASSERT_IFNOT(error.IsSuccess(), "this->GetTypeHostManager()->Test_GetTypeHostCount(typeHostCount)");

    Trace.WriteNoise(TraceType, "typeHostCount={0}, expectedCount={1}.", typeHostCount, expectedCount);

    ASSERT_IFNOT(typeHostCount == expectedCount, "typeHostCount == expectedCount");
}

void OnDemandCodePackageActivationTestClass::GetDeployedCodePackagesResult(
    wstring const & codePackageName,
    vector<DeployedCodePackageQueryResult> & codePackageResult)
{
    QueryArgumentMap queryArgMap;
    queryArgMap.Insert(QueryResourceProperties::Application::ApplicationName, applicationName_);
    queryArgMap.Insert(QueryResourceProperties::ServiceType::ServiceManifestName, L"");
    queryArgMap.Insert(QueryResourceProperties::CodePackage::CodePackageName, codePackageName);

    QueryResult queryResult;
    auto error = this->GetHostingQueryManager()->GetCodePackageListDeployedOnNode(queryArgMap, ActivityId(), queryResult);

    ASSERT_IF(!error.IsSuccess(), "VerifyDeployedCodePackages-GetServiceManifestListDeployedOnNode-ErrorCode");

    error = queryResult.MoveList(codePackageResult);

    ASSERT_IF(!error.IsSuccess(), "VerifyDeployedCodePackages-queryResult.MoveList-ErrorCode");
}


void OnDemandCodePackageActivationTestClass::VerifyDeployedCodePackages(bool checkPresense)
{
    for (auto const & codePkg : codePackagesPerServicePackage_)
    {
        auto codePackageName = codePkg.Name;

        vector<DeployedCodePackageQueryResult> codePackageResult;
        this->GetDeployedCodePackagesResult(codePackageName, codePackageResult);

        if (checkPresense)
        {
            this->VerifyDeployedCodePackagesResult(codePackageResult, codePackageName);
        }
        else
        {
            ASSERT_IF(codePackageResult.size() != 0, "codePackageResult.size() != 0");
        }
    }
}

void OnDemandCodePackageActivationTestClass::VerifyDeployedCodePackagesResult(
    vector<DeployedCodePackageQueryResult> const & codePackageResult,
    wstring const & codePackageName)
{
    Trace.WriteNoise(
        TraceType,
        "VerifyDeployedCodePackages() - QueryResultCount=[{0}].",
        codePackageResult.size());

    ASSERT_IF(codePackageResult.size() != 1, "codePackageResult.size() != 1");

    auto codePkgRes = codePackageResult[0];

    if (codePkgRes.CodePackageName != codePackageName)
    {
        TRACE_LEVEL_AND_ASSERT(
            Trace.WriteError,
            TraceType, 
            "VerifyDeployedCodePackages() - MISMATCH CodePackageName. CodePackageResult=[{0}].",
            codePkgRes);
    }

    if (codePkgRes.HostType != HostType::ExeHost)
    {
        TRACE_LEVEL_AND_ASSERT(
            Trace.WriteError,
            TraceType, 
            "VerifyDeployedCodePackages() - UNEXPECTED HostType. CodePackageResult=[{0}].", 
            codePkgRes);
    }

    if (codePkgRes.HostIsolationMode != HostIsolationMode::None)
    {
        TRACE_LEVEL_AND_ASSERT(
            Trace.WriteError, 
            TraceType, 
            "VerifyDeployedCodePackages() - UNEXPECTED HostIsolationMode. CodePackageResult=[{0}].", 
            codePkgRes);
    }

    if (codePkgRes.ServiceManifestName != lastActivatedPackage_.ServicePackageName ||
        codePkgRes.ServicePackageActivationId != lastActivatedPackage_.PublicActivationId)
    {
        TRACE_LEVEL_AND_ASSERT(
            Trace.WriteError,
            TraceType, 
            "VerifyDeployedCodePackages() - MISSING FROM QUERY ServicePackage=[{0}].",
            lastActivatedPackage_);
    }
}

void OnDemandCodePackageActivationTestClass::VerifyRegistrationsRemoved()
{
    ::Sleep(10000); // ensure that all types are unregistered

    FabricRuntimeManagerUPtr const & runtimeManager = GetFabricRuntimeManager();

    int tries = 0;
    do
    {
        // Ensure that the ServicePackage is deactivated
        ServiceTypeState state;
        auto error = runtimeManager->ServiceTypeStateManagerObj->Test_GetStateIfPresent(
            ServiceTypeInstanceIdentifier(
                expectedRegistrations_[0].first.Id,
                expectedRegistrations_[0].second.ActivationContext,
                expectedRegistrations_[0].second.PublicActivationId),
            state);

        if(error.IsError(ErrorCodeValue::NotFound))
        {
            break;
        }

        ::Sleep(5000);
    }while(++tries < 10);


    for(auto iter = this->expectedRegistrations_.begin();  iter != this->expectedRegistrations_.end(); ++iter)
    {
        ServiceTypeState state;
        ServiceTypeInstanceIdentifier serviceTypeInstanceId(
            iter->first.Id,
            iter->second.ActivationContext,
            iter->second.PublicActivationId);

        auto error = runtimeManager->ServiceTypeStateManagerObj->Test_GetStateIfPresent(serviceTypeInstanceId, state);

        Trace.WriteNoise(
            TraceType,
            "Get registration for {0} (ErrorCode={1}, State={2})",
            serviceTypeInstanceId,
            error,
            state);

        ASSERT_IF(!error.IsError(ErrorCodeValue::NotFound), "VerifyExpectedRegistration-ErrorCode");
    }
}

void OnDemandCodePackageActivationTestClass::GetServiceTypeHost(
    GuestServiceTypeHostSPtr & guestTypeHost)
{
    auto error = this->GetTypeHostManager()->Test_GetTypeHost(
        lastActivatedPackage_,
        guestTypeHost);

    ASSERT_IFNOT(error.IsSuccess(), "this->GetTypeHostManager()->Test_GetTypeHost(...)");
}

HostingSubsystem & OnDemandCodePackageActivationTestClass::GetHosting()
{
    return this->fabricNodeHost_->GetHosting();
}

FabricRuntimeManagerUPtr const & OnDemandCodePackageActivationTestClass::GetFabricRuntimeManager()
{
    return this->GetHosting().FabricRuntimeManagerObj;
}

HostingQueryManagerUPtr & OnDemandCodePackageActivationTestClass::GetHostingQueryManager()
{
    return this->GetHosting().Test_GetHostingQueryManager();
}

GuestServiceTypeHostManagerUPtr const & OnDemandCodePackageActivationTestClass::GetTypeHostManager()
{
    return this->GetHosting().GuestServiceTypeHostManagerObj;
}

ApplicationHostActivationTableUPtr const & OnDemandCodePackageActivationTestClass::GetApplicationHostActivationTable()
{
    return this->GetHosting().ApplicationHostManagerObj->Test_ApplicationHostActivationTable;
}

bool OnDemandCodePackageActivationTestClass::Setup()
{
    HostingConfig::GetConfig().EnableActivateNoWindow = true;
    HostingConfig::GetConfig().EndpointProviderEnabled = true;
    HostingConfig::GetConfig().DeployedServiceFailoverContinuousFailureThreshold = 1;

    wstring nttree;
    if (!Environment::GetEnvironmentVariableW(L"_NTTREE", nttree, Common::NOTHROW()))
    {
        nttree = Directory::GetCurrentDirectoryW();
    }

    if (!this->fabricNodeHost_->Open()) { return false; }

    // copy OnDemandCodePackageActivationTestHost.exe to the code packages of the application
    StoreLayoutSpecification storeLayout(this->fabricNodeHost_->GetImageStoreRoot());
    RunLayoutSpecification runLayout(this->fabricNodeHost_->GetDeploymentRoot());

#if !defined(PLATFORM_UNIX)
    auto OnDemandCodePackageActivationHostExePath = 
        Path::Combine(this->fabricNodeHost_->GetTestDataRoot(), L"..\\WorkingFolderTestHost.exe");
#else
    auto OnDemandCodePackageActivationHostExePath = 
        Path::Combine(this->fabricNodeHost_->GetTestDataRoot(), L"../WorkingFolderTestHost.exe");
#endif

    for (auto const & servicePackageName : servicePackageNames_)
    {
       auto serviceManifestFile = storeLayout.GetServiceManifestFile(
           *OnDemandCodePackageActivationAppTypeName, 
           servicePackageName,
           *ManifestVersion);

       ServiceManifestDescription serviceManifest;
       auto error = ServiceModel::Parser::ParseServiceManifest(serviceManifestFile, serviceManifest);
       if (!error.IsSuccess())
       {
           Trace.WriteError(
               TraceType,
               "Failed to parse ServiceManifest file {0}. ErrorCode={1}",
               serviceManifestFile,
               error);
           return false;
       }

       wstring programPath;
       for (auto const & codePkg : serviceManifest.CodePackages)
       {
           programPath = GetProgramPath(storeLayout, serviceManifest.Name, codePkg.Name, codePkg.EntryPoint.ExeEntryPoint);
           auto directory = Path::GetDirectoryName(programPath);
           if (!Directory::Exists(directory))
           {
               Directory::Create(directory);
           }

           error = File::Copy(OnDemandCodePackageActivationHostExePath, programPath, true);
           if (!error.IsSuccess())
           {
               Trace.WriteError(
                   TraceType,
                   "Failed to copy OnDemandCodePackageActivationTestHost.exe to {0}. ErrorCode={1}",
                   programPath,
                   error);
               return false;
           }
       }

       typesPerPackage_ = serviceManifest.ServiceTypeNames;
       codePackagesPerServicePackage_ = serviceManifest.CodePackages;
    }

    return true;
}

bool OnDemandCodePackageActivationTestClass::Cleanup()
{
    return fabricNodeHost_->Close();
}

 wstring OnDemandCodePackageActivationTestClass::GetProgramPath(
     Management::ImageModel::StoreLayoutSpecification const & storeLayout,
     wstring const & serviceManifestName,
     wstring const & codePackageName,
     ExeEntryPointDescription const & entryPoint)
 {
    auto programPath = storeLayout.GetCodePackageFolder(
             *OnDemandCodePackageActivationAppTypeName,
             serviceManifestName,
             codePackageName,
             *ManifestVersion);
    programPath = Path::Combine(programPath, entryPoint.Program);
    return programPath;
}
