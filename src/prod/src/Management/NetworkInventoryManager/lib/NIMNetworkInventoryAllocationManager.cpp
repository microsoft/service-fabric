#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ServiceModel;
using namespace Management::NetworkInventoryManager;

StringLiteral const NIMAllocationManagerProvider("NIMAllocationManager");

class NetworkInventoryAllocationManager::PublishNetworkMappingJobItem : public AsyncWorkJobItem
{
    DENY_COPY(PublishNetworkMappingJobItem)
public:
    PublishNetworkMappingJobItem()
        : AsyncWorkJobItem()
        , owner_()
        , error_(ErrorCodeValue::Success)
        , operation_()
    {
    }

    PublishNetworkMappingJobItem(
        __in NetworkInventoryAllocationManager * owner,
        __in const Federation::NodeInstance nodeInstance,
        __in PublishNetworkTablesRequestMessage & publishRequest,
        TimeSpan const & workTimeout)
        : AsyncWorkJobItem(workTimeout)
        , owner_(owner)
        , nodeInstance_(nodeInstance)
        , publishRequest_(publishRequest)
        , error_(ErrorCodeValue::Success)
        , operation_()
    {
    }

    PublishNetworkMappingJobItem(PublishNetworkMappingJobItem && other)
        : AsyncWorkJobItem(move(other))
        , owner_(move(other.owner_))
        , nodeInstance_(move(other.nodeInstance_))
        , publishRequest_(move(other.publishRequest_))
        , error_(move(other.error_))
        , operation_(move(other.operation_))
    {
    }

    PublishNetworkMappingJobItem & operator=(PublishNetworkMappingJobItem && other)
    {
        if (this != &other)
        {
            owner_ = move(other.owner_);
            nodeInstance_ = other.nodeInstance_;
            publishRequest_ = move(other.publishRequest_);
            error_.ReadValue();
            error_ = move(other.error_);
            operation_ = move(other.operation_);
        }

        __super::operator=(move(other));
        return *this;
    }

    virtual void OnEnqueueFailed(ErrorCode && error) override
    {
        WriteInfo(NIMAllocationManagerProvider, "Job queue rejected work: {0}", error);
        if (error.IsError(ErrorCodeValue::JobQueueFull))
        {
            error = ErrorCode(ErrorCodeValue::NamingServiceTooBusy);
        }

        //owner_->OnProcessRequestFailed(move(error), *context_);
    }

protected:
    virtual void StartWork(
        AsyncWorkReadyToCompleteCallback const & completeCallback,
        __out bool & completedSync) override
    {
        completedSync = false;

        WriteNoise(NIMAllocationManagerProvider, "Start processing PublishNetworkMappingJobItem to node: {0}: RemainingTime: {1}, job queue sequence number {2}: {3}",
            this->nodeInstance_.ToString(),
            this->RemainingTime,
            this->SequenceNumber,
            this->publishRequest_);

        NodeId target = this->nodeInstance_.Id;

        operation_ = this->owner_->service_.BeginSendPublishNetworkTablesRequestMessage(this->publishRequest_,
            this->nodeInstance_,
            this->RemainingTime,
            [this, completeCallback](AsyncOperationSPtr const & contextSPtr) -> void
            {
                NetworkErrorCodeResponseMessage reply;
                auto error = this->owner_->service_.EndSendPublishNetworkTablesRequestMessage(contextSPtr, reply);

                if (!error.IsSuccess())
                {
                    WriteError(NIMAllocationManagerProvider, "Failed to send PublishNetworkTablesRequestMessage to node: [{0}], {1}",
                        this->nodeInstance_.ToString(),
                        error.ErrorCodeValueToString());
                }

                completeCallback(this->SequenceNumber);
            });
    }

    virtual void EndWork() override
    {
    }

    virtual void OnStartWorkTimedOut() override
    {
    }

private:
    NetworkInventoryAllocationManager * owner_;
    Federation::NodeInstance nodeInstance_;
    PublishNetworkTablesRequestMessage publishRequest_;

    ErrorCode error_;
    AsyncOperationSPtr operation_;
};

template<class T>
using InvokeMethod = std::function<Common::ErrorCode(T &, AsyncOperationSPtr const &)>;

template <class TRequest, class TReply>
class NetworkInventoryAllocationManager::InvokeActionAsyncOperation : public AsyncOperation
{
public:

    InvokeActionAsyncOperation(
        __in NetworkInventoryAllocationManager & owner,
        TRequest & request,
        TimeSpan const timeout,
        InvokeMethod<TRequest> const & invokeMethod,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & operation)
        : AsyncOperation(callback, operation),
        owner_(owner),
        timeoutHelper_(timeout),
        invokeMethod_(invokeMethod),
        request_(request)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation,
        __out std::shared_ptr<TReply> & reply)
    {
        auto thisPtr = AsyncOperation::End<InvokeActionAsyncOperation>(operation);
        reply = move(thisPtr->reply_);
        reply->ErrorCode = thisPtr->Error;
        return thisPtr->Error;
    }

    _declspec(property(get = get_Reply, put = set_Reply)) std::shared_ptr<TReply> Reply;
    std::shared_ptr<TReply> get_Reply() { return this->reply_; };
    void set_Reply(std::shared_ptr<TReply> & value) { this->reply_ = value; }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(NIMAllocationManagerProvider, "InvokeActionAsyncOperation: starting.");
        this->reply_ = make_shared<TReply>();

        auto error = this->invokeMethod_(request_, thisSPtr);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
    }

private:
    const InvokeMethod<TRequest> invokeMethod_;
    TimeoutHelper timeoutHelper_;
    NetworkInventoryAllocationManager & owner_;

    TRequest & request_;
    std::shared_ptr<TReply> reply_;
};


// ********************************************************************************************************************
// NetworkInventoryAllocationManager::AssociateApplicationAsyncOperation Implementation
// ********************************************************************************************************************
class NetworkInventoryAllocationManager::AssociateApplicationAsyncOperation
    : public AsyncOperation
{
    DENY_COPY(AssociateApplicationAsyncOperation)

public:
    AssociateApplicationAsyncOperation(
        __in NetworkInventoryAllocationManager & owner,
        std::wstring const & networkName,
        std::wstring const & applicationName,
        bool isAssociation,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        networkName_(networkName),
        applicationName_(applicationName),
        isAssociation_(isAssociation)
    {
    }

    virtual ~AssociateApplicationAsyncOperation()
    {
    }

    static ErrorCode AssociateApplicationAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<AssociateApplicationAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);

        AcquireExclusiveLock lock(owner_.managerLock_);

        if (StringUtility::CompareCaseInsensitive(networkName_, NetworkType::OpenStr) != 0 &&
            StringUtility::CompareCaseInsensitive(networkName_, NetworkType::OtherStr) != 0)
        {
            auto netIter = owner_.networkMgrMap_.find(networkName_);
            if (netIter != owner_.networkMgrMap_.end())
            {
                WriteInfo(NIMAllocationManagerProvider, "AssociateApplication to Network: found network: [{0}] => [{1}]",
                    networkName_, this->applicationName_);

                auto nm = netIter->second;

                std::vector<NIMNetworkNodeAllocationStoreDataSPtr> nodeEntries;
                auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(nm));
                auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

                error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
                if (error.IsSuccess())
                {
                    auto networkMgr = lockedNetworkMgr->Get();
                    networkMgr->AssociateApplication(this->applicationName_, this->isAssociation_);

                    auto a = networkMgr->BeginUpdatePersistentData(nodeEntries,
                        FederationConfig::GetConfig().MessageTimeout,
                        [this, lockedNetworkMgr, thisSPtr](AsyncOperationSPtr const & contextSPtr)
                    {
                        std::shared_ptr<ErrorCode> reply;
                        auto error = lockedNetworkMgr->Get()->EndUpdatePersistentData(contextSPtr, reply);

                        thisSPtr->TryComplete(thisSPtr, error);
                    });
                }
            }
            else
            {
                WriteError(NIMAllocationManagerProvider, "AssociateApplication to Network: Failed to find network: [{0}] -> [{1}]",
                    networkName_, this->applicationName_);

                error = ErrorCode(ErrorCodeValue::NetworkNotFound);
                TryComplete(thisSPtr, error);
            }
        }
        else
        {
            // TODO: handle accounting for open network as well
            TryComplete(thisSPtr, error);
        }
    }

private:
    NetworkInventoryAllocationManager & owner_;
    std::wstring const & networkName_;
    std::wstring const & applicationName_;
    bool isAssociation_;
};



NetworkInventoryAllocationManager::NetworkInventoryAllocationManager(NetworkInventoryService & service) : 
    service_(service)
{}

ErrorCode NetworkInventoryAllocationManager::Initialize(MACAllocationPoolSPtr macPool,
    VSIDAllocationPoolSPtr vsidPool)
{
    ErrorCode error(ErrorCodeValue::Success);

    wstring uniqueId = wformatString("NIMServiceQueue");

    publishNetworkMappingJobQueue_ = make_shared<PublishNetworkMappingAsyncJobQueue>(
        L"Publish Network Mapping queue",
        uniqueId,
        this->service_.FM.CreateComponentRoot(),
        make_unique<NIMPublishMappingJobQueueConfigSettingsHolder>());

    //--------
    // Rehydrate the state from the persistent store.
    error = this->LoadFromStore(macPool, vsidPool);

    return error;
}


NetworkInventoryAllocationManager::~NetworkInventoryAllocationManager()
{
}

void NetworkInventoryAllocationManager::Close()
{
    if (publishNetworkMappingJobQueue_)
    {
        publishNetworkMappingJobQueue_->Close();
        publishNetworkMappingJobQueue_.reset();
    }
}

ErrorCode NetworkInventoryAllocationManager::LoadFromStore(MACAllocationPoolSPtr macPool,
    VSIDAllocationPoolSPtr vsidPool)
{
    ErrorCode error;

    {
        AcquireExclusiveLock lock(managerLock_);

        std::vector<NIMNetworkMACAddressPoolStoreDataSPtr> macAddressPools;
        std::vector<NIMNetworkIPv4AddressPoolStoreDataSPtr> ipAddressPools;
        std::vector<NIMNetworkDefinitionStoreDataSPtr> networkDefinitions;
        std::vector<NIMNetworkNodeAllocationStoreDataSPtr> networkNodeAllocations;

        error = this->service_.FM.Store.LoadAll(macAddressPools);

        if (error.IsSuccess())
        {
            //--------
            // Bootstrap the global MAC address pool.
            if (macAddressPools.size() == 0)
            {
                WriteInfo(NIMAllocationManagerProvider, "LoadFromStore: bootstrapping global MAC address pool");

                this->macPool_ = make_shared<NIMNetworkMACAddressPoolStoreData>(macPool, vsidPool, L"MAC", L"global");
                this->macPool_->PersistenceState = PersistenceState::ToBeInserted;

                macAddressPools.emplace_back(this->macPool_);
            }
            else
            {
                ASSERT_IFNOT(macAddressPools.size() == 1, "LoadFromStore: only one MAC pool expected!");
                this->macPool_ = macAddressPools[0];
            }

            WriteInfo(NIMAllocationManagerProvider, "{0} MAC address pools loaded", macAddressPools.size());
            error = this->service_.FM.Store.LoadAll(ipAddressPools);
        }

        if (error.IsSuccess())
        {
            WriteInfo(NIMAllocationManagerProvider, "{0} IPv4 address pools loaded", ipAddressPools.size());
            error = this->service_.FM.Store.LoadAll(networkDefinitions);
        }

        if (error.IsSuccess())
        {
            WriteInfo(NIMAllocationManagerProvider, "{0} network definitions loaded", networkDefinitions.size());
            error = this->service_.FM.Store.LoadAll(networkNodeAllocations);
        }

        if (error.IsSuccess())
        {
            //--------
            // Regenerate the internal running state using the persisted data.
            for (const auto & item : networkDefinitions)
            {
                NetworkManagerSPtr networkMgr;

                networkMgr = make_shared<NetworkManager>(service_);
                networkMgr->Initialize(item, macPool_, true);
                if (!error.IsSuccess())
                {
                    WriteError(NIMAllocationManagerProvider, "Failed to initialize NetworkManager for network: [{0}]: {1}",
                        item->NetworkDefinition->NetworkName,
                        error.ErrorCodeValueToString());

                    return error;
                }

                //--------
                // Handle IP address pool data.
                for (auto ipPool : ipAddressPools)
                {
                    if (StringUtility::AreEqualCaseInsensitive(ipPool->NetworkId, item->NetworkDefinition->NetworkName))
                    {
                        networkMgr->SetIPv4AddressPool(ipPool);
                    }
                }

                //--------
                // Handle node -> allocation data.
                for (auto nodeMapAlloc : networkNodeAllocations)
                {
                    if (StringUtility::AreEqualCaseInsensitive(nodeMapAlloc->NetworkId, item->NetworkDefinition->NetworkName))
                    {
                        networkMgr->SetNodeAllocMap(nodeMapAlloc);
                    }
                }

                this->networkMgrMap_.emplace(item->Id, networkMgr);
            }
        }

        if (!error.IsSuccess())
        {
            WriteError(NIMAllocationManagerProvider, "Failed to LoadFromStore: error: {0}",
                error.ErrorCodeValueToString());
        }
    }

    this->CleanUpNetworks();

    return error;
}


ErrorCode NetworkInventoryAllocationManager::CleanUpNetworks()
{
    ErrorCode error(ErrorCodeValue::Success);
    std::vector<NodeInfoSPtr> nodeInfos;
    this->service_.FM.NodeCacheObj.GetNodes(nodeInfos);
    AcquireExclusiveLock lock(managerLock_);

    for (const auto & i : this->networkMgrMap_)
    {
        auto networkMgr = i.second;

        int index = 0;
        {
            auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
            auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

            error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
            if (error.IsSuccess())
            {
                auto nodeInfo = nodeInfos[index];

                //--------
                // Clean up all the stale allocations based on node instanceId changing after crash/reboot.
                auto a = lockedNetworkMgr->Get()->BeginReleaseAllIpsForNode(nodeInfo,
                    false,
                    FederationConfig::GetConfig().MessageTimeout,
                    [this, lockedNetworkMgr, nodeInfos, index](AsyncOperationSPtr const & contextSPtr)
                {
                    this->ReleaseAllIpsForNodeCallback(lockedNetworkMgr, nodeInfos, index, contextSPtr);
                });
            }
        }
    }

    return error;
}


ErrorCode NetworkInventoryAllocationManager::ReleaseAllIpsForNodeCallback(
    const LockedNetworkManagerSPtr & lockedNetworkMgr,
    const std::vector<NodeInfoSPtr> & nodeInfos,
    int index,
    AsyncOperationSPtr const & contextSPtr)
{
    std::shared_ptr<InternalError> reply;
    auto error = lockedNetworkMgr->Get()->EndReleaseAllIpsForNode(contextSPtr, reply);

    if (error.IsSuccess())
    {
        if (reply->UpdateNeeded)
        {
            //--------
            // Enqueues sending the mapping table to the hosts.
            this->EnqueuePublishNetworkMappings(lockedNetworkMgr);
        }
    }
    else
    {
        WriteError(NIMAllocationManagerProvider, "ReleaseAllIpsForNodeCallback: Failed for release ips: [{0}] for node: [{1}], error: {2}",
            lockedNetworkMgr->Get()->NetworkDefinition->NetworkId,
            nodeInfos[index]->NodeName,
            error.ErrorCodeValueToString());
    }

    //--------
    // Next iteration.
    if (++index < nodeInfos.size())
    {
        auto nodeInfo = nodeInfos[index];

        auto a = lockedNetworkMgr->Get()->BeginReleaseAllIpsForNode(nodeInfo,
            false,
            FederationConfig::GetConfig().MessageTimeout,
            [this, lockedNetworkMgr, nodeInfos, index](AsyncOperationSPtr const & contextSPtr)
        {
            this->ReleaseAllIpsForNodeCallback(lockedNetworkMgr, nodeInfos, index, contextSPtr);
        });
    }

    return error;
}


ErrorCode NetworkInventoryAllocationManager::NetworkCreateAsync(
    NetworkCreateRequestMessage & request,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error(ErrorCodeValue::Success);

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        NetworkDefinitionSPtr network = request.NetworkDefinition;

        auto netIter = this->networkMgrMap_.find(request.NetworkName);
        if (netIter != this->networkMgrMap_.end())
        {
            WriteError(NIMAllocationManagerProvider, "NetworkCreateAsync Failed: Network name already defined: [{0}]",
                request.NetworkName);

            return ErrorCode(ErrorCodeValue::NameAlreadyExists);
        }

        std::vector<uint> vsidList;
        error = this->macPool_->VSIDPool->ReserveElements(1, vsidList);
        if(this->macPool_->PersistenceState != PersistenceState::ToBeInserted)
        {
            this->macPool_->PersistenceState = PersistenceState::ToBeUpdated;
        }

        if (!error.IsSuccess())
        {
            WriteError(NIMAllocationManagerProvider, "NetworkCreateAsync: Failed for getting VSID from pool, error: {0}",
                error.ErrorCodeValueToString());
            return error;
        }

        network->OverlayId = vsidList[0];

        WriteInfo(NIMAllocationManagerProvider, "NetworkCreateAsync: going to create isolated network: [{0}], subnet: [{1}], vsid: [{2}]",
            request.NetworkName, network->Subnet, network->OverlayId);

        auto networkDefinitionStoreData = make_shared<NIMNetworkDefinitionStoreData>(network);
        networkDefinitionStoreData->PersistenceState = PersistenceState::ToBeInserted;

        networkMgr = make_shared<NetworkManager>(service_);
        error = networkMgr->Initialize(networkDefinitionStoreData, macPool_);
        if (!error.IsSuccess())
        {
            WriteError(NIMAllocationManagerProvider, "NetworkCreateAsync: Failed to initialize NetworkManager for network: [{0}]",
                network->NetworkName);

            this->macPool_->VSIDPool->ReturnElements(vsidList);
            return error;
        }

        error = networkMgr->PersistData();
        if (!error.IsSuccess())
        {
            WriteError(NIMAllocationManagerProvider, "NetworkCreateAsync: Failed to PersistData for network: [{0}]: {1}",
                network->NetworkName,
                error.ErrorCodeValueToString());

            this->macPool_->VSIDPool->ReturnElements(vsidList);
            return error;
        }

        this->networkMgrMap_.emplace(network->NetworkName, networkMgr);

        //--------
        // Set the reply.
        auto p = (InvokeActionAsyncOperation<NetworkCreateRequestMessage, NetworkCreateResponseMessage>*)(operation.get());
        p->Reply->NetworkDefinition = networkMgr->NetworkDefinition->Clone();

        operation->TryComplete(operation, error);
    }

    return error;
}


ErrorCode NetworkInventoryAllocationManager::NetworkRemoveAsync(
    NetworkRemoveRequestMessage & request,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error(ErrorCodeValue::Success);

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(request.NetworkId);
        if (netIter == this->networkMgrMap_.end())
        {
            WriteError(NIMAllocationManagerProvider, "NetworkRemoveAsync Failed: Network not found: [{0}]",
                request.NetworkId);

            return ErrorCode(ErrorCodeValue::NetworkNotFound);
        }

        networkMgr = netIter->second;
        auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
        auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

        error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
        if (error.IsSuccess())
        {
            //--------
            // Get the allocation from the network pools.
            auto a = lockedNetworkMgr->Get()->BeginNetworkRemove(request,
                FederationConfig::GetConfig().MessageTimeout,
                [this, lockedNetworkMgr, operation](AsyncOperationSPtr const & contextSPtr)
            {
                std::shared_ptr<NetworkErrorCodeResponseMessage> reply;
                auto error = lockedNetworkMgr->Get()->EndNetworkRemove(contextSPtr, reply);

                if (error.IsSuccess())
                {
                    //--------
                    // Remove the network from runtime.
                    this->networkMgrMap_.erase(lockedNetworkMgr->Get()->NetworkDefinition->NetworkName);
                }
                else
                {
                    WriteWarning(NIMAllocationManagerProvider, "NetworkRemoveAsync Failed with {0}: => not removing runtime data [{1}]",
                        error.ErrorCodeValueToString(),
                        lockedNetworkMgr->Get()->NetworkDefinition->NetworkName);
                }

                operation->TryComplete(operation, error);
            });
        }
    }

    return error;
}

ErrorCode NetworkInventoryAllocationManager::NetworkNodeRemoveAsync(
    NetworkRemoveRequestMessage & request,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error(ErrorCodeValue::Success);

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(request.NetworkId);
        if (netIter == this->networkMgrMap_.end())
        {
            WriteError(NIMAllocationManagerProvider, "NetworkNodeRemoveAsync Failed: Network not found: [{0}]",
                request.NetworkId);

            return ErrorCode(ErrorCodeValue::NetworkNotFound);
        }

        networkMgr = netIter->second;
        auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
        auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

        error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
        if (error.IsSuccess())
        {
            //--------
            // Get the allocation from the network pools.
            auto a = lockedNetworkMgr->Get()->BeginNetworkNodeRemove(request,
                FederationConfig::GetConfig().MessageTimeout,
                [this, lockedNetworkMgr, operation](AsyncOperationSPtr const & contextSPtr)
            {
                std::shared_ptr<NetworkErrorCodeResponseMessage> reply;
                auto error = lockedNetworkMgr->Get()->EndNetworkNodeRemove(contextSPtr, reply);

                if (!error.IsSuccess())
                {
                    WriteWarning(NIMAllocationManagerProvider, "NetworkNodeRemoveAsync Failed with {0}: => not removing runtime data [{1}]",
                        error.ErrorCodeValueToString(),
                        lockedNetworkMgr->Get()->NetworkDefinition->NetworkName);
                }

                operation->TryComplete(operation, error);
            });
        }
    }

    return error;
}

std::vector<ModelV2::NetworkResourceDescriptionQueryResult> NetworkInventoryAllocationManager::NetworkEnumerate(
    const NetworkQueryFilter & filter)
{
    std::vector<ModelV2::NetworkResourceDescriptionQueryResult> networkList;

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        for (const auto & i : this->networkMgrMap_)
        {
            auto net = i.second->NetworkDefinition;
            ModelV2::NetworkResourceDescriptionQueryResult netInfo(net->NetworkName,
                FABRIC_NETWORK_TYPE::FABRIC_NETWORK_TYPE_LOCAL,
                wformatString("{0}/{1}", net->Subnet, net->Prefix),
                FABRIC_NETWORK_STATUS::FABRIC_NETWORK_STATUS_READY);

            if (filter(netInfo))
            {
                networkList.push_back(move(netInfo));
            }
        }
    }

    return move(networkList);
}


std::vector<ServiceModel::NetworkApplicationQueryResult> NetworkInventoryAllocationManager::GetNetworkApplicationList(
    const std::wstring & networkName)
{
    UNREFERENCED_PARAMETER(networkName);
    std::vector<ServiceModel::NetworkApplicationQueryResult> applicationList;

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(networkName);
        if (netIter != this->networkMgrMap_.end())
        {
            networkMgr = netIter->second;

            for (const auto & applicationName : networkMgr->GetAssociatedApplications())
            {
                ServiceModel::NetworkApplicationQueryResult networkApplicationQR(applicationName);
                applicationList.push_back(move(networkApplicationQR));
            }
        }
    }

    return move(applicationList);
}

std::vector<ServiceModel::ApplicationNetworkQueryResult> NetworkInventoryAllocationManager::GetApplicationNetworkList(
    const std::wstring & applicationName)
{
    std::vector<ServiceModel::ApplicationNetworkQueryResult> networkList;

    AcquireExclusiveLock lock(this->managerLock_);
    for ( auto netIter = this->networkMgrMap_.begin(); netIter != this->networkMgrMap_.end(); netIter++ )
    {
        auto applications = netIter->second->GetAssociatedApplications();
        if (find(applications.begin(), applications.end(), applicationName) != applications.end())
        {
            ServiceModel::ApplicationNetworkQueryResult applicationNetworkQR(netIter->first);
            networkList.push_back(move(applicationNetworkQR));
        }
    }

    return move(networkList);
}

std::vector<ServiceModel::NetworkNodeQueryResult> NetworkInventoryAllocationManager::GetNetworkNodeList(
    const std::wstring & networkName)
{
    std::vector<ServiceModel::NetworkNodeQueryResult> nodeList;

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(networkName);
        if (netIter != this->networkMgrMap_.end())
        {
            networkMgr = netIter->second;

            for (const auto & nodeName : networkMgr->GetAssociatedNodes())
            {
                ServiceModel::NetworkNodeQueryResult nodeQR(nodeName);
                nodeList.push_back(move(nodeQR));
            }
        }
    }

    return move(nodeList);
}

std::vector<ServiceModel::DeployedNetworkQueryResult> NetworkInventoryAllocationManager::GetDeployedNetworkList()
{
    std::vector<ServiceModel::DeployedNetworkQueryResult> deployedNetworkList;

    return move(deployedNetworkList);
}


NetworkDefinitionSPtr NetworkInventoryAllocationManager::GetNetworkByName(const std::wstring & networkName)
{
    NetworkDefinitionSPtr network;

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(networkName);
        if (netIter != this->networkMgrMap_.end())
        {
            network = netIter->second->NetworkDefinition->Clone();
        }
    }

    return network;
}

ErrorCode NetworkInventoryAllocationManager::NetworkEnumerateAsync(
    NetworkEnumerateRequestMessage & request,
    Common::AsyncOperationSPtr const & operation)
{
    UNREFERENCED_PARAMETER(request);

    ErrorCode error(ErrorCodeValue::Success);
    std::vector<NetworkDefinitionSPtr> networkList;

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        for (const auto & i : this->networkMgrMap_)
        {
            auto net = i.second->NetworkDefinition->Clone();
            networkList.push_back(net);
        }

        auto reply = make_shared<NetworkEnumerateResponseMessage>();
        reply->NetworkDefinitionList = networkList;

        auto p = (InvokeActionAsyncOperation<NetworkEnumerateRequestMessage, NetworkEnumerateResponseMessage>*)(operation.get());
        p->Reply = reply;
    }

    operation->TryComplete(operation, error);

    return error;
}

ErrorCode NetworkInventoryAllocationManager::AcquireEntryBlockAsync(
    NetworkAllocationRequestMessage & allocationRequest,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error(ErrorCodeValue::Success);

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(allocationRequest.NetworkName);
        if (netIter != this->networkMgrMap_.end())
        {
            WriteInfo(NIMAllocationManagerProvider, "AcquireEntryBlockAsync: found network: [{0}]",
                allocationRequest.NetworkName); 
            networkMgr = netIter->second;
            
        }
        else
        {
            WriteError(NIMAllocationManagerProvider, "AcquireEntryBlockAsync: Failed to find network: [{0}]",
                allocationRequest.NetworkName);

            return ErrorCode(ErrorCodeValue::NetworkNotFound);
        }

        auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
        auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

        error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
        if (error.IsSuccess())
        {
            //--------
            // Get the allocation from the network pools.
            auto a = lockedNetworkMgr->Get()->BeginAcquireEntryBlock(allocationRequest,
               FederationConfig::GetConfig().MessageTimeout,
                [this, lockedNetworkMgr, operation](AsyncOperationSPtr const & contextSPtr)
            {
                std::shared_ptr<NetworkAllocationResponseMessage> reply;
                auto error = lockedNetworkMgr->Get()->EndAcquireEntryBlock(contextSPtr, reply);

                auto p = (InvokeActionAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage>*)(operation.get());
                p->Reply = reply;

                if (error.IsSuccess())
                {
                    //--------
                    // Enqueues sending the mapping table to the hosts.
                    this->EnqueuePublishNetworkMappings(lockedNetworkMgr);
                }
                else
                {
                    WriteError(NIMAllocationManagerProvider, "AcquireEntryBlock: failed to get addresses: error: {0}",
                        error.ErrorCodeValueToString());
                }

                operation->TryComplete(operation, error);
            });
        }
    }

    return error;
}

//--------
// Enqueue a job to send the mapping table.
Common::ErrorCode NetworkInventoryAllocationManager::EnqueuePublishNetworkMappings(const std::wstring& networkName)
{
    ErrorCode error(ErrorCodeValue::Success);

    NetworkManagerSPtr networkMgr;
    {
        AcquireExclusiveLock lock(this->managerLock_);

        auto netIter = this->networkMgrMap_.find(networkName);
        if (netIter == this->networkMgrMap_.end())
        {
            WriteError(NIMAllocationManagerProvider, "EnqueuePublishNetworkMappings Failed: Network not found: [{0}]",
                networkName);
            return ErrorCode(ErrorCodeValue::NetworkNotFound);
        }

        networkMgr = netIter->second;
        auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
        auto lockedNetworkMgr = make_shared<LockedNetworkManager>();
        error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);

        if (error.IsSuccess())
        {
            EnqueuePublishNetworkMappings(lockedNetworkMgr);
        }
        else
        {
            WriteError(NIMAllocationManagerProvider, "EnqueuePublishNetworkMappings: Failed to lock network: [{0}]",
                error.ErrorCodeValueToString());
        }
    }

    return error;
}

void NetworkInventoryAllocationManager::EnqueuePublishNetworkMappings(
    const LockedNetworkManagerSPtr & lockedNetworkMgr)
{
    //--------
    // Get current mappings.
    std::vector<Federation::NodeInstance> nodeInstances;
    std::vector<NetworkAllocationEntrySPtr> networkMappingTable;
    ErrorCode error = lockedNetworkMgr->Get()->GetNetworkMappingTable(nodeInstances, networkMappingTable);

    if (error.IsSuccess())
    {
        std::wstring networkName(lockedNetworkMgr->Get()->NetworkDefinition->NetworkName);
        PublishNetworkTablesRequestMessage publishRequest(networkName, 
            nodeInstances, 
            networkMappingTable,
            false,
            lockedNetworkMgr->Get()->NetworkDefinition->InstanceID,
            lockedNetworkMgr->Get()->GetSequenceNumber());

        //--------
        // Enqueues sending the mapping table to the hosts.
        TimeSpan timeout = TimeSpan::FromSeconds(60);

        for (const auto & nodeInstance : publishRequest.NodeInstances)
        {
            PublishNetworkMappingJobItem jobItem(
                this,
                nodeInstance,
                publishRequest,
                timeout);

            publishNetworkMappingJobQueue_->Enqueue(move(jobItem));
        }
    }
}


Common::AsyncOperationSPtr NetworkInventoryAllocationManager::BeginNetworkCreate(
    NetworkCreateRequestMessage & allocationRequest,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkCreateRequestMessage, NetworkCreateResponseMessage> >(
        *this, allocationRequest, timeout,
        [this](NetworkCreateRequestMessage & request, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
    {
        return this->NetworkCreateAsync(request, thisSPtr);
    },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAllocationManager::EndNetworkCreate(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkCreateResponseMessage> & reply)
{
    return NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkCreateRequestMessage, NetworkCreateResponseMessage>::End(
        operation,
        reply);
}


Common::AsyncOperationSPtr NetworkInventoryAllocationManager::BeginNetworkRemove(
    NetworkRemoveRequestMessage & allocationRequest,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, allocationRequest, timeout,
        [this](NetworkRemoveRequestMessage & request, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
    {
        return this->NetworkRemoveAsync(request, thisSPtr);
    },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAllocationManager::EndNetworkRemove(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply)
{
    return NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}

Common::AsyncOperationSPtr NetworkInventoryAllocationManager::BeginNetworkNodeRemove(
    NetworkRemoveRequestMessage & allocationRequest,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, allocationRequest, timeout,
        [this](NetworkRemoveRequestMessage & request, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
    {
        return this->NetworkNodeRemoveAsync(request, thisSPtr);
    },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAllocationManager::EndNetworkNodeRemove(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply)
{
    return NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}

Common::AsyncOperationSPtr NetworkInventoryAllocationManager::BeginNetworkEnumerate(
    NetworkEnumerateRequestMessage & allocationRequest,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkEnumerateRequestMessage, NetworkEnumerateResponseMessage> >(
        *this, allocationRequest, timeout,
        [this](NetworkEnumerateRequestMessage & request, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
    {
        return this->NetworkEnumerateAsync(request, thisSPtr);
    },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAllocationManager::EndNetworkEnumerate(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkEnumerateResponseMessage> & reply)
{
    return NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkEnumerateRequestMessage, NetworkEnumerateResponseMessage>::End(
        operation,
        reply);
}

//--------
// Start the BeginAssociateApplication invoke async operation.
Common::AsyncOperationSPtr NetworkInventoryAllocationManager::BeginAssociateApplication(
    std::wstring const & networkName,
    std::wstring const & applicationName,
    bool isAssociation,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<AssociateApplicationAsyncOperation>(
        *this,
        networkName,
        applicationName,
        isAssociation,
        callback,
        parent);
}

//--------
// Complete the invoke request and get the reply.
Common::ErrorCode NetworkInventoryAllocationManager::EndAssociateApplication(Common::AsyncOperationSPtr const & operation)
{
    return AssociateApplicationAsyncOperation::End(operation);
}


Common::AsyncOperationSPtr NetworkInventoryAllocationManager::BeginAcquireEntryBlock(
    NetworkAllocationRequestMessage & allocationRequest,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage> >(
        *this, allocationRequest, timeout, 
        [this](NetworkAllocationRequestMessage & request, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
        {
            return this->AcquireEntryBlockAsync(request, thisSPtr);
        },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkInventoryAllocationManager::EndAcquireEntryBlock(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkAllocationResponseMessage> & reply)
{
    return NetworkInventoryAllocationManager::InvokeActionAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage>::End(
        operation,
        reply);
}
