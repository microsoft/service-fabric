// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------// Complete invoke request and get the reply.

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Management::NetworkInventoryManager;
using namespace ServiceModel;
using namespace Transport;

StringLiteral const NIMNetworkManagerProvider("NIMNetworkManager");


template<class T, class TReply>
using InvokeMethod = std::function<Common::ErrorCode(T &, std::shared_ptr<TReply> & , AsyncOperationSPtr const &)>;

template <class TRequest, class TReply>
class NetworkManager::InvokeActionAsyncOperation : public AsyncOperation
{
public:

    InvokeActionAsyncOperation(
        __in NetworkManager & owner,
        TRequest & request,
        TimeSpan const timeout,
        InvokeMethod<TRequest, TReply> const & invokeMethod,
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
        return thisPtr->Error;
    }

    _declspec(property(get = get_Reply, put = set_Reply)) std::shared_ptr<TReply> Reply;
    std::shared_ptr<TReply> get_Reply() const { return this->reply_; };
    void set_Reply(std::shared_ptr<TReply> & value) { this->reply_ = value; }

protected:

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->reply_ = make_shared<TReply>();
        auto error = this->invokeMethod_(request_, this->reply_, thisSPtr);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
    }

private:
    const InvokeMethod<TRequest, TReply> invokeMethod_;
    TimeoutHelper timeoutHelper_;
    NetworkManager & owner_;

    TRequest & request_;
    std::shared_ptr<TReply> reply_;
};

NetworkManager::NetworkManager(NetworkInventoryService & service) :
    service_(service)
{
    WriteInfo(NIMNetworkManagerProvider, "NetworkManager: constructing...");
}

Common::ErrorCode NetworkManager::Initialize(NIMNetworkDefinitionStoreDataSPtr network,
    NIMNetworkMACAddressPoolStoreDataSPtr macPool,
    bool isLoading)
{
    ErrorCode error(ErrorCodeValue::Success);

    this-> network_ = network;
    this->macPool_ = macPool;

    if (!isLoading)
    {
        //--------
        // Initialize the IPv4 pool.
        auto ipPool = make_shared<NIMIPv4AllocationPool>();
        ipPool->Initialize(network->NetworkDefinition->Subnet, network->NetworkDefinition->Prefix, 4);

        this->ipPool_ = make_shared<NIMNetworkIPv4AddressPoolStoreData>(ipPool, network->NetworkDefinition->NetworkId, network->NetworkDefinition->Subnet);
        this->ipPool_->PersistenceState = PersistenceState::ToBeInserted;
    }

    return error;
}

int64 NetworkManager::GetSequenceNumber() const
{
    int64 sequenceNumber = 0;
    if(this->network_)
    {
        sequenceNumber = this->network_->SequenceNumber;
    }

    return sequenceNumber;
}

Common::StringCollection NetworkManager::GetAssociatedApplications()
{
    Common::StringCollection applications;
    if (network_)
    {
        applications = network_->AssociatedApplications;
    }
    return applications;
}

void NetworkManager::AssociateApplication(
    std::wstring const & applicationName,
    bool isAssociation)
{
    auto pos = std::find(network_->AssociatedApplications.begin(), 
        network_->AssociatedApplications.end(),
        applicationName);

    if (pos != network_->AssociatedApplications.end())
    {
        if(!isAssociation)
        {
            network_->AssociatedApplications.erase(pos);
        }
    }
    else
    {
        if(isAssociation)
        {
            this->network_->AssociatedApplications.push_back(applicationName);
        }
    }
}

Common::StringCollection NetworkManager::GetAssociatedNodes()
{
    Common::StringCollection nodeNames;
    if (network_)
    {
        AcquireExclusiveLock lock(this->managerLock_);

        for(auto node : this->nodeAllocationMap_)
        {
            nodeNames.push_back(node.second->NodeName);
        }
    }
    return nodeNames;
}


Federation::NodeInstance NetworkManager::GetNodeInstance(const std::wstring & nodeName)
{
    Federation::NodeInstance nodeInstance;
    std::vector<NodeInfoSPtr> nodes;
    this->service_.FM.NodeCacheObj.GetNodes(nodes);

    for (const auto & i : nodes)
    {
        if (StringUtility::Compare(nodeName, i->NodeName) == 0)
        {
            nodeInstance = i->NodeInstance;
            break;
        }
    }

    return nodeInstance;
}

NIMNetworkNodeAllocationStoreDataSPtr NetworkManager::GetOrCreateNodeAllocationMap(
    const std::wstring & networkId,
    const std::wstring & nodeName)
{
    NIMNetworkNodeAllocationStoreDataSPtr nodeEntry;
    {
        auto nodeInstance = this->GetNodeInstance(nodeName);
        std::wstring nodeKey = wformatString("{0}-{1}-{2}", networkId, nodeName, nodeInstance.InstanceId);

        AcquireExclusiveLock lock(this->managerLock_);

        auto nodeIter = this->nodeAllocationMap_.find(nodeKey);
        if (nodeIter != this->nodeAllocationMap_.end())
        {
            nodeEntry = nodeIter->second;
            nodeEntry->PersistenceState = PersistenceState::ToBeUpdated;
        }
        else
        {
            auto innerNodeEntry = make_shared<std::map<std::wstring, NetworkAllocationEntrySPtr>>();
            nodeEntry = make_shared<NIMNetworkNodeAllocationStoreData>(innerNodeEntry, networkId, nodeName, nodeInstance);
            nodeEntry->PersistenceState = PersistenceState::ToBeInserted;

            this->nodeAllocationMap_.emplace(nodeKey, nodeEntry);
        }
    }

    return nodeEntry;
}

void NetworkManager::SetNodeAllocMap(NIMNetworkNodeAllocationStoreDataSPtr & nodeAllocMap)
{
    AcquireExclusiveLock lock(this->managerLock_);

    std::wstring nodeKey = wformatString("{0}-{1}-{2}", nodeAllocMap->NetworkId, nodeAllocMap->NodeName, nodeAllocMap->NodeInstance.InstanceId);
    auto nodeIter = this->nodeAllocationMap_.find(nodeKey);
    ASSERT_IFNOT(nodeIter == this->nodeAllocationMap_.end(), "NetworkManager: SetNodeAllocMap: node {0} is duplicated!", nodeKey);
    
    this->nodeAllocationMap_.emplace(nodeKey, nodeAllocMap);
}

void NetworkManager::SetIPv4AddressPool(NIMNetworkIPv4AddressPoolStoreDataSPtr & ipPool)
{
    AcquireExclusiveLock lock(this->managerLock_);
    this->ipPool_ = ipPool;
}



//--------
// Adquires an IP block from a particular node on this network.
ErrorCode NetworkManager::AcquireIPBlockAsync(
    const Management::NetworkInventoryManager::NetworkAllocationRequestMessage & allocationRequest,
    __out std::shared_ptr<NetworkAllocationResponseMessage> & allocationResponse,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error(ErrorCodeValue::Success);
    int batchLen = NetworkInventoryManagerConfig::GetConfig().NIMAllocationRequestBatchSize;

    auto networkMgr = shared_from_this();
    auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
    auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

    error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "AcquireIPBlockAsync: failed to get lock for network, Failed : [{0}]",
            error.ErrorCodeValueToString());
        return error;
    }

    //--------
    // Get IP addresses.
    std::vector<uint> ipList;
    error = this->ipPool_->IpPool->ReserveElements(batchLen, ipList);
    this->ipPool_->PersistenceState = PersistenceState::ToBeUpdated;

    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "AcquireIPBlock: Failed to reserve IP addresses from pool: [{0}]",
            error.ErrorCodeValueToString());

        return error;
    }

    //--------
    // Get MAC addresses.
    std::vector<uint64> macList;
    error = this->macPool_->MacPool->ReserveElements(batchLen, macList);

    if(this->macPool_->PersistenceState != PersistenceState::ToBeInserted)
    {
        this->macPool_->PersistenceState = PersistenceState::ToBeUpdated;
    }

    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "AcquireIPBlock: Failed to reserve MAC addresses from pool: [{0}]",
            error.ErrorCodeValueToString());

        this->ipPool_->IpPool->ReturnElements(ipList);
        return error;
    }

    NIMNetworkNodeAllocationStoreDataSPtr nodeEntry = GetOrCreateNodeAllocationMap(allocationRequest.NetworkName, 
        allocationRequest.NodeName); 

    auto network = this->network_->NetworkDefinition;
    allocationResponse->NetworkId = network->NetworkId;
    allocationResponse->NetworkName = network->NetworkName;
    allocationResponse->StartMacPoolAddress = this->macPool_->MacPool->GetBottomMAC();
    allocationResponse->EndMacPoolAddress = this->macPool_->MacPool->GetTopMAC();
    allocationResponse->Subnet = network->Subnet;
    allocationResponse->Prefix = network->Prefix;
    allocationResponse->OverlayId = network->OverlayId;
    allocationResponse->NodeIpAddress = allocationRequest.InfraIpAddress;
    allocationResponse->Gateway = network->Gateway;

    {
        AcquireExclusiveLock lock(this->managerLock_);
        auto ipIter = ipList.begin();
        auto macIter = macList.begin();

        for (int i = 0; i < batchLen; i++, ipIter++, macIter++)
        {
            NetworkAllocationEntrySPtr entry = make_shared<NetworkAllocationEntry>();
            entry->IpAddress = this->ipPool_->IpPool->GetIpString(*ipIter);
            entry->MacAddress = this->macPool_->MacPool->GetMacString(*macIter);
            entry->InfraIpAddress = allocationRequest.InfraIpAddress;
            entry->NodeName = allocationRequest.NodeName;

            //--------
            // Add it to my structures.
            nodeEntry->NodeAllocationMap->emplace(entry->IpAddress, entry);

            //--------
            // Return it to the caller as well.
            allocationResponse->IpMacAddressMapToBeAdded.emplace(entry->IpAddress, entry->MacAddress);
        }

        //--------
        // Persist the modified objects.
        std::vector<NIMNetworkNodeAllocationStoreDataSPtr> nodeEntries;
        nodeEntries.emplace_back(nodeEntry);

        auto a = this->BeginUpdatePersistentData(nodeEntries,
            FederationConfig::GetConfig().MessageTimeout,
            [this, &nodeEntry, allocationResponse, lockedNetworkMgr, operation](AsyncOperationSPtr const & contextSPtr)
        {
            std::shared_ptr<ErrorCode> reply;
            auto error = this->EndUpdatePersistentData(contextSPtr, reply);

            if (!error.IsSuccess())
            {
                WriteError(NIMNetworkManagerProvider, "AcquireIPBlock: Failed to persist data, deallocating: [{0}]",
                    error.ErrorCodeValueToString());

                ReleaseEntryAllocations(nodeEntry, allocationResponse->IpMacAddressMapToBeAdded);
            }

            operation->TryComplete(operation, error);
        });
    }

    return error;
}

void NetworkManager::ReleaseEntryAllocations(
    NIMNetworkNodeAllocationStoreDataSPtr & nodeEntry,
    const std::map<std::wstring, std::wstring> & ipMacMap)
{
    std::vector<uint64> macListToReturn;
    std::vector<uint> ipListToReturn;

    for (const auto & i : ipMacMap)
    {
        uint64 macScalar;
        this->macPool_->MacPool->GetScalarFromMacAddress(i.second, macScalar);
        macListToReturn.emplace_back(macScalar);

        uint ipScalar;
        this->ipPool_->IpPool->GetScalarFromIpAddress(i.first, ipScalar);
        ipListToReturn.emplace_back(ipScalar);

        nodeEntry->NodeAllocationMap->erase(i.first);
    }

    this->ipPool_->IpPool->ReturnElements(ipListToReturn);
    this->macPool_->MacPool->ReturnElements(macListToReturn);

    if (nodeEntry->PersistenceState == PersistenceState::ToBeInserted)
    {
        auto nodeKey = nodeEntry->GetStoreKey();
        this->nodeAllocationMap_.erase(nodeKey);
    }
}



ErrorCode NetworkManager::PersistData()
{
    //--------
    // Persist the modified objects.
    auto networkMgr = shared_from_this();
    auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
    auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

    ErrorCode error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "PersistData: failed to get lock for network, Failed : [{0}]",
            error.ErrorCodeValueToString());
        return error;
    }

    auto completed = make_shared<ManualResetEvent>();
    std::vector<NIMNetworkNodeAllocationStoreDataSPtr> nodeEntries;

    auto a = this->BeginUpdatePersistentData(nodeEntries,
        FederationConfig::GetConfig().MessageTimeout,
        [this, completed](AsyncOperationSPtr const & contextSPtr)
    {
        std::shared_ptr<ErrorCode> reply;
        auto error = this->EndUpdatePersistentData(contextSPtr, reply);

        completed->Set();
    });

    completed->WaitOne();

    return a->Error;
}

//--------
// Remove the network and its allocations.
ErrorCode NetworkManager::NetworkRemoveAsync(
    const Management::NetworkInventoryManager::NetworkRemoveRequestMessage & request,
    __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);

    UNREFERENCED_PARAMETER(request);
    UNREFERENCED_PARAMETER(reply);

    NodeInfoSPtr nodeInfo;

    WriteInfo(NIMNetworkManagerProvider, "NetworkRemoveAsync: trying to remove network: [{0}], subnet: [{1}], vsid: [{2}]",
        this->NetworkDefinition->NetworkName, this->NetworkDefinition->Subnet, this->NetworkDefinition->OverlayId);

    //--------
    // Fail if there are still apps referencing the network.
    if(this->network_->AssociatedApplications.size() != 0)
    {
        std::wstring appList;
        for(auto appName : this->network_->AssociatedApplications) {  appList = wformatString("{0}, [{1}]", appList, appName); }

        WriteInfo(NIMNetworkManagerProvider, "NetworkRemoveAsync: network is still referenced by apps : [{0}]",
            appList);

        error = ErrorCode(ErrorCodeValue::NetworkInUse);
        return error;
    }

    //--------
    // Fail if there are still nodes using the network.
    if(this->nodeAllocationMap_.size() != 0)
    {
        std::wstring nodeList;

        for(auto node : this->nodeAllocationMap_) 
        {
            nodeList = wformatString("{0}, [{1}]", nodeList, node.first);
        }

        WriteInfo(NIMNetworkManagerProvider, "NetworkRemoveAsync: network is still referenced by nodes : [{0}]",
            nodeList);
            
        error = ErrorCode(ErrorCodeValue::NetworkInUse);
        return error;
    }

    //--------
    // Release the VSID.
    std::vector<uint> vsids;
    vsids.push_back(this->NetworkDefinition->OverlayId);
    this->macPool_->VSIDPool->ReturnElements(vsids);
    this->macPool_->PersistenceState = PersistenceState::ToBeUpdated;

    this->network_->PersistenceState = PersistenceState::ToBeDeleted;
    this->ipPool_->PersistenceState = PersistenceState::ToBeDeleted;

    auto a = this->BeginReleaseAllIpsForNode(nodeInfo,
        true,
        FederationConfig::GetConfig().MessageTimeout,
        [this, operation, reply](AsyncOperationSPtr const & contextSPtr)
    {
        std::shared_ptr<InternalError> r;
        auto error = this->EndReleaseAllIpsForNode(contextSPtr, r);

        operation->TryComplete(operation, error);
    });

    return error;
}

//--------
// Remove the node from the network and its allocations.
ErrorCode NetworkManager::NetworkNodeRemoveAsync(
    const Management::NetworkInventoryManager::NetworkRemoveRequestMessage & request,
    __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);

    NodeId nodeId;
    if (!NodeId::TryParse(request.NodeId, nodeId))
    {
        error = ErrorCode(ErrorCodeValue::InvalidArgument);
        return error;
    }

    auto nodeInfo = this->service_.FM.NodeCacheObj.GetNode(nodeId);

    this->macPool_->PersistenceState = PersistenceState::ToBeUpdated;
    this->ipPool_->PersistenceState = PersistenceState::ToBeDeleted;

    auto a = this->BeginReleaseAllIpsForNode(nodeInfo,
        true,
        FederationConfig::GetConfig().MessageTimeout,
        [this, operation, reply](AsyncOperationSPtr const & contextSPtr)
    {
        std::shared_ptr<InternalError> r;
        auto error = this->EndReleaseAllIpsForNode(contextSPtr, r);

        operation->TryComplete(operation, error);
    });

    return error;
}


ErrorCode NetworkManager::ReleaseAllIpsForNodeAsync(
    NodeInfoSPtr & nodeInfo,
    bool releaseAllInstances,
    __out std::shared_ptr<InternalError> & response,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);

    bool needToUpdate = false;
    {
        auto networkMgr = shared_from_this();
        auto networkMgrCache = make_shared<FailoverManagerComponent::CacheEntry<NetworkManager>>(move(networkMgr));
        auto lockedNetworkMgr = make_shared<LockedNetworkManager>();

        error = networkMgrCache->Lock(networkMgrCache, *lockedNetworkMgr);
        if (!error.IsSuccess())
        {
            WriteError(NIMNetworkManagerProvider, "ReleaseAllIpsForNodeAsync: failed to get lock for network, Failed : [{0}]",
                error.ErrorCodeValueToString());
            return error;
        }

        std::vector<NIMNetworkNodeAllocationStoreDataSPtr> nodeEntries;
        std::vector<uint64> macListToReturn;
        std::vector<uint> ipListToReturn;
        std::vector<std::wstring> elementsToDelete;

        for (const auto & i : this->nodeAllocationMap_)
        {
            auto nodeMap = i.second;
            nodeEntries.emplace_back(nodeMap);

            if (nodeInfo == NULL ||
                nodeInfo->Id == nodeMap->NodeInstance.Id &&
                (releaseAllInstances || nodeInfo->NodeInstance.InstanceId != nodeMap->NodeInstance.InstanceId))
            {
                elementsToDelete.emplace_back(i.first);

                for (auto j = nodeMap->NodeAllocationMap->begin(); j != nodeMap->NodeAllocationMap->end(); j++)
                {
                    auto entry = j->second;

                    uint64 macScalar;
                    this->macPool_->MacPool->GetScalarFromMacAddress(entry->MacAddress, macScalar);
                    macListToReturn.emplace_back(macScalar);

                    uint ipScalar;
                    this->ipPool_->IpPool->GetScalarFromIpAddress(entry->IpAddress, ipScalar);
                    ipListToReturn.emplace_back(ipScalar);
                }

                nodeMap->PersistenceState = PersistenceState::ToBeDeleted;

                this->ipPool_->IpPool->ReturnElements(ipListToReturn);
                this->ipPool_->PersistenceState = PersistenceState::ToBeUpdated;

                this->macPool_->MacPool->ReturnElements(macListToReturn);
                this->macPool_->PersistenceState = PersistenceState::ToBeUpdated;

                needToUpdate = true;
            }
        }

        if (nodeInfo == NULL)
        {
            needToUpdate = true;
            this->ipPool_->PersistenceState = PersistenceState::ToBeDeleted;
            this->macPool_->PersistenceState = PersistenceState::ToBeUpdated;
        }

        if (needToUpdate)
        {
            //--------
            // Remove the elements
            for (const auto & entryName : elementsToDelete)
            {
                this->nodeAllocationMap_.erase(entryName);
            }

            //--------
            // Persist the modified objects.
            auto a = this->BeginUpdatePersistentData(nodeEntries,
                FederationConfig::GetConfig().MessageTimeout,
                [this, networkMgr, operation, response](AsyncOperationSPtr const & contextSPtr)
            {
                std::shared_ptr<ErrorCode> reply;
                auto error = this->EndUpdatePersistentData(contextSPtr, reply);

                bool needToUpdate = true;
                response->Error = error;
                response->UpdateNeeded = needToUpdate;
                operation->TryComplete(operation, error);
            });
        }
    }

    if (!needToUpdate)
    {
        response->Error = error;
        response->UpdateNeeded = needToUpdate;

        operation->TryComplete(operation, error);
    }

    if (!error.IsSuccess())
    {
    }

    return error;
}


ErrorCode NetworkManager::UpdatePersistentDataAsync(const std::vector<NIMNetworkNodeAllocationStoreDataSPtr> & nodeEntries,
    Common::AsyncOperationSPtr const & operation)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);

    Store::IStoreBase::TransactionSPtr transactionSPtr;
    error = this->service_.FM.Store.BeginTransaction(transactionSPtr);
    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "Store.BeginTransaction: Failed : [{0}]",
            error.ErrorCodeValueToString());
        return error;
    }

    this->network_->SequenceNumber = this->network_->SequenceNumber + 1;
    if (this->network_->PersistenceState != PersistenceState::ToBeDeleted &&
        this->network_->PersistenceState != PersistenceState::ToBeInserted)
    {
        this->network_->PersistenceState = PersistenceState::ToBeUpdated;
    }

    error = this->service_.FM.Store.UpdateData(transactionSPtr, *(this->network_.get()));
    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "Store.UpdateData: Failed for network definition: [{0}], error: {1}",
            this->network_->GetStoreKey(),
            error.ErrorCodeValueToString());

        transactionSPtr->Rollback();
        return error;
    }

    //--------
    // Persist the node -> allocation list.
    for (const auto & nodeEntry : nodeEntries)
    {
        if (nodeEntry->PersistenceState != PersistenceState::NoChange)
        {
            error = this->service_.FM.Store.UpdateData(transactionSPtr, *(nodeEntry.get()));
            if (!error.IsSuccess())
            {
                WriteError(NIMNetworkManagerProvider, "Store.UpdateData: Failed for nodeEntry: [{0}], error: {1}",
                    nodeEntry->GetStoreKey(),
                    error.ErrorCodeValueToString());

                transactionSPtr->Rollback();
                return error;
            }
        }
    }

    auto macPool = macPool_;
    auto macPoolCache = make_shared<FailoverManagerComponent::CacheEntry<NIMNetworkMACAddressPoolStoreData>>(move(macPool));
    auto lockedMacPool = make_shared<LockedNetworkMACAddressPool>();

    error = macPoolCache->Lock(macPoolCache, *lockedMacPool);
    if (!error.IsSuccess())
    {
        WriteError(NIMNetworkManagerProvider, "UpdatePersistentDataAsync: failed to get lock for global macPool, Failed : [{0}]",
            error.ErrorCodeValueToString());
        return error;
    }

    if (macPool_->PersistenceState != PersistenceState::NoChange)
    {
        error = this->service_.FM.Store.UpdateData(transactionSPtr, *(macPool_.get()));
        if (!error.IsSuccess())
        {
            WriteError(NIMNetworkManagerProvider, "Store.UpdateData: Failed for macPool_: [{0}], error: {1}",
                ipPool_->GetStoreKey(),
                error.ErrorCodeValueToString());

            transactionSPtr->Rollback();
            return error;
        }
    }

    if (ipPool_->PersistenceState != PersistenceState::NoChange)
    {
        error = this->service_.FM.Store.UpdateData(transactionSPtr, *(ipPool_.get()));
        if (!error.IsSuccess())
        {
            WriteError(NIMNetworkManagerProvider, "Store.UpdateData: Failed for ipPool_: [{0}], error: {1}",
                ipPool_->GetStoreKey(),
                error.ErrorCodeValueToString());

            transactionSPtr->Rollback();
            return error;
        }
    }

    auto ipPool = ipPool_;
    auto network = network_;
    auto completed = make_shared<ManualResetEvent>();

    auto t = transactionSPtr->BeginCommit(
        TimeSpan::MaxValue,
        [network, lockedMacPool, ipPool, nodeEntries, operation, transactionSPtr, completed](AsyncOperationSPtr const & contextSPtr) mutable
        {
            int64 operationLSN = 0;
            ErrorCode result = transactionSPtr->EndCommit(contextSPtr, operationLSN);
            operationLSN = 0;

            if (result.IsSuccess())
            {
                for (const auto & nodeEntry : nodeEntries)
                {
                    nodeEntry->PostCommit(operationLSN);
                }
                network->PostCommit(operationLSN);
                lockedMacPool->Get()->PostCommit(operationLSN);
                ipPool->PostCommit(operationLSN);
            }
            else
            {
                WriteError(NIMNetworkManagerProvider, "Store.BeginCommit: Failed to commit: [{0}], error: {1}",
                    ipPool->GetStoreKey(),
                    result.ErrorCodeValueToString());
            }

            completed->Set();
            operation->TryComplete(operation, result);
        },
        this->service_.FM.CreateAsyncOperationRoot());

    completed->WaitOne();

    return t->Error;
}




Common::AsyncOperationSPtr NetworkManager::BeginAcquireEntryBlock(
    NetworkAllocationRequestMessage & allocationRequest,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkManager::InvokeActionAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage> >(
        *this, allocationRequest, timeout,
        [this](NetworkAllocationRequestMessage & request, std::shared_ptr<NetworkAllocationResponseMessage> & reply, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
        {
            WriteNoise(NIMNetworkManagerProvider, "NetworkManager::AcquireIPBlockAsync: starting.");
            return this->AcquireIPBlockAsync(request, reply, thisSPtr);
        },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

Common::ErrorCode NetworkManager::EndAcquireEntryBlock(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkAllocationResponseMessage> & reply)
{
    WriteNoise(NIMNetworkManagerProvider, "NetworkManager::AcquireIPBlockAsync: ending.");
    return NetworkManager::InvokeActionAsyncOperation<NetworkAllocationRequestMessage, NetworkAllocationResponseMessage>::End(
        operation,
        reply);
}

//--------
// Start the BeginRemoveNetwork invoke async operation.
Common::AsyncOperationSPtr NetworkManager::BeginNetworkRemove(
    NetworkRemoveRequestMessage & request,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, request, timeout,
        [this](NetworkRemoveRequestMessage & request, std::shared_ptr<NetworkErrorCodeResponseMessage> & reply, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
    {
        WriteNoise(NIMNetworkManagerProvider, "NetworkManager::NetworkRemoveAsync: starting.");
        return this->NetworkRemoveAsync(request, reply, thisSPtr);
    },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

//--------
// Complete invoke request and get the reply.
Common::ErrorCode NetworkManager::EndNetworkRemove(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply)
{
    WriteNoise(NIMNetworkManagerProvider, "NetworkManager::EndNetworkRemove: ending.");
    return NetworkManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}


//--------
// Start the BeginNetworkNodeRemove invoke async operation.
Common::AsyncOperationSPtr NetworkManager::BeginNetworkNodeRemove(
    NetworkRemoveRequestMessage & request,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage> >(
        *this, request, timeout,
        [this](NetworkRemoveRequestMessage & request, std::shared_ptr<NetworkErrorCodeResponseMessage> & reply, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
    {
        WriteNoise(NIMNetworkManagerProvider, "NetworkManager::NetworkNodeRemoveAsync: starting.");
        return this->NetworkNodeRemoveAsync(request, reply, thisSPtr);
    },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

//--------
// Complete invoke request and get the reply.
Common::ErrorCode NetworkManager::EndNetworkNodeRemove(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<NetworkErrorCodeResponseMessage> & reply)
{
    WriteNoise(NIMNetworkManagerProvider, "NetworkManager::EndNetworkNodeRemove: ending.");
    return NetworkManager::InvokeActionAsyncOperation<NetworkRemoveRequestMessage, NetworkErrorCodeResponseMessage>::End(
        operation,
        reply);
}

//--------
// Start the BeginReleaseAllIpsForNode invoke async operation.
Common::AsyncOperationSPtr NetworkManager::BeginReleaseAllIpsForNode(
    NodeInfoSPtr & nodeInfo,
    bool releaseAllInstances,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    return AsyncOperation::CreateAndStart<NetworkManager::InvokeActionAsyncOperation<NodeInfoSPtr, InternalError> >(
        *this, nodeInfo, timeout,
        [this, releaseAllInstances](NodeInfoSPtr & nodeInfo, std::shared_ptr<InternalError> & reply, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
        {
            WriteNoise(NIMNetworkManagerProvider, "NetworkManager::ReleaseAllIpsForNodeAsync: starting.");
            return this->ReleaseAllIpsForNodeAsync(nodeInfo, releaseAllInstances, reply, thisSPtr);
        },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

//--------
// Complete invoke request and get the reply.
Common::ErrorCode NetworkManager::EndReleaseAllIpsForNode(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<InternalError> & reply)
{
    WriteNoise(NIMNetworkManagerProvider, "NetworkManager::EndReleaseAllIpsForNode: ending.");
    return NetworkManager::InvokeActionAsyncOperation<NodeInfoSPtr, InternalError>::End(
        operation,
        reply);
}


//--------
// Start the BeginUpdatePersistentData invoke async operation.
Common::AsyncOperationSPtr NetworkManager::BeginUpdatePersistentData(
    std::vector<NIMNetworkNodeAllocationStoreDataSPtr> & request,
    Common::TimeSpan const & timeout,
    Common::AsyncCallback const & callback)
{
    WriteNoise(NIMNetworkManagerProvider, "NetworkManager::BeginUpdatePersistentData: starting.");
    return AsyncOperation::CreateAndStart<NetworkManager::InvokeActionAsyncOperation<std::vector<NIMNetworkNodeAllocationStoreDataSPtr>, ErrorCode> >(
        *this, request, timeout,
        [this](std::vector<NIMNetworkNodeAllocationStoreDataSPtr> & request, std::shared_ptr<ErrorCode> & reply, AsyncOperationSPtr const & thisSPtr) -> ErrorCode
        {
            UNREFERENCED_PARAMETER(reply);
            WriteNoise(NIMNetworkManagerProvider, "NetworkManager::BeginUpdatePersistentData: ending.");
            return this->UpdatePersistentDataAsync(request, thisSPtr);
        },
        callback,
        this->service_.FM.CreateAsyncOperationRoot());
}

//--------
// Complete invoke request and get the reply.
Common::ErrorCode NetworkManager::EndUpdatePersistentData(
    Common::AsyncOperationSPtr const & operation,
    __out std::shared_ptr<ErrorCode> & reply)
{
    return NetworkManager::InvokeActionAsyncOperation<std::vector<NIMNetworkNodeAllocationStoreDataSPtr>, ErrorCode>::End(
        operation,
        reply);
}

ErrorCode NetworkManager::GetNetworkMappingTable(
    std::vector<Federation::NodeInstance>& nodeInstances,
    std::vector<NetworkAllocationEntrySPtr>& networkMappingTable)
{
    ErrorCode error = ErrorCode(ErrorCodeValue::Success);
    {
        AcquireExclusiveLock lock(this->managerLock_);

        for (auto nodeIter = this->nodeAllocationMap_.begin(); nodeIter != this->nodeAllocationMap_.end(); nodeIter++)
        {
            NIMNetworkNodeAllocationStoreDataSPtr p = nodeIter->second;
            
            auto nodeInfo = this->service_.FM.NodeCacheObj.GetNode(p->NodeInstance.Id);
            if (nodeInfo &&
                nodeInfo->NodeInstance.InstanceId == p->NodeInstance.InstanceId)
            {
                nodeInstances.push_back(p->NodeInstance);
                auto nodeAllocMap = p->NodeAllocationMap;
                for (auto entryIter = nodeAllocMap->begin(); entryIter != nodeAllocMap->end(); entryIter++)
                {
                    networkMappingTable.push_back(entryIter->second);
                }
            }
            else
            {
                WriteError(NIMNetworkManagerProvider, "GetNetworkMappingTable: node instance ID [{0}] not found for node ID [{1}]! (probably node recycled)",
                    p->NodeInstance.Id, p->NodeInstance.InstanceId);
            }
        }
    }

    return error;
}

