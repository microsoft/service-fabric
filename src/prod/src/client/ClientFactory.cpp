// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace Transport;

ClientFactory::ClientFactory()
    : fabricClientSPtr_()
    , lock_()
{
}

ClientFactory::ClientFactory(FabricClientImplSPtr const & fabricClientImpl)
    : fabricClientSPtr_(fabricClientImpl)
    , lock_()
{
}

ClientFactory::~ClientFactory()
{
    if (fabricClientSPtr_)
    {
        fabricClientSPtr_->Close();
    }
}

ErrorCode ClientFactory::CreateClientFactory(
    USHORT connectionStringSize,
    __deref_in_ecount(connectionStringSize) LPCWSTR const *connectionStrings,
    __out IClientFactoryPtr &clientFactoryPtr)
{
    return CreateClientFactory(
        connectionStringSize,
        connectionStrings,
        IServiceNotificationEventHandlerPtr(),
        IClientConnectionEventHandlerPtr(),
        clientFactoryPtr);
}

ErrorCode ClientFactory::CreateClientFactory(
    USHORT connectionStringSize,
    __deref_in_ecount(connectionStringSize) LPCWSTR const *connectionStrings,
    IServiceNotificationEventHandlerPtr const & notificationHandler,
    IClientConnectionEventHandlerPtr const & connectionHandler,
    __out IClientFactoryPtr &clientFactoryPtr)
{
    if (connectionStringSize == 0 || connectionStrings == NULL)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    vector<wstring> connectionStringList;
    auto error = ParseConnectionStringsParameter(
        connectionStringSize,
        connectionStrings,
        connectionStringList);

    if (!error.IsSuccess()) { return error; }

    return CreateClientFactory(
        make_shared<FabricClientImpl>(
            move(connectionStringList),
            notificationHandler,
            connectionHandler),
        clientFactoryPtr);
}

ErrorCode ClientFactory::CreateClientFactory(
    std::vector<std::wstring> &&connectionStringList,
    __out IClientFactoryPtr &clientFactoryPtr)
{
    return CreateClientFactory(make_shared<FabricClientImpl>(move(connectionStringList)), clientFactoryPtr);
}

ErrorCode ClientFactory::CreateClientFactory(
    FabricClientImplSPtr const & fabricClientImpl,
    __out IClientFactoryPtr & clientFactoryPtr)
{
    shared_ptr<ClientFactory> clientFactorySPtr(new ClientFactory(fabricClientImpl));

    clientFactoryPtr = RootedObjectPointer<IClientFactory>(clientFactorySPtr.get(), clientFactorySPtr->CreateComponentRoot());

    return ErrorCode::Success();
}

ErrorCode ClientFactory::CreateLocalClientFactory(
    FabricNodeConfigSPtr const& config,
    __out IClientFactoryPtr &clientFactoryPtr)
{
    return CreateLocalClientFactory(config, RoleMask::None, clientFactoryPtr);
}

ErrorCode ClientFactory::CreateLocalClientFactory(
    FabricNodeConfigSPtr const& config,
    RoleMask::Enum role,
    __out IClientFactoryPtr &clientFactoryPtr)
{
    return CreateClientFactory(make_shared<FabricClientImpl>(config, role), clientFactoryPtr);
}

ErrorCode ClientFactory::CreateLocalClientFactory(
    FabricNodeConfigSPtr const& config,
    RoleMask::Enum clientRole,
    IServiceNotificationEventHandlerPtr const & notificationHandler,
    IClientConnectionEventHandlerPtr const & connectionHandler,
    __out Api::IClientFactoryPtr &clientFactoryPtr)
{
    return CreateClientFactory(
        make_shared<FabricClientImpl>(
            config,
            clientRole,
            notificationHandler,
            connectionHandler),
        clientFactoryPtr);
}

ErrorCode ClientFactory::CreateLocalClientFactory(
    IServiceNotificationEventHandlerPtr const & notificationHandler,
    INamingMessageProcessorSPtr const & namingMessageProcessor,
    __out Api::IClientFactoryPtr &clientFactoryPtr)
{
    return CreateClientFactory(
        make_shared<FabricClientImpl>(
            namingMessageProcessor,
            notificationHandler,
            IClientConnectionEventHandlerPtr()),
        clientFactoryPtr);
}

ErrorCode ClientFactory::CreateLocalClientFactory(
    FabricNodeConfigSPtr const& config,
    RoleMask::Enum clientRole,
    bool isClientRoleAuthorized,
    __out IClientFactoryPtr &clientFactoryPtr)
{
    return CreateClientFactory(make_shared<FabricClientImpl>(config, clientRole, isClientRoleAuthorized), clientFactoryPtr);
}

ErrorCode ClientFactory::ParseConnectionStringsParameter(
    USHORT connectionStringsSize,
    LPCWSTR const *connectionStrings,
    __inout std::vector<std::wstring> & connectionStringList)
{
    if (connectionStrings == NULL)
    {
        return ErrorCodeValue::InvalidAddress;
    }

    if (connectionStringsSize < 0)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    for(size_t i = 0; i < connectionStringsSize; i++)
    {
        auto rawConnectionString = connectionStrings[i];
        auto error = ParameterValidator::IsValid(
            rawConnectionString,
            ParameterValidator::MinStringSize,
            ParameterValidator::MaxConnectionStringSize);

        if (!error.IsSuccess())
        {
            return error;
        }

        wstring connectionString(rawConnectionString);
        connectionStringList.push_back(move(connectionString));
    }

    return ErrorCode::Success();
}

ServiceModel::FabricClientSettings ClientFactory::GetFabricClientDefaultSettings()
{
    wstring traceId(Guid::Empty().ToString());
    FabricClientInternalSettings settings(traceId);
    return settings.GetSettings();
}

ErrorCode ClientFactory::ValidateFabricClient()
{
    if (!fabricClientSPtr_)
    {
        return ErrorCodeValue::NotImplemented;
    }

    if (fabricClientSPtr_->State.Value == FabricComponentState::Closing ||
        fabricClientSPtr_->State.Value == FabricComponentState::Closed ||
        fabricClientSPtr_->State.Value == FabricComponentState::Aborted)
    {
        return ErrorCodeValue::FabricComponentAborted;
    }

    return ErrorCode::Success();
}

ErrorCode ClientFactory::CreateQueryClient(__out IQueryClientPtr &queryClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        queryClientPtr = RootedObjectPointer<IQueryClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateSettingsClient(__out IClientSettingsPtr &clientSettingsPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientSettingsPtr = RootedObjectPointer<IClientSettings>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateServiceManagementClient(__out IServiceManagementClientPtr &servicemgmtPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        servicemgmtPtr = RootedObjectPointer<IServiceManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }
    return error;
}

ErrorCode ClientFactory::CreateApplicationManagementClient(__out IApplicationManagementClientPtr &applicationmgmtPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        applicationmgmtPtr = RootedObjectPointer<IApplicationManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateClusterManagementClient(__out Api::IClusterManagementClientPtr &clustermgmtPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clustermgmtPtr = RootedObjectPointer<IClusterManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateHealthClient(__out IHealthClientPtr &healthClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        healthClientPtr = RootedObjectPointer<IHealthClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreatePropertyManagementClient(__out IPropertyManagementClientPtr &propertyMgmtClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        propertyMgmtClientPtr = RootedObjectPointer<IPropertyManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateServiceGroupManagementClient(__out IServiceGroupManagementClientPtr &serviceGroupMgmtClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        serviceGroupMgmtClientPtr = RootedObjectPointer<IServiceGroupManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateInfrastructureServiceClient(__out IInfrastructureServiceClientPtr &infraClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        infraClientPtr = RootedObjectPointer<IInfrastructureServiceClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateInternalInfrastructureServiceClient(__out IInternalInfrastructureServiceClientPtr &infraClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        infraClientPtr = RootedObjectPointer<IInternalInfrastructureServiceClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateTestManagementClient(__out ITestManagementClientPtr &testManagementClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        testManagementClientPtr = RootedObjectPointer<ITestManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateTestManagementClientInternal(__out ITestManagementClientInternalPtr &testManagementClientInternalPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        testManagementClientInternalPtr = RootedObjectPointer<ITestManagementClientInternal>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateFaultManagementClient(__out IFaultManagementClientPtr &faultManagementClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        faultManagementClientPtr = RootedObjectPointer<IFaultManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateImageStoreClient(__out IImageStoreClientPtr & fileImageStoreClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        fileImageStoreClientPtr = RootedObjectPointer<IImageStoreClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateFileStoreServiceClient(__out IFileStoreServiceClientPtr & fileStoreServiceClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        fileStoreServiceClientPtr = RootedObjectPointer<IFileStoreServiceClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateInternalFileStoreServiceClient(__out IInternalFileStoreServiceClientPtr & fileStoreServiceClientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        fileStoreServiceClientPtr = RootedObjectPointer<IInternalFileStoreServiceClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateTestClient(__out ITestClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<ITestClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateInternalTokenValidationServiceClient(__out IInternalTokenValidationServiceClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IInternalTokenValidationServiceClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

_Use_decl_annotations_
ErrorCode ClientFactory::CreateAccessControlClient(IAccessControlClientPtr & clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IAccessControlClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateRepairManagementClient(__out IRepairManagementClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IRepairManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateComposeManagementClient(__out IComposeManagementClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IComposeManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateResourceManagementClient(__out IResourceManagementClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IResourceManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateSecretStoreClient(__out ISecretStoreClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<ISecretStoreClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateResourceManagerClient(__out IResourceManagerClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IResourceManagerClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateNetworkManagementClient(__out INetworkManagementClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<INetworkManagementClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }

    return error;
}

ErrorCode ClientFactory::CreateGatewayResourceManagerClient(__out IGatewayResourceManagerClientPtr &clientPtr)
{
    auto error = ValidateFabricClient();
    if (error.IsSuccess())
    {
        clientPtr = RootedObjectPointer<IGatewayResourceManagerClient>(
            fabricClientSPtr_.get(),
            this->CreateComponentRoot());
    }
    return error;
}
