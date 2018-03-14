// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ServiceModel;
using namespace ClientServerTransport;
using namespace Naming;

StringLiteral const TraceComponent("ServiceNotificationSender");
    
//
// NotificationPage - The smallest unit of a notification message
//                    after additional filtering and paging.
//
class ServiceNotificationSender::NotificationPage
{
public:
    NotificationPage(
        ServiceNotificationSPtr && notification,
        VersionRangeCollectionSPtr && cacheVersions)
        : notification_(move(notification))
        , cacheVersionsBackup_(move(cacheVersions))
    {
    }

    ServiceNotificationPageId const & GetPageId() { return notification_->GetNotificationPageId(); }
    ServiceNotificationSPtr && TakeNotification() { return move(notification_); }
    VersionRangeCollectionSPtr && TakeCacheVersionsBackup() { return move(cacheVersionsBackup_); }

private:
    ServiceNotificationSPtr notification_;
    VersionRangeCollectionSPtr cacheVersionsBackup_;
};

//
// QueuedNotification - A single in-memory notification before any
//                      additional filtering or paging. Represents
//                      a notification resulting from the processing
//                      of a single cache update message from the FM
//                      or a single pass over the cache during client
//                      reconnection. The latter case is much more
//                      likely to result in paging and is marked
//                      with isForClientSynchronization == true.
//
class ServiceNotificationSender::QueuedNotification
{
public:
    QueuedNotification(
        ActivityId const & activityId,
        GenerationNumber const & cacheGeneration,
        VersionRangeCollectionSPtr && cacheVersions,
        MatchedServiceTableEntryMap && matchedPartitions,
        VersionRangeCollectionSPtr && updateVersions,
        bool isForClientSynchronization)
        : activityId_(activityId)
        , cacheGeneration_(cacheGeneration)
        , cacheVersions_(move(cacheVersions))
        , matchedPartitions_(move(matchedPartitions))
        , updateVersions_(move(updateVersions))
        , isForClientSynchronization_(isForClientSynchronization)
    {
    }

    __declspec (property(get=get_IsForClientSynchronization)) bool IsForClientSynchronization;
    bool get_IsForClientSynchronization() const { return isForClientSynchronization_; }

    vector<NotificationPageSPtr> CreateNotificationPages(
        __in ServiceNotificationSender & owner)
    { 
        //
        // The versions sent to the client with each notification are
        // used to track how up-to-date the client is. If we deliver
        // notifications one at a time to the client, then the gateway
        // can always send the complete gateway cache versions. However,
        // in order to support multiple pending notifications, only
        // the pending notification at the front of the delivery queue
        // can contain the complete gateway cache. Consider the two
        // following notifications:
        //
        // A) Actual Update Versions = [5, 6) Cache Versions = [0, 6)
        // B) Actual Update Versions = [6, 7) Cache Versions = [0, 7)
        //
        // Suppose the client version is currently [0, 5).
        //
        // If the client receives B before A, then the client version
        // should be {[0, 5), [6, 7)} not [0, 7).
        //
        // Until we know the client has received A, notification B must
        // be sent with its update versions rather than the cache versions.
        // Once the client receives A, then B can be sent with the cache
        // versions (e.g. if being retried).
        //
        // Since notifications are only triggered by FM broadcasts, which
        // occur periodically, we normally expect to be able to send
        // the complete cache version if the client is responsive.
        //
        // Notifications that were initialized with update versions can
        // be swapped to use the full cache versions if they have moved
        // to the front of the pending queue by the time they are sent
        // (or retried).
        //

        bool initializeWithCacheVersions = isForClientSynchronization_ ? true : owner.CanInitializeFullCacheVersions();
        auto estimatedClientVersions = owner.GetEstimatedClientVersions();

        uint64 pageIndex = 0;
        vector<NotificationPageSPtr> notificationPages;
        vector<ServiceTableEntryNotificationSPtr> partitionNotifications;

        auto pageSizeLimit = static_cast<size_t>(
            ServiceModelConfig::GetConfig().MaxMessageSize * ServiceModelConfig::GetConfig().MessageContentBufferRatio);

        size_t serializationOverhead = 
            ServiceNotification::SerializationOverheadEstimate 
            + ServiceNotificationRequestBody::SerializationOverheadEstimate;

        size_t estimatedVersionsSize = this->EstimateCurrentVersionsSize();

        size_t estimatedPartitionsSize = 0;

        auto it = matchedPartitions_.begin();
        while (it != matchedPartitions_.end())
        {
            // Notifications are filtered out before actually sending if needed. 
            //
            // Currently, a notification is filtered out if it only matched 
            // "primary-only" notification filters and the client has already 
            // received a notification with the primary address. This is only
            // needed for notifications that result from matching incoming
            // broadcasts against registered filters and not when processing
            // a client reconnection.
            //
            // For client reconnections, primary-only filtering is already
            // done when the client's filters are matched against the gateway
            // cache (see ServiceTableEntryNameIndex::GetMatches).
            //
            ServiceTableEntryNotificationSPtr partitionNotification;
            if (it->second->TryGetServiceTableEntry(
                activityId_,
                estimatedClientVersions,
                partitionNotification))
            {
                auto const & partition = partitionNotification->GetPartition();

                auto partitionSize = partitionNotification->EstimateSize();

                auto currentPageSize = serializationOverhead
                    + estimatedVersionsSize
                    + estimatedPartitionsSize;

                auto estimatedPageSize = currentPageSize + partitionSize;

                if (estimatedPageSize > pageSizeLimit)
                {
                    if (!partitionNotifications.empty())
                    {
                        auto index = ++pageIndex;

                        WriteNoise(
                            TraceComponent,
                            owner.TraceId,
                            "{0}: paging notification: page={1} vers={2} size={3}B limit={4}B vers={5}B overhead={6}B next={7}B",
                            activityId_,
                            index,
                            partition->Version,
                            currentPageSize,
                            pageSizeLimit,
                            estimatedVersionsSize,
                            serializationOverhead,
                            partitionSize);

                        auto remainingCacheVersions = make_shared<VersionRangeCollection>(); 
                        auto remainingUpdateVersions = make_shared<VersionRangeCollection>(); 

                        cacheVersions_->Split(partition->Version, *remainingCacheVersions);
                        updateVersions_->Split(partition->Version, *remainingUpdateVersions);

                        notificationPages.push_back(this->CreateNotificationPage(
                            move(partitionNotifications),
                            index,
                            initializeWithCacheVersions));
                        
                        cacheVersions_ = move(remainingCacheVersions);
                        updateVersions_ = move(remainingUpdateVersions);

                        // Update estimate after the version split.
                        //
                        estimatedVersionsSize = this->EstimateCurrentVersionsSize();

                        estimatedPartitionsSize = 0;
                    }
                }

                partitionNotifications.push_back(partitionNotification);

                estimatedPartitionsSize += partitionSize;
            }
            else if (!initializeWithCacheVersions)
            {
                // Only need to account for the change in update versions
                // resulting from gateway-side version-based filtering.
                //
                updateVersions_->Remove(it->second->Version);

                // This notification may arrive at the client before
                // the notification containing the actual primary change.
                //
                updateVersions_->Remove(it->second->LastPrimaryVersion);

                // Update estimate since we are likely introducing holes now.
                //
                estimatedVersionsSize = this->EstimateCurrentVersionsSize();
            }

            ++it;

        } // for each matched partition

        // Must send at least one notification to client during initial connection
        // to update the client's version ranges on connection.
        //
        if (!partitionNotifications.empty() || (isForClientSynchronization_ && notificationPages.empty()))
        {
            notificationPages.push_back(this->CreateNotificationPage(
                move(partitionNotifications),
                ++pageIndex,
                initializeWithCacheVersions));
        }

        return notificationPages;
    }

private:

    size_t EstimateCurrentVersionsSize()
    {
        // This is an upper bound estimate since the version ranges after
        // any splitting due to paging will be less than or equal to
        // the original. The size contribution of the actual partition data
        // is expected to be much more than the version ranges, so this should not 
        // significantly affect paging calculations.
        //
        // We use this upper bound estimate to avoid repeated splitting and merging
        // as we calculate the page boundaries.
        //
        size_t estimatedCacheVersionsSize = cacheVersions_->EstimateSize();
        size_t estimatedUpdateVersionsSize = updateVersions_->EstimateSize();
        size_t estimatedVersionsSize = (estimatedCacheVersionsSize < estimatedUpdateVersionsSize)
            ? estimatedUpdateVersionsSize 
            : estimatedCacheVersionsSize;

        return estimatedVersionsSize;
    }

    NotificationPageSPtr CreateNotificationPage(
        vector<ServiceTableEntryNotificationSPtr> && partitionNotifications,
        uint64 pageIndex,
        bool initializeWithCacheVersions)
    {
        // The service notification will be initialized with either
        // the full cache versions or the update versions depending
        // on the state of this client (sychronized?) and this
        // notification's position in the pending queue (first?).
        //
        // Whichever versions were NOT initialized into the notification body
        // will be backed up in case we need to switch the body contents.
        // The main scenario for this is when the notification has moved
        // to the front of the pending queue and/or the client has 
        // synchronized by the time we send (or retry sending) this notification.
        //
        auto notification = make_shared<ServiceNotification>(
            ServiceNotificationPageId(activityId_, pageIndex),
            cacheGeneration_,
            initializeWithCacheVersions ? move(cacheVersions_) : move(updateVersions_),
            move(partitionNotifications));

        // Cache versions will (intentionally) be empty if they've been
        // used in the initial notification already.
        //
        auto notificationPage = make_shared<NotificationPage>(
            move(notification), 
            move(cacheVersions_));

        return notificationPage;
    }

    ActivityId activityId_;
    GenerationNumber cacheGeneration_;
    VersionRangeCollectionSPtr cacheVersions_;
    MatchedServiceTableEntryMap matchedPartitions_;
    VersionRangeCollectionSPtr updateVersions_;
    bool isForClientSynchronization_;
};

//
// SendNotificationPageAsyncOperation 
//

class ServiceNotificationSender::SendNotificationPageAsyncOperation
    : public AsyncOperation
    , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
{
public:
    SendNotificationPageAsyncOperation(
        ServiceNotificationSender & owner,
        NotificationPageSPtr const & notificationPage,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
        , owner_(owner)
        , pageId_(notificationPage->GetPageId())
        , initialNotification_(notificationPage->TakeNotification())
        , cacheVersionsBackup_(notificationPage->TakeCacheVersionsBackup())
        , body_()
        , request_()
        , retryTimer_()
        , timerLock_()
    {
    }

    // No return value - we retry indefinitely until the notification succeeds or the client
    // is disconnected.
    //
    static void End(AsyncOperationSPtr const & operation, __out ServiceNotificationPageId & pageId)
    {
        auto casted = AsyncOperation::End<SendNotificationPageAsyncOperation>(operation);

        pageId = casted->pageId_;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (initialNotification_)
        {
            // The estimated client versions are used for server-side filtering of notifications
            // based on version (i.e. filtering primary-only changes).
            //
            // This queued notification will eventually be delivered due to infinite retries. 
            // Therefore, we can update the estimated client versions before actually sending.
            // Subsequent notifications can be filtered out based on the assumption that this 
            // notification will eventually reach the client.
            //
            // Note that this is NOT for duplicate notification detection, which is different
            // from version-based filtering. Duplicate notification detection must happen
            // on the client based on actual delivered notifications.
            //
            owner_.UpdateEstimatedClientVersions(
                pageId_,
                initialNotification_->GetVersions());

            body_ = make_shared<ServiceNotificationRequestBody>(move(initialNotification_));

            this->BuildRequestFromCurrentBody();

            this->SendNotificationPage(thisSPtr);
        }
        else
        {
            WriteNoise(
                TraceComponent,
                owner_.TraceId,
                "{0}: dropping empty notification",
                pageId_);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

private:

    void SendNotificationPage(AsyncOperationSPtr const & thisSPtr)
    {
        if (owner_.IsStopped)
        {
            WriteNoise(
                TraceComponent,
                owner_.TraceId,
                "{0}: sender stopped - dropping notification",
                pageId_);

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            Stopwatch stopwatch;
            stopwatch.Start();

            auto timeout = ServiceModelConfig::GetConfig().ServiceNotificationTimeout;

            this->RebuildRequestIfNeeded();

            ServiceNotificationPageId nextPage;
            auto pendingCount = owner_.GetPendingNotificationPagesCount(nextPage);

            WriteNoise(
                TraceComponent,
                owner_.TraceId,
                "{0}: sending notification (pending={1}{2})",
                pageId_,
                pendingCount,
                (pendingCount > 0 ? wformatString(" next={0}", nextPage) : L""));

            auto operation = owner_.transport_->BeginNotification(
                request_->Clone(),
                timeout,
                [this, stopwatch](AsyncOperationSPtr const & operation) { this->OnNotificationComplete(stopwatch, operation, false); },
                thisSPtr);
            this->OnNotificationComplete(stopwatch, operation, true);
        }
    }

    void OnNotificationComplete(
        Stopwatch const & stopwatch,
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        MessageUPtr reply;
        auto error = owner_.transport_->EndNotification(operation, reply);

        if (error.IsSuccess())
        {
            ServiceNotificationReplyBody body;
            if (reply->GetBody(body))
            {
                error = body.Error;
            }
            else
            {
                error = ErrorCode::FromNtStatus(reply->GetStatus());
            }
        }

        if (error.IsSuccess())
        {
            ServiceNotificationPageId nextPage;
            auto pendingCount = owner_.GetPendingNotificationPagesCount(nextPage);

            WriteNoise(
                TraceComponent,
                owner_.TraceId,
                "{0}: notification succeeded (pending={1}{2})",
                pageId_,
                pendingCount,
                (pendingCount > 0 ? wformatString(" next={0}", nextPage) : L""));

            this->TryComplete(operation->Parent, error);
        }
        else if (IsFaultTargetError(error))
        {
            WriteWarning(
                TraceComponent,
                owner_.TraceId,
                "{0}: faulting client registration on {1}",
                pageId_,
                error);

            owner_.RaiseTargetFault();

            // This error is not observed. The client will be disconnected
            // and all notifications are dropped along with the connection.
            //
            this->TryComplete(operation->Parent, ErrorCodeValue::Success);
        }
        else if (owner_.IsStopped)
        {
            WriteNoise(
                TraceComponent,
                owner_.TraceId,
                "{0}: sender stopped - dropping notification",
                pageId_);

            this->TryComplete(operation->Parent, ErrorCodeValue::Success);
        }
        else
        {
            auto timeout = ServiceModelConfig::GetConfig().ServiceNotificationTimeout;
            auto delay = timeout.SubtractWithMaxAndMinValueCheck(stopwatch.Elapsed);

            if (delay < TimeSpan::Zero)
            {
                delay = TimeSpan::Zero;
            }

            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "{0}: scheduling retry in {1}: {2}",
                pageId_,
                delay,
                error);

            this->ScheduleRetry(operation->Parent, delay);
        }
    }

    void ScheduleRetry(AsyncOperationSPtr const & thisSPtr, TimeSpan const delay)
    {
        AcquireExclusiveLock lock(timerLock_);

        if (retryTimer_)
        {
            retryTimer_->Cancel();
        }

        retryTimer_ = Timer::Create(TimerTagDefault, [this, thisSPtr](TimerSPtr const & timer)
            {
                timer->Cancel();
                this->SendNotificationPage(thisSPtr);
            });
        retryTimer_->Change(delay);
    }

    void RebuildRequestIfNeeded()
    {
        bool bodyUpdated = false;

        if (owner_.CanSendFullCacheVersions(pageId_))
        {
            bodyUpdated = this->RebuildBodyWithCacheVersionsIfNeeded();
        }
        else
        {
            //
            // If the notification was initialized with the full cache versions, 
            // it should not have to be rebuilt with the update versions since
            // that would mean it is no longer at the front of the pending
            // queue or the client became unsynchronized - neither of
            // which should ever happen.
            //
        }

        if (bodyUpdated)
        {
            this->BuildRequestFromCurrentBody();
        }
    }

    bool RebuildBodyWithCacheVersionsIfNeeded()
    {
        if (cacheVersionsBackup_)
        {
            auto currentNotification = body_->TakeNotification();

            WriteInfo(
                TraceComponent,
                owner_.TraceId,
                "{0}: rebuilding request with cache versions={1}",
                pageId_,
                *cacheVersionsBackup_);

            auto newNotification = make_shared<ServiceNotification>(
                currentNotification->GetNotificationPageId(),
                currentNotification->GetGenerationNumber(),
                move(cacheVersionsBackup_),
                currentNotification->TakePartitions());

            body_ = make_shared<ServiceNotificationRequestBody>(move(newNotification));

            return true;
        }
        else
        {
            // existing request already contains cache versions
            //
            return false;
        }
    }

    void BuildRequestFromCurrentBody()
    {
        // This will serialize (copy) the body into the request message
        // but leave the original body_ intact. We use it to change
        // the version range of the request if needed when (if) the
        // notification is retried.
        // 
        request_ = NamingTcpMessage::GetServiceNotificationRequest(Common::make_unique<ServiceNotificationRequestBody>(*body_))->GetTcpMessage(); // todo: refactor to avoid copy
        
        // Generate a new message Id explicitly only when the request
        // content changes.
        //
        // Keep the message Id unchanged to take advantage of transport-level 
        // duplicate detection during retries of the same request content.
        //
        request_->Headers.Add(MessageIdHeader(MessageId()));
        owner_.WrapForEntreeServiceProxy(request_);
    }

    static bool IsFaultTargetError(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
        case ErrorCodeValue::CannotConnectToAnonymousTarget:
        case ErrorCodeValue::TransportSendQueueFull:
            return true;

        default:
            return false;
        }
    }

    ServiceNotificationSender & owner_;

    ServiceNotificationPageId pageId_;
    ServiceNotificationSPtr initialNotification_;
    VersionRangeCollectionSPtr cacheVersionsBackup_;

    ServiceNotificationRequestBodySPtr body_;
    MessageUPtr request_;

    TimerSPtr retryTimer_;
    ExclusiveLock timerLock_;
};

//
// ServiceNotificationSender
//

ServiceNotificationSender::ServiceNotificationSender(
    std::wstring const & traceId,
    EntreeServiceTransportSPtr const & transport,
    ClientIdentityHeader const & clientIdentity,
    Common::VersionRangeCollectionSPtr const & initialClientVersions,
    TargetFaultHandler const & targetFaultHandler,
    Common::ComponentRoot const & root)
    : RootedObject(root)
    , traceId_(traceId)
    , transport_(transport)
    , clientIdentity_(clientIdentity)
    , notificationQueue_(ReaderQueue<QueuedNotification>::Create())
    , isStopped_(false)
    , pendingNotificationPages_()
    , completedNotificationPages_()
    , synchronizationPageId_()
    , isClientSynchronized_(false)
    , notificationPagesLock_()
    , estimatedClientVersions_(initialClientVersions)
    , clientVersionsLock_()
    , targetFaultHandler_(targetFaultHandler)
{
}

void ServiceNotificationSender::Start()
{
    WriteInfo(
        TraceComponent,
        this->TraceId,
        "starting notification sender");

    this->SchedulePumpNotifications();
}

void ServiceNotificationSender::Stop()
{
    if (!isStopped_)
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "stopping notification sender");

        notificationQueue_->Abort();

        isStopped_ = true;
    }
}

void ServiceNotificationSender::CreateAndEnqueueNotificationForClientSynchronization(
    ActivityId const & activityId,
    GenerationNumber const & cacheGeneration,
    VersionRangeCollectionSPtr && cacheVersions,
    MatchedServiceTableEntryMap && matchedPartitions,
    VersionRangeCollectionSPtr && updateVersions)
{
    this->CreateAndEnqueueNotification(
        activityId,
        cacheGeneration,
        move(cacheVersions),
        move(matchedPartitions),
        move(updateVersions),
        true);
}

void ServiceNotificationSender::CreateAndEnqueueNotification(
    ActivityId const & activityId,
    GenerationNumber const & cacheGeneration,
    VersionRangeCollectionSPtr && cacheVersions,
    MatchedServiceTableEntryMap && matchedPartitions,
    VersionRangeCollectionSPtr && updateVersions)
{
    this->CreateAndEnqueueNotification(
        activityId,
        cacheGeneration,
        move(cacheVersions),
        move(matchedPartitions),
        move(updateVersions),
        false);
}

void ServiceNotificationSender::CreateAndEnqueueNotification(
    ActivityId const & activityId,
    GenerationNumber const & cacheGeneration,
    VersionRangeCollectionSPtr && cacheVersions,
    MatchedServiceTableEntryMap && matchedPartitions,
    VersionRangeCollectionSPtr && updateVersions,
    bool isForClientSynchronization)
{
    if (notificationQueue_->EnqueueWithoutDispatch(make_unique<QueuedNotification>(
        activityId,
        cacheGeneration,
        move(cacheVersions),
        move(matchedPartitions),
        move(updateVersions),
        isForClientSynchronization)))
    {
        auto root = this->Root.CreateComponentRoot();
        Threadpool::Post([this, root]() { notificationQueue_->Dispatch(); });
    }
}

void ServiceNotificationSender::SchedulePumpNotifications()
{
    auto root = this->Root.CreateComponentRoot();
    Threadpool::Post([this, root]() { this->PumpNotifications(); });
}

void ServiceNotificationSender::PumpNotifications()
{
    bool shouldPump = false;
    
    do
    {

        auto operation = notificationQueue_->BeginDequeue(
            TimeSpan::MaxValue,
            [this](AsyncOperationSPtr const & operation) { this->OnDequeueComplete(operation, false); },
            this->Root.CreateAsyncOperationRoot());

        shouldPump = this->OnDequeueComplete(operation, true);

    } while (shouldPump);
}

bool ServiceNotificationSender::OnDequeueComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    bool completedSynchronously = operation->CompletedSynchronously;

    if (completedSynchronously != expectedCompletedSynchronously) { return false; }

    QueuedNotificationUPtr queuedNotificationUPtr;
    auto error = notificationQueue_->EndDequeue(operation, queuedNotificationUPtr);

    if (!error.IsSuccess())
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "dequeue failed - stopping pump: {0}",
            error);

        return false;
    }

    if (!queuedNotificationUPtr)
    {
        WriteInfo(
            TraceComponent,
            this->TraceId,
            "pumped null - stopping pump");

        return false;
    }

    this->SendNotification(move(queuedNotificationUPtr));

    if (!completedSynchronously)
    {
        this->SchedulePumpNotifications();
    }

    return completedSynchronously;
}

void ServiceNotificationSender::SendNotification(QueuedNotificationUPtr && queuedNotificationUPtr)
{
    auto notificationPages = queuedNotificationUPtr->CreateNotificationPages(*this);

    if (this->TryStartSendNotificationPages(notificationPages, queuedNotificationUPtr->IsForClientSynchronization))
    {
        for (auto const & page : notificationPages)
        {
            auto operation = AsyncOperation::CreateAndStart<SendNotificationPageAsyncOperation>(
                *this,
                move(page),
                [this](AsyncOperationSPtr const & operation) { this->OnSendNotificationPageComplete(operation, false); },
                this->Root.CreateAsyncOperationRoot());
            this->OnSendNotificationPageComplete(operation, true);
        }
    }
    else
    {
       this->RaiseTargetFault();
    }
}

void ServiceNotificationSender::OnSendNotificationPageComplete(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    ServiceNotificationPageId pageId;
    SendNotificationPageAsyncOperation::End(operation, pageId);

    this->FinishSendNotificationPage(pageId);
}

bool ServiceNotificationSender::TryStartSendNotificationPages(
    vector<NotificationPageSPtr> const & notificationPages,
    bool isForClientSynchronization)
{
    AcquireWriteLock lock(notificationPagesLock_);

    auto pendingLimit = static_cast<size_t>(ServiceModelConfig::GetConfig().MaxOutstandingNotificationsPerClient);

    if (pendingNotificationPages_.size() + notificationPages.size() > pendingLimit)
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "outstanding notification limit exceeded: limit={0} pending={1} incoming={2}",
            pendingLimit,
            pendingNotificationPages_.size(),
            notificationPages.size());

        //
        // target_->Reset() here will generally close the underlying connections. However,
        // a misbehaving client that is deadlocked on the receive thread will prevent
        // this from happening. In such cases, we must at least clean up
        // registration resources. Raise a fault to ServiceNotificationManager so it
        // can remove the offending client registration.
        //

        return false;
    }

    for (auto const & page : notificationPages)
    {
        pendingNotificationPages_.push(page->GetPageId());
    }

    if (isForClientSynchronization && !notificationPages.empty())
    {
        auto pageId = notificationPages.back()->GetPageId();

        WriteInfo(
            TraceComponent,
            this->TraceId,
            "synchronization page ID={0}",
            pageId);

        synchronizationPageId_ = make_unique<ServiceNotificationPageId>(pageId);
    }

    return true;
}

void ServiceNotificationSender::FinishSendNotificationPage(ServiceNotificationPageId const & pageId)
{
    AcquireWriteLock lock(notificationPagesLock_);

    if (pendingNotificationPages_.front() == pageId)
    {
        this->CheckSynchronizationCompleteCallerHoldsLock(pageId);

        pendingNotificationPages_.pop();

        size_t count = 1;
        ServiceNotificationPageId endPageId = pageId;

        bool isDone = false;
        while (!pendingNotificationPages_.empty() && !isDone)
        {
            auto findIt = completedNotificationPages_.find(pendingNotificationPages_.front());
            if (findIt != completedNotificationPages_.end())
            {
                this->CheckSynchronizationCompleteCallerHoldsLock(*findIt);

                endPageId = *findIt;
                ++count;

                pendingNotificationPages_.pop();
                completedNotificationPages_.erase(findIt);
            }
            else
            {
                isDone = true;
            }
        }

        WriteNoise(
            TraceComponent,
            this->TraceId,
            "cleared pending notifications: [{0}, {1}] count={2} remaining={3}",
            pageId,
            endPageId,
            count,
            pendingNotificationPages_.size());
    }
    else
    {
        completedNotificationPages_.insert(pageId);
    }
}

void ServiceNotificationSender::CheckSynchronizationCompleteCallerHoldsLock(ServiceNotificationPageId const & pageId)
{
    if (synchronizationPageId_ && pageId == *synchronizationPageId_)
    {
        isClientSynchronized_ = true;

        synchronizationPageId_.reset();

        WriteInfo(
            TraceComponent,
            this->TraceId,
            "client synchronized at notification {0}",
            pageId);
    }
}

bool ServiceNotificationSender::CanInitializeFullCacheVersions()
{
    AcquireReadLock lock(notificationPagesLock_);

    return pendingNotificationPages_.empty() && isClientSynchronized_;
}

bool ServiceNotificationSender::CanSendFullCacheVersions(ServiceNotificationPageId const & pageId)
{
    AcquireReadLock lock(notificationPagesLock_);

    bool isFirstPendingNotification = (!pendingNotificationPages_.empty() && pendingNotificationPages_.front() == pageId);

    return isFirstPendingNotification && isClientSynchronized_;
}

void ServiceNotificationSender::RaiseTargetFault()
{
    if (targetFaultHandler_)
    {
        targetFaultHandler_(clientIdentity_);
        auto request = NamingMessage::GetDisconnectClient(Actor::EntreeServiceProxy);
        WrapForEntreeServiceProxy(request);
        auto operation = transport_->BeginNotification(
            move(request),
            ServiceModelConfig::GetConfig().ServiceNotificationTimeout,
            [this](AsyncOperationSPtr const & operation) { this->OnTargetFaultNotificationComplete(operation); },
            Root.CreateAsyncOperationRoot());
    }
}

void ServiceNotificationSender::OnTargetFaultNotificationComplete(AsyncOperationSPtr const &operation)
{
    MessageUPtr reply;
    auto error = transport_->EndNotification(operation, reply);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceComponent,
            this->TraceId,
            "{0}: TargetFault notification failed {1}",
            clientIdentity_.Id,
            error);
    }
}

VersionRangeCollectionSPtr ServiceNotificationSender::GetEstimatedClientVersions() const
{
    AcquireReadLock lock(clientVersionsLock_);

    return estimatedClientVersions_;
}

void ServiceNotificationSender::UpdateEstimatedClientVersions(
    ServiceNotificationPageId const & pageId,
    VersionRangeCollectionSPtr const & versions)
{
    AcquireWriteLock lock(clientVersionsLock_);

    WriteNoise(
        TraceComponent,
        this->TraceId,
        "{0}: updating estimated client versions: {1} + {2}",
        pageId,
        estimatedClientVersions_,
        *versions);

    estimatedClientVersions_->Merge(*versions);
}

size_t ServiceNotificationSender::GetPendingNotificationPagesCount(__out ServiceNotificationPageId & next) const
{
    AcquireReadLock lock(notificationPagesLock_);

    if (!pendingNotificationPages_.empty())
    {
        next = pendingNotificationPages_.front();
    }

    return pendingNotificationPages_.size();
}

void ServiceNotificationSender::WrapForEntreeServiceProxy(MessageUPtr &request)
{
    ForwardMessageHeader fwdHeader(request->Headers.Actor, request->Headers.Action);
    request->Headers.Replace(fwdHeader);
    request->Headers.Replace(ActorHeader(Actor::EntreeServiceProxy));
    request->Headers.Replace(clientIdentity_);
}
