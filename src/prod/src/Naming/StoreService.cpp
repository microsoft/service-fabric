// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace SystemServices;
using namespace Reliability;
using namespace Transport;
using namespace ClientServerTransport;

namespace Naming
{
    StringLiteral const TraceComponent("Replica");
    StringLiteral const TraceComponentOpen("Open");
    StringLiteral const TraceComponentChangeRole("ChangeRole");
    StringLiteral const TraceComponentClose("Close");

    const StoreServiceEventSource Events;

    class StoreService::StoreServiceRequestJobItem : public AsyncWorkJobItem
    {
        DENY_COPY(StoreServiceRequestJobItem)
    public:
        StoreServiceRequestJobItem()
            : AsyncWorkJobItem()
            , owner_()
            , message_()
            , context_()
            , error_(ErrorCodeValue::Success)
            , operation_()
            , activityId_()
        {
        }

        StoreServiceRequestJobItem(
            __in StoreService * owner,
            Transport::MessageUPtr && message,
            RequestReceiverContextUPtr && context,
            TimeSpan const & workTimeout)
            : AsyncWorkJobItem(workTimeout)
            , owner_(owner)
            , message_(move(message))
            , context_(move(context))
            , error_(ErrorCodeValue::Success)
            , operation_()
            , activityId_()
        {
            activityId_ = FabricActivityHeader::FromMessage(*message_).ActivityId;
        }

        StoreServiceRequestJobItem(StoreServiceRequestJobItem && other)
            : AsyncWorkJobItem(move(other))
            , owner_(move(other.owner_))
            , message_(move(other.message_))
            , context_(move(other.context_))
            , error_(move(other.error_))
            , operation_(move(other.operation_))
            , activityId_(move(other.activityId_))
        {
        }

        StoreServiceRequestJobItem & operator=(StoreServiceRequestJobItem && other)
        {
            if (this != &other)
            {
                owner_ = move(other.owner_);
                message_ = move(other.message_);
                context_ = move(other.context_);
                error_.ReadValue();
                error_ = move(other.error_);
                operation_ = move(other.operation_);
                activityId_ = move(other.activityId_);
            }

            __super::operator=(move(other));
            return *this;
        }

        virtual void OnEnqueueFailed(ErrorCode && error) override
        {
            ASSERT_IF(error.IsSuccess(), "{0}: OnEnqueueFailed but error is success, job queue sequence number {1}", activityId_, this->SequenceNumber);
            WriteInfo(TraceComponent, "{0}: Job queue rejected work: {1}", activityId_, error);
            if (error.IsError(ErrorCodeValue::JobQueueFull))
            {
                error = ErrorCode(
                    ErrorCodeValue::NamingServiceTooBusy,
                    wformatString(GET_NAMING_RC(JobQueueFull), owner_->ReplicaId, owner_->PartitionId));
            }

            ASSERT_IF(owner_ == nullptr, "{0}: OnEnqueueFailed: owner not set, job queue sequence number {1}", activityId_, this->SequenceNumber);
            owner_->OnProcessRequestFailed(move(error), *context_);
        }

    protected:
        virtual void StartWork(
            AsyncWorkReadyToCompleteCallback const & completeCallback,
            __out bool & completedSync) override
        {
            ASSERT_IF(owner_ == nullptr, "{0}: StartWork: owner not set, job queue sequence number {1}", activityId_, this->SequenceNumber);
            error_ = owner_->TryAcceptMessage(message_);
            if (!error_.IsSuccess())
            {
                WriteNoise(
                    TraceComponent,
                    owner_->TraceId,
                    "{0}: StartWork, Action: {1}, TryAcceptMessage returned error: {2}, job queue sequence number {3}",
                    activityId_,
                    message_->Action,
                    error_,
                    this->SequenceNumber);

                completedSync = true;
                return;
            }

            WriteNoise(
                TraceComponent,
                owner_->TraceId,
                "{0}: StartWork, Action: {1}, RemainingTime: {2}, job queue sequence number {3}",
                activityId_,
                message_->Action,
                this->RemainingTime,
                this->SequenceNumber);

            auto inner = owner_->BeginProcessRequest(
                std::move(message_),
                this->RemainingTime,
                [this, completeCallback](AsyncOperationSPtr const & operation)
                {
                    if (!operation->CompletedSynchronously)
                    {
                        completeCallback(this->SequenceNumber);
                    }
                },
                owner_->CreateAsyncOperationRoot());
            completedSync = inner->CompletedSynchronously;
            operation_ = move(inner);
        }

        virtual void EndWork() override
        {
            ASSERT_IF(owner_ == nullptr, "{0}: EndWork: owner not set, job queue sequence number {1}", activityId_, this->SequenceNumber);
            if (!operation_)
            {
                // This can happen when TryAcceptMessage failed.
                ASSERT_IF(error_.IsSuccess(), "{0}: EndWork: operation not set, but error is success", activityId_);
                owner_->OnProcessRequestFailed(move(error_), *context_);
                return;
            }

            MessageUPtr reply;
            ErrorCode error = owner_->EndProcessRequest(operation_, reply);

            if (error.IsSuccess())
            {
                if (reply)
                {
                    context_->Reply(std::move(reply));
                }
                else
                {
                    // It is a bug for the reply to be null, but handle it gracefully
                    // instead of crashing the process. Return an error that is not
                    // expected and not automatically retried by the system internally.
                    //
                    owner_->OnProcessRequestFailed(ErrorCode(ErrorCodeValue::OperationFailed), *context_);
                }
            }
            else
            {
                owner_->OnProcessRequestFailed(move(error), *context_);
            }
        }

        virtual void OnStartWorkTimedOut() override
        {
            ASSERT_IF(owner_ == nullptr, "{0}: OnStartWorkTimedOut: owner not set, job queue sequence number {1}", activityId_, this->SequenceNumber);
            ASSERT_IFNOT(
                this->State == AsyncWorkJobItemState::NotStarted, "{0}: OnStartWorkTimedOut called in state {1}, expected NotStarted. Job queue sequence number: {2}",
                activityId_,
                this->State,
                this->SequenceNumber);

            WriteInfo(
                TraceComponent,
                owner_->TraceId,
                "{0}: OnStartWorkTimedOut: timed out while waiting in queue, action={1}, job queue sequence number {2}",
                activityId_,
                message_->Action,
                this->SequenceNumber);
            owner_->OnProcessRequestFailed(ErrorCode(ErrorCodeValue::Timeout), *context_);
        }

    private:
        StoreService * owner_;
        Transport::MessageUPtr message_;
        RequestReceiverContextUPtr context_;
        ErrorCode error_;
        AsyncOperationSPtr operation_;
        ActivityId activityId_;
    };

    StoreService::StoreService(
        Guid const & partitionId,
        FABRIC_REPLICA_ID replicaId,
        NodeInstance const & nodeInstance, 
        __in FederationWrapper & federation, 
        __in ServiceAdminClient & adminClient, 
        __in ServiceResolver & fmClient, 
        Common::ComponentRoot const & root)
        : KeyValueStoreReplica(partitionId, replicaId)
        , federationRoot_(root.CreateComponentRoot())
        , OnChangeRoleCallback()
        , OnCloseReplicaCallback()
        , namingConfig_(NamingConfig::GetConfig())
        , namingStoreUPtr_()
        , requestInstanceUPtr_()
        , authorityOwnerRequestTrackerUPtr_()
        , nameOwnerRequestTrackerUPtr_()
        , propertyRequestTrackerUPtr_()
        , perfCounters_()
        , psdCache_(NamingConfig::GetConfig().ServiceDescriptionCacheLimit)
        , healthMonitorUPtr_()
        , propertiesUPtr_()
        , federation_(federation)
        , serviceLocation_()
        , messageFilterSPtr_()
        , pendingRequestCount_(0)
        , pendingPrimaryRecoveryOperations_()
        , recoverPrimaryLock_()
        , namingAOJobQueue_()
        , namingNOJobQueue_()
    { 
        auto tempRequestInstance = make_unique<RequestInstance>();

        auto tempAuthorityOwnerRequestTracker = make_unique<NameRequestInstanceTracker>(*this, L"AO", this->PartitionedReplicaId);

        auto tempNameOwnerRequestTracker = make_unique<NameRequestInstanceTracker>(*this, L"NO", this->PartitionedReplicaId);

        auto tempPropertyRequestTracker = make_unique<NamePropertyRequestInstanceTracker>(*this, L"Prop", this->PartitionedReplicaId);

        auto tempHealthMonitor = make_unique<StoreServiceHealthMonitor>(*this, partitionId, replicaId, federation);

        auto tempProperties = make_unique<StoreServiceProperties>(
            nodeInstance,
            *this,
            adminClient, 
            fmClient,
            *tempAuthorityOwnerRequestTracker,
            *tempNameOwnerRequestTracker,
            *tempPropertyRequestTracker,
            psdCache_,
            Events,
            *tempHealthMonitor);

        this->InitializeHandlers();

        requestInstanceUPtr_.swap(tempRequestInstance);
        authorityOwnerRequestTrackerUPtr_.swap(tempAuthorityOwnerRequestTracker);
        nameOwnerRequestTrackerUPtr_.swap(tempNameOwnerRequestTracker);
        propertyRequestTrackerUPtr_.swap(tempPropertyRequestTracker);
        propertiesUPtr_.swap(tempProperties);
        healthMonitorUPtr_.swap(tempHealthMonitor);

        WriteNoise(
            TraceComponent, 
            "{0} StoreService::ctor: node instance = {1}",
            static_cast<void*>(this),
            nodeInstance);
    }

    StoreService::~StoreService()
    {
        WriteNoise(
            TraceComponent, 
            "{0} StoreService::~dtor: trace ID = {1}",
            static_cast<void*>(this),
            this->TraceId);

        OnChangeRoleCallback = nullptr;
        OnCloseReplicaCallback = nullptr;
    }

    void StoreService::InitializeHandlers()
    {
        map<wstring, ProcessRequestHandler> t;

        this->AddHandler(t, NamingTcpMessage::CreateNameAction, CreateHandler<ProcessCreateNameRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::DeleteNameAction, CreateHandler<ProcessDeleteNameRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::NameExistsAction, CreateHandler<ProcessNameExistsRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::InnerCreateNameAction, CreateHandler<ProcessInnerCreateNameRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::InnerDeleteNameAction, CreateHandler<ProcessInnerDeleteNameRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::CreateServiceAction, CreateHandler<ProcessCreateServiceRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::UpdateServiceAction, CreateHandler<ProcessUpdateServiceRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::DeleteServiceAction, CreateHandler<ProcessDeleteServiceRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::InnerCreateServiceAction, CreateHandler<ProcessInnerCreateServiceRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::InnerUpdateServiceAction, CreateHandler<ProcessInnerUpdateServiceRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::InnerDeleteServiceAction, CreateHandler<ProcessInnerDeleteServiceRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::EnumerateSubNamesAction, CreateHandler<ProcessEnumerateSubNamesRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::PropertyBatchAction, CreateHandler<ProcessPropertyBatchRequestAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::EnumeratePropertiesAction, CreateHandler<ProcessEnumeratePropertiesRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::ServiceExistsAction, CreateHandler<ProcessServiceExistsRequestAsyncOperation>);
        this->AddHandler(t, NamingMessage::PrefixResolveAction, CreateHandler<ProcessPrefixResolveRequestAsyncOperation>);

        requestHandlers_.swap(t);
    }

    void StoreService::AddHandler(map<wstring, ProcessRequestHandler> & temp, wstring const & action, ProcessRequestHandler const & handler)
    {
        temp.insert(make_pair(action, handler));
    }

    template <class TAsyncOperation>
    AsyncOperationSPtr StoreService::CreateHandler(
        __in MessageUPtr & request,
        __in NamingStore & store,
        __in StoreServiceProperties & properties,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<TAsyncOperation>(
            move(request),
            store,
            properties,
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr StoreService::BeginProcessRequest(
        MessageUPtr && request,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        ++pendingRequestCount_;

        auto const & action = request->Action;

        auto it = requestHandlers_.find(action);
        if (it != requestHandlers_.end())
        {
            return (it->second)(
                request, 
                *namingStoreUPtr_,
                *propertiesUPtr_,
                timeout, 
                callback, 
                parent);
        }
        else
        {
            WriteWarning(
                TraceComponent,
                "{0} received message {1} with invalid action {2}",
                this->TraceId,
                request->MessageId,
                action);

            return AsyncOperation::CreateAndStart<ProcessInvalidRequestAsyncOperation>(
                move(request),
                *namingStoreUPtr_,
                *propertiesUPtr_,
                ErrorCodeValue::InvalidMessage,
                timeout,
                callback,
                parent);
        }
    }

    ErrorCode StoreService::EndProcessRequest(
        AsyncOperationSPtr const & asyncOperation,
        __out MessageUPtr & reply)
    {
        --pendingRequestCount_;

        return ProcessRequestAsyncOperation::End(asyncOperation, reply);
    }

    AsyncOperationSPtr StoreService::BeginRequestReplyToPeer(
        MessageUPtr && message,
        NodeInstance const & target,
        FabricActivityHeader const & activityHeader,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        auto instance = requestInstanceUPtr_->GetNextInstance();

        message->Headers.Replace(activityHeader);
        message->Headers.Replace(TimeoutHeader(timeout));
        message->Headers.Replace(RequestInstanceHeader(instance));

        return federation_.BeginRequestToNode(
            std::move(message),
            target,
            true,
            TimeSpan::MaxValue,
            timeout,
            callback,
            root);
    }

    ErrorCode StoreService::EndRequestReplyToPeer(
        AsyncOperationSPtr const & operation,
        __out MessageUPtr & reply)
    {
        ErrorCode error = federation_.EndRequestToNode(operation, reply);

        if (error.IsSuccess())
        {
            error = RequestInstanceTrackerHelper::UpdateInstanceOnStaleRequestFailureReply(reply, *requestInstanceUPtr_);
        }

        return error;
    }

    void StoreService::InitializeMessageHandler()
    {
        auto selfRoot = this->CreateComponentRoot();
        auto filter = messageFilterSPtr_;

        // Register to receive routed messages from federation layer
        //
        federation_.Federation.RegisterMessageHandler(
            Transport::Actor::NamingStoreService,
            [](Transport::MessageUPtr &, OneWayReceiverContextUPtr &)
            { 
                Assert::CodingError("StoreService does not support oneway messages");
            },
            [this, selfRoot](Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr & requestReceiverContext)
            { 
                TimeoutHeader timeoutHeader;
                NamingMessage::TryGetNamingHeader(*message, timeoutHeader);
                bool isNoRequest = NamingMessage::IsNORequest(message->Action);
                StoreServiceRequestJobItem jobItem(
                    this,
                    move(message),
                    move(requestReceiverContext),
                    timeoutHeader.Timeout); 
                
                if (isNoRequest)
                {
                    namingNOJobQueue_->Enqueue(move(jobItem)).ReadValue();
                }
                else
                {
                    namingAOJobQueue_->Enqueue(move(jobItem)).ReadValue();
                }
            },
            true /*dispatchOnTransportThread*/,
            std::move(filter));
    }

    // ******************* 
    // StatefulServiceBase 
    // ******************* 
    
    ErrorCode StoreService::OnOpen(ComPointer<IFabricStatefulServicePartition> const & servicePartition)
    {
        UNREFERENCED_PARAMETER(servicePartition);

        propertiesUPtr_->PartitionId = this->PartitionedReplicaId.PartitionId.ToString();

        auto replicaTimestamp = DateTime::Now().Ticks;

        // Cache service location since it will not change after open
        //
        serviceLocation_ = SystemServiceLocation(
            federation_.Instance,
            this->PartitionedReplicaId.PartitionId, 
            this->PartitionedReplicaId.ReplicaId,
            replicaTimestamp);

        messageFilterSPtr_ = make_shared<SystemServiceMessageFilter>(serviceLocation_);

        wstring uniqueId = wformatString("{0}:{1}", propertiesUPtr_->PartitionId, PartitionedReplicaId.ReplicaId);
        namingAOJobQueue_ = make_unique<NamingAsyncJobQueue>(
            L"NamingStore.AO.",
            uniqueId,
            this->CreateComponentRoot(),
            make_unique<NamingStoreJobQueueConfigSettingsHolder>());
        namingNOJobQueue_ = make_unique<NamingAsyncJobQueue>(
            L"NamingStore.NO.",
            uniqueId,
            this->CreateComponentRoot(),
            make_unique<NamingStoreJobQueueConfigSettingsHolder>());

        WriteInfo(
            TraceComponentOpen, 
            "{0} OnOpen(): node instance = {1} trace ID = {2} service location = {3}",
            static_cast<void*>(this),
            propertiesUPtr_->Instance,
            this->TraceId,
            serviceLocation_);

        requestInstanceUPtr_->SetTraceId(this->TraceId);

        perfCounters_ = NamingPerformanceCounters::CreateInstance(wformatString(
            "{0}:{1}:{2}", 
            this->PartitionId, 
            this->ReplicaId,
            replicaTimestamp));

        propertiesUPtr_->PerfCounters = perfCounters_;

        namingStoreUPtr_ = make_unique<NamingStore>(*(this->ReplicatedStore), *propertiesUPtr_, this->PartitionId, this->ReplicaId);

        // The actor string that gets registered must be unique for all StoreService instances on a 
        // Federation node
        //
        InitializeMessageHandler();

        Events.StoreOpenComplete(this->TraceId);

        OpenReplicaCallback callback = OnOpenReplicaCallback;
        if (callback)
        {
            auto root = shared_from_this();
            auto owner_SPtr = std::static_pointer_cast<StoreService>(root);
            callback(owner_SPtr);
        }

        return ErrorCodeValue::Success;
    }
    
    ErrorCode StoreService::OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out wstring & serviceLocation)
    {
        WriteInfo(
            TraceComponentChangeRole, 
            "{0} OnChangeRole(): role = {1}. ServiceLocation {2}",
            this->TraceId,
            static_cast<int>(newRole),
            serviceLocation_);

        serviceLocation = serviceLocation_.Location;

        // Repair needs to read/write from ReplicatedStore, which is
        // only allowed in the Primary role
        //
        if (newRole == ::FABRIC_REPLICA_ROLE_PRIMARY)
        {
            healthMonitorUPtr_->Open();
            this->StartRecoveringPrimary();
        }
        else
        {
            healthMonitorUPtr_->Close();
        }

        // StoreServiceFactory maintains a list of open StoreService instances
        // for debugging and test purposes
        //
        ChangeRoleCallback snapshot = OnChangeRoleCallback;
        if (snapshot)
        {
            snapshot(
                serviceLocation_,
                this->PartitionedReplicaId.PartitionId,
                this->PartitionedReplicaId.ReplicaId, 
                newRole);
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode StoreService::OnClose()
    {
        WriteInfo(
            TraceComponentClose, 
            "{0} OnClose()",
            this->TraceId);

        healthMonitorUPtr_->Close();

        authorityOwnerRequestTrackerUPtr_->Stop();
        nameOwnerRequestTrackerUPtr_->Stop();
        propertyRequestTrackerUPtr_->Stop();

        federation_.Federation.UnRegisterMessageHandler(Transport::Actor::NamingStoreService, messageFilterSPtr_);

        messageFilterSPtr_.reset();

        if (namingAOJobQueue_ != nullptr)
        {
            namingAOJobQueue_->Close();
        }

        if (namingNOJobQueue_ != nullptr)
        {
            namingNOJobQueue_->Close();
        }

        federationRoot_.reset();

        CloseReplicaCallback snapshot = OnCloseReplicaCallback;
        OnCloseReplicaCallback = nullptr;
        OnChangeRoleCallback = nullptr;

        if (snapshot)
        {
            snapshot(serviceLocation_, static_pointer_cast<StoreService>(shared_from_this()));
        }

        return ErrorCodeValue::Success;
    }

    void StoreService::OnAbort()
    {
        WriteInfo(
            TraceComponent, 
            "{0} OnAbort()",
            this->TraceId);

        OnClose();
    }

    bool StoreService::ValidateMessage(__in Transport::MessageUPtr & message)
    {
        bool success = true;
        {
            FabricActivityHeader header;
            success = message->Headers.TryReadFirst(header);
        }

        if (success) 
        { 
            TimeoutHeader header;
            success = message->Headers.TryReadFirst(header);
        }

        if (success)
        {
            success = (message->Actor == Transport::Actor::NamingStoreService);
        }

        return success;
    }

    Common::ErrorCode StoreService::TryAcceptMessage(
        __in MessageUPtr & message)
    {
        if (!this->IsActivePrimary)
        {
            return ErrorCode(
                ErrorCodeValue::NotPrimary,
                wformatString(GET_COMMON_RC(Not_Primary), this->ReplicaId, this->PartitionId));
        }
        else
        {
            size_t recoveryOperationCount;
            { // lock
                AcquireReadLock lock(recoverPrimaryLock_);
                recoveryOperationCount = pendingPrimaryRecoveryOperations_.size();
            } // endlock

            if (recoveryOperationCount > 0)
            {
                WriteNoise(
                    TraceComponent,
                    "{0} primary still being recovered: {1} pending operations",
                    this->TraceId,
                    recoveryOperationCount);
                return ErrorCode(
                    ErrorCodeValue::NotPrimary,
                    wformatString(GET_NAMING_RC(Not_Primary_RecoveryInProgress), this->ReplicaId, this->PartitionId));
            }
        }

        if (!ValidateMessage(message))
        {
            WriteWarning(
                TraceComponent,
                "{0}: validation of message failed [{1}]",
                this->TraceId,
                *message);

            return ErrorCode(ErrorCodeValue::InvalidMessage);
        }

        return ErrorCode::Success();
    }
    
    void StoreService::OnProcessRequestFailed(
        Common::ErrorCode && error, 
        RequestReceiverContext & requestReceiverContext)
    {
        requestReceiverContext.Reject(namingStoreUPtr_->ToRemoteError(move(error)));
    }

    void StoreService::StartRecoveringPrimary()
    {
        auto operation = make_shared<RecoverPrimaryAsyncOperation>(
            *this,
            *namingStoreUPtr_,
            *propertiesUPtr_,
            [this](AsyncOperationSPtr const & operation) { this->FinishRecoveringPrimary(operation, false); },
            this->CreateAsyncOperationRoot());

        // Only use this make_shared/Start() pattern if the async operation will not
        // be cancelled before Start() finishes since Cancel() is not supported on unstarted async operations
        //
        {
            AcquireWriteLock lock(recoverPrimaryLock_);

            pendingPrimaryRecoveryOperations_.push_back(operation);

            WriteInfo(
                TraceComponent,
                "{0} added primary recovery operation {1}",
                this->TraceId,
                static_cast<void*>(operation.get()));
        }
        
        operation->Start(operation);
            
        this->FinishRecoveringPrimary(operation, true);
    }

    void StoreService::RecoverPrimaryStartedOrCancelled(Common::AsyncOperationSPtr const & operation)
    {
        AcquireWriteLock lock(recoverPrimaryLock_);

        pendingPrimaryRecoveryOperations_.erase(
            find_if(pendingPrimaryRecoveryOperations_.begin(), pendingPrimaryRecoveryOperations_.end(), 
                [&operation](RecoverPrimaryAsyncOperationSPtr const & item) -> bool { return item.get() == operation.get(); }),
            pendingPrimaryRecoveryOperations_.end());

        WriteInfo(
            TraceComponent,
            "{0} removed primary recovery operation {1}",
            this->TraceId,
            static_cast<void*>(operation.get()));
    }

    bool StoreService::IsPendingPrimaryRecovery()
    {
        AcquireReadLock lock(recoverPrimaryLock_);

        return !pendingPrimaryRecoveryOperations_.empty();
    }

    bool StoreService::ShouldAbortProcessing()
    {
        return (this->IsPendingPrimaryRecovery() || !this->IsActivePrimary);
    }

    void StoreService::FinishRecoveringPrimary(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        RecoverPrimaryAsyncOperation::End(operation);
    }
}
