// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace Client;
using namespace FabricTest;
using namespace FederationTestCommon;
using namespace Naming;
using namespace Reliability;
using namespace ServiceModel;
using namespace std;
using namespace TestCommon;
using namespace Transport;
using namespace ClientServerTransport;

const StringLiteral TraceSource("TestClientGateway");
const GatewayEventSource EventSource;

class TestClientGateway::ProcessClientRequestAsyncOperation : public TimedAsyncOperation
{
public:
    ProcessClientRequestAsyncOperation(
        TestClientGateway &owner,
        MessageUPtr &&message,
        ISendTarget::SPtr const & sender,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , request_(move(message))
        , sender_(sender)
        , action_(request_->Action)
        , id_(request_->MessageId)
        , reply_()
    {
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);
        MessageUPtr reply;

        auto it = owner_.messageHandlers_.find(request_->Action);
        if (it != owner_.messageHandlers_.end())
        {
            error = it->second(owner_, sender_, request_, reply_);
            if (error.IsSuccess())
            {
                OnSuccess(thisSPtr);
            }
            else
            {
                OnFailure(thisSPtr, error);
            }

            return;
        }

        auto op = owner_.serviceNotificationManagerProxy_->BeginProcessRequest(
            move(request_),
            sender_,
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRequestComplete(operation);
            },
            thisSPtr);
    }

    void OnRequestComplete(AsyncOperationSPtr const &operation)
    {
        auto error = owner_.serviceNotificationManagerProxy_->EndProcessRequest(operation, reply_);
        if (error.IsSuccess())
        {
            OnSuccess(operation->Parent);
        }
        else
        {
            OnFailure(operation->Parent, error);
        }
    }

    void OnSuccess(AsyncOperationSPtr const &operation)
    {
        reply_->Headers.Add(RelatesToHeader(id_));
        reply_->SetLocalTraceContext(wformatString("TR-OK({0})", action_));
        TryComplete(operation, ErrorCodeValue::Success);
    }

    void OnFailure(AsyncOperationSPtr const &operation, ErrorCode error)
    {
        auto tempError = error;
        TestSession::WriteWarning(
            TraceSource,
            "Failing action:{0} with error {1}",
            action_,
            error);

        reply_->Headers.Add(RelatesToHeader(id_));
        reply_ = NamingMessage::GetClientOperationFailure(id_, move(tempError));
        reply_->SetLocalTraceContext(wformatString("TR-Fail({0}, {1})", action_, error));
        TryComplete(operation, error);
    }

    static ErrorCode End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<ProcessClientRequestAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }
        return casted->Error;
    }

    TestClientGateway &owner_;
    MessageUPtr request_;
    MessageUPtr reply_;
    ISendTarget::SPtr sender_;
    MessageId id_;
    wstring action_;
};

class TestClientGateway::ProcessIpcRequestAsyncOperation : public TimedAsyncOperation
{
public:
    ProcessIpcRequestAsyncOperation(
        TestClientGateway &owner,
        MessageUPtr &&message,
        TimeSpan const timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
        , request_(move(message))
        , reply_()
    {
    }

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        auto error = owner_.serviceNotificationManager_->ProcessRequest(request_, reply_);
        TryComplete(thisSPtr, error);
    }

    static ErrorCode End(AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<ProcessIpcRequestAsyncOperation>(operation);
        if (casted->Error.IsSuccess())
        {
            reply = move(casted->reply_);
        }
        return casted->Error;
    }

    TestClientGateway &owner_;
    MessageUPtr request_;
    MessageUPtr reply_;
};

//
// TestClientGateway
//

TestClientGateway::TestClientGateway(
    NodeId const & nodeId,
    int replicaCount,
    int sgMemberCount)
    : nodeInstance_(nodeId, DateTime::Now().Ticks)
    , replicaCount_(replicaCount)
    , sgMemberCount_(sgMemberCount)
    , publicTransport_()
    , mockCache_()
    , serviceNotificationManager_()
    , messageHandlers_()
    , mockPsdTemplate_()
    , mockRspTemplates_()
    , notificationBurst_(0)
    , currentVersion_(0)
{
    this->AddHandler(NamingTcpMessage::PingRequestAction, ProcessPing);
    this->AddHandler(NamingTcpMessage::GetServiceDescriptionAction, ProcessGetServiceDescription);
    this->AddHandler(NamingTcpMessage::ResolveServiceAction, ProcessResolveService);
    this->AddHandler(NamingTcpMessage::ServiceLocationChangeListenerAction, ProcessNotification);
}

ErrorCode TestClientGateway::Open(__out wstring & address)
{
    TestSession::WriteInfo(
            TraceSource,
            "Requesting 3 ports");
    vector<USHORT> ports;
    FABRICSESSION.AddressHelperObj.GetAvailablePorts(nodeInstance_.Id.ToString(), ports);
    address = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[0]));

    wstring ipcServerAddress = AddressHelper::BuildAddress(AddressHelper::GetLoopbackAddress(), wformatString(ports[1]));

    internalTransport_ = make_shared<EntreeServiceTransport>(
        *this,
        ipcServerAddress,
        nodeInstance_,
        L"TestClientGateway",
        EventSource);

    auto error = internalTransport_->Open();
    if (!error.IsSuccess()) { return error; }

    auto transport = DatagramTransportFactory::CreateTcp(address, nodeInstance_.Id.ToString());

    publicTransport_ = DatagramTransportFactory::CreateUnreliable(*this, transport);

    demuxer_ = make_unique<Demuxer>(*this, publicTransport_);

    error = publicTransport_->Start();
    if (!error.IsSuccess()) { return error; }

    error = demuxer_->Open();
    if (!error.IsSuccess()) { return error; }

    requestReply_ = make_shared<RequestReply>(*this, publicTransport_);
    requestReply_->Open();
    demuxer_->SetReplyHandler(*requestReply_);

    proxyIpcClient_ = make_unique<ProxyToEntreeServiceIpcChannel>(*this, nodeInstance_.Id.ToString(), internalTransport_->TransportListenAddress);

    auto waiter = make_shared<AsyncOperationWaiter>();
    auto op = proxyIpcClient_->BeginOpen(
        TimeSpan::FromSeconds(30),
        [this, waiter](AsyncOperationSPtr const &)
        {
            waiter->Set();
        },
        this->CreateAsyncOperationRoot());

    waiter->WaitOne();
    error = proxyIpcClient_->EndOpen(op);
    if (!error.IsSuccess()) { return error; }

    serviceNotificationManager_ = make_unique<ServiceNotificationManager>(
        nodeInstance_, 
        wformatString("node:{0}", nodeInstance_),
        mockCache_, 
        EventSource,
        *this);

    serviceNotificationManagerProxy_ = make_unique<ServiceNotificationManagerProxy>(
        *this,
        nodeInstance_,
        requestReply_,
        *proxyIpcClient_,
        publicTransport_);

    auto selfRoot = this->CreateComponentRoot();

    demuxer_->RegisterMessageHandler(
        Actor::NamingGateway,
        [this, selfRoot](MessageUPtr & r, ReceiverContextUPtr & receiver)
        {
            this->ProcessClientMessage(r, receiver->ReplyTarget);
        },
        false);

    internalTransport_->RegisterMessageHandler(
        Actor::NamingGateway,
        [this, selfRoot] (MessageUPtr &message, TimeSpan const timeout, AsyncCallback const &callback, AsyncOperationSPtr const &parent)
        {
            return AsyncOperation::CreateAndStart<ProcessIpcRequestAsyncOperation>(
                *this,
                move(message),
                timeout,
                callback,
                parent);
        },
        [this, selfRoot] (AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
        {
            return ProcessIpcRequestAsyncOperation::End(operation, reply);
        });

    serviceNotificationManager_->Open(internalTransport_);
    serviceNotificationManagerProxy_->Open();

    address = publicTransport_->ListenAddress();

    return error;
}

void TestClientGateway::Close()
{
    serviceNotificationManager_->Close();
    serviceNotificationManagerProxy_->Close();

    demuxer_->UnregisterMessageHandler(Actor::NamingGateway);
    internalTransport_->UnregisterMessageHandler(Actor::NamingGateway);

    auto waiter = make_shared<AsyncOperationWaiter>();
    auto op = proxyIpcClient_->BeginClose(
        TimeSpan::FromSeconds(30),
        [this, waiter](AsyncOperationSPtr const &)
    {
        waiter->Set();
    },
        this->CreateAsyncOperationRoot());

    waiter->WaitOne();
    proxyIpcClient_->EndClose(op);

    internalTransport_->Close();
    demuxer_->Close();
    requestReply_->Close();
}

void TestClientGateway::ProcessClientMessage(
    MessageUPtr & request,
    ISendTarget::SPtr const & sender)
{
    TimeoutHeader timeoutHeader;
    TimeSpan timeout(TimeSpan::FromMinutes(2));
    if (request->Headers.TryReadFirst(timeoutHeader)) { timeout = timeoutHeader.Timeout; }
    
    auto op = AsyncOperation::CreateAndStart<ProcessClientRequestAsyncOperation>(
        *this,
        move(request),
        sender,
        timeout,
        [this, sender](AsyncOperationSPtr const &operation)
        {
            this->OnProcessClientMessageComplete(sender, operation);
        },
        this->CreateAsyncOperationRoot());
}

void TestClientGateway::OnProcessClientMessageComplete(ISendTarget::SPtr const & sender, AsyncOperationSPtr const &operation)
{
    MessageUPtr reply;
    auto error = ProcessClientRequestAsyncOperation::End(operation, reply);
    if (!error.IsSuccess())
    {
        reply = NamingMessage::GetClientOperationFailure(
                MessageId(),
                move(error));
    }
    
    publicTransport_->SendOneWay(sender, move(reply));
}

ErrorCode TestClientGateway::ProcessPing(
    TestClientGateway & gateway,
    ISendTarget::SPtr const &,
    MessageUPtr &,
    __out MessageUPtr & reply)
{
    reply = NamingTcpMessage::GetGatewayPingReply(Common::make_unique<PingReplyMessageBody>(GatewayDescription(
        L"",
        gateway.nodeInstance_,
        L"TestClientGateway")))->GetTcpMessage();

    reply->Headers.Add(*ClientProtocolVersionHeader::CurrentVersionHeader);

    return ErrorCodeValue::Success;
}

ErrorCode TestClientGateway::ProcessGetServiceDescription(
    TestClientGateway & gateway,
    ISendTarget::SPtr const &,
    MessageUPtr & request,
    __out MessageUPtr & reply)
{
    NamingUri name;
    if (!request->GetBody(name))
    {
        return ErrorCodeValue::InvalidNameUri;
    }

    if (gateway.mockPsdsByServiceName_.empty())
    {
        // generate dynamically
        
        gateway.mockPsdTemplate_->MutableService.Name = name.ToString();

        // FM version does not change on PSD
        gateway.mockPsdTemplate_->MutableService.UpdateVersion = 0;

        // Store version (i.e. PSD version)
        gateway.mockPsdTemplate_->Version = gateway.currentVersion_++;

        reply = NamingMessage::GetServiceDescriptionReply(*(gateway.mockPsdTemplate_));                        
    }
    else
    {
        // look for pregenerated PSD (when CUID matters)

        auto findIt = gateway.mockPsdsByServiceName_.find(name);
        if (findIt == gateway.mockPsdsByServiceName_.end())
        {
            return ErrorCodeValue::UserServiceNotFound;
        }
        else
        {
            auto const & mockPsd = findIt->second;

            reply = NamingMessage::GetServiceDescriptionReply(*mockPsd);
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode TestClientGateway::ProcessResolveService(
    TestClientGateway & gateway,
    ISendTarget::SPtr const &,
    MessageUPtr & request,
    __out MessageUPtr & reply)
{
    ServiceResolutionMessageBody body;
    if (!request->GetBody(body))
    {
        return ErrorCodeValue::InvalidMessage;
    }

    auto const & name = body.Name;
    auto const & data = body.Request;

    ResolvedServicePartitionSPtr rsp;
    auto error = gateway.GetCurrentMockRsp(name, data, rsp);
    if (!error.IsSuccess()) { return error; }

    reply = NamingMessage::GetResolveServiceReply(*rsp);

    return ErrorCodeValue::Success;
}

ErrorCode TestClientGateway::ProcessNotification(
    TestClientGateway & gateway,
    ISendTarget::SPtr const &,
    MessageUPtr & request,
    __out MessageUPtr & reply)
{
    ServiceLocationNotificationRequestBody body;
    if (!request->GetBody(body))
    {
        return ErrorCodeValue::InvalidMessage;
    }

    vector<ResolvedServicePartition> replies;
    vector<ServiceLocationNotificationRequestData> const & requests = body.Requests;
    for (auto it = requests.begin(); it != requests.end(); ++it)
    {
        auto const & name = it->Name;
        ServiceResolutionRequestData data(it->Partition);

        ResolvedServicePartitionSPtr rsp;
        auto error = gateway.GetCurrentMockRsp(name, data, rsp);
        if (!error.IsSuccess()) { return error; }

        replies.push_back(move(*rsp));
    }

    reply = NamingMessage::GetServiceLocationChangeListenerReply(ServiceLocationNotificationReplyBody(
        move(replies),
        vector<AddressDetectionFailure>(),
        0));

    return ErrorCodeValue::Success;
}

GlobalWString ServiceNamePlaceHolder = make_global<wstring>(L"fabric:/[servicename]");

void TestClientGateway::SetMockServiceTemplate(
    PartitionedServiceDescriptor const & psd)
{
    mockPsdTemplate_ = make_shared<PartitionedServiceDescriptor>(psd);

    mockRspTemplates_.resize(psd.Service.PartitionCount);

    vector<wstring> mockEndpoints;

    for (auto ix=0; ix<replicaCount_; ++ix)
    {
        wstring endpoint;

        if (sgMemberCount_ > 0)
        {
            for (auto jx=0; jx<sgMemberCount_; ++jx)
            {
                endpoint.append(L"%%");
                endpoint.append(ServiceNamePlaceHolder);
                endpoint.append(wformatString("#Member{0}#MemberReplicaEndpoint{1}-{2}", jx, jx, ix));
            }
        }
        else
        {
            endpoint = wformatString("ServiceReplicaEndpoint{0}", ix);
        }

        mockEndpoints.push_back(move(endpoint));
    }

    for (auto ix=0; ix<psd.Service.PartitionCount; ++ix)
    {
        auto primaryCopy = mockEndpoints[0];
        auto replicasCopy = mockEndpoints;

        ServiceTableEntry endpoints(
            psd.Cuids[ix],
            ServiceNamePlaceHolder,
            ServiceReplicaSet(
                true, // isStateful
                !primaryCopy.empty(), // isPrimaryLocationvalid,
                move(primaryCopy),
                move(replicasCopy),
                0));

        auto rsp = make_shared<ResolvedServicePartition>(
            (sgMemberCount_ > 0),
            endpoints,
            psd.GetPartitionInfo(psd.Cuids[ix]),
            GenerationNumber(),
            0,
            nullptr);

        mockRspTemplates_[ix] = rsp;
    }
}

void TestClientGateway::SetNotificationBurst(int burstSize)
{
    notificationBurst_ = burstSize;
}

void TestClientGateway::GenerateMockServicePsds(NamingUri const & prefix, int serviceCount, int subnameCount)
{
    for (auto ix=0; ix<serviceCount; ++ix)
    {
        auto parentName = this->CreateServiceNameFromPrefix(prefix, ix);

        for (auto jx=0; jx<subnameCount; ++jx)
        {
            auto childName = this->CreateServiceNameFromPrefix(parentName, jx);

            vector<ConsistencyUnitId> cuids;
            for (auto cx=0; cx<mockPsdTemplate_->Cuids.size(); ++cx)
            {
                cuids.push_back(ConsistencyUnitId());
            }

            mockPsdTemplate_->Test_UpdateCuids(move(cuids));
            mockPsdTemplate_->MutableService.Name = childName.ToString();
            mockPsdTemplate_->MutableService.UpdateVersion = 0;
            mockPsdTemplate_->Version = currentVersion_++;

            mockPsdsByServiceName_.insert(make_pair(
                childName,
                make_shared<PartitionedServiceDescriptor>(*mockPsdTemplate_)));
        }
    }
}

ErrorCode TestClientGateway::GetCurrentMockRsp(
    NamingUri const & name,
    ServiceResolutionRequestData const & requestData,
    __out ResolvedServicePartitionSPtr & rsp)
{
    int pIndex = -1;
    if (!mockPsdTemplate_->TryGetPartitionIndex(requestData.Key, pIndex))
    {
        return ErrorCodeValue::InvalidServicePartition;
    }

    auto const & mock = mockRspTemplates_[pIndex];
    auto const & mockReplicaSet = mock->Locations.ServiceReplicaSet;

    vector<wstring> mockEndpoints;

    if (mock->IsServiceGroup)
    {
        auto serviceName = name.GetTrimQueryAndFragment().ToString();

        for (auto & endpoint : mockReplicaSet.IsStateful 
                ? mockReplicaSet.SecondaryLocations
                : mockReplicaSet.ReplicaLocations)
        {
            auto endpointCopy = endpoint;

            StringUtility::Replace<wstring>(
                endpointCopy,
                ServiceNamePlaceHolder,
                serviceName,
                0);

            mockEndpoints.push_back(move(endpointCopy));
        }
    }
    else
    {
        mockEndpoints = mockReplicaSet.IsStateful 
                ? mockReplicaSet.SecondaryLocations
                : mockReplicaSet.ReplicaLocations;
    }

    auto mockPrimary = mockReplicaSet.IsStateful ? mockReplicaSet.PrimaryLocation : L"";

    rsp = make_shared<ResolvedServicePartition>(
        mock->IsServiceGroup,
        ServiceTableEntry(
            mock->Locations.ConsistencyUnitId,
            name.ToString(),
            ServiceReplicaSet(
                mockReplicaSet.IsStateful,
                mockReplicaSet.IsPrimaryLocationValid,
                move(mockPrimary),
                move(mockEndpoints),
                currentVersion_++)),
        mock->PartitionData,
        mock->Generation,
        currentVersion_++,
        nullptr);

    return ErrorCodeValue::Success;
}

ErrorCode TestClientGateway::GetCurrentMockStes(
    NamingUri const & name,
    bool includeEndpoints,
    __out vector<ServiceTableEntrySPtr> & result)
{
    auto findIt = mockPsdsByServiceName_.find(name);
    if (findIt == mockPsdsByServiceName_.end())
    {
        return ErrorCodeValue::UserServiceNotFound;
    }

    auto const & mockPsd = findIt->second;

    vector<ServiceTableEntrySPtr> stes;
    for (auto const & cuid : mockPsd->Cuids)
    {
        auto const & mock = mockRspTemplates_[0];
        auto const & mockReplicaSet = mock->Locations.ServiceReplicaSet;

        auto mockPrimary = (mockReplicaSet.IsStateful && mockReplicaSet.IsPrimaryLocationValid) 
            ? mockReplicaSet.PrimaryLocation 
            : L"";

        vector<wstring> mockEndpoints;
        if (includeEndpoints)
        {
            mockEndpoints = mockReplicaSet.IsStateful 
                    ? mockReplicaSet.SecondaryLocations
                    : mockReplicaSet.ReplicaLocations;
        }

        stes.push_back(make_shared<ServiceTableEntry>(
            cuid,
            name.ToString(),
            ServiceReplicaSet(
                mockReplicaSet.IsStateful,
                mockReplicaSet.IsPrimaryLocationValid,
                move(mockPrimary),
                move(mockEndpoints),
                currentVersion_++)));
    }

    result.swap(stes);

    return ErrorCodeValue::Success;
}

ErrorCode TestClientGateway::GenerateCacheUpdates(
    NamingUri const & prefix,
    int serviceCount,
    int subnameCount,
    int batchCount)
{
    return this->GenerateCacheEntries(
        prefix, 
        true, 
        serviceCount, 
        subnameCount, 
        batchCount);
}

ErrorCode TestClientGateway::GenerateCacheDeletes(
    NamingUri const & prefix,
    int serviceCount,
    int subnameCount,
    int batchCount)
{
    return this->GenerateCacheEntries(
        prefix, 
        false, 
        serviceCount, 
        subnameCount, 
        batchCount);
}


ErrorCode TestClientGateway::GenerateCacheEntries(
    NamingUri const & prefix,
    bool includeEndpoints,
    int serviceCount,
    int subnameCount,
    int batchCount)
{
    int burstCount = 0;

    vector<ServiceTableEntrySPtr> updates;
    for (auto ix=0; ix<serviceCount; ++ix)
    {
        auto parentName = this->CreateServiceNameFromPrefix(prefix, ix);

        for (auto jx=0; jx<subnameCount; ++jx)
        {
            auto childName = this->CreateServiceNameFromPrefix(parentName, jx);

            vector<ServiceTableEntrySPtr> stes;
            auto error = this->GetCurrentMockStes(childName, includeEndpoints, stes);
            if (!error.IsSuccess()) { return error; }

            updates.insert(updates.end(), stes.begin(), stes.end());

            burstCount += static_cast<int>(updates.size());

            if (updates.size() >= batchCount)
            {
                mockCache_.UpdateCacheEntries(updates);

                updates.clear();
            }

            if (notificationBurst_ > 0 && burstCount >= notificationBurst_)
            {
                Sleep(1000);

                burstCount = 0;
            }
        }
    }

    return ErrorCodeValue::Success;
}

void TestClientGateway::AddHandler(
    wstring const & key,
    MessageHandler const & handler)
{
    messageHandlers_.insert(make_pair(key, handler));
}


NamingUri TestClientGateway::CreateServiceNameFromPrefix(NamingUri const & prefix, int index)
{
    return NamingUri::Combine(prefix, wformatString("_{0}", index));
}
