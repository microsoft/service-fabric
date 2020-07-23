//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#include "FabricRuntime.h"
#include "Common/Common.h"
#include "FabricNode/fabricnode.h"
#include "Hosting2/FabricNodeHost.Test.h"
#include "api/definitions/ApiDefinitions.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Fabric;
using namespace ServiceModel;
using namespace Management;
using namespace Management::CentralSecretService;
using namespace Client;
using namespace Api;

const StringLiteral TraceType("LocalSecretServiceManagerTest");

class DummySecretStoreClient :
    public Api::ISecretStoreClient
{
public:
    DummySecretStoreClient() {};

    AsyncOperationSPtr BeginGetSecrets(
        GetSecretsDescription & getSecretsDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(getSecretsDescription);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndGetSecrets(
        Common::AsyncOperationSPtr const & operation,
        __out SecretsDescription & result)
    {
        UNREFERENCED_PARAMETER(result);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginSetSecrets(
        Management::CentralSecretService::SecretsDescription & secrets,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(secrets);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    ErrorCode EndSetSecrets(
        Common::AsyncOperationSPtr const & operation,
        __out Management::CentralSecretService::SecretsDescription & result)
    {
        UNREFERENCED_PARAMETER(result);
        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginRemoveSecrets(
        Management::CentralSecretService::SecretReferencesDescription & secretReferences,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(secretReferences);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    virtual Common::ErrorCode EndRemoveSecrets(
        Common::AsyncOperationSPtr const & operation,
        __out Management::CentralSecretService::SecretReferencesDescription & result)
    {
        UNREFERENCED_PARAMETER(result);
        return CompletedAsyncOperation::End(operation);
    }

    Common::AsyncOperationSPtr BeginGetSecretVersions(
        Management::CentralSecretService::SecretReferencesDescription & secretReferences,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(secretReferences);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }

    Common::ErrorCode EndGetSecretVersions(
        Common::AsyncOperationSPtr const & operation,
        __out Management::CentralSecretService::SecretReferencesDescription & result)
    {
        UNREFERENCED_PARAMETER(result);

        return CompletedAsyncOperation::End(operation);
    }

    AsyncOperationSPtr BeginSetCurrentSecrets(
        Management::CentralSecretService::SecretReferencesDescription & secretReferences,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const & parent)
    {
        UNREFERENCED_PARAMETER(secretReferences);
        UNREFERENCED_PARAMETER(timeout);
        UNREFERENCED_PARAMETER(callback);
        UNREFERENCED_PARAMETER(parent);

        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            ErrorCodeValue::Success,
            callback,
            parent);
    }
    virtual Common::ErrorCode EndSetCurrentSecrets(
        Common::AsyncOperationSPtr const & operation,
        __out Management::CentralSecretService::SecretReferencesDescription & result)
    {
        UNREFERENCED_PARAMETER(result);
        return CompletedAsyncOperation::End(operation);
    }
};

class TestClient
    : public Api::IClientFactory
    , public Common::ComponentRoot
{
    DENY_COPY(TestClient)
public:
    TestClient()
    {
        secretStoreclientSPtr_ = make_shared<DummySecretStoreClient>();
    }

    static ErrorCode CreateLocalClientFactory(
        __out IClientFactoryPtr &clientFactorPtr)
    {
        shared_ptr<TestClient> clientFactorySPtr(new TestClient());
        clientFactorPtr = RootedObjectPointer<IClientFactory>(clientFactorySPtr.get(), clientFactorySPtr->CreateComponentRoot());
        return ErrorCodeValue::Success;
    }

    ~TestClient() {};

    ErrorCode CreateQueryClient(__out Api::IQueryClientPtr & queryClientPtr)
    {
        queryClientPtr = RootedObjectPointer<IQueryClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateSettingsClient(__out IClientSettingsPtr &clientSettingsPtr)
    {
        clientSettingsPtr = RootedObjectPointer<IClientSettings>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateServiceManagementClient(__out IServiceManagementClientPtr &servicemgmtPtr)
    {
        servicemgmtPtr = RootedObjectPointer<IServiceManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateApplicationManagementClient(__out IApplicationManagementClientPtr &applicationmgmtPtr)
    {
        applicationmgmtPtr = RootedObjectPointer<IApplicationManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateClusterManagementClient(__out Api::IClusterManagementClientPtr &clustermgmtPtr)
    {
        clustermgmtPtr = RootedObjectPointer<IClusterManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateAccessControlClient(IAccessControlClientPtr & accessControlPtr)
    {
        accessControlPtr = RootedObjectPointer<IAccessControlClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateHealthClient(__out IHealthClientPtr &healthClientPtr)
    {
        healthClientPtr = RootedObjectPointer<IHealthClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreatePropertyManagementClient(__out IPropertyManagementClientPtr &propertyMgmtClientPtr)
    {
        propertyMgmtClientPtr = RootedObjectPointer<IPropertyManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateServiceGroupManagementClient(__out IServiceGroupManagementClientPtr &serviceGroupMgmtClientPtr)
    {
        serviceGroupMgmtClientPtr = RootedObjectPointer<IServiceGroupManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateInfrastructureServiceClient(__out IInfrastructureServiceClientPtr &infraClientPtr)
    {
        infraClientPtr = RootedObjectPointer<IInfrastructureServiceClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateInternalInfrastructureServiceClient(__out IInternalInfrastructureServiceClientPtr &infraClientPtr)
    {
        infraClientPtr = RootedObjectPointer<IInternalInfrastructureServiceClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateTestManagementClient(__out ITestManagementClientPtr &testManagementClientPtr)
    {
        testManagementClientPtr = RootedObjectPointer<ITestManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateTestManagementClientInternal(__out ITestManagementClientInternalPtr &testManagementClientInternalPtr)
    {
        testManagementClientInternalPtr = RootedObjectPointer<ITestManagementClientInternal>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateFaultManagementClient(__out IFaultManagementClientPtr &faultManagementClientPtr)
    {
        faultManagementClientPtr = RootedObjectPointer<IFaultManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateFileStoreServiceClient(__out IFileStoreServiceClientPtr & fileStoreServiceClientPtr)
    {
        fileStoreServiceClientPtr = RootedObjectPointer<IFileStoreServiceClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateInternalFileStoreServiceClient(__out Api::IInternalFileStoreServiceClientPtr &internalFileStoreClientPtr)
    {
        internalFileStoreClientPtr = RootedObjectPointer<IInternalFileStoreServiceClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateImageStoreClient(__out Api::IImageStoreClientPtr &imageStoreClientPtr)
    {
        imageStoreClientPtr = RootedObjectPointer<IImageStoreClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateTestClient(__out ITestClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<ITestClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateInternalTokenValidationServiceClient(__out IInternalTokenValidationServiceClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<IInternalTokenValidationServiceClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateRepairManagementClient(__out IRepairManagementClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<IRepairManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateComposeManagementClient(__out IComposeManagementClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<IComposeManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateSecretStoreClient(__out Api::ISecretStoreClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<Api::ISecretStoreClient>(secretStoreclientSPtr_.get(), this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateResourceManagerClient(__out Api::IResourceManagerClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<IResourceManagerClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateResourceManagementClient(__out Api::IResourceManagementClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<IResourceManagementClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

    ErrorCode CreateNetworkManagementClient(__out Api::INetworkManagementClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<INetworkManagementClient>(nullptr, this->CreateComponentRoot());
            return ErrorCode::Success();
    }

    ErrorCode CreateGatewayResourceManagerClient(__out Api::IGatewayResourceManagerClientPtr &clientPtr)
    {
        clientPtr = RootedObjectPointer<IGatewayResourceManagerClient>(nullptr, this->CreateComponentRoot());
        return ErrorCode::Success();
    }

private:
    std::shared_ptr<DummySecretStoreClient> secretStoreclientSPtr_;
};

class LocalSecretServiceManagerTest
{
protected:
    LocalSecretServiceManagerTest() : fabricNodeHost_(make_shared<TestFabricNodeHost>()) { BOOST_REQUIRE(Setup()); }
    TEST_CLASS_SETUP(Setup);
    ~LocalSecretServiceManagerTest() { BOOST_REQUIRE(Cleanup()); }
    TEST_CLASS_CLEANUP(Cleanup);

    LocalSecretServiceManager & GetLocalSecretServiceManager();
    shared_ptr<TestFabricNodeHost> fabricNodeHost_;
};

BOOST_FIXTURE_TEST_SUITE(LocalSecretServiceManagerTestSuite, LocalSecretServiceManagerTest)

BOOST_AUTO_TEST_CASE(VerifyLSSManagerParseIncorrectSecrets)
{
    wstring secretStoreRef1 = L"fabric/App1/Svc1/Password:";
    vector<wstring> secretStoreRefs1;
    secretStoreRefs1.push_back(secretStoreRef1);
    GetSecretsDescription getSecretsDesc;
    ErrorCode error = GetLocalSecretServiceManager().ParseSecretStoreRef(
        secretStoreRefs1,
        getSecretsDesc);
    VERIFY_IS_TRUE(!error.IsSuccess(), L"ParseSecretStoreRef succeeded for incorrect secretStoreRef.");

    wstring secretStoreRef2 = L"fabric/App1/Svc1/Password";
    vector<wstring> secretStoreRefs2;
    secretStoreRefs2.push_back(secretStoreRef2);
    error = GetLocalSecretServiceManager().ParseSecretStoreRef(
        secretStoreRefs2,
        getSecretsDesc);
    VERIFY_IS_TRUE(!error.IsSuccess(), L"ParseSecretStoreRef succeeded for incorrect secretStoreRef.");
}

BOOST_AUTO_TEST_CASE(VerifyLSSManagerParseAndGetSecrets)
{
    wstring secretStoreRef1 = L"fabric:/App1/Svc1/Password:V2";
    wstring secretStoreRef2 = L"fabric:/App1/Svc2/ConnectionString:V1";
    vector<wstring> secretStoreRefs;
    secretStoreRefs.push_back(secretStoreRef1);
    secretStoreRefs.push_back(secretStoreRef2);
    GetSecretsDescription getSecretsDesc;

    ErrorCode error = GetLocalSecretServiceManager().ParseSecretStoreRef(
        secretStoreRefs,
        getSecretsDesc);

    VERIFY_IS_TRUE(error.IsSuccess(), L"ParseSecretStoreRef failed to parse.");
    VERIFY_IS_TRUE(getSecretsDesc.IncludeValue, L"GetSecretsDescription included values.");
    VERIFY_IS_TRUE(getSecretsDesc.SecretReferences.size() == 2, L"getSecretsDesc.SecretReferences.size() is not 2");
    VERIFY_ARE_EQUAL(getSecretsDesc.SecretReferences[0].Name, L"fabric:/App1/Svc1/Password", L"getSecretsDesc.SecretReferences[0].Name does not match fabric:/App1/Svc1/Password");
    VERIFY_ARE_EQUAL(getSecretsDesc.SecretReferences[0].Version, L"V2", L"getSecretsDesc.SecretReferences[0].Version does not match V2");
    VERIFY_ARE_EQUAL(getSecretsDesc.SecretReferences[1].Name, L"fabric:/App1/Svc2/ConnectionString", L"getSecretsDesc.SecretReferences[1].Name does not match fabric:/App1/Svc2/ConnectionString");
    VERIFY_ARE_EQUAL(getSecretsDesc.SecretReferences[1].Version, L"V1", L"getSecretsDesc.SecretReferences[0].Version does not match V1");

    SecretsDescription secretValues;
    ManualResetEvent deployDone;
    GetLocalSecretServiceManager().BeginGetSecrets(
        getSecretsDesc,
        TimeSpan::MaxValue,
        [this, &deployDone, &secretValues](AsyncOperationSPtr const & operation)
    {
        auto error = GetLocalSecretServiceManager().EndGetSecrets(operation, secretValues);
        Trace.WriteNoise(TraceType, "EndGetSecrets Error = {0}", error);
        deployDone.Set();
    },
        AsyncOperationSPtr());

    VERIFY_IS_TRUE(deployDone.WaitOne(10000), L"DownloadServicePackageTest completed before timeout.");
}

BOOST_AUTO_TEST_SUITE_END()

LocalSecretServiceManager & LocalSecretServiceManagerTest::GetLocalSecretServiceManager()
{
    return *this->fabricNodeHost_->GetHosting().LocalSecretServiceManagerObj;
}

bool LocalSecretServiceManagerTest::Setup()
{
    CentralSecretServiceConfig::GetConfig().Test_Reset();
    CentralSecretServiceConfig::GetConfig().TargetReplicaSetSize = 3;

    if (!fabricNodeHost_->Open())
    {
        return false;
    }

    Api::ISecretStoreClientPtr secretStoreClientPtr;
    IClientFactoryPtr factoryPtr;
    TestClient::CreateLocalClientFactory(factoryPtr);
    factoryPtr->CreateSecretStoreClient(secretStoreClientPtr);
    this->fabricNodeHost_->GetHosting().Test_SetSecretStoreClient(secretStoreClientPtr);
    return true;
}

bool LocalSecretServiceManagerTest::Cleanup()
{
    auto result = fabricNodeHost_->Close();
    CentralSecretServiceConfig::GetConfig().Test_Reset();
    return result;
}
