// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace std;
    using namespace Api;
    using namespace Common;
    using namespace Transport;
    using namespace ServiceModel;
    using namespace Query;
    using namespace Management::ClusterManager;
    using namespace Management::HealthManager;
    using namespace Management::InfrastructureService;
    using namespace Management::FileStoreService;
    using namespace Management::RepairManager;
    using namespace Management::TokenValidationService;
    using namespace Management::ImageStore;
    using namespace Management::FaultAnalysisService;
    using namespace Management::UpgradeOrchestrationService;
    using namespace Management::CentralSecretService;
    using namespace Management::ResourceManager;
    using namespace Management::NetworkInventoryManager;
    using namespace Management::GatewayResourceManager;
    using namespace Reliability;
    using namespace SystemServices;
    using namespace Naming;
    using namespace ClientServerTransport;
    using namespace ServiceModel;

    const ClientEventSource FabricClientImpl::Trace;
    const StringLiteral FabricClientImpl::CommunicationTraceComponent("Communication");
    const StringLiteral FabricClientImpl::LifeCycleTraceComponent("LifeCycle");

    StringLiteral const TraceComponent("FabricClientImpl");

    FabricClientImpl::FabricClientImpl(
        vector<wstring> && gatewayAddresses,
        IServiceNotificationEventHandlerPtr const & serviceNotificationEventHandler,
        IClientConnectionEventHandlerPtr const & connectionEventHandler)
        : traceContext_(Guid::NewGuid().ToString())
        , securitySettingsUpdateLock_()
        , config_()
        , settings_(make_shared<FabricClientInternalSettings>(traceContext_))
        , gatewayAddresses_(move(gatewayAddresses))
        , clientConnectionManager_()
        , connectionHandlersId_(0)
        , serviceNotificationEventHandler_(serviceNotificationEventHandler)
        , notificationEventHandlerLock_()
        , connectionEventHandler_(connectionEventHandler)
        , connectionEventHandlerLock_()
        , lruCacheManager_()
        , notificationClient_()
        , fileTransferClient_()
        , healthClient_()
        , serviceAddressManager_()
        , stateLock_()
        , isOpened_(false)
        , imageStoreClientsMap_()
        , mapLock_()
        , role_(RoleMask::Enum::None)
        , isClientRoleAuthorized_(false)
    {
        this->SetTraceId(traceContext_);
        this->InitializeConnectionManager();
    }

    FabricClientImpl::FabricClientImpl(
        FabricNodeConfigSPtr const& config,
        Transport::RoleMask::Enum clientRole,
        IServiceNotificationEventHandlerPtr const & serviceNotificationEventHandler,
        IClientConnectionEventHandlerPtr const & connectionEventHandler)
        : traceContext_(Guid::NewGuid().ToString())
        , securitySettingsUpdateLock_()
        , config_(config)
        , settings_(make_shared<FabricClientInternalSettings>(traceContext_))
        , clientConnectionManager_()
        , connectionHandlersId_(0)
        , serviceNotificationEventHandler_(serviceNotificationEventHandler)
        , notificationEventHandlerLock_()
        , connectionEventHandler_(connectionEventHandler)
        , connectionEventHandlerLock_()
        , lruCacheManager_()
        , notificationClient_()
        , fileTransferClient_()
        , healthClient_()
        , serviceAddressManager_()
        , stateLock_()
        , isOpened_(false)
        , imageStoreClientsMap_()
        , mapLock_()
        , role_(clientRole)
        , isClientRoleAuthorized_(false)
    {
        gatewayAddresses_.push_back(GetLocalGatewayAddress());

        this->SetTraceId(traceContext_);
        this->InitializeConnectionManager();
    }

    FabricClientImpl::FabricClientImpl(
        INamingMessageProcessorSPtr const &namingMessageProcessorSPtr,
        IServiceNotificationEventHandlerPtr const & serviceNotificationEventHandler,
        IClientConnectionEventHandlerPtr const & connectionEventHandler)
        : traceContext_(Guid::NewGuid().ToString())
        , securitySettingsUpdateLock_()
        , config_()
        , settings_(make_shared<FabricClientInternalSettings>(traceContext_))
        , clientConnectionManager_()
        , connectionHandlersId_(0)
        , serviceNotificationEventHandler_(serviceNotificationEventHandler)
        , notificationEventHandlerLock_()
        , connectionEventHandler_(connectionEventHandler)
        , connectionEventHandlerLock_()
        , lruCacheManager_()
        , notificationClient_()
        , fileTransferClient_()
        , healthClient_()
        , serviceAddressManager_()
        , stateLock_()
        , isOpened_(false)
        , imageStoreClientsMap_()
        , mapLock_()
        , role_(RoleMask::Enum::None)
        , isClientRoleAuthorized_(false)
    {
        this->SetTraceId(traceContext_);
        this->InitializeConnectionManager(namingMessageProcessorSPtr);
    }

    FabricClientImpl::FabricClientImpl(
        FabricNodeConfigSPtr const& config,
        Transport::RoleMask::Enum clientRole,
        bool isClientRoleAuthorized,
        IServiceNotificationEventHandlerPtr const & serviceNotificationEventHandler,
        IClientConnectionEventHandlerPtr const & connectionEventHandler)
        : FabricClientImpl(config, clientRole, serviceNotificationEventHandler, connectionEventHandler)
    {
        this->isClientRoleAuthorized_ = isClientRoleAuthorized;
    }

    FabricClientImpl::~FabricClientImpl()
    {
        if (State.Value == FabricComponentState::Opened)
        {
            Close();
        }
    }

    IClientCache & FabricClientImpl::get_Cache()
    {
        ThrowIfCreatedOrOpening();

        return *lruCacheManager_;
    }

    void FabricClientImpl::InitializeConnectionManager(INamingMessageProcessorSPtr const &namingMessageProcessorSPtr)
    {
       // ClientConnectionManager must be available for applying security settings
       // before the client is opened.
       //
       auto tempClientConnectionManager = make_shared<ClientConnectionManager>(
          this->TraceContext,
          make_unique<FabricClientInternalSettingsHolder>(*this),
          move(gatewayAddresses_),
          namingMessageProcessorSPtr,
          *this);

          clientConnectionManager_.swap(tempClientConnectionManager);
    }

    void FabricClientImpl::InitializeTraceContextFromSettings()
    {
       if (!settings_->ClientFriendlyName.empty())
       {
          // create a unique identifier that includes friendly name to eliminate potential naming collisions across clients...
          wstring uniqueName = wformatString("{0}_{1}", settings_->ClientFriendlyName, traceContext_);

          WriteInfo(
               Constants::FabricClientSource,
               traceContext_,
               "Prepending client trace context ID with user-supplied friendly name '{0}'", 
               settings_->ClientFriendlyName);

          // Guarded by EnsureOpened()
          traceContext_ = move(uniqueName);
       }
    }

    ErrorCode FabricClientImpl::InitializeSecurity()
    {
        if (config_)
        {
            weak_ptr<ComponentRoot> rootWPtr = shared_from_this();

            // Attach handlers to dynamic node-config-entries for default admin role client.
            config_->ClientAuthX509StoreNameEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            config_->ClientAuthX509FindTypeEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            config_->ClientAuthX509FindValueEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            config_->ClientAuthX509FindValueSecondaryEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });

            // Attach handlers to dynamic node-config-entries for default user role client.
            config_->UserRoleClientX509StoreNameEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            config_->UserRoleClientX509FindTypeEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            config_->UserRoleClientX509FindValueEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            config_->UserRoleClientX509FindValueSecondaryEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });

            SecurityConfig::GetConfig().ServerCertThumbprintsEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            SecurityConfig::GetConfig().ServerX509NamesEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            SecurityConfig::GetConfig().ServerAuthAllowedCommonNamesEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            SecurityConfig::GetConfig().ServerCertIssuersEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });
            SecurityConfig::GetConfig().ServerCertificateIssuerStoresEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });

            // Negotiate/Kerberos related
            SecurityConfig::GetConfig().ClusterIdentitiesEntry.AddHandler(
                [rootWPtr](EventArgs const&) { UpdateSecurityFromConfig(rootWPtr); });

            auto error = this->SetSecurityFromConfig();
            if (!error.IsSuccess())
            {
                Trace.OpenFailed(traceContext_, L"SetSecurityFromConfig", error);
                return error;
            }
        }

        return ErrorCodeValue::Success;
    }

    void FabricClientImpl::InitializeInnerClients()
    {
        auto tempLruCacheManager = make_unique<LruClientCacheManager>(*this, *settings_);

        auto tempNotificationClient = make_unique<ServiceNotificationClient>(
            this->TraceContext,
            make_unique<FabricClientInternalSettingsHolder>(*this),
            *clientConnectionManager_,
            *tempLruCacheManager,
            [this](vector<ServiceNotificationResultSPtr> const & notificationResults) -> ErrorCode
        {
            return this->OnNotificationReceived(notificationResults);
        },
            *this);

        auto tempFileTransferClient = make_unique<FileTransferClient>(clientConnectionManager_, true, *this);

        auto tempHealthClient = make_shared<ClientHealthReporting>(
            *this,
            *this,
            traceContext_);

        auto tempServiceAddressManager = make_shared<ServiceAddressTrackerManager>(*this);

        lruCacheManager_.swap(tempLruCacheManager);
        notificationClient_.swap(tempNotificationClient);
        fileTransferClient_.swap(tempFileTransferClient);
        healthClient_.swap(tempHealthClient);
        serviceAddressManager_.swap(tempServiceAddressManager);
    }

    ErrorCode FabricClientImpl::OpenInnerClients()
    {
        auto root = this->CreateComponentRoot();
        connectionHandlersId_ = clientConnectionManager_->RegisterConnectionHandlers(
            [this, root](ClientConnectionManager::HandlerId id, ISendTarget::SPtr const & st, GatewayDescription const & g)
        {
            this->OnConnected(id, st, g);
        },
            [this, root](ClientConnectionManager::HandlerId id, ISendTarget::SPtr const & st, GatewayDescription const & g, ErrorCode const & err)
        {
            this->OnDisconnected(id, st, g, err);
        },
            [this, root](ClientConnectionManager::HandlerId id, shared_ptr<ClaimsRetrievalMetadata> const & m, __out wstring & t)
        {
            return this->OnClaimsRetrieval(id, m, t);
        });

        auto error = notificationClient_->Open();
        if (!error.IsSuccess())
        {
            Trace.OpenFailed(traceContext_, L"ServiceNotificationClient::Open", error);
            return error;
        }

        error = fileTransferClient_->Open();
        if (!error.IsSuccess())
        {
            Trace.OpenFailed(traceContext_, L"FileTransferClient::Open", error);
            return error;
        }

        error = healthClient_->Open();
        if (!error.IsSuccess())
        {
            Trace.OpenFailed(traceContext_, L"HealthReportingComponent::Open", error);
            return error;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode FabricClientImpl::EnsureOpened()
    {
        AcquireExclusiveLock acquire(stateLock_);
        if (isOpened_)
        {
            return ErrorCode::Success();
        }
        else
        {
            auto error = Open();

            if (!error.IsSuccess())
            {
                WriteWarning(
                    Constants::FabricClientSource,
					this->TraceContext,
                    "Open failed due to {0}: state = {1}",
                    error,
                    State);
            }
            else
            {
                isOpened_ = true;
            }

            return error;
        }
    }

    ErrorCode FabricClientImpl::OnOpen()
    {
        WriteNoise(LifeCycleTraceComponent, traceContext_, "Entering OnOpen");

        this->InitializeTraceContextFromSettings();

        auto error = this->InitializeSecurity();
        if (!error.IsSuccess()) { return error; }

        this->InitializeInnerClients();

        error = this->OpenInnerClients();
        if (!error.IsSuccess())
        {
            lruCacheManager_.reset();
            return error;
        }

        Trace.Open(traceContext_);
        return error;
    }

    ErrorCode FabricClientImpl::OnClose()
    {
        WriteNoise(LifeCycleTraceComponent, traceContext_, "Entering OnClose");

        this->CleanupOnClose();

        ErrorCode closeError;

        if (healthClient_)
        {
            auto error = healthClient_->Close();
            if (!error.IsSuccess())
            {
                Trace.CloseFailed(traceContext_, L"HealthReportingComponent::Close", error);
                closeError = ErrorCode::FirstError(closeError, error);
            }
        }

        if (fileTransferClient_)
        {
            auto error = fileTransferClient_->Close();
            if (!error.IsSuccess())
            {
                Trace.CloseFailed(traceContext_, L"FileTransferClient::Close", error);
                closeError = ErrorCode::FirstError(closeError, error);
            }
        }

        if (notificationClient_)
        {
            auto error = notificationClient_->Close();
            if (!error.IsSuccess())
            {
                Trace.CloseFailed(traceContext_, L"ServiceNotificationClient::Close", error);
                closeError = ErrorCode::FirstError(closeError, error);
            }
        }

        if (closeError.IsSuccess())
        {
            Trace.Close(traceContext_);
        }

        return closeError;
    }

    void FabricClientImpl::OnAbort()
    {
        WriteNoise(LifeCycleTraceComponent, traceContext_, "Entering OnAbort");

        this->CleanupOnClose();

        if (notificationClient_)
        {
            notificationClient_->Abort();
        }

        if (healthClient_)
        {
            healthClient_->Abort();
        }

        if (fileTransferClient_)
        {
            fileTransferClient_->Abort();
        }

        Trace.Abort(traceContext_);
    }

    void FabricClientImpl::CleanupOnClose()
    {
        {
            AcquireWriteLock lock(notificationEventHandlerLock_);

            serviceNotificationEventHandler_ = IServiceNotificationEventHandlerPtr();
        }

        {
            AcquireWriteLock lock(connectionEventHandlerLock_);

            connectionEventHandler_ = IClientConnectionEventHandlerPtr();
        }

        if (serviceAddressManager_)
        {
            serviceAddressManager_->CancelPendingRequests();
        }

        if (clientConnectionManager_)
        {
            clientConnectionManager_->UnregisterConnectionHandlers(connectionHandlersId_);
        }

        if (lruCacheManager_)
        {
            // Clear notification callbacks even if application forgets to unregister
            //
            lruCacheManager_->ClearCacheCallbacks();
        }
    }

#pragma region IServiceManagementClient methods

    //
    // IServiceManagementClient methods
    //

    AsyncOperationSPtr FabricClientImpl::BeginCreateService(
        PartitionedServiceDescWrapper const &partitionDesc,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        Naming::PartitionedServiceDescriptor psd;

        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        error = psd.FromWrapper(partitionDesc);
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        if (partitionDesc.ApplicationName.empty())
        {
            //
            // Adhoc services to Naming system.
            //
            return BeginInternalCreateService(
                psd,
                ServicePackageVersion(),
                0,
                timeout,
                callback,
                parent);
        }
        else
        {
            //
            // Regular services to cluster manager.
            //
            NamingUri appNameUri;
            if (!NamingUri::TryParse(partitionDesc.ApplicationName, appNameUri))
            {
                Trace.BeginCreateServiceNameValidationFailure(
                    traceContext_,
                    partitionDesc.ApplicationName);

                error = ErrorCode(ErrorCodeValue::InvalidNameUri, wformatString("{0} {1}", GET_NS_RC(Invalid_Uri),
                    partitionDesc.ApplicationName));
                return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                    error,
                    callback,
                    parent);
            }

            ClientServerRequestMessageUPtr message = ClusterManagerTcpMessage::GetCreateService(
                Common::make_unique<Management::ClusterManager::CreateServiceMessageBody>(psd));

            Trace.BeginCreateService(
                traceContext_,
                message->ActivityId,
                psd.Service);

            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                appNameUri,
                timeout,
                callback,
                parent);
        }
    }

    ErrorCode FabricClientImpl::EndCreateService(AsyncOperationSPtr const & operation)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }
        else if (dynamic_cast<ForwardToServiceAsyncOperation*>(operation.get()) != NULL)
        {
            ClientServerReplyMessageUPtr reply;
            return ForwardToServiceAsyncOperation::End(operation, reply);
        }
        else
        {
            return EndInternalCreateService(operation);
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalCreateService(
        PartitionedServiceDescriptor const &psd,
        ServicePackageVersion const &packageVersion,
        ULONGLONG packageInstance,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        NamingUri serviceNameUri;
        ClientServerRequestMessageUPtr message;

        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            if (NamingUri::TryParse(psd.Service.Name, serviceNameUri))
            {
                PartitionedServiceDescriptor localPsd = psd;
                const_cast<ServicePackageVersionInstance&>(localPsd.Service.PackageVersionInstance) = ServicePackageVersionInstance(
                    packageVersion,
                    packageInstance);

                message = NamingTcpMessage::GetCreateService(Common::make_unique<Naming::CreateServiceMessageBody>(serviceNameUri, localPsd));
                Trace.BeginCreateService(
                    traceContext_,
                    message->ActivityId,
                    psd.Service);
            }
            else
            {
                Trace.BeginCreateServiceNameValidationFailure(
                    traceContext_,
                    psd.Service.Name);

                error = ErrorCodeValue::InvalidNameUri;
            }
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            serviceNameUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalCreateService(AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateServiceFromTemplate(
        ServiceFromTemplateDescription const & serviceFromTemplateDesc,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetCreateServiceFromTemplate(
                Common::make_unique<CreateServiceFromTemplateMessageBody>(serviceFromTemplateDesc));

            Trace.BeginCreateServiceFromTemplate(
                traceContext_,
                message->ActivityId,
                serviceFromTemplateDesc.ApplicationName,
                serviceFromTemplateDesc.ServiceName,
                serviceFromTemplateDesc.ServiceTypeName);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            serviceFromTemplateDesc.ApplicationName,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCreateServiceFromTemplate(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpdateService(
        NamingUri const & serviceName,
        ServiceUpdateDescription const & updateDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetUpdateService(Common::make_unique<Naming::UpdateServiceRequestBody>(serviceName, updateDescription));

            Trace.BeginUpdateService(
                traceContext_,
                message->ActivityId,
                serviceName,
                updateDescription);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            serviceName,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpdateService(AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteService(
        DeleteServiceDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        return AsyncOperation::CreateAndStart<DeleteServiceAsyncOperation>(
            *this,
            description,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeleteService(AsyncOperationSPtr const & operation)
    {
        return DeleteServiceAsyncOperation::End(operation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceDescription(
        NamingUri const & serviceName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        return AsyncOperation::CreateAndStart<GetServiceDescriptionAsyncOperation>(
            *this,
            serviceName,
            false, // fetch cached
            FabricActivityHeader(Guid::NewGuid()),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetServiceDescription(
        AsyncOperationSPtr const & operation,
        __inout Naming::PartitionedServiceDescriptor & description)
    {
        return GetServiceDescriptionAsyncOperation::EndGetServiceDescription(operation, description);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetCachedServiceDescription(
        NamingUri const & serviceName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        return AsyncOperation::CreateAndStart<GetServiceDescriptionAsyncOperation>(
            *this,
            serviceName,
            true, // fetch cached
            FabricActivityHeader(Guid::NewGuid()),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetCachedServiceDescription(
        AsyncOperationSPtr const & operation,
        __inout Naming::PartitionedServiceDescriptor & description)
    {
        return GetServiceDescriptionAsyncOperation::EndGetServiceDescription(operation, description);
    }

    AsyncOperationSPtr FabricClientImpl::BeginResolveServicePartition(
        NamingUri const & serviceName,
        ServiceResolutionRequestData const & resolveRequest,
        Api::IResolvedServicePartitionResultPtr & previousResult,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ResolvedServicePartitionMetadataSPtr previousRspMetadata;
        if (previousResult.get())
        {
            auto casted = dynamic_cast<ResolvedServicePartitionResult*>(previousResult.get());

            if (casted)
            {
                ResolvedServicePartitionSPtr previousRsp;
                previousRsp = casted->ResolvedServicePartition;
                if (previousRsp)
                {
                    previousRspMetadata = make_shared<ResolvedServicePartitionMetadata>(
                        previousRsp->Locations.ConsistencyUnitId.Guid,
                        previousRsp->FMVersion,
                        previousRsp->StoreVersion,
                        previousRsp->Generation);
                }
            }
        }

        return BeginResolveServicePartition(
            serviceName,
            resolveRequest.Key,
            previousRspMetadata,
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginResolveServicePartition(
        NamingUri const & serviceName,
        Naming::PartitionKey const &key,
        Naming::ResolvedServicePartitionMetadataSPtr const &previousRspMetadata,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto activityHeader = FabricActivityHeader(Guid::NewGuid());
        auto error = EnsureOpened();

        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ServiceResolutionRequestData request(key);
        if (previousRspMetadata)
        {
            Trace.BeginResolveServiceWPrev(
                traceContext_,
                activityHeader.ActivityId,
                serviceName,
                key,
                previousRspMetadata->Generation,
                previousRspMetadata->FMVersion,
                previousRspMetadata->StoreVersion,
                false, // prefix
                false); // bypassCache
        }
        else
        {
            Trace.BeginResolveService(
                traceContext_,
                activityHeader.ActivityId,
                serviceName,
                key,
                false, // prefix
                false); // bypassCache
        }

        return lruCacheManager_->BeginResolveService(
            serviceName,
            request,
            previousRspMetadata,
            move(activityHeader),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndResolveServicePartition(
        AsyncOperationSPtr const & operation,
        __inout Api::IResolvedServicePartitionResultPtr & result)
    {
        ResolvedServicePartitionSPtr rsp;
        auto error = EndResolveServicePartition(operation, rsp);
        if (error.IsSuccess())
        {
            auto rspResult = make_shared<ResolvedServicePartitionResult>(rsp);
            result = RootedObjectPointer<IResolvedServicePartitionResult>(
                rspResult.get(),
                rspResult->CreateComponentRoot());
        }

        return error;
    }

    ErrorCode FabricClientImpl::EndResolveServicePartition(
        AsyncOperationSPtr const & operation,
        __inout ResolvedServicePartitionSPtr & rsp)
    {
        ErrorCode error;
        ActivityId activityId;

        if (operation->FailedSynchronously && !lruCacheManager_)
        {
            error = AsyncOperation::End(operation);
        }
        else
        {
            error = lruCacheManager_->EndResolveService(operation, rsp, activityId);
        }

        if (error.IsSuccess())
        {
            Trace.EndResolveServiceSuccess(
                traceContext_,
                activityId,
                rsp->Generation,
                rsp->FMVersion,
                rsp->StoreVersion);
        }
        else
        {
            Trace.EndResolveServiceFailure(traceContext_, activityId, error);
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginResolveSystemServicePartition(
        NamingUri const & serviceName,
        Naming::PartitionKey const &key,
        Common::Guid const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            ServiceResolutionRequestData request(key);
            auto serviceResolutionMessageBody = Common::make_unique<ServiceResolutionMessageBody>(
                serviceName,
                move(request));
            message = NamingTcpMessage::GetResolveSystemService(Common::ActivityId(activityId), move(serviceResolutionMessageBody));

            Trace.BeginResolveSystemService(
                traceContext_,
                message->ActivityId,
                serviceName,
                key);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            serviceName,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndResolveSystemServicePartition(
        AsyncOperationSPtr const & operation,
        __inout ResolvedServicePartitionSPtr & rsp)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            ResolvedServicePartition body;
            if (reply->GetBody(body))
            {
                rsp = make_shared<ResolvedServicePartition>(move(body));
                Trace.EndResolveSystemServiceSuccess(
                    traceContext_,
                    reply->ActivityId,
                    rsp->Generation,
                    rsp->FMVersion,
                    rsp->StoreVersion);
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
                Trace.EndResolveSystemServiceFailure(
                    traceContext_,
                    reply->ActivityId,
                    error);
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginPrefixResolveServicePartition(
        Common::NamingUri const & serviceName,
        Naming::ServiceResolutionRequestData const & resolveRequest,
        IResolvedServicePartitionResultPtr & previousResult,
        bool bypassCache,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginPrefixResolveServicePartitionInternal(
            serviceName,
            resolveRequest,
            previousResult,
            bypassCache,
            Guid::NewGuid(),
            false, //default tracing mode: log all the details
            timeout,
            callback,
            parent
        );
    }

    ErrorCode FabricClientImpl::EndPrefixResolveServicePartition(
        AsyncOperationSPtr const & operation,
        __inout ResolvedServicePartitionSPtr & rsp)
    {
        return EndPrefixResolveServicePartitionInternal(
            operation,
            false, //default tracing mode: log all the details
            rsp);
    }

    AsyncOperationSPtr FabricClientImpl::BeginPrefixResolveServicePartitionInternal(
        Common::NamingUri const & serviceName,
        Naming::ServiceResolutionRequestData const & resolveRequest,
        IResolvedServicePartitionResultPtr & previousResult,
        bool bypassCache,
        Common::Guid const & activityId,
        bool suppressSuccessLogs,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ResolvedServicePartitionMetadataSPtr previousRspMetadata;
        if (previousResult.get())
        {
            auto casted = dynamic_cast<ResolvedServicePartitionResult*>(previousResult.get());

            if (casted)
            {
                ResolvedServicePartitionSPtr previousRsp;
                previousRsp = casted->ResolvedServicePartition;
                if (previousRsp)
                {
                    previousRspMetadata = make_shared<ResolvedServicePartitionMetadata>(
                        previousRsp->Locations.ConsistencyUnitId.Guid,
                        previousRsp->FMVersion,
                        previousRsp->StoreVersion,
                        previousRsp->Generation);
                }
            }
        }

        auto activityHeader = FabricActivityHeader(activityId);
        if (!suppressSuccessLogs)
        {
            if (previousRspMetadata)
            {
                Trace.BeginResolveServiceWPrev(
                    traceContext_,
                    activityHeader.ActivityId,
                    serviceName,
                    resolveRequest.Key,
                    previousRspMetadata->Generation,
                    previousRspMetadata->FMVersion,
                    previousRspMetadata->StoreVersion,
                    true, // prefix
                    bypassCache);
            }
            else
            {
                Trace.BeginResolveService(
                    traceContext_,
                    activityHeader.ActivityId,
                    serviceName,
                    resolveRequest.Key,
                    true, // prefix
                    bypassCache);
            }
        }

        return lruCacheManager_->BeginPrefixResolveService(
            serviceName,
            resolveRequest,
            previousRspMetadata,
            bypassCache,
            move(activityHeader),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndPrefixResolveServicePartitionInternal(
        AsyncOperationSPtr const & operation,
        bool suppressSuccessLogs,
        __inout ResolvedServicePartitionSPtr & rsp)
    {
        ErrorCode error;
        ActivityId activityId;

        if (operation->FailedSynchronously && !lruCacheManager_)
        {
            error = AsyncOperation::End(operation);
        }
        else
        {
            error = lruCacheManager_->EndPrefixResolveService(operation, rsp, activityId);
        }

        if (error.IsSuccess())
        {
            if (!suppressSuccessLogs)
            {
                Trace.EndResolveServiceSuccess(
                    traceContext_,
                    activityId,
                    rsp->Generation,
                    rsp->FMVersion,
                    rsp->StoreVersion);
            }
        }
        else
        {
            Trace.EndResolveServiceFailure(traceContext_, activityId, error);
        }

        return error;
    }

    ErrorCode FabricClientImpl::RegisterServicePartitionResolutionChangeHandler(
        NamingUri const& name,
        PartitionKey const& key,
        Api::ServicePartitionResolutionChangeCallback const &handler,
        __inout LONGLONG * callbackHandle)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return error;
        }

        if (name.IsRootNamingUri)
        {
            WriteWarning(
                Constants::FabricClientSource,
                TraceContext,
                "Can't register change handler for the root naming uri {0}",
                name);
            return ErrorCodeValue::AccessDenied;
        }

        return serviceAddressManager_->AddTracker(
            handler,
            name,
            key,
            *callbackHandle);
    }

    ErrorCode FabricClientImpl::UnRegisterServicePartitionResolutionChangeHandler(
        LONGLONG callbackHandle)
    {
        if (serviceAddressManager_->RemoveTracker(callbackHandle))
        {
            return ErrorCode::Success();
        }
        else
        {
            return ErrorCodeValue::NotFound;
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceManifest(
        wstring const &applicationTypeName,
        wstring const &applicationTypeVersion,
        wstring const &serviceManifestName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeName,
            applicationTypeName);

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeVersion,
            applicationTypeVersion);

        argMap.Insert(
            QueryResourceProperties::ServiceManifest::ServiceManifestName,
            serviceManifestName);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceManifest),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceManifest(
        AsyncOperationSPtr const &operation,
        __inout wstring &serviceManifest)
    {
        QueryResult queryResult;
        auto err = EndInternalQuery(operation, queryResult);
        if (!err.IsSuccess())
        {
            return err;
        }
        return queryResult.MoveItem(serviceManifest);
    }

    AsyncOperationSPtr FabricClientImpl::BeginReportFault(
        wstring const & nodeName,
        Guid const & partitionId,
        int64 replicaId,
        FaultType::Enum faultType,
        bool isForce,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;

        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<Reliability::ReportFaultRequestMessageBody>(nodeName, faultType, replicaId, partitionId, isForce, ActivityDescription(ActivityId(), ActivityType::Enum::ClientReportFaultEvent));
            ReportFaultRequestMessageBody& bodyRef = *body;
            message = NamingTcpMessage::GetReportFaultRequest(std::move(body));
            Trace.BeginReportFault(traceContext_, message->ActivityId, bodyRef);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndReportFault(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = RequestReplyAsyncOperation::End(operation, reply);
        if (!error.IsSuccess())
        {
            return error;
        }

        Reliability::ReportFaultReplyMessageBody body;
        if (!reply->GetBody(body))
        {
            return ErrorCode::FromNtStatus(reply->GetStatus());
        }

        error = body.Error;
        Trace.EndReportFault(traceContext_, reply->ActivityId, error);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginRegisterServiceNotificationFilter(
        ServiceNotificationFilterSPtr const & filter,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        if (error.IsSuccess())
        {
            return notificationClient_->BeginRegisterFilter(
                ActivityId(),
                filter,
                timeout,
                callback,
                parent);
        }
        else
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }
    }

    ErrorCode FabricClientImpl::EndRegisterServiceNotificationFilter(
        AsyncOperationSPtr const & operation,
        __out uint64 & filterId)
    {
        if (operation->FailedSynchronously)
        {
            return AsyncOperation::End(operation);
        }
        else
        {
            return notificationClient_->EndRegisterFilter(operation, filterId);
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginUnregisterServiceNotificationFilter(
        uint64 filterId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        if (error.IsSuccess())
        {
            return notificationClient_->BeginUnregisterFilter(
                ActivityId(),
                filterId,
                timeout,
                callback,
                parent);
        }
        else
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }
    }

    ErrorCode FabricClientImpl::EndUnregisterServiceNotificationFilter(
        AsyncOperationSPtr const & operation)
    {
        if (operation->FailedSynchronously)
        {
            return AsyncOperation::End(operation);
        }
        else
        {
            return notificationClient_->EndUnregisterFilter(operation);
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginMovePrimary(
        wstring const & nodeName,
        Guid const & partitionId,
        bool ignoreConstraints,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (!nodeName.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Node::Name,
                nodeName);
        }

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        argMap.Insert(
            QueryResourceProperties::QueryMetadata::ForceMove,
            ignoreConstraints ? L"True" : L"False"
        );

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::MovePrimary),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndMovePrimary(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginMoveSecondary(
        wstring const & currentNodeName,
        wstring const & newNodeName,
        Guid const & partitionId,
        bool ignoreConstraints,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::CurrentNodeName,
            currentNodeName);

        if (!newNodeName.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Node::NewNodeName,
                newNodeName);
        }

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        argMap.Insert(
            QueryResourceProperties::QueryMetadata::ForceMove,
            ignoreConstraints ? L"True" : L"False");

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::MoveSecondary),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndMoveSecondary(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

#pragma endregion

#pragma region IQueryClient Methods

    //
    // IQueryClient methods
    //

    AsyncOperationSPtr FabricClientImpl::BeginInternalQuery(
        wstring const & queryName,
        QueryArgumentMap const & queryArgs,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const & operationRoot)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = QueryTcpMessage::GetQueryRequest(Common::make_unique<QueryRequestMessageBody>(queryName, queryArgs));
            Trace.BeginInternalQuery(traceContext_, message->ActivityId, queryName, queryArgs);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            operationRoot,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalQuery(
        AsyncOperationSPtr const & asyncOperation,
        __inout QueryResult & queryResult)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(asyncOperation, reply);
        if (error.IsSuccess())
        {
            if (reply->GetBody(queryResult))
            {
                error = queryResult.QueryProcessingError;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNodeList(
        wstring const & nodeNameFilter,
        wstring const & continuationToken,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        NodeQueryDescription queryDescription;
        queryDescription.NodeNameFilter = nodeNameFilter;
        QueryPagingDescription pagingDescription;
        pagingDescription.ContinuationToken = continuationToken;
        queryDescription.PagingDescription = make_unique<QueryPagingDescription>(move(pagingDescription));

        return this->BeginGetNodeList(
            move(queryDescription),
            false,
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNodeList(
        wstring const & nodeNameFilter,
        DWORD nodeStatusFilter,
        bool excludeStoppedNodeInfo,
        wstring const & continuationToken,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        NodeQueryDescription queryDescription;
        queryDescription.NodeNameFilter = nodeNameFilter;
        queryDescription.NodeStatusFilter = nodeStatusFilter;
        QueryPagingDescription pagingDescription;
        pagingDescription.ContinuationToken = continuationToken;
        queryDescription.PagingDescription = make_unique<QueryPagingDescription>(move(pagingDescription));

        return this->BeginGetNodeList(
            move(queryDescription),
            excludeStoppedNodeInfo,
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNodeList(
        NodeQueryDescription const & queryDescription,
        bool excludeStoppedNodeInfo,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (excludeStoppedNodeInfo)
        {
            argMap.Insert(
                QueryResourceProperties::Node::ExcludeStoppedNodeInfo,
                L"True");
        }

        // Inserts values into argMap based on data in queryDescription
        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetNodeList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetNodeList(
        AsyncOperationSPtr const &operation,
        __inout vector<NodeQueryResult> &nodeList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<NodeQueryResult>(nodeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetFMNodeList(
        wstring const & nodeNameFilter,
        wstring const & continuationToken,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        if (!nodeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Node::Name,
                nodeNameFilter);
        }

        if (!continuationToken.empty())
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::ContinuationToken,
                continuationToken);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetFMNodeList),
            argMap,
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetComposeDeploymentStatusList(
        ComposeDeploymentStatusQueryDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;
        if (!description.DeploymentNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Deployment::DeploymentName,
                description.DeploymentNameFilter);
        }

        if (!description.ContinuationToken.empty())
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::ContinuationToken,
                description.ContinuationToken);
        }

        if (description.MaxResults != 0)
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::MaxResults,
                StringUtility::ToWString(description.MaxResults));
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetComposeDeploymentStatusList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetComposeDeploymentStatusList(
        AsyncOperationSPtr const & operation,
        __inout vector<ComposeDeploymentStatusQueryResult> & dockerComposeDeploymentList,
        __inout PagingStatusUPtr & pagingStatus)
    {
        QueryResult queryResult;
        ErrorCode error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ComposeDeploymentStatusQueryResult>(dockerComposeDeploymentList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationTypeList(
        wstring const & applicationtypeNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        if (!applicationtypeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ApplicationType::ApplicationTypeName,
                applicationtypeNameFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationTypeList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationTypeList(
        AsyncOperationSPtr const &operation,
        __inout vector<ApplicationTypeQueryResult> &applicationtypeList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<ApplicationTypeQueryResult>(applicationtypeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationTypePagedList(
        ApplicationTypeQueryDescription const & applicationTypeQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        // This argMap is thing that gets turned into the query description that you see in IDL
        QueryArgumentMap argMap;

        if (!applicationTypeQueryDescription.ApplicationTypeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ApplicationType::ApplicationTypeName,
                applicationTypeQueryDescription.ApplicationTypeNameFilter);
        }

        if (!applicationTypeQueryDescription.ApplicationTypeVersionFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ApplicationType::ApplicationTypeVersion,
                applicationTypeQueryDescription.ApplicationTypeVersionFilter);
        }

        if (applicationTypeQueryDescription.ApplicationTypeDefinitionKindFilter != static_cast<DWORD>(FABRIC_APPLICATION_TYPE_DEFINITION_KIND_FILTER_DEFAULT))
        {
            argMap.Insert(
                QueryResourceProperties::Deployment::ApplicationTypeDefinitionKindFilter,
                wformatString(applicationTypeQueryDescription.ApplicationTypeDefinitionKindFilter));
        }

        argMap.Insert(
            QueryResourceProperties::QueryMetadata::ExcludeApplicationParameters,
            wformatString(applicationTypeQueryDescription.ExcludeApplicationParameters));

        if (applicationTypeQueryDescription.MaxResults != 0)
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::MaxResults,
                StringUtility::ToWString(applicationTypeQueryDescription.MaxResults));
        }

        if (!applicationTypeQueryDescription.ContinuationToken.empty())
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::ContinuationToken,
                applicationTypeQueryDescription.ContinuationToken);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationTypePagedList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationTypePagedList(
        AsyncOperationSPtr const &operation,
        __inout vector<ApplicationTypeQueryResult> &applicationtypeList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ApplicationTypeQueryResult>(applicationtypeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceTypeList(
        wstring const &applicationTypeName,
        wstring const &applicationTypeVersion,
        wstring const &serviceTypeNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeName,
            applicationTypeName);
        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeVersion,
            applicationTypeVersion);

        if (!serviceTypeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceType::ServiceTypeName,
                serviceTypeNameFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceTypeList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceTypeList(
        AsyncOperationSPtr const &operation,
        __inout vector<ServiceTypeQueryResult> &servicetypeList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<ServiceTypeQueryResult>(servicetypeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceGroupMemberTypeList(
        wstring const &applicationTypeName,
        wstring const &applicationTypeVersion,
        wstring const &serviceGroupTypeNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeName,
            applicationTypeName);
        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeVersion,
            applicationTypeVersion);

        if (!serviceGroupTypeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceType::ServiceGroupTypeName,
                serviceGroupTypeNameFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceGroupMemberTypeList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceGroupMemberTypeList(
        AsyncOperationSPtr const &operation,
        __inout vector<ServiceGroupMemberTypeQueryResult> &servicegroupmembertypeList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<ServiceGroupMemberTypeQueryResult>(servicegroupmembertypeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationList(
        ApplicationQueryDescription const & applicationQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;

        // Inserts values into argMap based on data in queryDescription
        applicationQueryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationList(
        AsyncOperationSPtr const & operation,
        __inout vector<ApplicationQueryResult> &applicationList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ApplicationQueryResult>(applicationList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceList(
        ServiceQueryDescription const & serviceQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (!serviceQueryDescription.ApplicationName.IsRootNamingUri)
        {
            argMap.Insert(
                QueryResourceProperties::Application::ApplicationName,
                serviceQueryDescription.ApplicationName.ToString());
        }

        if (!serviceQueryDescription.ServiceNameFilter.IsRootNamingUri)
        {
            argMap.Insert(
                QueryResourceProperties::Service::ServiceName,
                serviceQueryDescription.ServiceNameFilter.ToString());
        }

        if (!serviceQueryDescription.ServiceTypeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceType::ServiceTypeName,
                serviceQueryDescription.ServiceTypeNameFilter);
        }

        if (serviceQueryDescription.PagingDescription != nullptr)
        {
            serviceQueryDescription.PagingDescription->SetQueryArguments(argMap);
        }

        if (SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(serviceQueryDescription.ApplicationName))
        {
            return this->BeginInternalQuery(
                QueryNames::ToString(QueryNames::GetSystemServicesList),
                argMap,
                timeout,
                callback,
                parent);
        }
        else
        {
            return this->BeginInternalQuery(
                QueryNames::ToString(QueryNames::GetApplicationServiceList),
                argMap,
                timeout,
                callback,
                parent);
        }
    }

    ErrorCode FabricClientImpl::EndGetServiceList(
        AsyncOperationSPtr const &operation,
        __inout vector<ServiceQueryResult> &serviceList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ServiceQueryResult>(serviceList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceGroupMemberList(
        NamingUri const & applicationName,
        NamingUri const & serviceNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (!applicationName.IsRootNamingUri)
        {
            argMap.Insert(
                QueryResourceProperties::Application::ApplicationName,
                applicationName.ToString());
        }

        if (!serviceNameFilter.IsRootNamingUri)
        {
            argMap.Insert(
                QueryResourceProperties::Service::ServiceName,
                serviceNameFilter.ToString());
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationServiceGroupMemberList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceGroupMemberList(
        AsyncOperationSPtr const &operation,
        __inout vector<ServiceGroupMemberQueryResult> &serviceGroupMemberList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<ServiceGroupMemberQueryResult>(serviceGroupMemberList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetPartitionList(
        NamingUri const &serviceName,
        Guid const &partitionIdFilter,
        wstring const &continuationToken,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (serviceName.Type != UriType::Empty)
        {
            argMap.Insert(
                QueryResourceProperties::Service::ServiceName,
                serviceName.ToString());
        }

        if (partitionIdFilter != Guid::Empty())
        {
            argMap.Insert(
                QueryResourceProperties::Partition::PartitionId,
                partitionIdFilter.ToString());
        }

        if (!continuationToken.empty())
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::ContinuationToken,
                continuationToken);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServicePartitionList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetPartitionList(
        AsyncOperationSPtr const &operation,
        __inout vector<ServicePartitionQueryResult> &partitionList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ServicePartitionQueryResult>(partitionList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetReplicaList(
        Guid const &partitionId,
        int64 replicaOrInstanceIdFilter,
        DWORD replicaStatusFilter,
        wstring const &continuationToken,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        if (replicaOrInstanceIdFilter != 0)
        {
            argMap.Insert(
                QueryResourceProperties::Replica::ReplicaId,
                StringUtility::ToWString(replicaOrInstanceIdFilter));
        }

        if (replicaStatusFilter != (DWORD)FABRIC_QUERY_SERVICE_REPLICA_STATUS_FILTER_DEFAULT)
        {
            argMap.Insert(
                QueryResourceProperties::Replica::ReplicaStatusFilter,
                StringUtility::ToWString<DWORD>(replicaStatusFilter));
        }

        if (!continuationToken.empty())
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::ContinuationToken,
                continuationToken);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServicePartitionReplicaList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetReplicaList(
        AsyncOperationSPtr const &operation,
        __inout vector<ServiceReplicaQueryResult> &replicaList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ServiceReplicaQueryResult>(replicaList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedApplicationList(
        wstring const &nodeName,
        NamingUri const &applicationNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        if (!applicationNameFilter.IsRootNamingUri)
        {
            argMap.Insert(
                QueryResourceProperties::Application::ApplicationName,
                applicationNameFilter.ToString());
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationListDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedApplicationList(
        AsyncOperationSPtr const &operation,
        __inout vector<DeployedApplicationQueryResult> &deployedApplicationList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<DeployedApplicationQueryResult>(deployedApplicationList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedApplicationPagedList(
        ServiceModel::DeployedApplicationQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        // Inserts values into argMap based on data in queryDescription
        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationPagedListDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedApplicationPagedList(
        AsyncOperationSPtr const &operation,
        __inout vector<DeployedApplicationQueryResult> &deployedApplicationList,
        __inout ServiceModel::PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<DeployedApplicationQueryResult>(deployedApplicationList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedServicePackageList(
        wstring const &nodeName,
        NamingUri const &applicationName,
        wstring const &serviceManifestNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        if (!serviceManifestNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceManifest::ServiceManifestName,
                serviceManifestNameFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceManifestListDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedServicePackageList(
        AsyncOperationSPtr const &operation,
        __out vector<DeployedServiceManifestQueryResult> &deployedServiceManifestList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<DeployedServiceManifestQueryResult>(deployedServiceManifestList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedServiceTypeList(
        wstring const &nodeName,
        NamingUri const &applicationName,
        wstring const &serviceManifestNameFilter,
        wstring const &serviceTypeNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        if (!serviceManifestNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceManifest::ServiceManifestName,
                serviceManifestNameFilter);
        }

        if (!serviceTypeNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceType::ServiceTypeName,
                serviceTypeNameFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceTypeListDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedServiceTypeList(
        AsyncOperationSPtr const &operation,
        __out vector<DeployedServiceTypeQueryResult> &deployedServiceTypeList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<DeployedServiceTypeQueryResult>(deployedServiceTypeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginStopNode(
        wstring const & nodeName,
        wstring const & instanceId,
        bool restart,
        bool createFabricDump,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Node::InstanceId,
            instanceId);

        argMap.Insert(
            QueryResourceProperties::Node::Restart,
            wformatString(restart));

        argMap.Insert(
            QueryResourceProperties::Node::CreateFabricDump,
            wformatString(createFabricDump));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::StopNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndStopNode(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginAddUnreliableTransportBehavior(
        wstring const & nodeName,
        wstring const & name,
        wstring const & behaviorString,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::UnreliableTransportBehavior::Name,
            name);

        argMap.Insert(
            QueryResourceProperties::UnreliableTransportBehavior::Behavior,
            behaviorString);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::AddUnreliableTransportBehavior),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndAddUnreliableTransportBehavior(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginRemoveUnreliableTransportBehavior(
        wstring const & nodeName,
        wstring const & name,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::UnreliableTransportBehavior::Name,
            name);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::RemoveUnreliableTransportBehavior),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndRemoveUnreliableTransportBehavior(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetTransportBehaviorList(
        std::wstring const & nodeName,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetTransportBehaviors),
            argMap,
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode FabricClientImpl::EndGetTransportBehaviorList(
        Common::AsyncOperationSPtr const & operation,
        std::vector<std::wstring>& transportBehavior)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<wstring>(transportBehavior);
    }

    AsyncOperationSPtr FabricClientImpl::BeginAddUnreliableLeaseBehavior(
        wstring const & nodeName,
        wstring const & behaviorString,
        wstring const & alias,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::UnreliableLeaseBehavior::Alias,
            alias);

        argMap.Insert(
            QueryResourceProperties::UnreliableLeaseBehavior::Behavior,
            behaviorString);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::AddUnreliableLeaseBehavior),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndAddUnreliableLeaseBehavior(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginRemoveUnreliableLeaseBehavior(
        wstring const & nodeName,
        wstring const & alias,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::UnreliableLeaseBehavior::Alias,
            alias);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::RemoveUnreliableLeaseBehavior),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndRemoveUnreliableLeaseBehavior(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginStartNode(
        wstring const & nodeName,
        uint64 instanceId,
        wstring const & ipAddressOrFQDN,
        ULONG clusterConnectionPort,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;

        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<Naming::StartNodeRequestMessageBody>(nodeName, instanceId, ipAddressOrFQDN, clusterConnectionPort);
            message = NamingTcpMessage::GetStartNode(std::move(body));

            Trace.BeginStartNode(
                traceContext_,
                message->ActivityId,
                nodeName,
                instanceId,
                ipAddressOrFQDN,
                static_cast<uint64>(clusterConnectionPort));
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndStartNode(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginRestartDeployedCodePackage(
        wstring const & nodeName,
        NamingUri const & applicationName,
        wstring const & serviceManifestName,
        wstring const & codePackageName,
        wstring const & instanceId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        argMap.Insert(
            QueryResourceProperties::ServiceManifest::ServiceManifestName,
            serviceManifestName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::CodePackageName,
            codePackageName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::InstanceId,
            instanceId);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::RestartDeployedCodePackage),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndRestartDeployedCodePackage(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedCodePackageList(
        wstring const &nodeName,
        NamingUri const &applicationName,
        wstring const &serviceManifestNameFilter,
        wstring const &codePackageNameFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        if (!serviceManifestNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceManifest::ServiceManifestName,
                serviceManifestNameFilter);
        }

        if (!codePackageNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::CodePackage::CodePackageName,
                codePackageNameFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetCodePackageListDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedCodePackageList(
        AsyncOperationSPtr const &operation,
        __out vector<DeployedCodePackageQueryResult> &deployedCodePackageList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<DeployedCodePackageQueryResult>(deployedCodePackageList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedReplicaList(
        wstring const &nodeName,
        NamingUri const &applicationName,
        wstring const &serviceManifestNameFilter,
        Guid const &partitionIdFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        if (!serviceManifestNameFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::ServiceManifest::ServiceManifestName,
                serviceManifestNameFilter);
        }

        if (partitionIdFilter != Guid::Empty())
        {
            argMap.Insert(
                QueryResourceProperties::Partition::PartitionId,
                partitionIdFilter.ToString());
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceReplicaListDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedReplicaList(
        AsyncOperationSPtr const &operation,
        __out vector<DeployedServiceReplicaQueryResult> &deployedReplicaList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<DeployedServiceReplicaQueryResult>(deployedReplicaList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedServiceReplicaDetail(
        wstring const &nodeName,
        Guid const &partitionId,
        FABRIC_REPLICA_ID replicaId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            wformatString(partitionId));

        argMap.Insert(
            QueryResourceProperties::Replica::ReplicaId,
            wformatString(replicaId));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeployedServiceReplicaDetail),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedServiceReplicaDetail(
        AsyncOperationSPtr const &operation,
        __out DeployedServiceReplicaDetailQueryResult & outResult)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem(outResult);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetClusterLoadInformation(
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetClusterLoadInformation),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetClusterLoadInformation(
        AsyncOperationSPtr const &operation,
        __inout ClusterLoadInformationQueryResult & clusterLoadInformation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem(clusterLoadInformation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetPartitionLoadInformation(
        Guid const &partitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetPartitionLoadInformation),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetPartitionLoadInformation(
        AsyncOperationSPtr const &operation,
        __inout PartitionLoadInformationQueryResult &partitionInformation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem<PartitionLoadInformationQueryResult>(partitionInformation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetProvisionedFabricCodeVersionList(
        wstring const & codeVersionFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (!codeVersionFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Cluster::CodeVersionFilter,
                codeVersionFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetProvisionedFabricCodeVersionList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetProvisionedFabricCodeVersionList(
        AsyncOperationSPtr const &operation,
        __out vector<ProvisionedFabricCodeVersionQueryResultItem> &codeVersionList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<ProvisionedFabricCodeVersionQueryResultItem>(codeVersionList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetProvisionedFabricConfigVersionList(
        wstring const & configVersionFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (!configVersionFilter.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Cluster::ConfigVersionFilter,
                configVersionFilter);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetProvisionedFabricConfigVersionList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetProvisionedFabricConfigVersionList(
        AsyncOperationSPtr const &operation,
        __out vector<ProvisionedFabricConfigVersionQueryResultItem> &configVersionList)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveList<ProvisionedFabricConfigVersionQueryResultItem>(configVersionList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNodeLoadInformation(
        wstring const &nodeName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetNodeLoadInformation),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetNodeLoadInformation(
        AsyncOperationSPtr const &operation,
        __inout NodeLoadInformationQueryResult & nodeLoadInformation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem(nodeLoadInformation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetReplicaLoadInformation(
        Guid const &partitionId,
        int64 replicaOrInstanceId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        argMap.Insert(
            QueryResourceProperties::Replica::ReplicaId,
            StringUtility::ToWString(replicaOrInstanceId));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetReplicaLoadInformation),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetReplicaLoadInformation(
        AsyncOperationSPtr const &operation,
        __inout ReplicaLoadInformationQueryResult &replicaLoadInformation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem<ReplicaLoadInformationQueryResult>(replicaLoadInformation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetUnplacedReplicaInformation(
        std::wstring const &serviceName,
        Common::Guid const &partitionId,
        bool onlyQueryPrimaries,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Service::ServiceName,
            serviceName);

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        argMap.Insert(
            QueryResourceProperties::QueryMetadata::OnlyQueryPrimaries,
            StringUtility::ToWString(onlyQueryPrimaries));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetUnplacedReplicaInformation),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetUnplacedReplicaInformation(
        AsyncOperationSPtr const &operation,
        __inout UnplacedReplicaInformationQueryResult &unplacedReplicaInformation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem<UnplacedReplicaInformationQueryResult>(unplacedReplicaInformation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationLoadInformation(
        wstring const &applicationName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationLoadInformation),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationLoadInformation(
        AsyncOperationSPtr const &operation,
        __inout ApplicationLoadInformationQueryResult & applicationLoadInformation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem(applicationLoadInformation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceName(
        Guid const &partitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            partitionId.ToString());

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceName),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceName(
        AsyncOperationSPtr const &operation,
        __inout ServiceNameQueryResult & serviceName)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem(serviceName);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationName(
        Common::NamingUri const &serviceName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Service::ServiceName,
            serviceName.ToString());

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationName),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationName(
        AsyncOperationSPtr const &operation,
        __inout ApplicationNameQueryResult & applicationName)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem(applicationName);
    }

#pragma endregion

#pragma region IApplicationManagementClient methods

    //
    // IApplicationManagementClient methods
    //

    AsyncOperationSPtr FabricClientImpl::BeginProvisionApplicationType(
        ProvisionApplicationTypeDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetProvisionApplicationType(make_unique<ProvisionApplicationTypeDescription>(description));

            Trace.BeginProvisionApplicationType(traceContext_, message->ActivityId, description.ApplicationTypeBuildPath, description.ApplicationPackageDownloadUri);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndProvisionApplicationType(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateComposeDeployment(
        std::wstring const & deploymentName,
        wstring const& composeFilePath,
        wstring const& overridesFilePath,
        wstring const &repositoryUserName,
        wstring const &repositoryPassword,
        bool const isPasswordEncrypted,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        if (composeFilePath.empty() ||
            (repositoryUserName.empty() && !repositoryPassword.empty()))
        {
            error = ErrorCodeValue::InvalidArgument;
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginCreateComposeDeployment(traceContext_, activityId, deploymentName, composeFilePath, overridesFilePath);

        return AsyncOperation::CreateAndStart<CreateContainerAppAsyncOperation>(
            *this,
            deploymentName,
            composeFilePath,
            overridesFilePath,
            repositoryUserName,
            repositoryPassword,
            isPasswordEncrypted,
            activityId,
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateComposeDeployment(
        wstring const & deploymentName,
        Common::ByteBuffer && composeFile,
        Common::ByteBuffer && overridesFile,
        wstring const &repositoryUserName,
        wstring const &repositoryPassword,
        bool const isPasswordEncrypted,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        if (composeFile.empty())
        {
            error = ErrorCodeValue::InvalidArgument;
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginCreateComposeDeployment2(traceContext_, activityId, deploymentName);

        return BeginInnerCreateComposeDeployment(
            deploymentName,
            move(composeFile),
            move(overridesFile),
            repositoryUserName,
            repositoryPassword,
            isPasswordEncrypted,
            activityId,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndCreateComposeDeployment(AsyncOperationSPtr const &operation)
    {
        if (dynamic_cast<CreateContainerAppAsyncOperation*>(operation.get()) != NULL)
        {
            return CreateContainerAppAsyncOperation::End(operation);
        }
        else
        {
            return EndInnerCreateComposeDeployment(operation);
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginInnerCreateComposeDeployment(
        wstring const &deploymentName,
        ByteBuffer && composeFile,
        ByteBuffer && overridesFile,
        wstring const &repositoryUserName,
        wstring const &repositoryPassword,
        bool const isPasswordEncrypted,
        ActivityId const &activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        vector<ByteBuffer> composeFiles, sfSettingsFiles;
        composeFiles.push_back(move(composeFile));

        if (!overridesFile.empty())
        {
            composeFiles.push_back(move(overridesFile));
        }
        auto message = ContainerOperationTcpMessage::GetCreateComposeDeploymentMessage(
            deploymentName,
            L"",
            move(composeFiles),
            move(sfSettingsFiles),
            repositoryUserName,
            repositoryPassword,
            isPasswordEncrypted,
            activityId);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndInnerCreateComposeDeployment(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteComposeDeployment(
        wstring const & deploymentName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginDeleteComposeDeployment(traceContext_, activityId, deploymentName);

        message = ContainerOperationTcpMessage::GetDeleteContainerApplicationMessage(
            deploymentName,
            NamingUri(L""),
            activityId);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteComposeDeployment(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpgradeComposeDeployment(
        ComposeDeploymentUpgradeDescription const &description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginUpgradeComposeDeployment(
            traceContext_,
            activityId,
            description.DeploymentName,
            description.ComposeFiles.size(),
            description.SFSettingsFiles.size());

        message = ContainerOperationTcpMessage::GetUpgradeComposeDeploymentMessage(
            description,
            activityId);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndUpgradeComposeDeployment(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetComposeDeploymentUpgradeProgress(
        wstring const & deploymentName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;
        argMap.Insert(
            QueryResourceProperties::Deployment::DeploymentName,
            deploymentName);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetComposeDeploymentUpgradeProgress),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetComposeDeploymentUpgradeProgress(
        AsyncOperationSPtr const & operation,
        __inout ComposeDeploymentUpgradeProgress & result)
    {
        QueryResult queryResult;
        ErrorCode error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        return queryResult.MoveItem<ComposeDeploymentUpgradeProgress>(result);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRollbackComposeDeployment(
        wstring const & deploymentName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginRollbackComposeDeployment(traceContext_, activityId, deploymentName);

        message = ContainerOperationTcpMessage::GetRollbackComposeDeploymentMessage(
            deploymentName,
            activityId);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndRollbackComposeDeployment(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateOrUpdateApplicationResource(
        ModelV2::ApplicationDescription && description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;

        ErrorCode error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        error = description.TryValidate(L"");
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        message = ClusterManagerTcpMessage::GetCreateApplicationResource(make_unique<CreateApplicationResourceMessageBody>(move(description)));

        Trace.BeginCreateOrUpdateApplicationResource(traceContext_, message->ActivityId, description.Name, description.Services.size());

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);

    }

    ErrorCode FabricClientImpl::EndCreateOrUpdateApplicationResource(
        AsyncOperationSPtr const &operation,
        __out ModelV2::ApplicationDescription & description)
    {
        // TODO get the description with the status.
        ClientServerReplyMessageUPtr reply;
        description;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationResourceList(
        ServiceModel::ApplicationQueryDescription && applicationQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        // Inserts values into argMap based on data in queryDescription
        applicationQueryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationResourceList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationResourceList(
        AsyncOperationSPtr const &operation,
        vector<ModelV2::ApplicationDescriptionQueryResult> & list,
        PagingStatusUPtr & pagingStatus)
    {
        QueryResult queryResult;
        ErrorCode error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ModelV2::ApplicationDescriptionQueryResult>(list);
    }

    Common::AsyncOperationSPtr FabricClientImpl::BeginGetServiceResourceList(
        ServiceQueryDescription const& serviceQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            serviceQueryDescription.ApplicationName.ToString());

        if (!serviceQueryDescription.ServiceNameFilter.IsRootNamingUri)
        {
            argMap.Insert(
                QueryResourceProperties::Service::ServiceName,
                serviceQueryDescription.ServiceNameFilter.ToString());
        }

        if (!serviceQueryDescription.PagingDescription->ContinuationToken.empty())
        {
            argMap.Insert(
                QueryResourceProperties::QueryMetadata::ContinuationToken,
                serviceQueryDescription.PagingDescription->ContinuationToken);
        }

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceResourceList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceResourceList(
        AsyncOperationSPtr const &operation,
        vector<ModelV2::ContainerServiceQueryResult> & list,
        PagingStatusUPtr & pagingStatus)
    {
        QueryResult queryResult;
        ErrorCode error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ModelV2::ContainerServiceQueryResult>(list);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetReplicaResourceList(
        ReplicasResourceQueryDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;
        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            description.ApplicationUri.ToString());

        argMap.Insert(
            QueryResourceProperties::Service::ServiceName,
            description.ServiceName.ToString());

        if (description.ReplicaId >= 0)
        {
            argMap.Insert(
                QueryResourceProperties::Replica::ReplicaId,
                StringUtility::ToWString(description.ReplicaId));
        }

        description.PagingDescription.SetQueryArguments(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetReplicaResourceList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetReplicaResourceList(
        AsyncOperationSPtr const & operation,
        vector<ReplicaResourceQueryResult> & list,
        PagingStatusUPtr & pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ReplicaResourceQueryResult>(list);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetContainerCodePackageLogs(
        NamingUri const & applicationName,
        NamingUri const & serviceName,
        wstring replicaName,
        wstring const & codePackageName,
        ServiceModel::ContainerInfoArgMap & containerInfoArgMap,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        argMap.Insert(
            QueryResourceProperties::Service::ServiceName,
            serviceName.ToString());

        argMap.Insert(
            QueryResourceProperties::Replica::ReplicaId,
            replicaName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::CodePackageName,
            codePackageName);

        wstring containerInfoArgMapString;
        auto error = JsonHelper::Serialize(containerInfoArgMap, containerInfoArgMapString);
        ASSERT_IF(!error.IsSuccess(), "Serializing containerInfoArgMap failed - {0}", error);

        argMap.Insert(
            QueryResourceProperties::ContainerInfo::InfoArgsFilter,
            containerInfoArgMapString);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetContainerCodePackageLogs),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetContainerCodePackageLogs(
        AsyncOperationSPtr const & operation,
        __inout wstring &containerInfo)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        if (!error.IsSuccess())
        {
            return error;
        }
        return queryResult.MoveItem(containerInfo);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteSingleInstanceDeployment(
        DeleteSingleInstanceDeploymentDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();

        if (error.IsSuccess())
        {
            ActivityId activityId;
            Trace.BeginDeleteSingleInstanceDeployment(traceContext_, activityId, description);

            message = ContainerOperationTcpMessage::GetDeleteSingleInstanceDeploymentMessage(description, activityId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteSingleInstanceDeployment(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateVolume(
        VolumeDescriptionSPtr const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginCreateVolume(traceContext_, activityId, description->VolumeName);

        message = VolumeOperationTcpMessage::GetCreateVolumeMessage(description, activityId);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndCreateVolume(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetVolumeResourceList(
        ServiceModel::ModelV2::VolumeQueryDescription const & volumeQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;
        // Inserts values into argMap based on data in queryDescription
        volumeQueryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetVolumeResourceList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetVolumeResourceList(
        AsyncOperationSPtr const & operation,
        vector<ServiceModel::ModelV2::VolumeQueryResult> & list,
        PagingStatusUPtr & pagingStatus)
    {
        QueryResult queryResult;
        ErrorCode error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ModelV2::VolumeQueryResult>(list);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteVolume(
        wstring const & volumeName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        ActivityId activityId;
        Trace.BeginDeleteVolume(traceContext_, activityId, volumeName);

        message = VolumeOperationTcpMessage::GetDeleteVolumeMessage(volumeName, activityId);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteVolume(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateApplication(
        ApplicationDescriptionWrapper const &description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        map<wstring, wstring> applicationParametersMap;
        NamingUri applicationNameUri;
        ClientServerRequestMessageUPtr message;

        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        //
        // Creating applications with system application name is not allowed.
        //
        bool ret = GetFabricUri(description.ApplicationName, applicationNameUri, false);
        if (!ret)
        {
            WriteWarning(
                Constants::FabricClientSource,
                TraceContext,
                "BeginCreateApplication: invalid ApplicationName {0}",
                description.ApplicationName);

            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                ErrorCodeValue::InvalidNameUri);
        }

        //
        // Validate that the application name URI is valid (no reserved characters etc).
        //
        error = NamingUri::ValidateName(applicationNameUri, description.ApplicationName, true /*allowFragment*/);
        if (!error.IsSuccess())
        {
            WriteWarning(
                Constants::FabricClientSource,
                TraceContext,
                "BeginCreateApplication: validate app name failed: {0} {1}",
                error,
                error.Message);

            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                move(error));
        }

        message = ClusterManagerTcpMessage::GetCreateApplication(
            Common::make_unique<CreateApplicationMessageBody>(
                applicationNameUri,
                description.ApplicationTypeName,
                description.ApplicationTypeVersion,
                description.Parameters,
                description.ApplicationCapacity));

        Trace.BeginCreateApplication(traceContext_, message->ActivityId, applicationNameUri);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndCreateApplication(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    Common::AsyncOperationSPtr FabricClientImpl::BeginUpdateApplication(
        ServiceModel::ApplicationUpdateDescriptionWrapper const &updateDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback callback,
        Common::AsyncOperationSPtr const& parent)
    {
        ClientServerRequestMessageUPtr message;
        NamingUri applicationNameUri;

        // Get the application name URI. Scaleout is not supported for system applications.
        bool ret = GetFabricUri(updateDescription.ApplicationName, applicationNameUri, false);
        if (!ret)
        {
            WriteWarning(
                Constants::FabricClientSource,
                TraceContext,
                "BeginUpdateApplication: invalid ApplicationName {0}",
                updateDescription.ApplicationName);

            return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                *this,
                move(message),
                NamingUri(),
                timeout,
                callback,
                parent,
                ErrorCodeValue::InvalidNameUri);
        }

        message = ClusterManagerTcpMessage::GetUpdateApplication(
            Common::make_unique<UpdateApplicationMessageBody>(updateDescription));

        Trace.BeginUpdateApplication(traceContext_, message->ActivityId, applicationNameUri);

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    Common::ErrorCode FabricClientImpl::EndUpdateApplication(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteApplication(
        DeleteApplicationDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            if (description.IsForce)
            {
                message = ClusterManagerTcpMessage::GetDeleteApplication2(Common::make_unique<DeleteApplicationMessageBody>(description));
            }
            else
            {
                // For backward compatibility
                message = ClusterManagerTcpMessage::GetDeleteApplication(Common::make_unique<NamingUriMessageBody>(description.ApplicationName));
            }
            Trace.BeginDeleteApplication(traceContext_, message->ActivityId, description);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteApplication(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUnprovisionApplicationType(
        UnprovisionApplicationTypeDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetUnprovisionApplicationType(
                make_unique<UnprovisionApplicationTypeDescription>(description));

            Trace.BeginUnprovisionApplicationType(
                traceContext_,
                message->ActivityId,
                description.ApplicationTypeName,
                description.ApplicationTypeVersion);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUnprovisionApplicationType(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpgradeApplication(
        ApplicationUpgradeDescriptionWrapper const &upgradeDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            ApplicationUpgradeDescription internalDescription;
            error = internalDescription.FromWrapper(upgradeDescription);
            if (error.IsSuccess())
            {
                message = ClusterManagerTcpMessage::GetUpgradeApplication(Common::make_unique<Management::ClusterManager::ApplicationUpgradeDescription>(internalDescription));
                Trace.BeginUpgradeApplication(traceContext_, message->ActivityId, internalDescription.ApplicationName);
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpgradeApplication(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationUpgradeProgress(
        NamingUri const &applicationName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetGetUpgradeStatus(Common::make_unique<NamingUriMessageBody>(applicationName));
            Trace.BeginGetApplicationUpgradeProgress(traceContext_, message->ActivityId, applicationName);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetApplicationUpgradeProgress(
        AsyncOperationSPtr const &operation,
        __inout IApplicationUpgradeProgressResultPtr &upgradeDescriptionPtr)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            ApplicationUpgradeStatusDescription body;
            if (reply->GetBody(body))
            {
                ApplicationUpgradeProgressResultSPtr resultSPtr =
                    make_shared<ApplicationUpgradeProgressResult>(move(body));
                upgradeDescriptionPtr = RootedObjectPointer<IApplicationUpgradeProgressResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginMoveNextApplicationUpgradeDomain(
        IApplicationUpgradeProgressResultPtr const &progressPtr,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            NamingUri applicationNameUri;
            auto ret = GetFabricUri(progressPtr->GetApplicationName().ToString(), applicationNameUri, false);
            if (!ret)
            {
                WriteWarning(
                    Constants::FabricClientSource,
                    TraceContext,
                    "BeginMoveNextApplicationUpgradeDomain: invalid ApplicationName {0}",
                    progressPtr->GetApplicationName());

                return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
                    *this,
                    move(message),
                    NamingUri(),
                    timeout,
                    callback,
                    parent,
                    ErrorCodeValue::InvalidNameUri);
            }

            wstring inProgressDomain;
            vector<wstring> pendingDomains;
            vector<wstring> completedDomains;
            error = progressPtr->GetUpgradeDomains(inProgressDomain, pendingDomains, completedDomains);
            if (error.IsSuccess())
            {
                uint64 upgradeInstance = progressPtr->GetUpgradeInstance();
                message = ClusterManagerTcpMessage::GetReportUpgradeHealth(
                    Common::make_unique<ReportUpgradeHealthMessageBody>(
                        applicationNameUri,
                        move(completedDomains),
                        upgradeInstance));

                Trace.BeginMoveNextApplicationUpgradeDomain(traceContext_, message->ActivityId, applicationNameUri);
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndMoveNextApplicationUpgradeDomain(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginMoveNextApplicationUpgradeDomain2(
        NamingUri const &applicationName,
        wstring const &nextUpgradeDomain,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetMoveNextUpgradeDomain(
                Common::make_unique<MoveNextUpgradeDomainMessageBody>(
                    applicationName,
                    nextUpgradeDomain));

            Trace.BeginMoveNextApplicationUpgradeDomain2(
                traceContext_,
                message->ActivityId,
                applicationName,
                nextUpgradeDomain);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndMoveNextApplicationUpgradeDomain2(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationManifest(
        wstring const &applicationTypeName,
        wstring const &applicationTypeVersion,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeName,
            applicationTypeName);

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeVersion,
            applicationTypeVersion);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationManifest),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationManifest(
        AsyncOperationSPtr const &operation,
        __inout wstring &applicationManifest)
    {
        QueryResult queryResult;
        auto err = EndInternalQuery(operation, queryResult);
        if (!err.IsSuccess())
        {
            return err;
        }
        return queryResult.MoveItem(applicationManifest);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRollbackApplicationUpgrade(
        NamingUri const & applicationName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetRollbackApplicationUpgrade(
                Common::make_unique<RollbackApplicationUpgradeMessageBody>(applicationName));

            Trace.BeginRollbackApplicationUpgrade(
                traceContext_,
                message->ActivityId,
                applicationName);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            applicationName,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRollbackApplicationUpgrade(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpdateApplicationUpgrade(
        ApplicationUpgradeUpdateDescription const &description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetUpdateApplicationUpgrade(
                Common::make_unique<UpdateApplicationUpgradeMessageBody>(description));

            Trace.BeginUpdateApplicationUpgrade(
                traceContext_,
                message->ActivityId,
                description.ApplicationName);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpdateApplicationUpgrade(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeployServicePackageToNode(
        wstring const & applicationTypeName,
        wstring const & applicationTypeVersion,
        wstring const & serviceManifestName,
        wstring const & sharingPolicies,
        wstring const & nodeName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeName,
            applicationTypeName);

        argMap.Insert(
            QueryResourceProperties::ApplicationType::ApplicationTypeVersion,
            applicationTypeVersion);

        argMap.Insert(
            QueryResourceProperties::ServiceType::ServiceManifestName,
            serviceManifestName);

        argMap.Insert(
            QueryResourceProperties::ServiceType::SharedPackages,
            sharingPolicies);

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::DeployServicePackageToNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeployServicePackageToNode(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto err = EndInternalQuery(operation, queryResult);
        if (!err.IsSuccess())
        {
            return err;
        }
        return err;
    }

#pragma endregion

#pragma region IClusterManagementClient methods

    //
    // IClusterManagementClient methods
    //

    AsyncOperationSPtr FabricClientImpl::BeginDeactivateNode(
        wstring const &nodeName,
        FABRIC_NODE_DEACTIVATION_INTENT const deactivationIntent,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<DeactivateNodeMessageBody>(nodeName, deactivationIntent);
            message = NamingTcpMessage::GetDeactivateNode(std::move(body));
            Trace.BeginDeactivateNode(traceContext_, message->ActivityId, nodeName);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeactivateNode(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginActivateNode(
        wstring const &nodeName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetActivateNode(nodeName);
            Trace.BeginActivateNode(traceContext_, message->ActivityId, nodeName);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndActivateNode(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeactivateNodesBatch(
        map<Federation::NodeId, Reliability::NodeDeactivationIntent::Enum> const & deactivations,
        wstring const & batchId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            // Gateway will populate instance
            auto body = Common::make_unique<DeactivateNodesRequestMessageBody>(deactivations, batchId, 0);
            message = NamingTcpMessage::GetDeactivateNodesBatch(std::move(body));

            WriteInfo(
                Constants::FabricClientSource,
                TraceContext,
                "{0} DeactivateNodesBatch: {1} : {2}",
                message->ActivityId,
                batchId,
                deactivations);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeactivateNodesBatch(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRemoveNodeDeactivations(
        wstring const & batchId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            // Gateway will populate instance
            auto body = Common::make_unique<RemoveNodeDeactivationRequestMessageBody>(batchId, 0);
            message = NamingTcpMessage::GetRemoveNodeDeactivations(std::move(body));

            WriteInfo(
                Constants::FabricClientSource,
                TraceContext,
                "{0} RemoveNodeDeactivations: {1}",
                message->ActivityId,
                batchId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRemoveNodeDeactivations(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNodeDeactivationStatus(
        wstring const & batchId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<NodeDeactivationStatusRequestMessageBody>(batchId);
            message = NamingTcpMessage::GetNodeDeactivationStatus(std::move(body));

            WriteInfo(
                Constants::FabricClientSource,
                TraceContext,
                "{0} GetNodeDeactivationStatus: {1}",
                message->ActivityId,
                batchId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetNodeDeactivationStatus(
        AsyncOperationSPtr const &operation,
        __out NodeDeactivationStatus::Enum & status)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = RequestReplyAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            NodeDeactivationStatusReplyMessageBody body;
            if (reply->GetBody(body))
            {
                status = body.DeactivationStatus;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginNodeStateRemoved(
        wstring const &nodeName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetNodeStateRemoved(nodeName);
            Trace.BeginNodeStateRemoved(traceContext_, message->ActivityId, nodeName);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndNodeStateRemoved(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRecoverPartitions(
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetRecoverPartitions();
            Trace.BeginRecoverPartitions(traceContext_, message->ActivityId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRecoverPartitions(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRecoverPartition(
        Guid partitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetRecoverPartition(PartitionId(partitionId));
            Trace.BeginRecoverPartition(traceContext_, message->ActivityId, partitionId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRecoverPartition(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRecoverServicePartitions(
        wstring const& serviceName,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetRecoverServicePartitions(serviceName);
            Trace.BeginRecoverServicePartitions(traceContext_, message->ActivityId, serviceName);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRecoverServicePartitions(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRecoverSystemPartitions(
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetRecoverSystemPartitions();
            Trace.BeginRecoverSystemPartitions(traceContext_, message->ActivityId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRecoverSystemPartitions(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginProvisionFabric(
        wstring const &codeFilePath,
        wstring const &clusterManifestFilepath,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetProvisionFabric(Common::make_unique<ProvisionFabricBody>(codeFilePath, clusterManifestFilepath));
            Trace.BeginProvisionFabric(traceContext_, message->ActivityId, codeFilePath, clusterManifestFilepath);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndProvisionFabric(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpgradeFabric(
        FabricUpgradeDescriptionWrapper const &wrapper,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto description = Common::make_unique<FabricUpgradeDescription>();
            if (description)
            {
                error = description->FromWrapper(wrapper);
                if (error.IsSuccess())
                {
                    Common::FabricVersion const & version = description->Version;
                    message = ClusterManagerTcpMessage::GetUpgradeFabric(std::move(description));
                    Trace.BeginUpgradeFabric(traceContext_, message->ActivityId, version);
                }
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpgradeFabric(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginRollbackFabricUpgrade(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetRollbackFabricUpgrade();

            Trace.BeginRollbackFabricUpgrade(
                traceContext_,
                message->ActivityId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRollbackFabricUpgrade(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpdateFabricUpgrade(
        Management::ClusterManager::FabricUpgradeUpdateDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetUpdateFabricUpgrade(Common::make_unique<UpdateFabricUpgradeMessageBody>(description));
            Trace.BeginUpdateFabricUpgrade(traceContext_, message->ActivityId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpdateFabricUpgrade(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetFabricUpgradeProgress(
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetGetFabricUpgradeStatus();
            Trace.BeginGetFabricUpgradeProgress(traceContext_, message->ActivityId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetFabricUpgradeProgress(
        AsyncOperationSPtr const &operation,
        __inout IUpgradeProgressResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            FabricUpgradeStatusDescription body;
            if (reply->GetBody(body))
            {
                UpgradeProgressResultSPtr resultSPtr =
                    make_shared<UpgradeProgressResult>(move(body));
                result = RootedObjectPointer<IUpgradeProgressResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetInvokeDataLossProgress(
        Common::Guid const & operationId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            GetInvokeDataLossProgressMessageBody body(operationId);

            auto bdy = Common::make_unique<GetInvokeDataLossProgressMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetGetPartitionDataLossProgress(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetInvokeDataLossProgress(
        AsyncOperationSPtr const &operation,
        __inout IInvokeDataLossProgressResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            GetInvokeDataLossProgressReplyMessageBody body;
            if (reply->GetBody<GetInvokeDataLossProgressReplyMessageBody>(body))
            {
                InvokeDataLossProgress progress;

                if (body.Progress.State == TestCommandProgressState::Completed
                    || body.Progress.State == TestCommandProgressState::Faulted)
                {
                    progress = InvokeDataLossProgress(move(body.Progress));
                }
                else
                {
                    progress = InvokeDataLossProgress(body.Progress.State, nullptr);
                }

                auto progressSPtr = make_shared<InvokeDataLossProgress>(move(progress));

                InvokeDataLossProgressResultSPtr resultSPtr =
                    make_shared<InvokeDataLossProgressResult>(move(progressSPtr));

                result = RootedObjectPointer<IInvokeDataLossProgressResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    // GetInvokeQuorumLoss
    AsyncOperationSPtr FabricClientImpl::BeginGetInvokeQuorumLossProgress(
        Common::Guid const & operationId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            GetInvokeQuorumLossProgressMessageBody body(operationId);

            auto bdy = Common::make_unique<GetInvokeQuorumLossProgressMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetGetPartitionQuorumLossProgress(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetInvokeQuorumLossProgress(
        AsyncOperationSPtr const &operation,
        __inout IInvokeQuorumLossProgressResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            GetInvokeQuorumLossProgressReplyMessageBody body;
            if (reply->GetBody<GetInvokeQuorumLossProgressReplyMessageBody>(body))
            {
                InvokeQuorumLossProgress progress;

                if (body.Progress.State == TestCommandProgressState::Completed
                    || body.Progress.State == TestCommandProgressState::Faulted)
                {
                    progress = InvokeQuorumLossProgress(move(body.Progress));
                }
                else
                {
                    progress = InvokeQuorumLossProgress(body.Progress.State, nullptr);
                }

                auto progressSPtr = make_shared<InvokeQuorumLossProgress>(move(progress));

                InvokeQuorumLossProgressResultSPtr resultSPtr =
                    make_shared<InvokeQuorumLossProgressResult>(move(progressSPtr));

                result = RootedObjectPointer<IInvokeQuorumLossProgressResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    // GetRestartPartitionProgress
    AsyncOperationSPtr FabricClientImpl::BeginGetRestartPartitionProgress(
        Common::Guid const & operationId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            GetRestartPartitionProgressMessageBody body(operationId);

            auto bdy = Common::make_unique<GetRestartPartitionProgressMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetGetPartitionRestartProgress(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetRestartPartitionProgress(
        AsyncOperationSPtr const &operation,
        __inout IRestartPartitionProgressResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            GetRestartPartitionProgressReplyMessageBody body;
            if (reply->GetBody<GetRestartPartitionProgressReplyMessageBody>(body))
            {
                RestartPartitionProgress progress;

                if (body.Progress.State == TestCommandProgressState::Completed
                    || body.Progress.State == TestCommandProgressState::Faulted)
                {
                    progress = RestartPartitionProgress(move(body.Progress));
                }
                else
                {
                    progress = RestartPartitionProgress(body.Progress.State, nullptr);
                }

                auto progressSPtr = make_shared<RestartPartitionProgress>(move(progress));

                RestartPartitionProgressResultSPtr resultSPtr =
                    make_shared<RestartPartitionProgressResult>(move(progressSPtr));

                result = RootedObjectPointer<IRestartPartitionProgressResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetTestCommandStatusList(
        DWORD stateFilter,
        DWORD typeFilter,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Action::StateFilter,
            StringUtility::ToWString<DWORD>(stateFilter));

        argMap.Insert(
            QueryResourceProperties::Action::TypeFilter,
            StringUtility::ToWString<DWORD>(typeFilter));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetTestCommandList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetTestCommandStatusList(
        AsyncOperationSPtr const &operation,
        __inout vector<TestCommandListQueryResult> &result)
    {
        QueryResult queryResult;

        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            if (IsFASOrUOSReconfiguring(error))
            {
                error = ErrorCodeValue::ReconfigurationPending;
            }

            WriteWarning(
                Constants::FabricClientSource,
                traceContext_,
                "EndGetTestCommandStatusList returning error {0} after EndInternalQuery", error);
            return error;
        }

        error = queryResult.MoveList<TestCommandListQueryResult>(result);
        if (!error.IsSuccess())
        {
            if (IsFASOrUOSReconfiguring(error))
            {
                error = ErrorCodeValue::ReconfigurationPending;
            }

            WriteWarning(
                Constants::FabricClientSource,
                traceContext_,
                "EndGetTestCommandStatusList returning error {0} after MoveList", error);
            return error;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNodeTransitionProgress(
        Common::Guid const & operationId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            GetNodeTransitionProgressMessageBody body(operationId);

            auto bdy = Common::make_unique<GetNodeTransitionProgressMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetNodeTransitionProgress(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetNodeTransitionProgress(
        AsyncOperationSPtr const &operation,
        __inout INodeTransitionProgressResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            GetNodeTransitionProgressReplyMessageBody body;
            if (reply->GetBody<GetNodeTransitionProgressReplyMessageBody>(body))
            {
                NodeTransitionProgress progress;

                if (body.Progress.State == TestCommandProgressState::Completed
                    || body.Progress.State == TestCommandProgressState::Faulted)
                {
                    progress = NodeTransitionProgress(move(body.Progress));
                }
                else
                {
                    progress = NodeTransitionProgress(body.Progress.State, nullptr);
                }

                auto progressSPtr = make_shared<NodeTransitionProgress>(move(progress));

                NodeTransitionProgressResultSPtr resultSPtr =
                    make_shared<NodeTransitionProgressResult>(move(progressSPtr));

                result = RootedObjectPointer<INodeTransitionProgressResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginMoveNextFabricUpgradeDomain(
        IUpgradeProgressResultPtr const &progressPtr,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            wstring inProgressDomain;
            vector<wstring> pendingDomains;
            vector<wstring> completedDomains;

            error = progressPtr->GetUpgradeDomains(inProgressDomain, pendingDomains, completedDomains);
            if (error.IsSuccess())
            {
                message = ClusterManagerTcpMessage::GetReportFabricUpgradeHealth(
                    Common::make_unique<ReportUpgradeHealthMessageBody>(
                        NamingUri::RootNamingUri,
                        move(completedDomains),
                        progressPtr->GetUpgradeInstance()));

                Trace.BeginMoveNextFabricUpgradeDomain(traceContext_, message->ActivityId, inProgressDomain);
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndMoveNextFabricUpgradeDomain(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginUnprovisionFabric(
        FabricCodeVersion const &codeVersion,
        FabricConfigVersion const &configVersion,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetUnprovisionFabric(Common::make_unique<FabricVersionMessageBody>(FabricVersion(codeVersion, configVersion)));
            Trace.BeginUnprovisionFabric(traceContext_, message->ActivityId, codeVersion, configVersion);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUnprovisionFabric(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetClusterManifest(
        Management::ClusterManager::ClusterManifestQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (queryDescription.ConfigVersion.IsValid)
        {
            argMap.Insert(QueryResourceProperties::Cluster::ConfigVersionFilter, queryDescription.ConfigVersion.ToString());
        }

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetClusterManifest),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetClusterManifest(
        AsyncOperationSPtr const &operation,
        __inout wstring &clusterManifest)
    {
        QueryResult queryResult;
        auto err = EndInternalQuery(operation, queryResult);
        if (!err.IsSuccess())
        {
            return err;
        }
        return queryResult.MoveItem(clusterManifest);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetClusterVersion(
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetClusterVersion),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetClusterVersion(
        AsyncOperationSPtr const &operation,
        __inout std::wstring &clusterVersion)
    {
        QueryResult queryResult;
        auto err = EndInternalQuery(operation, queryResult);
        if (!err.IsSuccess())
        {
            return err;
        }
        return queryResult.MoveItem(clusterVersion);
    }

    AsyncOperationSPtr FabricClientImpl::BeginResetPartitionLoad(
        Guid partitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetResetPartitionLoad(PartitionId(partitionId));
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndResetPartitionLoad(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginToggleVerboseServicePlacementHealthReporting(
        bool enabled,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetToggleVerboseServicePlacementHealthReporting(enabled);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndToggleVerboseServicePlacementHealthReporting(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginMoveNextFabricUpgradeDomain2(
        wstring const &nextDomain,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetMoveNextFabricUpgradeDomain(
                Common::make_unique<MoveNextUpgradeDomainMessageBody>(
                    NamingUri::RootNamingUri,
                    nextDomain));

            Trace.BeginMoveNextFabricUpgradeDomain2(traceContext_, message->ActivityId, nextDomain);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndMoveNextFabricUpgradeDomain2(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginStartInfrastructureTask(
        InfrastructureTaskDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetStartInfrastructureTask(Common::make_unique<InfrastructureTaskDescription>(description));

            Trace.BeginStartInfrastructureTask(
                traceContext_,
                message->ActivityId,
                description);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndStartInfrastructureTask(AsyncOperationSPtr const &operation, __out bool & isComplete)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            InfrastructureTaskReplyBody body;
            if (!reply->GetBody(body))
            {
                WriteWarning(
                    Constants::FabricClientSource,
                    TraceContext,
                    "failed to read start infrastructure task reply body");
                return reply->Headers.FaultErrorCodeValue;
            }

            isComplete = body.IsComplete;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginFinishInfrastructureTask(
        TaskInstance const & taskInstance,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = ClusterManagerTcpMessage::GetFinishInfrastructureTask(Common::make_unique<TaskInstance>(taskInstance));

            Trace.BeginFinishInfrastructureTask(
                traceContext_,
                message->ActivityId,
                taskInstance);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri(),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndFinishInfrastructureTask(
        AsyncOperationSPtr const &operation,
        __out bool & isComplete)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            InfrastructureTaskReplyBody body;
            if (!reply->GetBody(body))
            {
                WriteWarning(
                    Constants::FabricClientSource,
                    TraceContext,
                    "failed to read finish infrastructure task reply body");
                return reply->Headers.FaultErrorCodeValue;
            }

            isComplete = body.IsComplete;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetClusterConfiguration(
        wstring const & apiVersion,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            GetClusterConfigurationMessageBody body(apiVersion);
            auto bdy = Common::make_unique<GetClusterConfigurationMessageBody>(body);
            message = UpgradeOrchestrationServiceTcpMessage::GetClusterConfiguration(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetClusterConfiguration(
        AsyncOperationSPtr const &operation,
        __inout wstring &clusterConfiguration)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            GetClusterConfigurationReplyMessageBody body;
            if (reply->GetBody<GetClusterConfigurationReplyMessageBody>(body))
            {
                clusterConfiguration = move(body.ClusterConfiguration);
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetUpgradeOrchestrationServiceState(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = SystemServiceTcpMessageBase::GetSystemServiceMessage<UpgradeOrchestrationServiceTcpMessage>(UpgradeOrchestrationServiceTcpMessage::GetUpgradeOrchestrationServiceStateAction);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetUpgradeOrchestrationServiceState(
        AsyncOperationSPtr const &operation,
        __inout wstring &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                result = move(body.Content);
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginSetUpgradeOrchestrationServiceState(
        std::wstring const & state,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ClientServerRequestMessageUPtr message = SystemServiceTcpMessageBase::GetSystemServiceMessage<UpgradeOrchestrationServiceTcpMessage>(
            UpgradeOrchestrationServiceTcpMessage::SetUpgradeOrchestrationServiceStateAction,
            Common::make_unique<SystemServiceMessageBody>(state));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndSetUpgradeOrchestrationServiceState(
        AsyncOperationSPtr const & operation,
        __inout Api::IFabricUpgradeOrchestrationServiceStateResultPtr & result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                UpgradeOrchestrationServiceState state;
                error = JsonHelper::Deserialize(state, move(body.Content));
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        Constants::FabricClientSource,
                        traceContext_,
                        "SetUpgradeOrchestrationServiceState returning error {0} during message deserialization", error);

                    return error;
                }

                auto stateSPtr = make_shared<UpgradeOrchestrationServiceState>(move(state));
                UpgradeOrchestrationServiceStateResultSPtr resultSPtr = make_shared<UpgradeOrchestrationServiceStateResult>(move(stateSPtr));
                result = RootedObjectPointer<IUpgradeOrchestrationServiceStateResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

#pragma endregion

#pragma region ISecretStoreClient methods

    AsyncOperationSPtr FabricClientImpl::BeginGetSecrets(
        GetSecretsDescription & getSecretsDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ErrorCode error = ErrorCode::Success();

        error = getSecretsDescription.Validate();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        auto messageUPtr =
            CentralSecretServiceMessage::CreateRequestMessage(
                CentralSecretServiceMessage::GetSecretsAction,
                make_unique<GetSecretsDescription>(getSecretsDescription));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(messageUPtr),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetSecrets(
        AsyncOperationSPtr const & operation,
        __out SecretsDescription & result)
    {
        ClientServerReplyMessageUPtr reply;

        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }
        else if (dynamic_cast<ForwardToServiceAsyncOperation*>(operation.get()) != NULL)
        {
            auto error = ForwardToServiceAsyncOperation::End(operation, reply);

            if (error.IsSuccess())
            {
                if (!reply->GetBody<SecretsDescription>(result))
                {
                    error = ErrorCode::FromNtStatus(reply->GetStatus());
                }
            }

            return error;
        }
        else
        {
            Assert::CodingError("ISecretStoreClient.EndGetSecrets(): Unknown operation passed in.");
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginSetSecrets(
        SecretsDescription & secretsDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ErrorCode error = ErrorCode::Success();

        error = secretsDescription.Validate();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ClientServerRequestMessageUPtr messageUPtr =
            CentralSecretServiceMessage::CreateRequestMessage(
                CentralSecretServiceMessage::SetSecretsAction,
                make_unique<SecretsDescription>(secretsDescription));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(messageUPtr),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndSetSecrets(
        AsyncOperationSPtr const & operation,
        __out SecretsDescription & result)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }
        else if (dynamic_cast<ForwardToServiceAsyncOperation*>(operation.get()) != NULL)
        {
            ClientServerReplyMessageUPtr reply;
            auto error = ForwardToServiceAsyncOperation::End(operation, reply);

            if (error.IsSuccess())
            {
                if (!reply->GetBody<SecretsDescription>(result))
                {
                    error = ErrorCode::FromNtStatus(reply->GetStatus());
                }
            }

            return error;
        }
        else
        {
            Assert::CodingError("ISecretStoreClient.EndSetSecrets(): Unknown operation passed in.");
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginRemoveSecrets(
        SecretReferencesDescription & secretReferencesDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ErrorCode error = ErrorCode::Success();

        error = secretReferencesDescription.Validate();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ClientServerRequestMessageUPtr messageUPtr =
            CentralSecretServiceMessage::CreateRequestMessage(
                CentralSecretServiceMessage::RemoveSecretsAction,
                make_unique<SecretReferencesDescription>(secretReferencesDescription));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(messageUPtr),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRemoveSecrets(
        AsyncOperationSPtr const & operation,
        __out SecretReferencesDescription & result)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }
        else if (dynamic_cast<ForwardToServiceAsyncOperation*>(operation.get()) != NULL)
        {
            ClientServerReplyMessageUPtr reply;
            auto error = ForwardToServiceAsyncOperation::End(operation, reply);

            if (error.IsSuccess())
            {
                if (reply->GetBody<SecretReferencesDescription>(result))
                {
                    error = ErrorCode::FromNtStatus(reply->GetStatus());
                }
            }

            return error;
        }
        else
        {
            Assert::CodingError("ISecretStoreClient.EndRemoveSecrets(): Unknown operation passed in.");
        }
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetSecretVersions(
        SecretReferencesDescription & secretReferencesDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ErrorCode error = ErrorCode::Success();

        error = secretReferencesDescription.Validate();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        ClientServerRequestMessageUPtr messageUPtr =
            CentralSecretServiceMessage::CreateRequestMessage(
                CentralSecretServiceMessage::GetSecretVersionsAction,
                make_unique<SecretReferencesDescription>(secretReferencesDescription));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(messageUPtr),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetSecretVersions(
        AsyncOperationSPtr const & operation,
        __out SecretReferencesDescription & result)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }
        else if (dynamic_cast<ForwardToServiceAsyncOperation*>(operation.get()) != NULL)
        {
            ClientServerReplyMessageUPtr reply;
            auto error = ForwardToServiceAsyncOperation::End(operation, reply);

            if (error.IsSuccess())
            {
                if (reply->GetBody<SecretReferencesDescription>(result))
                {
                    error = ErrorCode::FromNtStatus(reply->GetStatus());
                }
            }

            return error;
        }
        else
        {
            Assert::CodingError("ISecretStoreClient.EndGetSecretVersions(): Unknown operation passed in.");
        }
    }

#pragma endregion

#pragma region IResourceManagerClient methods

    AsyncOperationSPtr FabricClientImpl::BeginClaimResource(
        Management::ResourceManager::Claim const & claim,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ErrorCode error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        auto msgUPtr = ResourceManagerMessage::CreateRequestMessage(
            ResourceManagerMessage::ClaimResourceAction,
            claim.ResourceId,
            make_unique<Management::ResourceManager::Claim>(claim),
            ActivityId());

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(msgUPtr),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndClaimResource(
        AsyncOperationSPtr const & operation,
        __out ResourceMetadata & result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            if (reply->GetBody<ResourceMetadata>(result))
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginReleaseResource(
        Management::ResourceManager::Claim const & claim,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ErrorCode error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        auto msgUPtr = ResourceManagerMessage::CreateRequestMessage(
            ResourceManagerMessage::ReleaseResourceAction,
            claim.ResourceId,
            make_unique<Management::ResourceManager::Claim>(claim),
            ActivityId());

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(msgUPtr),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndReleaseResource(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }

        return error;
    }

#pragma endregion

#pragma region IRepairManagementClient methods

    AsyncOperationSPtr FabricClientImpl::BeginCreateRepairTask(
        RepairTask const & repairTask,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = RepairManagerTcpMessage::GetCreateRepairRequest(Common::make_unique<UpdateRepairTaskMessageBody>(repairTask));

            Trace.BeginCreateRepairRequest(
                traceContext_,
                message->ActivityId,
                repairTask);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCreateRepairTask(
        AsyncOperationSPtr const & operation,
        __out int64 & commitVersion)
    {
        commitVersion = 0;

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            UpdateRepairTaskReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                commitVersion = replyBody.CommitVersion;
            }
        }

        Trace.EndCreateRepairRequest(
            traceContext_,
            activityId,
            error,
            commitVersion);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCancelRepairTask(
        CancelRepairTaskMessageBody const & requestBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = RepairManagerTcpMessage::GetCancelRepairRequest(Common::make_unique<CancelRepairTaskMessageBody>(requestBody));

            Trace.BeginCancelRepairRequest(
                traceContext_,
                message->ActivityId,
                requestBody.TaskId,
                requestBody.Version,
                requestBody.RequestAbort);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCancelRepairTask(
        AsyncOperationSPtr const & operation,
        __out int64 & commitVersion)
    {
        commitVersion = 0;

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            UpdateRepairTaskReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                commitVersion = replyBody.CommitVersion;
            }
        }

        Trace.EndCancelRepairRequest(
            traceContext_,
            activityId,
            error,
            commitVersion);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginForceApproveRepairTask(
        ApproveRepairTaskMessageBody const & requestBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = RepairManagerTcpMessage::GetForceApproveRepair(Common::make_unique<ApproveRepairTaskMessageBody>(requestBody));

            Trace.BeginForceApproveRepair(
                traceContext_,
                message->ActivityId,
                requestBody.TaskId,
                requestBody.Version);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndForceApproveRepairTask(
        AsyncOperationSPtr const & operation,
        __out int64 & commitVersion)
    {
        commitVersion = 0;

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            UpdateRepairTaskReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                commitVersion = replyBody.CommitVersion;
            }
        }

        Trace.EndForceApproveRepair(
            traceContext_,
            activityId,
            error,
            commitVersion);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteRepairTask(
        DeleteRepairTaskMessageBody const & requestBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = RepairManagerTcpMessage::GetDeleteRepairRequest(Common::make_unique<DeleteRepairTaskMessageBody>(requestBody));

            Trace.BeginDeleteRepairRequest(
                traceContext_,
                message->ActivityId,
                requestBody.TaskId,
                requestBody.Version);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeleteRepairTask(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);

        Trace.EndDeleteRepairRequest(
            traceContext_,
            activityId,
            error);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpdateRepairExecutionState(
        RepairTask const & repairTask,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = RepairManagerTcpMessage::GetUpdateRepairExecutionState(Common::make_unique<UpdateRepairTaskMessageBody>(repairTask));

            Trace.BeginUpdateRepairExecutionState(
                traceContext_,
                message->ActivityId,
                repairTask);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpdateRepairExecutionState(
        AsyncOperationSPtr const & operation,
        __out int64 & commitVersion)
    {
        commitVersion = 0;

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            UpdateRepairTaskReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                commitVersion = replyBody.CommitVersion;
            }
        }

        Trace.EndUpdateRepairExecutionState(
            traceContext_,
            activityId,
            error,
            commitVersion);

        return error;
    }

    // TODO: scope should be a const reference, but that requires making entire JSON writer const
    AsyncOperationSPtr FabricClientImpl::BeginGetRepairTaskList(
        RepairScopeIdentifierBase & scope,
        wstring const & taskIdFilter,
        RepairTaskState::Enum stateFilter,
        wstring const & executorFilter,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(QueryResourceProperties::Repair::TaskIdPrefix, taskIdFilter);
        argMap.Insert(QueryResourceProperties::Repair::ExecutorName, executorFilter);

        wstring stateFilterString;
        StringWriter(stateFilterString).Write("{0}", stateFilter);
        argMap.Insert(QueryResourceProperties::Repair::StateMask, stateFilterString);

        wstring scopeString;
        auto error = JsonHelper::Serialize(scope, scopeString);
        if (error.IsSuccess())
        {
            argMap.Insert(QueryResourceProperties::Repair::Scope, scopeString);
        }

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetRepairList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetRepairTaskList(
        AsyncOperationSPtr const & operation,
        __out vector<RepairTask> & result)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveList<RepairTask>(result);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpdateRepairTaskHealthPolicy(
        UpdateRepairTaskHealthPolicyMessageBody const & requestBody,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = RepairManagerTcpMessage::GetUpdateRepairTaskHealthPolicy(Common::make_unique<UpdateRepairTaskHealthPolicyMessageBody>(requestBody));

            Trace.BeginUpdateRepairTaskHealthPolicy(
                traceContext_,
                message->ActivityId,
                requestBody.TaskId,
                requestBody.Version);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpdateRepairTaskHealthPolicy(
        AsyncOperationSPtr const & operation,
        __out int64 & commitVersion)
    {
        commitVersion = 0;

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            UpdateRepairTaskReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                commitVersion = replyBody.CommitVersion;
            }
        }

        Trace.EndUpdateRepairTaskHealthPolicy(
            traceContext_,
            activityId,
            error,
            commitVersion);

        return error;
    }

#pragma endregion

#pragma region INetworkManagementClient methods

    AsyncOperationSPtr FabricClientImpl::BeginCreateNetwork(
        ModelV2::NetworkResourceDescription const &description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<CreateNetworkMessageBody>(description);
            message = NamingTcpMessage::GetCreateNetwork(std::move(body));

            Trace.BeginCreateNetwork(traceContext_, message->ActivityId, description.Name);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCreateNetwork(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteNetwork(
        DeleteNetworkDescription const &deleteDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<DeleteNetworkMessageBody>(deleteDescription);
            message = NamingTcpMessage::GetDeleteNetwork(std::move(body));

            Trace.BeginDeleteNetwork(traceContext_, message->ActivityId, deleteDescription.NetworkName);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeleteNetwork(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNetworkList(
        NetworkQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetNetworkList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetNetworkList(
        AsyncOperationSPtr const &operation,
        __inout vector<ModelV2::NetworkResourceDescriptionQueryResult> &networkList,
        __inout PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ModelV2::NetworkResourceDescriptionQueryResult>(networkList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNetworkApplicationList(
        NetworkApplicationQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetNetworkApplicationList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetNetworkApplicationList(
        AsyncOperationSPtr const &operation,
        __inout vector<NetworkApplicationQueryResult> &networkApplicationList,
        __inout PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<NetworkApplicationQueryResult>(networkApplicationList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetNetworkNodeList(
        NetworkNodeQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetNetworkNodeList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetNetworkNodeList(
        AsyncOperationSPtr const &operation,
        __inout vector<NetworkNodeQueryResult> &networkNodeList,
        __inout PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<NetworkNodeQueryResult>(networkNodeList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationNetworkList(
        ApplicationNetworkQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationNetworkList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationNetworkList(
        AsyncOperationSPtr const &operation,
        __inout vector<ApplicationNetworkQueryResult> &applicationNetworkList,
        __inout PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<ApplicationNetworkQueryResult>(applicationNetworkList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedNetworkList(
        DeployedNetworkQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeployedNetworkList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedNetworkList(
        AsyncOperationSPtr const &operation,
        __inout vector<DeployedNetworkQueryResult> &deployedNetworkList,
        __inout PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<DeployedNetworkQueryResult>(deployedNetworkList);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedNetworkCodePackageList(
        DeployedNetworkCodePackageQueryDescription const &queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        queryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeployedNetworkCodePackageList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedNetworkCodePackageList(
        AsyncOperationSPtr const &operation,
        __inout vector<DeployedNetworkCodePackageQueryResult> &deployedNetworkCodePackageList,
        __inout PagingStatusUPtr &pagingStatus)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<DeployedNetworkCodePackageQueryResult>(deployedNetworkCodePackageList);
    }
#pragma endregion

#pragma region IPropertyManagementClient methods

    AsyncOperationSPtr FabricClientImpl::BeginCreateName(
        NamingUri const & name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;

        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            if (SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(name))
            {
                error = ErrorCodeValue::InvalidNameUri;
            }
            else
            {
                message = NamingTcpMessage::GetNameCreate(Common::make_unique<NamingUriMessageBody>(name));
                Trace.BeginCreateName(traceContext_, message->ActivityId, name);
            }
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            name,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCreateName(AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteName(
        NamingUri const & name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return this->BeginInternalDeleteName(
            name,
            true,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteName(AsyncOperationSPtr const & operation)
    {
        return this->EndInternalDeleteName(operation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalDeleteName(
        NamingUri const & name,
        bool isExplicit,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            if (SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(name))
            {
                error = ErrorCodeValue::InvalidNameUri;
            }
            else
            {
                message = NamingTcpMessage::GetNameDelete(Common::make_unique<NamingUriMessageBody>(name));
                Trace.BeginDeleteName(traceContext_, message->ActivityId, name);
                message->Headers.Replace(DeleteNameHeader(isExplicit));
            }
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            name,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalDeleteName(AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        NamingUri name;
        FabricActivityHeader activityHeader;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply, name, activityHeader);

        // Even if the request was successful or not, always clear the cache entries
        // This is same behavior as on the gateway.
        this->Cache.ClearCacheEntriesWithName(
            name,
            activityHeader.ActivityId);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginNameExists(
        NamingUri const & name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetNameExists(Common::make_unique<NamingUriMessageBody>(name));
            Trace.BeginNameExists(traceContext_, message->ActivityId, name);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            name,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndNameExists(AsyncOperationSPtr const & operation, __out bool & nameExists)
    {
        ClientServerReplyMessageUPtr reply;
        NamingUri name;
        FabricActivityHeader activityHeader;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply, name, activityHeader);
        if (error.IsSuccess())
        {
            Naming::NameExistsReplyMessageBody body;
            if (reply->GetBody(body))
            {
                nameExists = body.NameExists;

                if (!nameExists)
                {
                    this->Cache.ClearCacheEntriesWithName(name, activityHeader.ActivityId);
                }
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginEnumerateSubNames(
        NamingUri const & name,
        bool doRecursive,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginEnumerateSubNames(name, EnumerateSubNamesToken(), doRecursive, timeout, callback, parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginEnumerateSubNames(
        NamingUri const & name,
        EnumerateSubNamesToken const & continuationToken,
        bool doRecursive,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetEnumerateSubNames(
                Common::make_unique<EnumerateSubNamesRequest>(name, continuationToken, doRecursive));
            Trace.BeginEnumerateSubNames(traceContext_, message->ActivityId, name, doRecursive);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            name,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndEnumerateSubNames(
        AsyncOperationSPtr const & operation,
        __inout EnumerateSubNamesResult & result)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            if (!reply->GetBody(result))
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginEnumerateProperties(
        NamingUri const & name,
        bool includeValues,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginEnumerateProperties(name, includeValues, EnumeratePropertiesToken(), timeout, callback, parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginEnumerateProperties(
        NamingUri const & name,
        bool includeValues,
        EnumeratePropertiesToken const & continuationToken,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetEnumerateProperties(
                Common::make_unique<EnumeratePropertiesRequest>(name, includeValues, continuationToken));

            Trace.BeginEnumerateProperties(traceContext_, message->ActivityId, name, includeValues);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            name,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndEnumerateProperties(
        AsyncOperationSPtr const & operation,
        __inout EnumeratePropertiesResult & result)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            if (!reply->GetBody(result))
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginSubmitPropertyBatch(
        NamePropertyOperationBatch && batch,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            FabricActivityHeader activityHeader;
            Trace.BeginSubmitPropertyBatch(traceContext_, activityHeader.ActivityId, batch);

            if (batch.IsValid())
            {
                message = NamingTcpMessage::GetPropertyBatch(Common::make_unique<NamePropertyOperationBatch>(std::move(batch)));
                message->Headers.Replace(activityHeader);
            }
            else
            {
                error = ErrorCodeValue::InvalidArgument;
            }
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            batch.NamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndSubmitPropertyBatch(
        AsyncOperationSPtr const & operation,
        __inout NamePropertyOperationBatchResult & result)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            if (!reply->GetBody(result))
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    ErrorCode FabricClientImpl::EndSubmitPropertyBatch(
        AsyncOperationSPtr const & operation,
        __inout IPropertyBatchResultPtr & resultPtr)
    {
        NamePropertyOperationBatchResult result;
        ErrorCode error = EndSubmitPropertyBatch(operation, result);
        if (error.IsSuccess())
        {
            PropertyBatchResultSPtr resultSPtr =
                make_shared<PropertyBatchResult>(move(result));
            resultPtr = RootedObjectPointer<IPropertyBatchResult>(
                resultSPtr.get(),
                resultSPtr->CreateComponentRoot());
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginPutProperty(
        NamingUri const & name,
        wstring const & propertyName,
        FABRIC_PROPERTY_TYPE_ID storageTypeId,
        ByteBuffer &&data,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        NamePropertyOperationBatch batch = NamePropertyOperationBatch(name);

        batch.AddPutPropertyOperation(
            propertyName,
            move(data),
            storageTypeId);

        return BeginSubmitPropertyBatch(
            move(batch),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndPutProperty(AsyncOperationSPtr const & operation)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }

        NamePropertyOperationBatchResult batchResult;
        auto error = EndSubmitPropertyBatch(operation, batchResult);
        if (error.IsSuccess())
        {
            error = batchResult.Error;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginPutCustomProperty(
        NamingUri const & name,
        wstring const & propertyName,
        FABRIC_PROPERTY_TYPE_ID storageTypeId,
        ByteBuffer &&data,
        wstring const& customTypeId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        NamePropertyOperationBatch batch = NamePropertyOperationBatch(name);

        batch.AddPutCustomPropertyOperation(
            propertyName,
            move(data),
            storageTypeId,
            move(wstring(customTypeId)));

        return BeginSubmitPropertyBatch(
            move(batch),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndPutCustomProperty(AsyncOperationSPtr const & operation)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }

        NamePropertyOperationBatchResult batchResult;
        auto error = EndSubmitPropertyBatch(operation, batchResult);
        if (error.IsSuccess())
        {
            error = batchResult.Error;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteProperty(
        NamingUri const & name,
        wstring const & propertyName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        NamePropertyOperationBatch batch = NamePropertyOperationBatch(name);

        batch.AddDeletePropertyOperation(propertyName);

        return BeginSubmitPropertyBatch(
            move(batch),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteProperty(AsyncOperationSPtr const & operation)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }

        NamePropertyOperationBatchResult batchResult;
        auto error = EndSubmitPropertyBatch(operation, batchResult);
        if (error.IsSuccess())
        {
            error = batchResult.Error;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetProperty(
        NamingUri const & name,
        wstring const   & propertyName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        NamePropertyOperationBatch batch = NamePropertyOperationBatch(name);

        batch.AddGetPropertyOperation(propertyName);

        return BeginSubmitPropertyBatch(
            move(batch),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetProperty(
        AsyncOperationSPtr const & operation,
        __inout NamePropertyResult & result)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }

        NamePropertyOperationBatchResult batchResult;
        auto error = EndSubmitPropertyBatch(operation, batchResult);
        if (error.IsSuccess())
        {
            if (batchResult.Error.IsSuccess())
            {
                if (batchResult.Properties.empty())
                {
                    TESTASSERT_IF(batchResult.PropertiesNotFound.empty(), "EndGetProperty: returned success, no property and no PropertyNotFound.");
                    error = ErrorCodeValue::PropertyNotFound;
                }
                else
                {
                    result = batchResult.Properties.front();
                }
            }
            else
            {
                return batchResult.Error;
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetPropertyMetadata(
        NamingUri const & name,
        wstring const & propertyName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        NamePropertyOperationBatch batch = NamePropertyOperationBatch(name);

        batch.AddGetPropertyOperation(propertyName, false);

        return BeginSubmitPropertyBatch(
            move(batch),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetPropertyMetadata(
        AsyncOperationSPtr const & operation,
        __inout NamePropertyMetadataResult & result)
    {
        if (dynamic_cast<CompletedAsyncOperation*>(operation.get()) != NULL)
        {
            return CompletedAsyncOperation::End(operation);
        }

        NamePropertyOperationBatchResult batchResult;
        auto error = EndSubmitPropertyBatch(operation, batchResult);
        if (error.IsSuccess())
        {
            if (batchResult.Error.IsSuccess())
            {
                if (batchResult.Metadata.empty())
                {
                    TESTASSERT_IF(batchResult.PropertiesNotFound.empty(), "EndGetPropertyMetadata: returned success, no property metadata and no PropertyNotFound.");
                    error = ErrorCodeValue::PropertyNotFound;
                }
                else
                {
                    result = batchResult.Metadata.front();
                }
            }
            else
            {
                return batchResult.Error;
            }
        }

        return error;
    }

#pragma endregion

#pragma region IServiceGroupManagementClient methods

    //
    // IServiceGroupManagementClient methods
    //

    AsyncOperationSPtr FabricClientImpl::BeginCreateServiceGroup(
        PartitionedServiceDescWrapper const &psdWrapper,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateService(
            psdWrapper,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndCreateServiceGroup(AsyncOperationSPtr const &operation)
    {
        auto error = EndCreateService(operation);
        if (!error.IsSuccess())
        {
            if (error.IsError(ErrorCodeValue::UserServiceAlreadyExists))
            {
                error = ErrorCodeValue::UserServiceGroupAlreadyExists;
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateServiceGroupFromTemplate(
        ServiceGroupFromTemplateDescription const & serviceGroupFromTemplateDesc,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginCreateServiceFromTemplate(
            serviceGroupFromTemplateDesc,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndCreateServiceGroupFromTemplate(
        AsyncOperationSPtr const & operation)
    {
        return EndCreateServiceFromTemplate(
            operation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteServiceGroup(
        NamingUri const& name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginDeleteService(
            DeleteServiceDescription(name),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndDeleteServiceGroup(AsyncOperationSPtr const &operation)
    {
        auto error = EndDeleteService(operation);
        if (error.IsError(ErrorCodeValue::ServiceNotFound))
        {
            error = ErrorCodeValue::UserServiceGroupNotFound;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpdateServiceGroup(
        NamingUri const& name,
        ServiceUpdateDescription const & updateDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return BeginUpdateService(
            name,
            updateDescription,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndUpdateServiceGroup(AsyncOperationSPtr const &operation)
    {
        return EndUpdateService(operation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetServiceGroupDescription(
        NamingUri const& name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        return AsyncOperation::CreateAndStart<GetServiceDescriptionAsyncOperation>(
            *this,
            name,
            false, // fetch cached
            FabricActivityHeader(Guid::NewGuid()),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetServiceGroupDescription(
        AsyncOperationSPtr const & operation,
        __inout ServiceGroupDescriptor & serviceGroupResult)
    {
        return GetServiceDescriptionAsyncOperation::EndGetServiceGroupDescription(operation, serviceGroupResult);
    }

#pragma endregion

#pragma region IInfrastructureServiceClient methods

    //
    // IInfrastructureServiceClient methods.
    //

    AsyncOperationSPtr FabricClientImpl::BeginInvokeInfrastructureCommand(
        bool isAdminOperation,
        NamingUri const & serviceName,
        wstring const & command,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            RunCommandMessageBody body(command);

            if (isAdminOperation)
            {
                message = InfrastructureServiceTcpMessage::GetInvokeInfrastructureCommand(Common::make_unique<RunCommandMessageBody>(command));
            }
            else
            {
                message = InfrastructureServiceTcpMessage::GetInvokeInfrastructureQuery(Common::make_unique<RunCommandMessageBody>(command));
            }

            message->Headers.Replace(ServiceTargetHeader(serviceName));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInvokeInfrastructureCommand(
        AsyncOperationSPtr const & operation,
        __out wstring & result)
    {
        ClientServerReplyMessageUPtr reply;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            RunCommandReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                result = move(replyBody.Text);
            }
        }

        return error;
    }

#pragma endregion

#pragma region IFaultManagementClient methods
    // IFaultManagementClient

    AsyncOperationSPtr FabricClientImpl::BeginRestartNode(
        wstring const & nodeName,
        wstring const & instanceId,
        bool shouldCreateFabricDump,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Node::InstanceId,
            instanceId);

        argMap.Insert(
            QueryResourceProperties::Node::Restart,
            wformatString(true));

        argMap.Insert(
            QueryResourceProperties::Node::CreateFabricDump,
            wformatString(shouldCreateFabricDump));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::StopNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndRestartNode(
        AsyncOperationSPtr const & operation,
        wstring const & nodeName,
        wstring const & instanceId,
        __inout Api::IRestartNodeResultPtr &result)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        FABRIC_NODE_INSTANCE_ID id;

        if (!StringUtility::TryFromWString<FABRIC_NODE_INSTANCE_ID>(instanceId, id))
        {
            WriteWarning(
                Constants::FabricClientSource,
                TraceContext,
                "FabricClientImpl::EndReStartNode/failed parsing node instance id");
        }

        NodeResult nr(nodeName, id);
        auto nrSPtr = make_shared<NodeResult>(nr);
        RestartNodeStatus nodeStatus(move(nrSPtr));
        RestartNodeResultSPtr resultSPtr = make_shared<RestartNodeResult>(move(nodeStatus));

        result = RootedObjectPointer<IRestartNodeResult>(
            resultSPtr.get(),
            resultSPtr->CreateComponentRoot());

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginStartNode(
        StartNodeDescriptionUsingNodeName const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;

        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto body = Common::make_unique<Naming::StartNodeRequestMessageBody>(
                description.NodeName,
                description.NodeInstanceId,
                description.IPAddressOrFQDN,
                description.ClusterConnectionPort);

            message = NamingTcpMessage::GetStartNode(std::move(body));

            Trace.BeginStartNode(
                traceContext_,
                message->ActivityId,
                description.NodeName,
                description.NodeInstanceId,
                description.IPAddressOrFQDN,
                static_cast<uint64>(description.ClusterConnectionPort));
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndStartNode(
        AsyncOperationSPtr const &operation,
        StartNodeDescriptionUsingNodeName const & description,
        __inout Api::IStartNodeResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);

        NodeResult nr(description.NodeName, description.NodeInstanceId);

        auto nrSPtr = make_shared<NodeResult>(nr);

        StartNodeStatus nodeStatus(move(nrSPtr));

        StartNodeResultSPtr resultSPtr = make_shared<StartNodeResult>(move(nodeStatus));

        result = RootedObjectPointer<IStartNodeResult>(
            resultSPtr.get(),
            resultSPtr->CreateComponentRoot());

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginStopNodeInternal(
        wstring const & nodeName,
        wstring const & instanceId,
        DWORD durationInSeconds,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Node::InstanceId,
            instanceId);

        argMap.Insert(
            QueryResourceProperties::Node::Restart,
            wformatString(false));

        argMap.Insert(
            QueryResourceProperties::Node::StopDurationInSeconds,
            wformatString(durationInSeconds));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::StopNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndStopNodeInternal(
        AsyncOperationSPtr const &operation)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginStopNode(
        StopNodeDescriptionUsingNodeName const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            description.NodeName);

        argMap.Insert(
            QueryResourceProperties::Node::InstanceId,
            StringUtility::ToWString(description.NodeInstanceId));

        argMap.Insert(
            QueryResourceProperties::Node::Restart,
            wformatString(false));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::StopNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndStopNode(
        AsyncOperationSPtr const &operation,
        StopNodeDescriptionUsingNodeName const & description,
        __inout Api::IStopNodeResultPtr &result)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        NodeResult nr(description.NodeName, description.NodeInstanceId);

        auto nrSPtr = make_shared<NodeResult>(nr);

        StopNodeStatus nodeStatus(move(nrSPtr));

        StopNodeResultSPtr resultSPtr = make_shared<StopNodeResult>(move(nodeStatus));

        result = RootedObjectPointer<IStopNodeResult>(
            resultSPtr.get(),
            resultSPtr->CreateComponentRoot());

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginRestartDeployedCodePackage(
        RestartDeployedCodePackageDescriptionUsingNodeName const & description,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            description.NodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            description.ApplicationName.ToString());

        argMap.Insert(
            QueryResourceProperties::ServiceManifest::ServiceManifestName,
            description.ServiceManifestName);

        argMap.Insert(
            QueryResourceProperties::ServicePackage::ServicePackageActivationId,
            description.ServicePackageActivationId);

        argMap.Insert(
            QueryResourceProperties::CodePackage::CodePackageName,
            description.CodePackageName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::InstanceId,
            StringUtility::ToWString(description.CodePackageInstanceId));

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::RestartDeployedCodePackage),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndRestartDeployedCodePackage(
        AsyncOperationSPtr const &operation,
        RestartDeployedCodePackageDescriptionUsingNodeName const & description,
        __inout Api::IRestartDeployedCodePackageResultPtr &result)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        DeployedCodePackageResult nr(
            description.NodeName,
            description.ApplicationName,
            description.ServiceManifestName,
            description.ServicePackageActivationId,
            description.CodePackageName,
            description.CodePackageInstanceId);

        auto nrSPtr = make_shared<DeployedCodePackageResult>(nr);

        RestartDeployedCodePackageStatus status(move(nrSPtr));

        RestartDeployedCodePackageResultSPtr resultSPtr = make_shared<RestartDeployedCodePackageResult>(move(status));

        result = RootedObjectPointer<IRestartDeployedCodePackageResult>(
            resultSPtr.get(),
            resultSPtr->CreateComponentRoot());

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInvokeContainerApi(
        wstring const & nodeName,
        NamingUri const & applicationName,
        wstring const & serviceManifestName,
        wstring const & codePackageName,
        FABRIC_INSTANCE_ID codePackageInstance,
        ContainerInfoArgMap & containerInfoArgMap,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        argMap.Insert(
            QueryResourceProperties::ServiceManifest::ServiceManifestName,
            serviceManifestName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::CodePackageName,
            codePackageName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::InstanceId,
            StringUtility::ToWString(codePackageInstance));

        wstring containerInfoArgMapString;
        auto error = JsonHelper::Serialize(containerInfoArgMap, containerInfoArgMapString);
        TESTASSERT_IF(!error.IsSuccess(), "Serializing containerInfoArgMap failed - {0}", error);
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
        }

        argMap.Insert(
            QueryResourceProperties::ContainerInfo::InfoArgsFilter,
            containerInfoArgMapString);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::InvokeContainerApiOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndInvokeContainerApi(
        AsyncOperationSPtr const &operation,
        __inout wstring & result)
    {
        if (auto completedAsyncOperation = dynamic_cast<CompletedAsyncOperation*>(operation.get()))
        {
            return completedAsyncOperation->End(operation);
        }

        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        if (!error.IsSuccess())
        {
            return error;
        }
        return queryResult.MoveItem(result);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetContainerInfo(
        wstring const & nodeName,
        NamingUri const & applicationName,
        wstring const & serviceManifestName,
        wstring const & codePackageName,
        wstring const & containerInfoType,
        ContainerInfoArgMap & containerInfoArgMap,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::Name,
            nodeName);

        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationName.ToString());

        argMap.Insert(
            QueryResourceProperties::ServiceManifest::ServiceManifestName,
            serviceManifestName);

        argMap.Insert(
            QueryResourceProperties::CodePackage::CodePackageName,
            codePackageName);

        argMap.Insert(
            QueryResourceProperties::ContainerInfo::InfoTypeFilter,
            containerInfoType);

        wstring containerInfoArgMapString;
        auto error = JsonHelper::Serialize(containerInfoArgMap, containerInfoArgMapString);
        ASSERT_IF(!error.IsSuccess(), "Serializing containerInfoArgMap failed - {0}", error);

        argMap.Insert(
            QueryResourceProperties::ContainerInfo::InfoArgsFilter,
            containerInfoArgMapString);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetContainerInfoDeployedOnNode),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetContainerInfo(
        AsyncOperationSPtr const &operation,
        __inout wstring &containerInfo)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        if (!error.IsSuccess())
        {
            return error;
        }
        return queryResult.MoveItem(containerInfo);
    }

    AsyncOperationSPtr FabricClientImpl::BeginMovePrimary(
        MovePrimaryDescriptionUsingNodeName const & description,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;

        if (!description.NewPrimaryNodeName.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Node::Name,
                description.NewPrimaryNodeName);
        }

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            Guid(description.PartitionId).ToString());

        argMap.Insert(
            QueryResourceProperties::QueryMetadata::ForceMove,
            description.IgnoreConstraints ? L"True" : L"False"
        );

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::MovePrimary),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndMovePrimary(
        AsyncOperationSPtr const &operation,
        MovePrimaryDescriptionUsingNodeName const & description,
        __inout Api::IMovePrimaryResultPtr &result)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        PrimaryMoveResult pmr(description.NewPrimaryNodeName, description.ServiceName, description.PartitionId);

        auto pmrSPtr = make_shared<PrimaryMoveResult>(pmr);

        MovePrimaryStatus status(move(pmrSPtr));

        MovePrimaryResultSPtr resultSPtr = make_shared<MovePrimaryResult>(move(status));

        result = RootedObjectPointer<IMovePrimaryResult>(
            resultSPtr.get(),
            resultSPtr->CreateComponentRoot());

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginMoveSecondary(
        MoveSecondaryDescriptionUsingNodeName const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;

        argMap.Insert(
            QueryResourceProperties::Node::CurrentNodeName,
            description.CurrentSecondaryNodeName);

        if (!description.NewSecondaryNodeName.empty())
        {
            argMap.Insert(
                QueryResourceProperties::Node::NewNodeName,
                description.NewSecondaryNodeName);
        }

        argMap.Insert(
            QueryResourceProperties::Partition::PartitionId,
            Guid(description.PartitionId).ToString());

        argMap.Insert(
            QueryResourceProperties::QueryMetadata::ForceMove,
            description.IgnoreConstraints ? L"True" : L"False");

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::MoveSecondary),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndMoveSecondary(
        AsyncOperationSPtr const &operation,
        MoveSecondaryDescriptionUsingNodeName const & description,
        __inout Api::IMoveSecondaryResultPtr &result)
    {
        QueryResult queryResult;
        auto error = this->EndInternalQuery(operation, queryResult);

        SecondaryMoveResult pmr(description.CurrentSecondaryNodeName, description.NewSecondaryNodeName, description.ServiceName, description.PartitionId);

        auto pmrSPtr = make_shared<SecondaryMoveResult>(pmr);

        MoveSecondaryStatus status(move(pmrSPtr));

        MoveSecondaryResultSPtr resultSPtr = make_shared<MoveSecondaryResult>(move(status));

        result = RootedObjectPointer<IMoveSecondaryResult>(
            resultSPtr.get(),
            resultSPtr->CreateComponentRoot());

        return error;
    }

#pragma endregion

#pragma region IUpgradeOrchestrationClient methods

    //
    // IUpgradeOrchestrationClient methods
    //

    //Start Upgrade
    Common::AsyncOperationSPtr FabricClientImpl::BeginUpgradeConfiguration(
        StartUpgradeDescription const & startUpgradeDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            StartUpgradeMessageBody body(startUpgradeDescription);
            auto bdy = Common::make_unique<StartUpgradeMessageBody>(body);
            message = UpgradeOrchestrationServiceTcpMessage::StartClusterConfigurationUpgrade(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndUpgradeConfiguration(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    //GetClusterConfigurationUpgradeStatus
    Common::AsyncOperationSPtr FabricClientImpl::BeginGetClusterConfigurationUpgradeStatus(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = UpgradeOrchestrationServiceTcpMessage::GetClusterConfigurationUpgradeStatus();
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndGetClusterConfigurationUpgradeStatus(
        Common::AsyncOperationSPtr const & operation,
        __out IFabricOrchestrationUpgradeStatusResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            GetUpgradeStatusReplyMessageBody body;
            if (reply->GetBody<GetUpgradeStatusReplyMessageBody>(body))
            {
                OrchestrationUpgradeProgress progress = OrchestrationUpgradeProgress(move(body.Progress));

                auto progressSPtr = make_shared<OrchestrationUpgradeProgress>(move(progress));

                OrchestrationUpgradeStatusResultSPtr resultSPtr =
                    make_shared<OrchestrationUpgradeStatusResult>(move(progressSPtr));

                result = RootedObjectPointer<IOrchestrationUpgradeStatusResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    //GetUpgradesPendingApproval
    Common::AsyncOperationSPtr FabricClientImpl::BeginGetUpgradesPendingApproval(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = UpgradeOrchestrationServiceTcpMessage::GetGetUpgradesPendingApproval();
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndGetUpgradesPendingApproval(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    //StartApprovedUpgrades
    Common::AsyncOperationSPtr FabricClientImpl::BeginStartApprovedUpgrades(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = UpgradeOrchestrationServiceTcpMessage::GetStartApprovedUpgrades();
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndStartApprovedUpgrades(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

#pragma endregion

#pragma region ITestManagementClient methods

    //
    // ITestManagementClient methods.
    //

    AsyncOperationSPtr FabricClientImpl::BeginInvokeDataLoss(
        InvokeDataLossDescription const & invokeDataLossDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            InvokeDataLossMessageBody body(invokeDataLossDescription);
            auto bdy = Common::make_unique<InvokeDataLossMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetStartPartitionDataLoss(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInvokeDataLoss(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// Invoke Quorum Loss
    AsyncOperationSPtr FabricClientImpl::BeginInvokeQuorumLoss(
        InvokeQuorumLossDescription const & invokeQuorumLossDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            InvokeQuorumLossMessageBody body(invokeQuorumLossDescription);
            auto bdy = Common::make_unique<InvokeQuorumLossMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetStartPartitionQuorumLoss(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInvokeQuorumLoss(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// Restart Partition
    AsyncOperationSPtr FabricClientImpl::BeginRestartPartition(
        RestartPartitionDescription const & restartPartitionDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            RestartPartitionMessageBody body(restartPartitionDescription);
            auto bdy = Common::make_unique<RestartPartitionMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetStartPartitionRestart(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRestartPartition(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCancelTestCommand(
        CancelTestCommandDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            CancelTestCommandMessageBody body(description);
            auto bdy = Common::make_unique<CancelTestCommandMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetCancelTestCommand(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCancelTestCommand(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// StartChaos
    AsyncOperationSPtr FabricClientImpl::BeginStartChaos(
        StartChaosDescription && startChaosDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            if (startChaosDescription.ChaosParametersUPtr != nullptr)
            {
                error = startChaosDescription.ChaosParametersUPtr->Validate();
            }

            if (error.IsSuccess())
            {
                message = FaultAnalysisServiceTcpMessage::GetStartChaos(make_unique<StartChaosMessageBody>(move(startChaosDescription)));
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndStartChaos(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// StopChaos
    AsyncOperationSPtr FabricClientImpl::BeginStopChaos(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            StopChaosMessageBody body;
            auto b = Common::make_unique<StopChaosMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetStopChaos(move(b));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndStopChaos(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// Get Chaos
    Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaos(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent
                );
        }

        ClientServerRequestMessageUPtr message = SystemServiceTcpMessageBase::GetSystemServiceMessage<FaultAnalysisServiceTcpMessage>(
            FaultAnalysisServiceTcpMessage::GetChaosAction,
            Common::make_unique<SystemServiceMessageBody>());

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndGetChaos(
        Common::AsyncOperationSPtr const & operation,
        __out IChaosDescriptionResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                ChaosDescription chaosDescription;

                error = JsonHelper::Deserialize(chaosDescription, move(body.Content));

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        Constants::FabricClientSource,
                        traceContext_,
                        "GetChaos returned error {0} during message deserialization", error);

                    return error;
                }

                auto descriptionSPtr = make_shared<ChaosDescription>(move(chaosDescription));
                auto resultSPtr = make_shared<ChaosDescriptionResult>(move(descriptionSPtr));
                result = RootedObjectPointer<IChaosDescriptionResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    Common::ErrorCode FabricClientImpl::EndGetChaos(
        Common::AsyncOperationSPtr const & operation,
        __out ISystemServiceApiResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                CallSystemServiceResultSPtr resultSPtr =
                    make_shared<CallSystemServiceResult>(move(body.Content));

                result = RootedObjectPointer<ISystemServiceApiResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// Get ChaosReport
    Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaosReport(
        GetChaosReportDescription const & getChaosReportDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            GetChaosReportMessageBody body(getChaosReportDescription);
            auto b = Common::make_unique<GetChaosReportMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetChaosReport(move(b));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndGetChaosReport(
        Common::AsyncOperationSPtr const &operation,
        __inout Api::IChaosReportResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            GetChaosReportReplyMessageBody body;
            if (reply->GetBody<GetChaosReportReplyMessageBody>(body))
            {
                ChaosReport report(move(body.Report));

                FabricClientImpl::WriteWarning(
                    Constants::FabricClientSource,
                    traceContext_,
                    "Inside FabricClientImpl's EndGetChaosReport, continuationToken {0}",
                    report.ContinuationToken);

                auto reportSPtr = make_shared<ChaosReport>(move(report));

                ChaosReportResultSPtr resultSPtr =
                    make_shared<ChaosReportResult>(move(reportSPtr));

                result = RootedObjectPointer<IChaosReportResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    /// Get ChaosEvents
    Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaosEvents(
        GetChaosEventsDescription const & getChaosEventsDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        wstring getChaosEventsJson;

        auto filterSPtr = make_shared<ChaosEventsFilter>(getChaosEventsDescription.FilterSPtr->StartTimeUtc, getChaosEventsDescription.FilterSPtr->EndTimeUtc);
        auto pagingDescriptionSPtr = make_shared<QueryPagingDescription>();
        wstring continuationToken(getChaosEventsDescription.PagingDescription->ContinuationToken);
        pagingDescriptionSPtr->ContinuationToken = move(continuationToken);
        pagingDescriptionSPtr->MaxResults = getChaosEventsDescription.PagingDescription->MaxResults;
        wstring clientType(getChaosEventsDescription.ClientType);

        GetChaosEventsDescription chaosEventsDescription(filterSPtr, pagingDescriptionSPtr, clientType);
        error = JsonHelper::Serialize(chaosEventsDescription, getChaosEventsJson, JsonSerializerFlags::DateTimeInIsoFormat);

        WriteInfo(
            Constants::FabricClientSource,
            TraceContext,
            "FabricClientImpl::BeginGetChaosEvents serialized request as {0}",
            getChaosEventsJson);

        ClientServerRequestMessageUPtr message = SystemServiceTcpMessageBase::GetSystemServiceMessage<FaultAnalysisServiceTcpMessage>(
            FaultAnalysisServiceTcpMessage::GetChaosEventsAction,
            Common::make_unique<SystemServiceMessageBody>(getChaosEventsJson));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndGetChaosEvents(
        Common::AsyncOperationSPtr const &operation,
        __inout Api::IChaosEventsSegmentResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                ChaosEventsSegment events;
                error = JsonHelper::Deserialize(events, move(body.Content));

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        Constants::FabricClientSource,
                        traceContext_,
                        "GetChaosEvents returning error {0} during message deserialization", error);

                    return error;
                }

                auto eventsSPtr = make_shared<ChaosEventsSegment>(move(events));
                auto resultSPtr = make_shared<ChaosEventsSegmentResult>(move(eventsSPtr));
                result = RootedObjectPointer<IChaosEventsSegmentResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    Common::ErrorCode FabricClientImpl::EndGetChaosEvents(
        Common::AsyncOperationSPtr const &operation,
        __inout Api::ISystemServiceApiResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                CallSystemServiceResultSPtr resultSPtr =
                    make_shared<CallSystemServiceResult>(move(body.Content));

                result = RootedObjectPointer<ISystemServiceApiResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    // Get Chaos Schedule
    Common::AsyncOperationSPtr FabricClientImpl::BeginGetChaosSchedule(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent
                );
        }

        WriteInfo(
            Constants::FabricClientSource,
            TraceContext,
            "Enter FabricClientImpl::BeginGetChaosSchedule, timeout={0}",
            timeout);

        ClientServerRequestMessageUPtr message = SystemServiceTcpMessageBase::GetSystemServiceMessage<FaultAnalysisServiceTcpMessage>(
            FaultAnalysisServiceTcpMessage::GetChaosScheduleAction,
            Common::make_unique<SystemServiceMessageBody>());

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndGetChaosSchedule(
        Common::AsyncOperationSPtr const &operation,
        __inout Api::IChaosScheduleDescriptionResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                ChaosScheduleDescription chaosScheduleDescription;
                error = JsonHelper::Deserialize(chaosScheduleDescription, move(body.Content));

                if (!error.IsSuccess())
                {
                    WriteWarning(
                        Constants::FabricClientSource,
                        traceContext_,
                        "GetChaosSchedule returning error {0} during message deserialization", error);

                    return error;
                }

                auto descriptionSPtr = make_shared<ChaosScheduleDescription>(move(chaosScheduleDescription));
                auto resultSPtr = make_shared<ChaosScheduleDescriptionResult>(move(descriptionSPtr));
                result = RootedObjectPointer<IChaosScheduleDescriptionResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    Common::ErrorCode FabricClientImpl::EndGetChaosSchedule(
        Common::AsyncOperationSPtr const &operation,
        __inout Api::ISystemServiceApiResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            SystemServiceReplyMessageBody body;
            if (reply->GetBody<SystemServiceReplyMessageBody>(body))
            {
                CallSystemServiceResultSPtr resultSPtr =
                    make_shared<CallSystemServiceResult>(move(body.Content));

                result = RootedObjectPointer<ISystemServiceApiResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }
        else if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    Common::AsyncOperationSPtr FabricClientImpl::BeginSetChaosSchedule(
        SetChaosScheduleDescription const & setChaosScheduleDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        WriteInfo(
            Constants::FabricClientSource,
            TraceContext,
            "Enter FabricClientImpl::BeginSetChaosSchedule timeout={0}",
            timeout);

        wstring scheduleString;
        error = JsonHelper::Serialize(*(setChaosScheduleDescription.ChaosScheduleDescriptionUPtr), scheduleString, JsonSerializerFlags::DateTimeInIsoFormat);

        WriteInfo(
            Constants::FabricClientSource,
            TraceContext,
            "FabricClientImpl::BeginSetChaosSchedule serialized schedule as {0}",
            scheduleString);

        ClientServerRequestMessageUPtr message = SystemServiceTcpMessageBase::GetSystemServiceMessage<FaultAnalysisServiceTcpMessage>(
            FaultAnalysisServiceTcpMessage::PostChaosScheduleAction,
            Common::make_unique<SystemServiceMessageBody>(scheduleString));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::AsyncOperationSPtr FabricClientImpl::BeginSetChaosSchedule(
        std::wstring const & schedule,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        WriteInfo(
            Constants::FabricClientSource,
            TraceContext,
            "Enter FabricClientImpl::BeginSetChaosSchedule, schedule={0}, timeout={1}",
            schedule,
            timeout);

        ClientServerRequestMessageUPtr message = SystemServiceTcpMessageBase::GetSystemServiceMessage<FaultAnalysisServiceTcpMessage>(
            FaultAnalysisServiceTcpMessage::PostChaosScheduleAction,
            Common::make_unique<SystemServiceMessageBody>(schedule));

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    Common::ErrorCode FabricClientImpl::EndSetChaosSchedule(
        Common::AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);

        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginStartNodeTransition(
        StartNodeTransitionDescription const & description,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();

        if (error.IsSuccess())
        {
            WriteInfo(
                Constants::FabricClientSource,
                TraceContext,
                "FabricClientImpl::BeginStartNodeTransition, opid={0}, type={1} {2}:{3}",
                description.OperationId,
                (int)description.NodeTransitionType,
                description.NodeName,
                description.NodeInstanceId);

            StartNodeTransitionMessageBody body(description);
            auto bdy = Common::make_unique<StartNodeTransitionMessageBody>(body);
            message = FaultAnalysisServiceTcpMessage::GetStartNodeTransition(move(bdy));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndStartNodeTransition(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (IsFASOrUOSReconfiguring(error))
        {
            error = ErrorCodeValue::ReconfigurationPending;
        }

        return error;
    }

#pragma endregion

#pragma region IInternalInfrastructureServiceClient methods

    //
    // IInternalInfrastructureServiceClient methods.
    //
    AsyncOperationSPtr FabricClientImpl::BeginRunCommand(
        wstring const &command,
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = InfrastructureServiceTcpMessage::GetRunCommand(Common::make_unique<RunCommandMessageBody>(command));

            if (targetPartitionId != Guid::Empty())
            {
                message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndRunCommand(
        AsyncOperationSPtr const & operation,
        wstring & result)
    {
        ClientServerReplyMessageUPtr reply;

        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            RunCommandReplyMessageBody replyBody;
            if (reply->GetBody(replyBody))
            {
                result = move(replyBody.Text);
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginReportStartTaskSuccess(
        wstring const &taskId,
        ULONGLONG instanceId,
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = InfrastructureServiceTcpMessage::GetReportStartTaskSuccess(Common::make_unique<ReportTaskMessageBody>(taskId, instanceId));

            if (targetPartitionId != Guid::Empty())
            {
                message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndReportStartTaskSuccess(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginReportFinishTaskSuccess(
        wstring const &taskId,
        ULONGLONG instanceId,
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = InfrastructureServiceTcpMessage::GetReportFinishTaskSuccess(Common::make_unique<ReportTaskMessageBody>(taskId, instanceId));

            if (targetPartitionId != Guid::Empty())
            {
                message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndReportFinishTaskSuccess(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginReportTaskFailure(
        wstring const &taskId,
        ULONGLONG instanceId,
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = InfrastructureServiceTcpMessage::GetReportTaskFailure(Common::make_unique<ReportTaskMessageBody>(taskId, instanceId));

            if (targetPartitionId != Guid::Empty())
            {
                message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            }
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndReportTaskFailure(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }

#pragma endregion

#pragma region IFileStoreServiceClient methods

    //
    // IImageStoreClient methods.
    //

    ErrorCode FabricClientImpl::Upload(
        wstring const & imageStoreConnectionString,
        wstring const & sourceFullpath,
        wstring const & destinationRelativePath,
        bool const shouldOverwrite)
    {
        if (sourceFullpath.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        if (destinationRelativePath.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        RootedObjectPointer<IImageStore> imageStore;
        auto error = this->GetImageStoreClient(imageStoreConnectionString, imageStore);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = imageStore->UploadContent(
            destinationRelativePath,
            sourceFullpath,
            ServiceModelConfig::GetConfig().MaxFileOperationTimeout,
            shouldOverwrite ? CopyFlag::Overwrite : CopyFlag::Echo);

        return error;
    }

    ErrorCode FabricClientImpl::Delete(
        wstring const & imageStoreConnectionString,
        wstring const & relativePath)
    {
        if (relativePath.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        RootedObjectPointer<IImageStore> imageStore;
        auto error = this->GetImageStoreClient(imageStoreConnectionString, imageStore);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = imageStore->RemoveRemoteContent(relativePath);
        if (!error.IsSuccess())
        {
            return error;
        }

        return ErrorCodeValue::Success;
    }

#pragma endregion

#pragma region IFileStoreServiceClient methods

    //
    // IFileStoreServiceClient methods.
    //
    AsyncOperationSPtr FabricClientImpl::BeginUploadFile(
        wstring const & serviceName,
        wstring const & sourceFullPath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        IFileStoreServiceClientProgressEventHandlerPtr && progressHandler,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();

        return fileTransferClient_->BeginUploadFile(
            serviceName,
            sourceFullPath,
            storeRelativePath,
            shouldOverwrite,
            move(progressHandler),
            timeout,
            callback,
            parent,
            error);
    }

    ErrorCode FabricClientImpl::EndUploadFile(AsyncOperationSPtr const &operation)
    {
        return fileTransferClient_->EndUploadFile(operation);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDownloadFile(
        wstring const & serviceName,
        wstring const & storeRelativePath,
        wstring const & destinationFullPath,
        StoreFileVersion const & version,
        vector<wstring> const & availableShares,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();

        return fileTransferClient_->BeginDownloadFile(
            serviceName,
            storeRelativePath,
            destinationFullPath,
            version,
            availableShares,
            timeout,
            callback,
            parent,
            error);
    }

    ErrorCode FabricClientImpl::EndDownloadFile(AsyncOperationSPtr const &operation)
    {
        return fileTransferClient_->EndDownloadFile(operation);
    }

#pragma endregion

#pragma region IInternalFileStoreServiceClient methods

    //
    // IInternalFileStoreServiceClient methods.
    //
    AsyncOperationSPtr FabricClientImpl::BeginGetStagingLocation(
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetStagingLocation();
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginGetStagingLocation(TraceContext, message->ActivityId, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetStagingLocation(
        AsyncOperationSPtr const & operation,
        __out wstring & stagingLocationShare)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            ShareLocationReply replyBody;
            if (reply->GetBody(replyBody))
            {
                stagingLocationShare = replyBody.ShareLocation;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        Trace.EndGetStagingLocation(TraceContext, activityId, error, stagingLocationShare);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginUpload(
        Guid const & targetPartitionId,
        wstring const & stagingRelativePath,
        wstring const & storeRelativePath,
        bool shouldOverwrite,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetUpload(Common::make_unique<UploadRequest>(stagingRelativePath, storeRelativePath, shouldOverwrite));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));

            auto activityId = message->ActivityId;
            Trace.BeginUpload(TraceContext, activityId, stagingRelativePath, storeRelativePath, shouldOverwrite, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUpload(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);

        Trace.EndUpload(TraceContext, activityId, error);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCopy(
        Guid const & targetPartitionId,
        wstring const & sourceStoreRelativePath,
        Management::FileStoreService::StoreFileVersion sourceVersion,
        wstring const & destinationStoreRelativePath,
        bool shouldOverwrite,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetCopy(Common::make_unique<CopyRequest>(sourceStoreRelativePath, sourceVersion, destinationStoreRelativePath, shouldOverwrite));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));

            auto activityId = message->ActivityId;
            Trace.BeginCopy(
                TraceContext,
                activityId,
                sourceStoreRelativePath,
                sourceVersion.ToString(),
                destinationStoreRelativePath,
                shouldOverwrite,
                targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCopy(
        AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);

        Trace.EndCopy(TraceContext, activityId, error);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCheckExistence(
        Guid const & targetPartitionId,
        wstring const & storeRelativePath,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetCheckExistence(Common::make_unique<ImageStoreBaseRequest>(storeRelativePath));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginCheckExistence(TraceContext, message->ActivityId, storeRelativePath);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCheckExistence(
        AsyncOperationSPtr const & operation,
        __out bool & result)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            ImageStoreContentExistsReplyMessageBody messageBody;
            if (reply->GetBody(messageBody))
            {
                result = messageBody.Exists;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        if (error.IsSuccess())
        {
            Trace.EndCheckExistence(TraceContext, activityId, error);
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginListFiles(
        Guid const & targetPartitionId,
        wstring const & storeRelativePath,
        wstring const & continuationToken,
        BOOLEAN const & shouldIncludeDetails,
        BOOLEAN const & isRecursive,
        bool isPaging,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetList(Common::make_unique<ListRequest>(storeRelativePath, shouldIncludeDetails, isRecursive, continuationToken, isPaging));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginListFiles(TraceContext, message->ActivityId, storeRelativePath, continuationToken, isRecursive ? true : false, isPaging, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndListFiles(
        AsyncOperationSPtr const & operation,
        __out vector<StoreFileInfo> & availableFiles,
        __out vector<StoreFolderInfo> & availableFolders,
        __out vector<wstring> & availableShares,
        __out wstring & continuationToken)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            ListReply replyBody;
            if (reply->GetBody(replyBody))
            {
                availableFiles = replyBody.Files;
                availableFolders = replyBody.Folders;
                continuationToken = replyBody.ContinuationToken;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }

            if (error.IsSuccess())
            {
                SecondaryLocationsHeader locationsHeader;
                if (reply->Headers.TryReadFirst(locationsHeader))
                {
                    availableShares = locationsHeader.SecondaryLocations;
                }

                availableShares.push_back(replyBody.PrimaryStoreShareLocation);
            }
        }

        Trace.EndListFiles(TraceContext, activityId, error, continuationToken, availableFiles.size(), availableFolders.size(), availableShares.size());
        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginDelete(
        Guid const & targetPartitionId,
        wstring const & storeRelativePath,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetDelete(Common::make_unique<ImageStoreBaseRequest>(storeRelativePath));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginDelete(TraceContext, message->ActivityId, storeRelativePath, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDelete(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);

        Trace.EndDelete(TraceContext, activityId, error);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalGetStoreLocation(
        NamingUri const & serviceName,
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetStoreLocation();
            message->Headers.Replace(NamingPropertyHeader(serviceName, targetPartitionId.ToString()));

            ServiceRoutingAgentMessage::WrapForForwardingToFileStoreService(*message);
            Trace.BeginInternalGetStoreLocation(TraceContext, message->ActivityId, serviceName, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalGetStoreLocation(
        AsyncOperationSPtr const & operation,
        __out wstring & storeLocationShare)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = RequestReplyAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            ShareLocationReply replyBody;
            if (reply->GetBody(replyBody))
            {
                storeLocationShare = replyBody.ShareLocation;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        Trace.EndInternalGetStoreLocation(TraceContext, activityId, error, storeLocationShare);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalGetStoreLocations(
        Guid const & targetPartitionId,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetStoreLocations();
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginInternalGetStoreLocations(TraceContext, message->ActivityId, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalGetStoreLocations(
        AsyncOperationSPtr const & operation,
        __out vector<wstring> & secondaryShares)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            // The reply body is of type ShareLocationReply containing
            // the primary location for when share location retrieval
            // is optimized in the File Store Service
            //
            SecondaryLocationsHeader locationsHeader;
            if (reply->Headers.TryReadFirst(locationsHeader))
            {
                secondaryShares = locationsHeader.SecondaryLocations;
            }
        }

        Trace.EndInternalGetStoreLocations(
            TraceContext,
            activityId,
            error,
            secondaryShares.size());

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalListFile(
        NamingUri const & serviceName,
        Guid const & targetPartitionId,
        wstring const & storeRelativePath,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetInternalList(Common::make_unique<ListRequest>(storeRelativePath, false, false, L"", true));
            message->Headers.Replace(NamingPropertyHeader(serviceName, targetPartitionId.ToString()));

            ServiceRoutingAgentMessage::WrapForForwardingToFileStoreService(*message);
            Trace.BeginInternalListFile(TraceContext, message->ActivityId, storeRelativePath, serviceName, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalListFile(
        AsyncOperationSPtr const &operation,
        __out bool & isPresent,
        __out Management::FileStoreService::FileState::Enum & currentState,
        __out Management::FileStoreService::StoreFileVersion & currentVersion,
        __out wstring & storeShareLocation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = RequestReplyAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            InternalListReply replyBody;
            if (reply->GetBody(replyBody))
            {
                isPresent = replyBody.IsPresent;
                currentState = replyBody.State;
                currentVersion = replyBody.CurrentVersion;
                storeShareLocation = replyBody.StoreShareLocation;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        Trace.EndInternalListFile(TraceContext, activityId, error, isPresent, wformatString("{0}", currentState), currentVersion.ToString(), storeShareLocation);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginListUploadSession(
        Common::Guid const & targetPartitionId,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const &callback,
        Common::AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetListUploadSession(Common::make_unique<UploadSessionRequest>(storeRelativePath, sessionId));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginListUploadSession(TraceContext, message->ActivityId, storeRelativePath, sessionId, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndListUploadSession(
        Common::AsyncOperationSPtr const & operation,
        __out UploadSession & uploadSessions)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (error.IsSuccess())
        {
            UploadSession retrievedUploadSession;
            if (reply->GetBody(retrievedUploadSession))
            {
                uploadSessions = std::move(retrievedUploadSession.UploadSessions);
            }
        }
        else
        {
            Trace.EndListUploadSession(TraceContext, activityId, error, uploadSessions.UploadSessions.size());
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCreateUploadSession(
        Common::Guid const & targetPartitionId,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        uint64 fileSize,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetCreateUploadSession(Common::make_unique<CreateUploadSessionRequest>(storeRelativePath, sessionId, fileSize));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginCreateUploadSession(TraceContext, message->ActivityId, storeRelativePath, sessionId, fileSize, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCreateUploadSession(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (!error.IsSuccess())
        {
            Trace.EndCreateUploadSession(TraceContext, activityId, error);
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginUploadChunk(
        Common::Guid const & targetPartitionId,
        std::wstring const & localSource,
        Common::Guid const & sessionId,
        uint64 startPosition,
        uint64 endPosition,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetUploadChunk(Common::make_unique<UploadChunkRequest>(localSource, sessionId, startPosition, endPosition));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginUploadChunk(TraceContext, message->ActivityId, localSource, sessionId, startPosition, endPosition, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUploadChunk(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (!error.IsSuccess())
        {
            Trace.EndUploadChunk(TraceContext, activityId, error);
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginUploadChunkContent(
        Common::Guid const & targetPartitionId,
        Transport::MessageUPtr & chunkContentMessage_,
        Management::FileStoreService::UploadChunkContentDescription & uploadChunkContentDescription,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto sessionId = uploadChunkContentDescription.SessionId;
            auto startPosition = uploadChunkContentDescription.StartPosition;
            auto endPosition = uploadChunkContentDescription.EndPosition;
            auto size = uploadChunkContentDescription.StartPosition == uploadChunkContentDescription.EndPosition ? 0 : (uploadChunkContentDescription.EndPosition - uploadChunkContentDescription.StartPosition + 1);

            auto uploadChunkContentRequest = Common::make_unique<UploadChunkContentRequest>(move(chunkContentMessage_), move(uploadChunkContentDescription));
            error = uploadChunkContentRequest->InitializeBuffer();
            if (!error.IsSuccess())
            {
                return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                    error,
                    callback,
                    parent);
            }
            message = FileStoreServiceTcpMessage::GetUploadChunkContent(move(uploadChunkContentRequest));
            Trace.BeginUploadChunkContent(TraceContext, message->ActivityId, sessionId, startPosition, endPosition, size, targetPartitionId);
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndUploadChunkContent(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);

        Trace.EndUploadChunkContent(TraceContext, activityId, error);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteUploadSession(
        Common::Guid const & targetPartitionId,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetDeleteUploadSession(Common::make_unique<DeleteUploadSessionRequest>(sessionId));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginDeleteUploadSession(TraceContext, message->ActivityId, sessionId, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeleteUploadSession(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (!error.IsSuccess())
        {
            Trace.EndDeleteUploadSession(TraceContext, activityId, error);
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginCommitUploadSession(
        Common::Guid const & targetPartitionId,
        std::wstring const & storeRelativePath,
        Common::Guid const & sessionId,
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = FileStoreServiceTcpMessage::GetCommitUploadSession(Common::make_unique<UploadSessionRequest>(storeRelativePath, sessionId));
            message->Headers.Replace(PartitionTargetHeader(targetPartitionId));
            Trace.BeginCommitUploadSession(TraceContext, message->ActivityId, sessionId, targetPartitionId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCommitUploadSession(
        Common::AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply, activityId);
        if (!error.IsSuccess())
        {
            Trace.EndCommitUploadSession(TraceContext, activityId, error);
        }

        return error;
    }

#pragma endregion

#pragma region IInternalTokenValidationServiceClient methods

    AsyncOperationSPtr FabricClientImpl::BeginGetMetadata(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = TokenValidationServiceTcpMessage::GetMetadata();
            FabricActivityHeader activityHeader = FabricActivityHeader(Guid::NewGuid());
            message->Headers.Replace(activityHeader);
            ServiceRoutingAgentMessage::WrapForForwardingToTokenValidationService(*message);
            Trace.BeginGetMetadata(TraceContext, activityHeader.ActivityId);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndGetMetadata(AsyncOperationSPtr const &operation, __out TokenValidationServiceMetadata &metadata)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = RequestReplyAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            if (!reply->GetBody(metadata))
            {
                error = ErrorCodeValue::SerializationError;
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginValidateToken(
        wstring const &token,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        ErrorCode error = ErrorCodeValue::InvalidArgument;

        if (!token.empty())
        {
            error.ReadValue();
            error = EnsureOpened();
            if (error.IsSuccess())
            {
                ValidateTokenMessageBody body(token);
                message = TokenValidationServiceTcpMessage::GetValidateToken(body);
                FabricActivityHeader activityHeader = FabricActivityHeader(Guid::NewGuid());
                message->Headers.Replace(activityHeader);
                ServiceRoutingAgentMessage::WrapForForwardingToTokenValidationService(*message);

                Trace.BeginValidateToken(TraceContext, activityHeader.ActivityId);
            }
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndValidateToken(
        AsyncOperationSPtr const &operation,
        __out TimeSpan &expirationTime,
        __out vector<ServiceModel::Claim> &claimsList)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = RequestReplyAsyncOperation::End(operation, reply);

        if (error.IsSuccess())
        {
            TokenValidationMessage body;
            if (reply->GetBody(body))
            {
                error = body.Error;
                claimsList = move(body.ClaimsResult.ClaimsList);
                expirationTime = body.TokenExpiryTime;
            }
            else
            {
                error = ErrorCodeValue::InvalidOperation; // TODO: bharatn: Better errorcode
            }
        }

        return error;
    }

#pragma endregion

#pragma region IGatewayResourceManager client methods

    AsyncOperationSPtr FabricClientImpl::BeginCreateOrUpdateGatewayResource(
        wstring && descriptionStr,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = GatewayResourceManagerTcpMessage::GetCreateGatewayResource(Common::make_unique<CreateGatewayResourceMessageBody>(move(descriptionStr)));
            Trace.BeginCreateOrUpdateGatewayResource(traceContext_, message->ActivityId);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndCreateOrUpdateGatewayResource(
        AsyncOperationSPtr const &operation,
        __out std::wstring & descriptionStr)
    {
        ClientServerReplyMessageUPtr reply;
        auto error = ForwardToServiceAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            CreateGatewayResourceMessageBody body;
            if (!reply->GetBody<CreateGatewayResourceMessageBody>(body))
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
            else
            {
                descriptionStr = body.TakeDescription();
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetGatewayResourceList(
        ServiceModel::ModelV2::GatewayResourceQueryDescription && gatewayQueryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        QueryArgumentMap argMap;
        // Inserts values into argMap based on data in queryDescription
        gatewayQueryDescription.GetQueryArgumentMap(argMap);

        return this->BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetGatewayResourceList),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetGatewayResourceList(
        AsyncOperationSPtr const & operation,
        vector<wstring> & list,
        ServiceModel::PagingStatusUPtr & pagingStatus)
    {
        QueryResult queryResult;
        ErrorCode error = this->EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        pagingStatus = queryResult.MovePagingStatus();
        return queryResult.MoveList<wstring>(list);
    }

    AsyncOperationSPtr FabricClientImpl::BeginDeleteGatewayResource(
        wstring const & name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = GatewayResourceManagerTcpMessage::GetDeleteGatewayResource(Common::make_unique<DeleteGatewayResourceMessageBody>(name));
            Trace.BeginDeleteGatewayResource(traceContext_, message->ActivityId, name);
        }

        return AsyncOperation::CreateAndStart<ForwardToServiceAsyncOperation>(
            *this,
            move(message),
            NamingUri::RootNamingUri,
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndDeleteGatewayResource(AsyncOperationSPtr const &operation)
    {
        ClientServerReplyMessageUPtr reply;
        return ForwardToServiceAsyncOperation::End(operation, reply);
    }
#pragma endregion

    //
    // ITestClient methods.
    //
    ErrorCode FabricClientImpl::GetNthNamingPartitionId(
        ULONG n,
        __inout Reliability::ConsistencyUnitId &partitionId)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }

        auto const & namingIdRange = *Reliability::ConsistencyUnitId::NamingIdRange;
        if (n > namingIdRange.Count)
        {
            return ErrorCodeValue::InvalidArgument;
        }

        partitionId = Reliability::ConsistencyUnitId::CreateReserved(namingIdRange, n);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginResolvePartition(
        ConsistencyUnitId const& partitionId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetResolvePartition(Common::make_unique<ConsistencyUnitId>(partitionId));
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndResolvePartition(
        AsyncOperationSPtr const & operation,
        __inout IResolvedServicePartitionResultPtr &result)
    {
        ResolvedServicePartitionSPtr rspSPtr;
        auto error = EndResolvePartition(operation, rspSPtr);
        if (error.IsSuccess())
        {
            ResolvedServicePartitionResultSPtr resultSPtr =
                make_shared<ResolvedServicePartitionResult>(rspSPtr);
            result = RootedObjectPointer<IResolvedServicePartitionResult>(
                resultSPtr.get(),
                resultSPtr->CreateComponentRoot());
        }

        return error;
    }

    ErrorCode FabricClientImpl::EndResolvePartition(
        AsyncOperationSPtr const & operation,
        __inout ResolvedServicePartitionSPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            ServiceTableEntry entry;
            if (reply->GetBody(entry))
            {
                ResolvedServicePartitionSPtr rspSPtr;
                rspSPtr = make_shared<ResolvedServicePartition>(
                    false, // isServiceGroup
                    entry,
                    PartitionInfo(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON),
                    GenerationNumber(),
                    -1,
                    nullptr); // psd

                result = move(rspSPtr);
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginResolveNameOwner(
        NamingUri const& name,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetResolveNameOwner(Common::make_unique<NamingUriMessageBody>(name));
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndResolveNameOwner(
        AsyncOperationSPtr const & operation,
        __inout IResolvedServicePartitionResultPtr &result)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, /*out*/reply);
        if (error.IsSuccess())
        {
            ServiceTableEntry entry;
            if (reply->GetBody(entry))
            {
                ResolvedServicePartitionSPtr rspSPtr;
                rspSPtr = make_shared<ResolvedServicePartition>(
                    false, // isServiceGroup
                    entry,
                    PartitionInfo(FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_SINGLETON),
                    GenerationNumber(),
                    -1,
                    nullptr);
                ResolvedServicePartitionResultSPtr resultSPtr =
                    make_shared<ResolvedServicePartitionResult>(rspSPtr);
                result = RootedObjectPointer<IResolvedServicePartitionResult>(
                    resultSPtr.get(),
                    resultSPtr->CreateComponentRoot());
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    ErrorCode FabricClientImpl::NodeIdFromNameOwnerAddress(
        wstring const& address,
        Federation::NodeId & federationNodeId)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }

        SystemServiceLocation serviceLocation;
        if (!SystemServiceLocation::TryParse(address, serviceLocation))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        federationNodeId = serviceLocation.NodeInstance.Id;
        return error;
    }

    ErrorCode FabricClientImpl::NodeIdFromFMAddress(
        wstring const& address,
        Federation::NodeId & federationNodeId)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }

        Federation::NodeInstance fmPrimaryNodeInstance;
        if (!Federation::NodeInstance::TryParse(address, fmPrimaryNodeInstance))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        federationNodeId = fmPrimaryNodeInstance.Id;
        return error;
    }

    ErrorCode FabricClientImpl::GetCurrentGatewayAddress(
        __inout wstring &result)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }

        auto currentTarget = clientConnectionManager_->CurrentTarget;

        if (!currentTarget)
        {
            return ErrorCodeValue::GatewayUnreachable;
        }

        result = currentTarget->Address();

        return ErrorCodeValue::Success;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalGetServiceDescription(
        NamingUri  const & nameUri,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return this->BeginInternalGetServiceDescription(nameUri, FabricActivityHeader(activityId), timeout, callback, parent);
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalGetServiceDescription(
        NamingUri  const & nameUri,
        FabricActivityHeader activityHeader,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetServiceDescription(Common::make_unique<NamingUriMessageBody>(nameUri));
            message->Headers.Replace(activityHeader);
            Trace.BeginGetServiceDescription(traceContext_, activityHeader.ActivityId, nameUri);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            nameUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalGetServiceDescription(
        AsyncOperationSPtr const & operation,
        __inout LruClientCacheEntrySPtr & cacheEntry)
    {
        PartitionedServiceDescriptor description;
        return this->EndInternalGetServiceDescription(
            operation,
            description,
            cacheEntry);
    }

    ErrorCode FabricClientImpl::EndInternalGetServiceDescription(
        AsyncOperationSPtr const & operation,
        __inout Naming::PartitionedServiceDescriptor & description)
    {
        LruClientCacheEntrySPtr cacheEntry;
        return this->EndInternalGetServiceDescription(
            operation,
            description,
            cacheEntry);
    }

    ErrorCode FabricClientImpl::EndInternalGetServiceDescription(
        AsyncOperationSPtr const & operation,
        __inout Naming::PartitionedServiceDescriptor & description,
        __inout LruClientCacheEntrySPtr & cacheEntry)
    {
        ClientServerReplyMessageUPtr reply;
        NamingUri name;
        FabricActivityHeader activityHeader;
        auto error = RequestReplyAsyncOperation::End(operation, reply, name, activityHeader);
        if (!error.IsSuccess())
        {
            return error;
        }

        if (!reply->GetBody(description))
        {
            error = ErrorCode::FromNtStatus(reply->GetStatus());
        }
        else
        {
            this->Cache.TryUpdateOrGetPsdCacheEntry(
                name,
                description,
                activityHeader.ActivityId,
                cacheEntry);
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalForwardToService(
        ClientServerRequestMessageUPtr && message,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            ServiceRoutingAgentMessage::WrapForForwarding(*message);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalForwardToService(AsyncOperationSPtr const & operation, __inout ClientServerReplyMessageUPtr & reply)
    {
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalDeleteService(
        ServiceModel::DeleteServiceDescription const & description,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetDeleteService(Common::make_unique<Naming::DeleteServiceMessageBody>(description));
            message->Headers.Replace(FabricActivityHeader(activityId));
            Trace.BeginDeleteService(traceContext_, activityId, description);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            description.ServiceName,
            move(message),
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndInternalDeleteService(AsyncOperationSPtr const & operation, __inout bool & nameDeleted)
    {
        ClientServerReplyMessageUPtr reply;
        NamingUri name;
        FabricActivityHeader activityHeader;
        auto error = RequestReplyAsyncOperation::End(operation, reply, name, activityHeader);

        if (error.IsSuccess())
        {
            // If the delete service request was processed by a V1
            // Naming service replica, then no recursive name deletion
            // will have occurred. If the request was routed through
            // a V1 Naming Gateway, then we don't know the version of the
            // service replica, so be conservative.
            //
            nameDeleted = false;

            if (reply && reply->Action == NamingTcpMessage::DeleteServiceReplyAction)
            {
                VersionedReplyBody body;
                if (reply->GetBody(body))
                {
                    // Recursive name deletion feature was added in V2
                    if (body.FabricVersion.CodeVersion.compare(FabricVersion::Baseline->CodeVersion) >= 0)
                    {
                        nameDeleted = true;
                    }
                }
            }
        }

        // Even if the request was successful or not, always clear the cache entries
        // This is same behavior as on the gateway.
        this->Cache.ClearCacheEntriesWithName(
            name,
            activityHeader.ActivityId);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalDeleteSystemService(
        NamingUri const & name,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            message = NamingTcpMessage::GetDeleteSystemServiceRequest(Common::make_unique<Naming::DeleteSystemServiceMessageBody>(name.ToString()));
            message->Headers.Replace(FabricActivityHeader(activityId));
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            name,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalDeleteSystemService(AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        NamingUri name;
        FabricActivityHeader activityHeader;
        auto error = RequestReplyAsyncOperation::End(operation, reply, name, activityHeader);

        // Even if the request was successful or not, always clear the cache entries
        // This is same behavior as on the gateway.
        this->Cache.ClearCacheEntriesWithName(
            name,
            activityHeader.ActivityId);

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::BeginInternalLocationChangeNotificationRequest(
        vector<ServiceLocationNotificationRequestData> && requests,
        FabricActivityHeader && activityHeader,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto error = EnsureOpened();

        return AsyncOperation::CreateAndStart<LocationChangeNotificationAsyncOperation>(
            *this,
            move(requests),
            move(activityHeader),
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndInternalLocationChangeNotificationRequest(
        AsyncOperationSPtr const & operation,
        __inout vector<ResolvedServicePartitionSPtr> & addresses,
        __inout vector<AddressDetectionFailureSPtr> & failures,
        __out bool & updateNonProcessedRequest,
        __inout unique_ptr<NameRangeTuple> & firstNonProcessedRequest,
        __inout FabricActivityHeader & activityHeader)
    {
        return LocationChangeNotificationAsyncOperation::End(
            operation,
            addresses,
            failures,
            updateNonProcessedRequest,
            firstNonProcessedRequest,
            activityHeader);
    }

    //
    // *** Temporary for testing only
    //

    AsyncOperationSPtr FabricClientImpl::Test_BeginCreateNamespaceManager(
        wstring const & svcName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        //
        // Only the service name and type are relevant. The other parameters
        // are restricted and determined by the system. Some can be updated
        // dynamically after service creation (e.g. target replica set size).
        //

        ServiceDescription svcDescription;

        svcDescription.Name = wstring(svcName);

        svcDescription.Type = ServiceTypeIdentifier(
            ServicePackageIdentifier(),
            ServiceTypeIdentifier::NamespaceManagerServiceTypeId->ServiceTypeName);

        svcDescription.TargetReplicaSetSize = 1;
        svcDescription.MinReplicaSetSize = 1;

        auto message = NamingTcpMessage::GetCreateSystemServiceRequest(Common::make_unique<CreateSystemServiceMessageBody>(svcDescription));

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::Test_EndCreateNamespaceManager(AsyncOperationSPtr const &operation)
    {
        if (operation->FailedSynchronously)
        {
            return AsyncOperation::End(operation);
        }

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        return RequestReplyAsyncOperation::End(operation, reply, activityId);
    }

    AsyncOperationSPtr FabricClientImpl::Test_BeginResolveNamespaceManager(
        wstring const & svcName,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess())
        {
            return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
                error,
                callback,
                parent);
        }

        auto targetServiceName = SystemServiceApplicationNameHelper::IsPublicServiceName(svcName)
            ? svcName
            : SystemServiceApplicationNameHelper::CreatePublicNamespaceManagerServiceName(svcName);
        auto message = NamingTcpMessage::GetResolveSystemServiceRequest(Common::make_unique<ResolveSystemServiceRequestBody>(targetServiceName));

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            NamingUri::RootNamingUri,
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::Test_EndResolveNamespaceManager(
        AsyncOperationSPtr const &operation,
        __out wstring & result)
    {
        if (operation->FailedSynchronously)
        {
            return AsyncOperation::End(operation);
        }

        ClientServerReplyMessageUPtr reply;
        ActivityId activityId;
        auto error = RequestReplyAsyncOperation::End(operation, reply, activityId);

        if (error.IsSuccess())
        {
            ResolveSystemServiceReplyBody body;
            if (reply->GetBody(body))
            {
                result = body.ServiceEndpoint;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    AsyncOperationSPtr FabricClientImpl::Test_BeginTestNamespaceManager(
        wstring const & svcName,
        size_t byteCount,
        ISendTarget::SPtr const & directTarget,
        SystemServiceLocation const & primaryLocation,
        bool gatewayProxy,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        UNREFERENCED_PARAMETER(svcName);
        UNREFERENCED_PARAMETER(byteCount);
        UNREFERENCED_PARAMETER(directTarget);
        UNREFERENCED_PARAMETER(primaryLocation);
        UNREFERENCED_PARAMETER(gatewayProxy);
        UNREFERENCED_PARAMETER(timeout);

        auto error = EnsureOpened();
        return AsyncOperation::CreateAndStart<CompletedAsyncOperation>(
            error,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::Test_EndTestNamespaceManager(AsyncOperationSPtr const &operation, __out vector<byte> & result)
    {
        UNREFERENCED_PARAMETER(result);
        if (operation->FailedSynchronously)
        {
            return AsyncOperation::End(operation);
        }

        return CompletedAsyncOperation::End(operation);
    }

    void FabricClientImpl::UpdateSecurityFromConfig(weak_ptr<ComponentRoot> const & rootWPtr)
    {
        auto rootSPtr = rootWPtr.lock();
        if (rootSPtr)
        {
            FabricClientImpl & client = *reinterpret_cast<FabricClientImpl*>(rootSPtr.get());
            ErrorCode error = client.SetSecurityFromConfig();

            if (!error.IsSuccess())
            {
                WriteWarning(LifeCycleTraceComponent, client.traceContext_, "UpdateSecurityFromConfig() failed with {0}", error);
                Assert::TestAssert();

                // no-op in production: the only consequence is that the client may not be able to communicate with the gateway
            }
        }
    }

    ErrorCode FabricClientImpl::SetSecurityFromConfig()
    {
        if (!config_)
        {
            WriteError(LifeCycleTraceComponent, traceContext_, "SetSecurityFromConfig(): FabricNodeConfig not set");
            Assert::TestAssert();

            return ErrorCodeValue::OperationFailed;
        }

        AcquireWriteLock writeLock(securitySettingsUpdateLock_);
        SecurityConfig & securityConfig = SecurityConfig::GetConfig();

        SecuritySettings clientSecuritySettings;

        ErrorCode error;

        if (role_ == RoleMask::Enum::User && !isClientRoleAuthorized_)
        {
            if (securityConfig.IsClientRoleInEffect() &&
                config_->UserRoleClientX509FindValue.empty())
            {
                WriteError(LifeCycleTraceComponent, traceContext_, "SetSecurityFromConfig(): UserRoleClientCertificate not configured.");
                return ErrorCodeValue::UserRoleClientCertificateNotConfigured;
            }

            error = SecuritySettings::FromConfiguration(
                securityConfig.ServerAuthCredentialType,
                config_->UserRoleClientX509StoreName,
                wformatString(X509Default::StoreLocation()),
                config_->UserRoleClientX509FindType,
                config_->UserRoleClientX509FindValue,
                config_->UserRoleClientX509FindValueSecondary,
                securityConfig.ClientServerProtectionLevel,
                securityConfig.ServerCertThumbprints,
                securityConfig.ServerX509Names,
                securityConfig.ServerCertificateIssuerStores,
                securityConfig.ServerAuthAllowedCommonNames,
                securityConfig.ServerCertIssuers,
                securityConfig.ClusterSpn,
                L"",
                clientSecuritySettings);
        }
        else
        {
            error = SecuritySettings::FromConfiguration(
                securityConfig.ServerAuthCredentialType,
                config_->ClientAuthX509StoreName,
                wformatString(X509Default::StoreLocation()),
                config_->ClientAuthX509FindType,
                config_->ClientAuthX509FindValue,
                config_->ClientAuthX509FindValueSecondary,
                securityConfig.ClientServerProtectionLevel,
                securityConfig.ServerCertThumbprints,
                securityConfig.ServerX509Names,
                securityConfig.ServerCertificateIssuerStores,
                securityConfig.ServerAuthAllowedCommonNames,
                securityConfig.ServerCertIssuers,
                securityConfig.ClusterSpn,
                L"",
                clientSecuritySettings);
        }

        if (!error.IsSuccess())
        {
            WriteWarning(
                LifeCycleTraceComponent,
                traceContext_,
                "Failed to create client security settings: error = {0}",
                error);
        }
        else
        {
            WriteInfo(LifeCycleTraceComponent, traceContext_, "SetSecurityFromConfig(): Settings - {0}", clientSecuritySettings);

            error = this->SetSecurity(move(clientSecuritySettings));

            if (!error.IsSuccess())
            {
                WriteWarning(
                    LifeCycleTraceComponent,
                    traceContext_,
                    "Failed to set client security credentials: error = {0}",
                    error);
            }
        }

        return error;
    }

    ErrorCode FabricClientImpl::SetSecurity(SecuritySettings && securitySettings)
    {
        if (clientConnectionManager_)
        {
#ifndef PLATFORM_UNIX
            securitySettings.SetFramingProtectionEnabledCallback([] { return SecurityConfig::GetConfig().FramingProtectionEnabledInClientMode; });
#endif
            //todo, return a gateway setting from callback, when auto refresh is implemented for high priority connection
            securitySettings.SetReadyNewSessionBeforeExpirationCallback([] { return false; });

            securitySettings.SetSessionDurationCallback([] { return ServiceModelConfig::GetConfig().SessionExpiration; });

            if (securitySettings.SecurityProvider() == SecurityProvider::Ssl)
            {
                securitySettings.SetX509CertType(L"client");
            }

            return clientConnectionManager_->SetSecurity(securitySettings);
        }
        else
        {
            FabricClientImpl::WriteWarning(
                Constants::FabricClientSource,
                traceContext_,
                "ClientConnectionManager not initialized");

            return ErrorCodeValue::InvalidState;
        }
    }

    ErrorCode FabricClientImpl::SetKeepAlive(ULONG keepAliveIntervalInSeconds)
    {
        AcquireWriteLock lock(stateLock_);
        // Check state under the lock
        if (State.Value != FabricComponentState::Created)
        {
            FabricClientImpl::WriteWarning(
                Constants::FabricClientSource,
                traceContext_,
                "Can't update KeepAliveInterval to {0} as the client is already opening/opened",
                keepAliveIntervalInSeconds);
            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        // Mark the settings as stale to let other users know that they should get the latest value
        settings_->MarkAsStale();
        settings_ = make_shared<FabricClientInternalSettings>(traceContext_, keepAliveIntervalInSeconds);

        if (clientConnectionManager_)
        {
            clientConnectionManager_->SetKeepAlive(settings_->KeepAliveInterval);

            return ErrorCodeValue::Success;
        }
        else
        {
            FabricClientImpl::WriteWarning(
                Constants::FabricClientSource,
                traceContext_,
                "ClientConnectionManager not initialized");

            return ErrorCodeValue::InvalidState;
        }
    }

    FabricClientSettings FabricClientImpl::GetSettings()
    {
        AcquireReadLock lock(stateLock_);
        return settings_->GetSettings();
    }

    ErrorCode FabricClientImpl::SetSettings(FabricClientSettings && newSettings)
    {
        AcquireWriteLock lock(stateLock_);

        // Check owner state under the lock
        if ((State.Value != FabricComponentState::Created) &&
            !settings_->CanDynamicallyUpdate(newSettings))
        {
            FabricClientImpl::WriteWarning(
                Constants::FabricClientSource,
                traceContext_,
                "Can't update settings as the client is already opening/opened");
            return ErrorCode(ErrorCodeValue::InvalidState);
        }

        // Mark the settings as stale to let other users know that they should get the latest value
        settings_->MarkAsStale();
        settings_ = make_shared<FabricClientInternalSettings>(traceContext_, move(newSettings));

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode FabricClientImpl::ReportHealth(
        HealthReport && healthReport,
        ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }
        return healthClient_->HealthReportingClient.AddHealthReport(move(healthReport), move(sendOptions));
    }

    // GetClusterHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetClusterHealth(
        ClusterHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetClusterHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetClusterHealth(
        AsyncOperationSPtr const &operation,
        __inout ClusterHealth & clusterHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<ClusterHealth>(clusterHealth);
        return error;
    }

    // GetNodeHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetNodeHealth(
        NodeHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetNodeHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetNodeHealth(
        AsyncOperationSPtr const &operation,
        __inout NodeHealth & nodeHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<NodeHealth>(nodeHealth);
        return error;
    }

    // GetApplicationHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetApplicationHealth(
        ApplicationHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetApplicationHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetApplicationHealth(
        AsyncOperationSPtr const &operation,
        __inout ApplicationHealth & applicationHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<ApplicationHealth>(applicationHealth);
        return error;
    }

    // GetServiceHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetServiceHealth(
        ServiceHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServiceHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetServiceHealth(
        AsyncOperationSPtr const &operation,
        __inout ServiceHealth & serviceHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<ServiceHealth>(serviceHealth);
        return error;
    }

    // GetPartitionHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetPartitionHealth(
        PartitionHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServicePartitionHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetPartitionHealth(
        AsyncOperationSPtr const &operation,
        __inout PartitionHealth & partitionHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<PartitionHealth>(partitionHealth);
        return error;
    }

    // GetReplicaHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetReplicaHealth(
        ReplicaHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetServicePartitionReplicaHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetReplicaHealth(
        AsyncOperationSPtr const &operation,
        __inout ReplicaHealth & replicaHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<ReplicaHealth>(replicaHealth);
        return error;
    }

    // GetDeployedApplicationHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedApplicationHealth(
        DeployedApplicationHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeployedApplicationHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedApplicationHealth(
        AsyncOperationSPtr const &operation,
        __inout DeployedApplicationHealth & deployedApplicationHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<DeployedApplicationHealth>(deployedApplicationHealth);
        return error;
    }

    // GetDeployedServicePackageHealth
    AsyncOperationSPtr FabricClientImpl::BeginGetDeployedServicePackageHealth(
        DeployedServicePackageHealthQueryDescription const & queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.SetQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetDeployedServicePackageHealth),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetDeployedServicePackageHealth(
        AsyncOperationSPtr const &operation,
        __inout DeployedServicePackageHealth & deployedServicePackageHealth)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<DeployedServicePackageHealth>(deployedServicePackageHealth);
        return error;
    }

    // GetClusterHealthChunk
    AsyncOperationSPtr FabricClientImpl::BeginGetClusterHealthChunk(
        ClusterHealthChunkQueryDescription && queryDescription,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const &parent)
    {
        QueryArgumentMap argMap;
        queryDescription.MoveToQueryArguments(argMap);

        return BeginInternalQuery(
            QueryNames::ToString(QueryNames::GetClusterHealthChunk),
            argMap,
            timeout,
            callback,
            parent);
    }

    ErrorCode FabricClientImpl::EndGetClusterHealthChunk(
        AsyncOperationSPtr const &operation,
        __inout ClusterHealthChunk & clusterHealthChunk)
    {
        QueryResult queryResult;
        auto error = EndInternalQuery(operation, queryResult);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = queryResult.MoveItem<ClusterHealthChunk>(clusterHealthChunk);
        return error;
    }

    ErrorCode FabricClientImpl::InternalReportHealth(
        vector<HealthReport> && reports,
        ServiceModel::HealthReportSendOptionsUPtr && sendOptions)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }
        return healthClient_->HealthReportingClient.AddHealthReports(move(reports), move(sendOptions));
    }

    ErrorCode FabricClientImpl::HealthPreInitialize(
        wstring const & sourceId,
        FABRIC_HEALTH_REPORT_KIND kind,
        FABRIC_INSTANCE_ID instance)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }
        return healthClient_->HealthReportingClient.HealthPreInitialize(sourceId, kind, instance);
    }

    ErrorCode FabricClientImpl::HealthPostInitialize(
        wstring const & sourceId,
        FABRIC_HEALTH_REPORT_KIND kind,
        FABRIC_SEQUENCE_NUMBER sequence,
        FABRIC_SEQUENCE_NUMBER invalidateSequence)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }
        return healthClient_->HealthReportingClient.HealthPostInitialize(sourceId, kind, sequence, invalidateSequence);
    }

    ErrorCode FabricClientImpl::HealthGetProgress(
        wstring const & sourceId,
        FABRIC_HEALTH_REPORT_KIND kind,
        __inout FABRIC_SEQUENCE_NUMBER & progress)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }
        return healthClient_->HealthReportingClient.HealthGetProgress(sourceId, kind, progress);
    }

    ErrorCode FabricClientImpl::HealthSkipSequence(
        wstring const & sourceId,
        FABRIC_HEALTH_REPORT_KIND kind,
        FABRIC_SEQUENCE_NUMBER sequence)
    {
        auto error = EnsureOpened();
        if (!error.IsSuccess()) { return error; }
        return healthClient_->HealthReportingClient.HealthSkipSequence(sourceId, kind, sequence);
    }

    AsyncOperationSPtr FabricClientImpl::BeginSetAcl(
        AccessControl::ResourceIdentifier const &resource,
        AccessControl::FabricAcl const & acl,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto setAclRequest = Common::make_unique<SetAclRequest>(resource.FabricUri(), acl);
            message = NamingTcpMessage::GetSetAcl(std::move(setAclRequest));
            Trace.BeginSetAcl(
                traceContext_,
                message->ActivityId,
                resource.FabricUri(),
                acl);
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            resource.FabricUri(),
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    ErrorCode FabricClientImpl::EndSetAcl(
        AsyncOperationSPtr const & operation)
    {
        ClientServerReplyMessageUPtr reply;
        return RequestReplyAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr FabricClientImpl::BeginGetAcl(
        AccessControl::ResourceIdentifier const &resource,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
    {
        ClientServerRequestMessageUPtr message;
        auto error = EnsureOpened();
        if (error.IsSuccess())
        {
            auto getAclRequest = Common::make_unique<GetAclRequest>(resource.FabricUri());
            message = NamingTcpMessage::GetGetAcl(std::move(getAclRequest));
            Trace.BeginGetAcl(
                traceContext_,
                message->ActivityId,
                resource.FabricUri());
        }

        return AsyncOperation::CreateAndStart<RequestReplyAsyncOperation>(
            *this,
            resource.FabricUri(),
            move(message),
            timeout,
            callback,
            parent,
            move(error));
    }

    _Use_decl_annotations_
        ErrorCode FabricClientImpl::EndGetAcl(
            AsyncOperationSPtr const & operation,
            AccessControl::FabricAcl & fabricAcl)
    {
        ClientServerReplyMessageUPtr reply;
        ErrorCode error = RequestReplyAsyncOperation::End(operation, reply);
        if (error.IsSuccess())
        {
            if (!reply->GetBody(fabricAcl))
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        return error;
    }

    // Public for tests only
    bool FabricClientImpl::Test_TryGetCachedServiceResolution(
        NamingUri const & name,
        PartitionKey const & key,
        __out ResolvedServicePartitionSPtr & result)
    {
        switch (key.KeyType)
        {
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64:
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING:
        case FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_NONE:
            break;
        default:
            Assert::CodingError("Unknown key type: {0}", key.KeyType);
        }

        if (!this->Cache.TryGetEntry(name, key, result))
        {
            return false;
        }

        if (result->Locations.ServiceReplicaSet.IsStateful)
        {
            return result->Locations.ServiceReplicaSet.IsPrimaryLocationValid;
        }
        else
        {
            size_t instances = result->Locations.ServiceReplicaSet.ReplicaLocations.size();
            return instances > 0;
        }
    }

    bool FabricClientImpl::GetFabricUri(wstring const& uriString, __out NamingUri &uri, bool allowSystemApplication)
    {
        if (!allowSystemApplication)
        {
            if (SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(uriString))
            {
                return false;
            }
        }

        return NamingUri::TryParse(uriString, uri);
    }

    ErrorCode FabricClientImpl::GetImageStoreClient(
        wstring const & imageStoreConnectionString,
        __out RootedObjectPointer<IImageStore> & imageStore)
    {
        AcquireExclusiveLock lock(mapLock_);
        auto iter = imageStoreClientsMap_.find(imageStoreConnectionString);
        if (iter != imageStoreClientsMap_.end())
        {
            imageStore = RootedObjectPointer<IImageStore>(iter->second.get(), this->CreateComponentRoot());
            return ErrorCodeValue::Success;
        }
        else
        {
            IClientFactoryPtr clientFactoryPtr;
            FabricClientImplSPtr self = dynamic_pointer_cast<FabricClientImpl>(this->shared_from_this());
            auto error = ClientFactory::CreateClientFactory(
                self,
                clientFactoryPtr);
            if (!error.IsSuccess())
            {
                return error;
            }

            ImageStoreUPtr store;
            error = ImageStoreFactory::CreateImageStore(
                store,
                SecureString(imageStoreConnectionString),
                Management::ManagementConfig::GetConfig().ImageStoreMinimumTransferBPS,
                clientFactoryPtr,
                false /*isInternal*/,
                L"" /*workingDir*/);
            if (!error.IsSuccess())
            {
                return error;
            }

            imageStoreClientsMap_.insert(make_pair(imageStoreConnectionString, move(store)));

            iter = imageStoreClientsMap_.find(imageStoreConnectionString);

            ASSERT_IF(iter == imageStoreClientsMap_.end(), "ImageStoreClient should be found in the map");

            imageStore = RootedObjectPointer<IImageStore>(iter->second.get(), this->CreateComponentRoot());

            return ErrorCodeValue::Success;
        }
    }

    void FabricClientImpl::OnConnected(
        ClientConnectionManager::HandlerId,
        ISendTarget::SPtr const & st,
        GatewayDescription const & gateway)
    {
        WriteInfo(
            Constants::FabricClientSource,
            traceContext_,
            "Connected: {0} ({1})",
            st->Id(),
            gateway);

        IClientConnectionEventHandlerPtr handler;
        {
            AcquireReadLock lock(connectionEventHandlerLock_);

            handler = connectionEventHandler_;
        }

        if (handler.get() != nullptr)
        {
            auto handlerError = handler->OnConnected(gateway);

            if (!handlerError.IsSuccess())
            {
                WriteWarning(
                    Constants::FabricClientSource,
                    traceContext_,
                    "OnConnection event handler failer: {0} ({1}) handler={2}",
                    st->Id(),
                    gateway,
                    handlerError);
            }
        }
    }

    void FabricClientImpl::OnDisconnected(
        ClientConnectionManager::HandlerId,
        ISendTarget::SPtr const & st,
        GatewayDescription const & gateway,
        ErrorCode const & error)
    {
        WriteInfo(
            Constants::FabricClientSource,
            traceContext_,
            "Disconnected: {0} ({1}) reason={2}",
            st->Id(),
            gateway,
            error);

        IClientConnectionEventHandlerPtr handler;
        {
            AcquireReadLock lock(connectionEventHandlerLock_);

            handler = connectionEventHandler_;
        }

        if (handler.get() != nullptr)
        {
            auto handlerError = handler->OnDisconnected(gateway, error);

            if (!handlerError.IsSuccess())
            {
                WriteWarning(
                    Constants::FabricClientSource,
                    traceContext_,
                    "OnDisconnected event handler failed: {0} ({1}) reason={2} handler={3}",
                    st->Id(),
                    gateway,
                    error,
                    handlerError);
            }
        }
    }

    ErrorCode FabricClientImpl::OnClaimsRetrieval(
        ClientConnectionManager::HandlerId,
        shared_ptr<ClaimsRetrievalMetadata> const & metadata,
        __out wstring & token)
    {
        IClientConnectionEventHandlerPtr handler;
        {
            AcquireReadLock lock(connectionEventHandlerLock_);

            handler = connectionEventHandler_;
        }

        if (handler.get() != nullptr)
        {
            return handler->OnClaimsRetrieval(metadata, token);
        }
        else
        {
            token.clear();

            return ErrorCodeValue::Success;
        }
    }

    ErrorCode FabricClientImpl::OnNotificationReceived(vector<ServiceNotificationResultSPtr> const & notificationResults)
    {
        IServiceNotificationEventHandlerPtr snap;
        {
            AcquireReadLock lock(notificationEventHandlerLock_);

            snap = serviceNotificationEventHandler_;
        }

        if (snap.get() != NULL)
        {
            for (auto const & notificationResult : notificationResults)
            {
                auto ptr = Api::IServiceNotificationPtr(
                    notificationResult.get(),
                    notificationResult->CreateComponentRoot());

                auto error = snap->OnNotification(ptr);

                if (!error.IsSuccess())
                {
                    return error;
                }
            }
        }

        return ErrorCodeValue::Success;
    }

    bool FabricClientImpl::IsFASOrUOSReconfiguring(ErrorCode const& error)
    {
        return error.ReadValue() == ErrorCodeValue::HostingDeploymentInProgress || error.ReadValue() == ErrorCodeValue::HostingServiceTypeNotRegistered;
    }

    wstring FabricClientImpl::GetLocalGatewayAddress()
    {
        //
        // For fabricclient created inside the container needs to talk to the gateway running in the host.
        // When a container is created, the config is setup such that the ClientConnectionAddress is the host ip, so
        // using the default config inside the container would work.
        //
        // There is a windows bug that prevents containers from talking to the HOST via the host IP. Till that bug is
        // fixed, this code works around by addressing the HOST using the default gateway address.
        //
#if !defined(PLATFORM_UNIX)
        wstring isContainerHostString;
        bool isContainerHost = ContainerEnvironment::IsContainerHost();
        if (isContainerHost)
        {
            if (!NetworkType::IsMultiNetwork(ContainerEnvironment::GetContainerNetworkingMode()))
            {
                    map<wstring, vector<wstring>> gatewayAddressPerAdapter;
                    auto error = IpUtility::GetGatewaysPerAdapter(gatewayAddressPerAdapter);

                    ASSERT_IF(!error.IsSuccess(), "Getting HOST IP failed - {0}", error);
                    ASSERT_IF(gatewayAddressPerAdapter.size() > 1, "Found more than one adapter in container");

                    USHORT port = TcpTransportUtility::ParsePortString(config_->ClientConnectionAddress);
                    return TcpTransportUtility::ConstructAddressString(gatewayAddressPerAdapter.begin()->second[0], port);
                }
                else
                {
                    return config_->ClientConnectionAddress;
                }
            }
            else
            {
                return config_->ClientConnectionAddress;
            }
#else
        return config_->ClientConnectionAddress;
#endif
    }
}
