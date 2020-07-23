// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestClientFactory.h"

using namespace std;
using namespace ClientTest;
using namespace Common;
using namespace Api;

TestClientFactory::TestClientFactory(__in FabricNodeConfigSPtr config)
{
    clientSPtr_ = make_shared<MockFabricClientImpl>(config);
}

ErrorCode TestClientFactory::CreateLocalClientFactory(
    __in FabricNodeConfigSPtr const& config,
    __out IClientFactoryPtr &clientFactorPtr)
{
    shared_ptr<TestClientFactory> clientFactorySPtr(new TestClientFactory(move(config)));

    clientFactorPtr = RootedObjectPointer<IClientFactory>(clientFactorySPtr.get(), clientFactorySPtr->CreateComponentRoot());

    return ErrorCodeValue::Success;
}

ErrorCode TestClientFactory::CreateQueryClient(__out Api::IQueryClientPtr & queryClientPtr)
{
    queryClientPtr = RootedObjectPointer<IQueryClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateSettingsClient(__out IClientSettingsPtr &clientSettingsPtr)
{
    clientSettingsPtr = RootedObjectPointer<IClientSettings>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateServiceManagementClient(__out IServiceManagementClientPtr &servicemgmtPtr)
{
    servicemgmtPtr = RootedObjectPointer<IServiceManagementClient>(clientSPtr_.get(), this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateApplicationManagementClient(__out IApplicationManagementClientPtr &applicationmgmtPtr)
{
    applicationmgmtPtr = RootedObjectPointer<IApplicationManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateClusterManagementClient(__out Api::IClusterManagementClientPtr &clustermgmtPtr)
{
    clustermgmtPtr = RootedObjectPointer<IClusterManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

_Use_decl_annotations_
ErrorCode TestClientFactory::CreateAccessControlClient(IAccessControlClientPtr & accessControlPtr)
{
    accessControlPtr = RootedObjectPointer<IAccessControlClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateHealthClient(__out IHealthClientPtr &healthClientPtr)
{
    healthClientPtr = RootedObjectPointer<IHealthClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreatePropertyManagementClient(__out IPropertyManagementClientPtr &propertyMgmtClientPtr)
{
    propertyMgmtClientPtr = RootedObjectPointer<IPropertyManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateServiceGroupManagementClient(__out IServiceGroupManagementClientPtr &serviceGroupMgmtClientPtr)
{
    serviceGroupMgmtClientPtr = RootedObjectPointer<IServiceGroupManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateInfrastructureServiceClient(__out IInfrastructureServiceClientPtr &infraClientPtr)
{
    infraClientPtr = RootedObjectPointer<IInfrastructureServiceClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateInternalInfrastructureServiceClient(__out IInternalInfrastructureServiceClientPtr &infraClientPtr)
{
    infraClientPtr = RootedObjectPointer<IInternalInfrastructureServiceClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateTestManagementClient(__out ITestManagementClientPtr &testManagementClientPtr)
{
    testManagementClientPtr = RootedObjectPointer<ITestManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateTestManagementClientInternal(__out ITestManagementClientInternalPtr &testManagementClientInternalPtr)
{
    testManagementClientInternalPtr = RootedObjectPointer<ITestManagementClientInternal>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateFaultManagementClient(__out IFaultManagementClientPtr &faultManagementClientPtr)
{
    faultManagementClientPtr = RootedObjectPointer<IFaultManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateFileStoreServiceClient(__out IFileStoreServiceClientPtr & fileStoreServiceClientPtr)
{
    fileStoreServiceClientPtr = RootedObjectPointer<IFileStoreServiceClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateInternalFileStoreServiceClient(__out Api::IInternalFileStoreServiceClientPtr &internalFileStoreClientPtr)
{
    internalFileStoreClientPtr = RootedObjectPointer<IInternalFileStoreServiceClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateImageStoreClient(__out Api::IImageStoreClientPtr &imageStoreClientPtr)
{
    imageStoreClientPtr = RootedObjectPointer<IImageStoreClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}


ErrorCode TestClientFactory::CreateTestClient(__out ITestClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<ITestClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateInternalTokenValidationServiceClient(__out IInternalTokenValidationServiceClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<IInternalTokenValidationServiceClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateRepairManagementClient(__out IRepairManagementClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<IRepairManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateComposeManagementClient(__out IComposeManagementClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<IComposeManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateSecretStoreClient(__out ISecretStoreClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<ISecretStoreClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateResourceManagerClient(__out Api::IResourceManagerClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<IResourceManagerClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateResourceManagementClient(__out IResourceManagementClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<IResourceManagementClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateNetworkManagementClient(__out INetworkManagementClientPtr &clientPtr)
{
	clientPtr = RootedObjectPointer<INetworkManagementClient>(nullptr, this->CreateComponentRoot());
	return ErrorCode::Success();
}

ErrorCode TestClientFactory::CreateGatewayResourceManagerClient(__out IGatewayResourceManagerClientPtr &clientPtr)
{
    clientPtr = RootedObjectPointer<IGatewayResourceManagerClient>(nullptr, this->CreateComponentRoot());
    return ErrorCode::Success();
}
