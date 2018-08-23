// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
using namespace Api;
using namespace Transport;
using namespace Common;
using namespace Client;
using namespace Reliability;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;

namespace Naming
{
    class NamingService::NamingServiceImpl 
        : public RootedObject
    {
    public:
        NamingServiceImpl(
            FederationWrapper & transport,
            Hosting2::IRuntimeSharingHelper & runtimeSharingHelper,
            ServiceAdminClient & adminClient,
            ServiceResolver & serviceResolver,
            std::wstring const & entreeServiceListenAddress,
            std::wstring const & replicatorAddress,
            SecuritySettings const& clusterSecuritySettings,
            std::wstring const & workingDir,
            Common::FabricNodeConfigSPtr const& nodeConfig,
            Hosting2::IFabricActivatorClientSPtr const & activatorClient,
            ComponentRoot const & root)
            : RootedObject(root)
            , entreeServiceListenAddress_(entreeServiceListenAddress)
            , replicatorAddress_(replicatorAddress)
            , workingDir_(workingDir)
            , resolver_(serviceResolver)
            , adminClient_(adminClient)
            , federationWrapper_(transport)
            , runtimeSharingHelper_(runtimeSharingHelper)
            , clusterSecuritySettings_(clusterSecuritySettings)
            , storeServiceFactoryCPtr_()
            , serviceFactoryLock_()
        {
            IGatewayManagerSPtr gatewayMgr;
            if (ServiceModelConfig::GetConfig().ActivateGatewayInproc)
            {
                gatewayMgr = TestGatewayManager::CreateGatewayManager(nodeConfig, root);
            }
            else
            {
                gatewayMgr = GatewayManagerWrapper::CreateGatewayManager(nodeConfig, activatorClient, root);
            }

            entreeService_ = make_shared<EntreeService>(
                federationWrapper_.Federation,
                resolver_,
                adminClient_,
                gatewayMgr,
                transport.Instance,
                entreeServiceListenAddress,
                nodeConfig,
                clusterSecuritySettings_,
                root);
        }

        __declspec(property(get=get_Gateway)) IGateway & Gateway;
        IGateway & get_Gateway() { return *entreeService_; }

        ~NamingServiceImpl()
        {
            Trace.WriteNoise(Constants::NamingServiceSource, "{0} destructing NamingServiceImpl", federationWrapper_.Instance); 
        }

        ErrorCode SetClusterSecurity(SecuritySettings const & value)
        {
            ComPointer<ComStatefulServiceFactory> snap;
            {
                AcquireReadLock lock(serviceFactoryLock_);

                snap = storeServiceFactoryCPtr_;
            }

            if (!snap)
            {
                return ErrorCodeValue::ObjectClosed;
            }

            auto * casted = dynamic_cast<StoreServiceFactory*>(snap->get_Impl().get());
            return casted->UpdateReplicatorSecuritySettings(value);
        }

        ErrorCode SetNamingGatewaySecurity(SecuritySettings const & securitySettings)
        {
            return entreeService_->SetSecurity(securitySettings);
        }

        AsyncOperationSPtr BeginCreateService(
            ServiceDescription const & service,
            std::vector<ConsistencyUnitDescription> descriptions,
            TimeSpan const timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            return adminClient_.BeginCreateService(
                service,
                descriptions,
                FabricActivityHeader(),
                timeout,
                callback,
                parent);
        }

        ErrorCode EndCreateService(AsyncOperationSPtr const & createServiceOp)
        {
            CreateServiceReplyMessageBody replyBody;
            ErrorCode error = adminClient_.EndCreateService(createServiceOp, replyBody);

            if (error.IsSuccess())
            {
                error = replyBody.ErrorCodeValue;
            }

            return error;
        }

        ActiveServiceMap const & Test_get_ActiveServices() const
        {
            ComPointer<ComStatefulServiceFactory> snap;
            {
                AcquireReadLock lock(serviceFactoryLock_);

                snap = storeServiceFactoryCPtr_;
            }

            if (!snap)
            {
                Assert::CodingError("Test_ActiveServices: Naming Service has been closed");
            }

            auto * casted = dynamic_cast<StoreServiceFactory*>(snap->get_Impl().get());
            return casted->Test_ActiveServices;
        }

        INamingMessageProcessorSPtr GetGateway()
        {
            return entreeService_;
        }

        void InitializeClientFactory(IClientFactoryPtr const & clientFactory)
        {
            clientFactory_ = clientFactory;
        }

        std::vector<ConsistencyUnitId> const & get_NamingServiceCuids() const
        {
            return entreeService_->NamingServiceCuids;
        }

        wstring const& Test_get_EntreeServiceListenAddress() const 
        { 
            return entreeServiceListenAddress_;
        }

        AsyncOperationSPtr BeginOpen(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent);
        ErrorCode EndOpen(AsyncOperationSPtr const & operation);

        AsyncOperationSPtr BeginClose(
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
        {
            Shutdown();
            return entreeService_->BeginClose(timeout, callback, parent);
        }

        ErrorCode EndClose(AsyncOperationSPtr const & operation)
        {
            return entreeService_->EndClose(operation);
        }

        void Abort()
        {
            Shutdown();
            entreeService_->Abort();
        }

    private:
        class OpenAsyncOperation;

        void Shutdown();

        ErrorCode InitializeSerializationEstimates();

        FederationWrapper & federationWrapper_;
        Hosting2::IRuntimeSharingHelper & runtimeSharingHelper_;
        ServiceAdminClient & adminClient_;
        ServiceResolver & resolver_;
        IClientFactoryPtr clientFactory_;

        std::wstring entreeServiceListenAddress_;
        std::wstring replicatorAddress_;
        std::wstring workingDir_;
        SecuritySettings clusterSecuritySettings_;

        std::shared_ptr<EntreeService> entreeService_;
        ComPointer<ComStatefulServiceFactory> storeServiceFactoryCPtr_;
        mutable RwLock serviceFactoryLock_;
    };

    // *****************
    // NamingServiceImpl
    // *****************
    
    class NamingService::NamingServiceImpl::OpenAsyncOperation : public TimedAsyncOperation
    {
    public:
        OpenAsyncOperation(
            NamingServiceImpl & namingServiceImpl,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & asyncOperationParent)
            : TimedAsyncOperation(timeout, callback, asyncOperationParent),
            owner_(namingServiceImpl),
            timeout_(timeout)
        {
        }

    protected:

        void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
        {
            auto error = owner_.InitializeSerializationEstimates();
            if (!error.IsSuccess())
            {
                this->TryComplete(thisSPtr, error);
                return;
            }

            auto op = owner_.entreeService_->BeginOpen(
                RemainingTime,
                [this](AsyncOperationSPtr const & operation)
            {
                OnEntreeServiceOpenComplete(operation, false);
            },
                thisSPtr);

            OnEntreeServiceOpenComplete(op, true);
        }

    private:

        void OnEntreeServiceOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
        {
            if (operation->CompletedSynchronously != expectedCompletedSynchronously)
            {
                return;
            }

            ErrorCode error = owner_.entreeService_->EndOpen(operation);
            if (!error.IsSuccess())
            {
                TryComplete(operation->Parent, error);
                return;
            }

            owner_.entreeServiceListenAddress_ = owner_.entreeService_->EntreeServiceListenAddress;
            ComPointer<IFabricRuntime> fabricRuntimeCPtr = owner_.runtimeSharingHelper_.GetRuntime();
            if (!fabricRuntimeCPtr)
            {
                Trace.WriteInfo(
                    Constants::NamingServiceSource,
                    "{0} failed to get COM fabric runtime object, there must be a racing close/abort",
                    owner_.federationWrapper_.Instance);
                TryComplete(operation->Parent, ErrorCodeValue::ObjectClosed);
                return;
            }

            auto factorySPtr = make_shared<StoreServiceFactory>(
                owner_.federationWrapper_,
                owner_.adminClient_,
                owner_.resolver_,
                owner_.clientFactory_,
                owner_.replicatorAddress_,
                owner_.clusterSecuritySettings_,
                owner_.workingDir_,
                owner_.Root);

            ComPointer<ComStatefulServiceFactory> snap;
            {
                AcquireWriteLock lock(owner_.serviceFactoryLock_);

                owner_.storeServiceFactoryCPtr_ = make_com<ComStatefulServiceFactory>(IStatefulServiceFactoryPtr(
                    factorySPtr.get(), 
                    factorySPtr->CreateComponentRoot()));

                snap = owner_.storeServiceFactoryCPtr_;
            }

            HRESULT hr = fabricRuntimeCPtr->RegisterStatefulServiceFactory(
                ServiceTypeIdentifier::NamingStoreServiceTypeId->ServiceTypeName.c_str(),
                snap.GetRawPointer());
            TryComplete(operation->Parent, ErrorCode::FromHResult(hr));
        }

        TimeSpan timeout_;
        NamingServiceImpl &owner_;
    };

    AsyncOperationSPtr NamingService::NamingServiceImpl::BeginOpen(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<NamingService::NamingServiceImpl::OpenAsyncOperation>(*this, timeout, callback, parent);
    }

    ErrorCode NamingService::NamingServiceImpl::EndOpen(AsyncOperationSPtr const & operation)
    {
        return OpenAsyncOperation::End(operation);
    }

    ErrorCode NamingService::NamingServiceImpl::InitializeSerializationEstimates()
    {
        auto error = EnumerateSubNamesResult::InitializeSerializationOverheadEstimate();
        if (!error.IsSuccess())
        {
            return error;
        }

        size_t replyThreshold = NamingUtility::GetMessageContentThreshold();

        // Ensure that we will always be able to enumerate (or get) at least one maximum sized named property
        // at a time. That is, verify that the MaxMessageSize and MessageContentBufferRatio configurations
        // are reasonable.
        //
        size_t maxReplySizeEstimate = EnumeratePropertiesResult::EstimateSerializedSize(
            CommonConfig::GetConfig().MaxNamingUriLength,
            ServiceModelConfig::GetConfig().MaxPropertyNameLength,
            ServiceModel::Constants::NamedPropertyMaxValueSize);

        if (NamingUtility::CheckEstimatedSize(maxReplySizeEstimate, replyThreshold).IsSuccess())
        {
            Trace.WriteInfo(
                Constants::NamingServiceSource,
                "{0} Configured (MaxMessageSize * MessageContentBufferRatio): allowed = {1} minimum = {2} (bytes)",
                federationWrapper_.Instance,
                replyThreshold,
                maxReplySizeEstimate);
        }
        else
        { 
            Trace.WriteError(
                Constants::NamingServiceSource,
                "{0} Configured (MaxMessageSize * MessageContentBufferRatio) value is too small: allowed = {1} minimum = {2}",
                federationWrapper_.Instance,
                replyThreshold,
                maxReplySizeEstimate);

            error = ErrorCodeValue::InvalidConfiguration;
        }

        return error;
    }

    void NamingService::NamingServiceImpl::Shutdown()
    {
        ComPointer<ComStatefulServiceFactory> snap;
        {
            AcquireWriteLock lock(serviceFactoryLock_);

            snap.Swap(storeServiceFactoryCPtr_);
        }
    }

    // ***************************
    // public -> impl API mappings
    // ***************************
    NamingService::~NamingService()
    {
    }
    
    NamingServiceUPtr NamingService::Create(
        FederationWrapper & transportSPtr,
        Hosting2::IRuntimeSharingHelper & runtimeSharingHelper,
        ServiceAdminClient & adminClientSPtr,
        ServiceResolver & serviceResolverSPtr,
        std::wstring const & entreeServiceListenAddress,
        std::wstring const & replicatorAddress,
        SecuritySettings const& clusterSecuritySettings,
        std::wstring const & workingDir,
        Common::FabricNodeConfigSPtr const& nodeConfig,
        Hosting2::IFabricActivatorClientSPtr const & activatorClient,
        ComponentRoot const & root)
    {
        return std::unique_ptr<NamingService>(new NamingService(
            transportSPtr,
            runtimeSharingHelper,
            adminClientSPtr,
            serviceResolverSPtr,
            entreeServiceListenAddress,
            replicatorAddress,
            clusterSecuritySettings,
            workingDir,
            nodeConfig,
            activatorClient,
            root));
    }

    NamingService::NamingService(
        FederationWrapper & transport,
        Hosting2::IRuntimeSharingHelper & runtimeSharingHelper,
        ServiceAdminClient & adminClient,
        ServiceResolver & serviceResolver,
        std::wstring const & entreeServiceListenAddress,
        std::wstring const & replicatorAddress,
        SecuritySettings const& clusterSecuritySettings,
        std::wstring const& workingDir,
        Common::FabricNodeConfigSPtr const& nodeConfig,
        Hosting2::IFabricActivatorClientSPtr const & activatorClient,
        ComponentRoot const & root)
        : RootedObject(root)
        , impl_(make_unique<NamingServiceImpl>(
            transport
            , runtimeSharingHelper
            , adminClient
            , serviceResolver
            , entreeServiceListenAddress
            , replicatorAddress
            , clusterSecuritySettings
            , workingDir
            , nodeConfig
            , activatorClient
            , root))
    {
    }

    ErrorCode NamingService::CreateSystemServiceDescriptionFromTemplate(
        ServiceDescription const & svcTemplate,
        __out ServiceDescription & svcResult)
    {
        return CreateSystemServiceDescriptionFromTemplate(ActivityId(), svcTemplate, svcResult);
    }

    ErrorCode NamingService::CreateSystemServiceDescriptionFromTemplate(
        ActivityId const & activityId,
        ServiceDescription const & svcTemplate,
        __out ServiceDescription & svcResult)
    {
        if (svcTemplate.Type.ServiceTypeName == ServiceTypeIdentifier::NamespaceManagerServiceTypeId->ServiceTypeName)
        {
            // Copy all fields.
            //
            ServiceDescription svcDescription = svcTemplate;

            // The following values are restricted and cannot be changed.
            //
            svcDescription.ApplicationName = wstring(SystemServiceApplicationNameHelper::SystemServiceApplicationName);
            svcDescription.Name = SystemServiceApplicationNameHelper::CreateInternalNamespaceManagerServiceName(svcTemplate.Name);
            svcDescription.Type = ServiceTypeIdentifier(ServiceTypeIdentifier::NamespaceManagerServiceTypeId);
            svcDescription.Instance = DateTime::Now().Ticks; 
            svcDescription.UpdateVersion = 0; 
            svcDescription.PartitionCount = 1;
            svcDescription.IsStateful = true; 
            svcDescription.HasPersistedState = true; 

            // The following values are defaulted but can be updated after service creation.
            //
            svcDescription.ReplicaRestartWaitDuration = NamingConfig::GetConfig().ReplicaRestartWaitDuration;
            svcDescription.QuorumLossWaitDuration = NamingConfig::GetConfig().QuorumLossWaitDuration;
            svcDescription.StandByReplicaKeepDuration = NamingConfig::GetConfig().StandByReplicaKeepDuration;

            svcResult = move(svcDescription);
            
            return ErrorCodeValue::Success;
        }
        else
        {
            Trace.WriteWarning(
                Constants::NamingServiceSource, 
                "{0}: unsupported dynamic system service type: {1}",
                activityId,
                svcTemplate.Type); 

            return ErrorCodeValue::InvalidServiceType;
        }
    }

    AsyncOperationSPtr NamingService::BeginCreateService(
         ServiceDescription const & service,
         std::vector<ConsistencyUnitDescription> descriptions,
         TimeSpan const timeout,
         AsyncCallback const & callback,
         AsyncOperationSPtr const & parent)
    {
        return impl_->BeginCreateService(service, descriptions, timeout, callback, parent);
    }

    ErrorCode NamingService::EndCreateService(AsyncOperationSPtr const & createServiceOp)
    {
        return impl_->EndCreateService(createServiceOp);
    }

    ActiveServiceMap const & NamingService::Test_get_ActiveServices() const
    {
        return impl_->Test_get_ActiveServices();
    }

    std::vector<ConsistencyUnitId> const & NamingService::get_NamingServiceCuids() const
    {
        return impl_->get_NamingServiceCuids();
    }

    wstring const& NamingService::Test_get_EntreeServiceListenAddress() const 
    { 
        return impl_->Test_get_EntreeServiceListenAddress();
    }

    AsyncOperationSPtr NamingService::OnBeginOpen(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return impl_->BeginOpen(timeout, callback, parent);
    }

    ErrorCode NamingService::OnEndOpen(AsyncOperationSPtr const & operation)
    {
        return impl_->EndOpen(operation);
    }

    AsyncOperationSPtr NamingService::OnBeginClose(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return impl_->BeginClose(timeout, callback, parent);
    }

    ErrorCode NamingService::OnEndClose(AsyncOperationSPtr const & operation)
    {
        return impl_->EndClose(operation);
    }

    void NamingService::OnAbort()
    {
        impl_->Abort();
    }

    IGateway & NamingService::get_Gateway() const
    {
        return impl_->Gateway;
    }

    void NamingService::InitializeClientFactory(IClientFactoryPtr const & clientFactory)
    {
        impl_->InitializeClientFactory(clientFactory);
    } 

    ErrorCode NamingService::SetClusterSecurity(SecuritySettings const & securitySettings)
    {
        return impl_->SetClusterSecurity(securitySettings);
    }

    ErrorCode NamingService::SetNamingGatewaySecurity(SecuritySettings const & securitySettings)
    {
        return impl_->SetNamingGatewaySecurity(securitySettings);
    }

    INamingMessageProcessorSPtr NamingService::GetGateway()
    {
        return impl_->GetGateway();
    }
}
