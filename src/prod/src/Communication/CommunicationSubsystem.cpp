// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Communication;
using namespace Naming;
using namespace Federation;
using namespace Hosting2;
using namespace Reliability;

class CommunicationSubsystem::Impl : public Common::RootedObject
{
public:
    Impl(
        __in Reliability::IReliabilitySubsystem &,
        __in IHostingSubsystem &,
        __in IRuntimeSharingHelper &,
        __in Transport::IpcServer &,
        std::wstring const & entreeServiceListenAddress,
        std::wstring const & replicatorAddress,
        Transport::SecuritySettings const& clusterSecuritySettings,
        std::wstring const & workingDir,
        Common::FabricNodeConfigSPtr const& nodeConfig,
        Common::ComponentRoot const &);
    ~Impl();

    __declspec (property(get=get_NamingService)) Naming::NamingService & NamingService;
    Naming::NamingService & get_NamingService() const {return *namingServiceUPtr_; }

    void EnableServiceRoutingAgent();

    Common::ErrorCode SetClusterSecurity(Transport::SecuritySettings const & securitySettings);
    Common::ErrorCode SetNamingGatewaySecurity(Transport::SecuritySettings const & securitySettings);

    Common::AsyncOperationSPtr BeginCreateNamingService(Common::TimeSpan, Common::AsyncCallback const &, Common::AsyncOperationSPtr const &);
    Common::ErrorCode EndCreateNamingService(Common::AsyncOperationSPtr const &);

    Common::AsyncOperationSPtr BeginOpen(
        Common::TimeSpan const &timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent);
    Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const &);

    Common::AsyncOperationSPtr BeginClose(
        Common::TimeSpan const &timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent);
    Common::ErrorCode EndClose(Common::AsyncOperationSPtr const &);
    void OnAbort();

private:
    class OpenAsyncOperation;
    class CloseAsyncOperation;

    void OnCreateNamingServicesComplete(Common::AsyncOperationSPtr const & creationOp);

    Naming::NamingServiceUPtr namingServiceUPtr_;
    SystemServices::ServiceRoutingAgentUPtr routingAgentUPtr_;
    bool isRoutingAgentEnabled_;
};

class CommunicationSubsystem::Impl::OpenAsyncOperation : public AsyncOperation
{
public:

    OpenAsyncOperation(
        CommunicationSubsystem::Impl & communicationSubsystem,
        TimeSpan const &timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent), 
        communicationSubsystem_(communicationSubsystem),
        timeout_(timeout)
    {
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
    {
        auto op = communicationSubsystem_.namingServiceUPtr_->BeginOpen(
            timeout_,
            [this](AsyncOperationSPtr const & op)
            {
                this->OnNamingServiceOpenComplete(op, false);
            },
            thisSPtr);

        OnNamingServiceOpenComplete(op, true);
    }

private:

    void OnNamingServiceOpenComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        auto error = communicationSubsystem_.namingServiceUPtr_->EndOpen(operation);
        if (!error.IsSuccess())
        {
            TryComplete(operation->Parent, error);
            return;
        }

        if (communicationSubsystem_.isRoutingAgentEnabled_)
        {
            error = communicationSubsystem_.routingAgentUPtr_->Open();
        }

        TryComplete(operation->Parent, error);
    }

    CommunicationSubsystem::Impl & communicationSubsystem_;
    TimeSpan timeout_;
};

class CommunicationSubsystem::Impl::CloseAsyncOperation : public AsyncOperation
{
public:

    CloseAsyncOperation(
        CommunicationSubsystem::Impl & communicationSubsystem,
        TimeSpan const &timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent)
        , communicationSubsystem_(communicationSubsystem)
        , timeout_(timeout)
        , innerError_(ErrorCodeValue::Success)

    {
    }

protected:

    void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
    {
       if (communicationSubsystem_.isRoutingAgentEnabled_)
       {
           innerError_ = communicationSubsystem_.routingAgentUPtr_->Close();
       }

       auto op = communicationSubsystem_.namingServiceUPtr_->BeginClose(
           timeout_,
           [this](AsyncOperationSPtr const & op)
           {
               this->OnNamingServiceCloseComplete(op, false);
           },
           thisSPtr);

       OnNamingServiceCloseComplete(op, true);
    }

private:

    void OnNamingServiceCloseComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        innerError_ = ErrorCode::FirstError(innerError_, communicationSubsystem_.namingServiceUPtr_->EndClose(operation));
        TryComplete(operation->Parent, innerError_);
    }

    CommunicationSubsystem::Impl & communicationSubsystem_;
    TimeSpan timeout_;
    ErrorCode innerError_;
};

CommunicationSubsystem::Impl::Impl(
    __in IReliabilitySubsystem & reliability,
    __in IHostingSubsystem & hosting,
    __in IRuntimeSharingHelper & runtime,
    __in Transport::IpcServer & ipcServer,
    wstring const & listenAddress,
    std::wstring const & replicatorAddress,
    Transport::SecuritySettings const& clusterSecuritySettings,
    std::wstring const & workingDir,
    Common::FabricNodeConfigSPtr const& nodeConfig,
    ComponentRoot const & root)
    : Common::RootedObject(root)
    , namingServiceUPtr_(NamingService::Create(
        reliability.FederationWrapper,
        runtime,
        reliability.AdminClient,
        reliability.Resolver,
        listenAddress,
        replicatorAddress,
        clusterSecuritySettings,
        workingDir,
        nodeConfig,
        hosting.FabricActivatorClientObj,
        root))
    , routingAgentUPtr_(SystemServices::ServiceRoutingAgent::Create(
        ipcServer,
        reliability.Federation,
        hosting,
        namingServiceUPtr_->Gateway,
        root))
    , isRoutingAgentEnabled_(false)
{
    Trace.WriteInfo(
        Constants::CommunicationSource,
        "{0} : constructor called",
        static_cast<void*>(this));
}

CommunicationSubsystem::Impl::~Impl()
{
    Trace.WriteInfo(
        Constants::CommunicationSource,
        "{0} : destructor called",
        static_cast<void*>(this));
}

void CommunicationSubsystem::Impl::EnableServiceRoutingAgent()
{
    isRoutingAgentEnabled_ = true;
}

AsyncOperationSPtr CommunicationSubsystem::Impl::BeginOpen(
    TimeSpan const &timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}
ErrorCode CommunicationSubsystem::Impl::EndOpen(AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr CommunicationSubsystem::Impl::BeginClose(
    TimeSpan const &timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
}
ErrorCode CommunicationSubsystem::Impl::EndClose(AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

void CommunicationSubsystem::Impl::OnAbort()
{
    if (isRoutingAgentEnabled_)
    {
        routingAgentUPtr_->Abort();
    }

    namingServiceUPtr_->Abort();
}

AsyncOperationSPtr CommunicationSubsystem::Impl::BeginCreateNamingService(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
{
    NamingConfig & config = NamingConfig::GetConfig();

    // ensure this service is isolated to other services
    vector<ServiceLoadMetricDescription> serviceMetricDescriptions;
    serviceMetricDescriptions.push_back(ServiceLoadMetricDescription(
        Naming::Constants::NamingServicePrimaryCountName,
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH,
        1,
        0));
    serviceMetricDescriptions.push_back(ServiceLoadMetricDescription(
        Naming::Constants::NamingServiceReplicaCountName,
        FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM,
        1,
        1));

    ServiceDescription namingDescription(
        ServiceModel::SystemServiceApplicationNameHelper::InternalNamingServiceName,
        DateTime::Now().Ticks,
        0, /* ServiceUpdateVersion */
        config.PartitionCount,
        config.TargetReplicaSetSize,
        config.MinReplicaSetSize,
        true,
        true,
        config.ReplicaRestartWaitDuration,
        config.QuorumLossWaitDuration,
        config.StandByReplicaKeepDuration,
        *ServiceModel::ServiceTypeIdentifier::NamingStoreServiceTypeId,
        vector<ServiceCorrelationDescription>(),
        config.PlacementConstraints,
        Naming::Constants::ScaleoutCount,
        move(serviceMetricDescriptions),
        Naming::Constants::DefaultMoveCost,
        vector<byte>());

    vector<ConsistencyUnitDescription> namingCUDescriptions(config.PartitionCount);        
    for (int i = 0; i < config.PartitionCount; ++i)
    {
        // NamingStoreService does not use conventional partition ranges for ownership,
        // so [0,0] is acceptable for the RA to pass to the service
        namingCUDescriptions[i] = ConsistencyUnitDescription(namingServiceUPtr_->NamingServiceCuids[i], 0, 0);
    }

    return namingServiceUPtr_->BeginCreateService(
        namingDescription,
        namingCUDescriptions,
        timeout,
        callback,
        root);
}

ErrorCode CommunicationSubsystem::Impl::EndCreateNamingService(AsyncOperationSPtr const & creationOp)
{
    return namingServiceUPtr_->EndCreateService(creationOp);
}

ErrorCode CommunicationSubsystem::Impl::SetClusterSecurity(Transport::SecuritySettings const & securitySettings)
{
    return this->namingServiceUPtr_->SetClusterSecurity(securitySettings);
}

ErrorCode CommunicationSubsystem::Impl::SetNamingGatewaySecurity(Transport::SecuritySettings const & securitySettings)
{
    return this->namingServiceUPtr_->SetNamingGatewaySecurity(securitySettings);
}

//
// *** Public class
//

CommunicationSubsystem::CommunicationSubsystem(
    __in IReliabilitySubsystem & reliability,
    __in IHostingSubsystem & hosting,
    __in IRuntimeSharingHelper & runtime,
    __in Transport::IpcServer & ipcServer,
    wstring const & entreeServiceListenAddress,
    std::wstring const & replicatorAddress,
    Transport::SecuritySettings const& clusterSecuritySettings,
    wstring const & workingDir,
    Common::FabricNodeConfigSPtr const& nodeConfig,
    ComponentRoot const & root)
    : implUPtr_(make_unique<Impl>(
        reliability,
        hosting,
        runtime,
        ipcServer,
        entreeServiceListenAddress,
        replicatorAddress,
        clusterSecuritySettings,
        workingDir,
        nodeConfig,
        root))
{
}

CommunicationSubsystem::~CommunicationSubsystem()
{
}

Naming::NamingService & CommunicationSubsystem::get_NamingService() const { return implUPtr_->NamingService; }

void CommunicationSubsystem::EnableServiceRoutingAgent()
{
    implUPtr_->EnableServiceRoutingAgent();
}

AsyncOperationSPtr CommunicationSubsystem::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return implUPtr_->BeginOpen(timeout, callback, parent);
}
ErrorCode CommunicationSubsystem::OnEndOpen(AsyncOperationSPtr const & operation)
{
    return implUPtr_->EndOpen(operation);
}

AsyncOperationSPtr CommunicationSubsystem::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return implUPtr_->BeginClose(timeout, callback, parent);
}
ErrorCode CommunicationSubsystem::OnEndClose(AsyncOperationSPtr const & operation)
{
    return implUPtr_->EndClose(operation);
}

void CommunicationSubsystem::OnAbort()
{
    return implUPtr_->OnAbort();
}

AsyncOperationSPtr CommunicationSubsystem::BeginCreateNamingService(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & root)
{
    return implUPtr_->BeginCreateNamingService(timeout, callback, root);
}

ErrorCode CommunicationSubsystem::EndCreateNamingService(AsyncOperationSPtr const & creationOp)
{
    return implUPtr_->EndCreateNamingService(creationOp);
}

ErrorCode CommunicationSubsystem::SetClusterSecurity(Transport::SecuritySettings const & securitySettings)
{
    return implUPtr_->SetClusterSecurity(securitySettings);
}

ErrorCode CommunicationSubsystem::SetNamingGatewaySecurity(Transport::SecuritySettings const & securitySettings)
{
    return implUPtr_->SetNamingGatewaySecurity(securitySettings);
}
