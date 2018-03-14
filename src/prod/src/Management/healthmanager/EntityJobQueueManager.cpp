// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("EntityJobQueueManager");

// ------------------------------------------------------------------
// Inner class: ReadyQueue
// ------------------------------------------------------------------
EntityJobQueueManager::ReadyQueue::ReadyQueue()
    : items_()
    , comp_() 
{
}

EntityJobQueueManager::ReadyQueue::~ReadyQueue()
{
}

void EntityJobQueueManager::ReadyQueue::Push(SortedJobItemsRPtr && item)
{
    // Insert at the end and bubble up as needed
    int pos = static_cast<int>(items_.size());
    item->Position = pos;
    items_.push_back(std::move(item));
    ShiftUp(pos);
}

void EntityJobQueueManager::ReadyQueue::Pop()
{
    ASSERT_IF(items_.empty(), "EntityJobQueueManager.ReadyQueue: Pop can't be called as items_ is empty");
    int last = static_cast<int>(items_.size()) - 1;
    // Clean up the position, since the item is not in ready queue anymore.
    items_[last]->Position = -1;
    Swap(0, last);
    items_.pop_back();
    ShiftDown(0);
}

void EntityJobQueueManager::ReadyQueue::IncreasePriority(__in SortedJobItems & prev)
{
    CheckPos(prev.Position);
    ASSERT_IFNOT(items_[prev.Position]->JobItems.top() == prev.JobItems.top(), "ReadyQueue::IncreasePriority: the expected job item {0} is not at position {1}", prev.JobItems.top()->SequenceNumber, prev.Position);
    ShiftUp(prev.Position);
}

void EntityJobQueueManager::ReadyQueue::Swap(int i, int j)
{
    // Swap the items and keep track of position
#ifdef DBG
    CheckPos(i);
    CheckPos(j);
#endif

    std::swap(items_[i]->Position, items_[j]->Position);
    std::swap(items_[i], items_[j]);
}

void EntityJobQueueManager::ReadyQueue::ShiftUp(int lastPos)
{
    int child = lastPos;
    while (child > 0)
    {
        int parent = GetParent(child);
        if (comp_(items_[parent], items_[child]))
        {
            Swap(child, parent);
        }
        else
        {
            break;
        }

        child = parent;
    }

    CheckHeapProperty();
}

void EntityJobQueueManager::ReadyQueue::ShiftDown(int firstPos)
{
    int parent = firstPos;
    int lastPos = static_cast<int>(items_.size());
    while (parent < lastPos)
    {
        int leftChild = GetLeft(parent);
        if (leftChild >= lastPos)
        {
            break;
        }

        // Swap the parent with the child with the highest priority.
        int swapIndex = parent;
        if (comp_(items_[swapIndex], items_[leftChild]))
        {
            swapIndex = leftChild;
        }

        int rightChild = leftChild + 1;
        if (rightChild < lastPos && comp_(items_[swapIndex], items_[rightChild]))
        {
            swapIndex = rightChild;
        }

        if (swapIndex != parent)
        {
            Swap(swapIndex, parent);
            parent = swapIndex;
        }
        else
        {
            break;
        }
    }

    CheckHeapProperty();
}

void EntityJobQueueManager::ReadyQueue::CheckPos(int i) const
{
    ASSERT_IF(i < 0 || i >= static_cast<int>(items_.size()), "ReadyQueue::CheckPos: {0} is out of bounds, {1} items", i, items_.size());
}

void EntityJobQueueManager::ReadyQueue::CheckHeapProperty() const
{
#ifdef DBG
    if (items_.empty()) { return; }
    
    int size = static_cast<int>(items_.size());

    bool isLeaf = false;
    for (int i = 0; i < size; ++i)
    {
        // Look at all items to check that their Position is correctly set.
        // The item Position must be equal to the position in the vector
        ASSERT_IFNOT(items_[i]->Position == i, "ReadyQueue:CheckHeap: Item at position {0} doesn't have correct position set, {1} instead.", i, items_[i]->Position);

        // If the items have children, check that the parent is greater than the children.
        if (!isLeaf)
        {
            int leftChild = GetLeft(i);
            if (leftChild < size)
            {
                ASSERT_IF(comp_(items_[i], items_[leftChild]), "ReadyQueue:CheckHeap: Heap doesn't respect heap properties: item at position {0} should be greater than left child at position {1}", i, leftChild);

                int rightChild = leftChild + 1;
                if (rightChild < size)
                {
                    ASSERT_IF(comp_(items_[i], items_[rightChild]), "ReadyQueue:CheckHeap: Heap doesn't respect heap properties: item at position {0} should be greater than right child at position {1}", i, rightChild);
                }
            }
            else
            {
                // No need to check children anymore
                isLeaf = true;
            }
        }
    }
#endif
}

// ------------------------------------------------------------------
// EntityJobQueueManager
// ------------------------------------------------------------------
EntityJobQueueManager::EntityJobQueueManager(
    Store::PartitionedReplicaId const & partitionedReplicaId,
    __in HealthManagerReplica & healthManagerReplica,
    int maxThreadCount)
    : PartitionedReplicaTraceComponent(partitionedReplicaId)
    , healthManagerReplica_(healthManagerReplica)
    , workMap_()
    , readyQueue_()
    , currentThreadCount_(0)
    , maxThreadCount_(maxThreadCount)
    , sequenceNumber_(0)
    , lock_()
    , closed_(false)
    , shouldThrottle_(false)
{
    if (maxThreadCount_ == 0)
    {
        maxThreadCount_ = Environment::GetNumberOfProcessors();
    }

    HealthManagerReplica::WriteInfo(
        TraceComponent,
        "{0}: ctor, max threads={1}",
        this->PartitionedReplicaId.TraceId,
        maxThreadCount_);

    healthManagerReplica_.StoreWrapper.SetThrottleCallback([this] (bool shouldThrottle) { this->SetThrottle(shouldThrottle); });
}

EntityJobQueueManager::~EntityJobQueueManager()
{
}

void EntityJobQueueManager::AssertInvariants() const
{
    ASSERT_IF(
        currentThreadCount_ < 0, 
        "{0}: Current threads {1} cannot be negative",
        this->PartitionedReplicaId.TraceId,
        currentThreadCount_);

    ASSERT_IF(
        currentThreadCount_ > maxThreadCount_, 
        "{0}: Current threads {1} cannot exceed max threads {2}",
        this->PartitionedReplicaId.TraceId,
        currentThreadCount_,
        maxThreadCount_);
}

bool EntityJobQueueManager::HasThreadsForProcessing() const
{
    return currentThreadCount_ != maxThreadCount_;
}

void EntityJobQueueManager::Test_SetThrottle(bool shouldThrottle)
{
    SetThrottle(shouldThrottle);
}

void EntityJobQueueManager::SetThrottle(bool shouldThrottle)
{
    Common::AcquireWriteLock lock(lock_);
    shouldThrottle_ = shouldThrottle;
    HealthManagerReplica::WriteInfo(
        TraceComponent, 
        "{0}: shouldThrottle = {1}. {2} jobs ready",
        this->PartitionedReplicaId.TraceId,
        shouldThrottle,
        readyQueue_.Size());
    
    if (!shouldThrottle)
    {
        // If there are work items that couldn't be executed previously, try to execute them now
        TryEnqueueWork();
    }
}
   
bool EntityJobQueueManager::HasWork(std::wstring const & entityId) const
{
    Common::AcquireReadLock lock(lock_);
    return workMap_.find(entityId) != workMap_.end();
}

void EntityJobQueueManager::AddWork(IHealthJobItemSPtr && jobItem)
{
    Common::AcquireWriteLock lock(lock_);
    AssertInvariants();

    if (closed_)
    {
        // Nothing to do, do not accept the work
        return;
    }

    // Assign a monotonically increasing sequence number
    jobItem->SequenceNumber = ++sequenceNumber_;

    HealthManagerReplica::WriteNoise(
        TraceComponent,
        "{0}: AddWork: {1}: sn {2}",
        jobItem->JobId,
        jobItem->TypeString,
        jobItem->SequenceNumber);

    // Find the pending work list for this Entity in the map
    auto const & id = jobItem->JobId;
    auto it = workMap_.find(id);
    if (it == workMap_.end())
    {
        // If the Entity does not exist at all then the Entity has not been seen. Create a pending work list
        auto entry = make_unique<SortedJobItems>(move(jobItem));
        auto result = workMap_.insert(make_pair(id, move(entry)));

        // If this Entity was never been seen before, schedule it
        // If we have capacity then run a work item from the ready queue.
        // If the Entity is currently being processed then the work would have been enqueued to the end of the
        // pending work list for that Entity and when the work completes the Entity will be scheduled to run again
        MoveToReadyQueueAndTryEnqueueWork(result.first->second);
    }
    else
    {
        auto & sortedList = *it->second;
        if (sortedList.Position == -1)
        {
            // The item doesn't exist in ready queue.
            sortedList.JobItems.push(move(jobItem));
        }
        else
        {
            ASSERT_IF(sortedList.JobItems.empty(), "AddWork {0}+{1}: the sorted list for the entity has Position {2} and no items.", id, jobItem->SequenceNumber, sortedList.Position);
            auto prevFirst = sortedList.JobItems.top().get();
            sortedList.JobItems.push(move(jobItem));
            if (sortedList.JobItems.top().get() != prevFirst)
            {
                // The top item changed. Since priority may have changed,
                // the ready queue may need to be adjusted.
                // The priority can only increase, so we may need to shift up the item.
                readyQueue_.IncreasePriority(sortedList);
            }
        }
    }
}

void EntityJobQueueManager::OnWorkComplete(
    __in IHealthJobItem & jobItem,
    Common::ErrorCode const & error)
{
    { //lock
        Common::AcquireWriteLock lock(lock_);
        AssertInvariants();
   
        // Change the state on the job item
        jobItem.OnWorkComplete();

        // The current thread has freed up
        --currentThreadCount_;
    
        if (!closed_)
        {
            // Find the Entity in the map that we are maintaining and remove this work item
            auto const & id = jobItem.JobId;
            
            auto it = workMap_.find(id);
            ASSERT_IF(it == workMap_.end(), "{0}: Job item not found", jobItem);

            if (it->second->JobItems.empty())
            {
                workMap_.erase(it);
                // This entity has no further work
                TryEnqueueWork();
            }
            else
            {
                // This Entity has more work, so add it to the ready queue
                // The ready queue is a priority queue so it will be inserted in the appropriate location
                // based on the sequence number (order) of the first item in the pending work list
                // Since we have a free thread see if we can execute any work
                MoveToReadyQueueAndTryEnqueueWork(it->second);
            }
        }
    } // endlock

    // Complete the work item outside the lock
    jobItem.ProcessCompleteResult(error);
}

void EntityJobQueueManager::OnAsyncWorkStarted(__in IHealthJobItem & jobItem)
{
    Common::AcquireWriteLock lock(lock_);
    AssertInvariants();

    // Change the entity state to mark that the async processing is pending
    jobItem.OnAsyncWorkStarted();
           
    // Decrement the thread count (this Entity is no longer being processed by a Entity queue thread)
    --currentThreadCount_;

    if (closed_)
    {
        return;
    }
  
    // Enqueue other work if possible
    TryEnqueueWork();
}

void EntityJobQueueManager::OnAsyncWorkCompleted(
    IHealthJobItemSPtr const & jobItem,
    Common::AsyncOperationSPtr const & operation)
{
    Common::AcquireWriteLock lock(lock_);
    AssertInvariants();

    // The async work that completed needs to be run again to do post processing
    // NOTE: This is inserted to the front of the list because sequence numbers increase monotonically
    // and we do not allow two items of the same Entity to execute at once
    
    if (closed_)
    {
        return;
    }

    jobItem->OnAsyncWorkReadyToComplete(operation);
       
    // The async work that completed needs to be run again to do post processing
    // NOTE: This is inserted to the front of the list because sequence numbers increase monotonically
    // and we do not allow two items of the same Entity to execute at once
    auto const & id = jobItem->JobId;
    auto it = workMap_.find(id);
    if (it == workMap_.end())
    {
        Assert::CodingError("{0}: RescheduleWork: entity not found", *jobItem);
    }

    auto & sortedItems = *it->second;
    ASSERT_IF(sortedItems.Position != -1, "EntityJobQueueManager: OnAsyncWorkCompleted: job item {0} async completed, needs processing. List has position {1}, expected -1", *jobItem, sortedItems.Position);
    sortedItems.JobItems.push(jobItem);

    // This Entity needs to be scheduled again
    MoveToReadyQueueAndTryEnqueueWork(it->second);
}

void EntityJobQueueManager::Close()
{
    vector<IHealthJobItemSPtr> pendingWork;
    vector<AsyncOperationSPtr> pendingCommits;

    { // lock
        Common::AcquireWriteLock lock(lock_);
        AssertInvariants();

        closed_ = true;

        // Cancel all job items
        for (auto it = workMap_.begin(); it != workMap_.end(); ++it)
        {
            auto & queue = it->second->JobItems;
            while (!queue.empty())
            {
                IHealthJobItemSPtr jobItem = queue.top();
                // Pop the items from queue before changing the state with OnWorkCanceled,
                // as that may affect the order between canceled and non-canceled items in the queue.
                queue.pop();

                auto commitOp = jobItem->OnWorkCanceled();
                if (commitOp)
                {
                    pendingCommits.push_back(move(commitOp));
                }

                pendingWork.push_back(move(jobItem));
            }
        }

        workMap_.clear();
    } // endlock

    HealthManagerReplica::WriteInfo(
        TraceComponent,
        "{0}: job queue manager close: cancel {1} pending work items, {2} pending commit async operations",
        this->PartitionedReplicaId.TraceId,
        pendingWork.size(),
        pendingCommits.size());

    for (auto & commitOp : pendingCommits)
    {
        commitOp->Cancel();
    }

    // Process complete results for all items outside the lock.
    // This will complete the initial async operation that started the work (process report, etc).
    for (auto it = pendingWork.begin(); it != pendingWork.end(); ++it)
    {
        (*it)->ProcessCompleteResult(ErrorCode(ErrorCodeValue::ObjectClosed));
    }
}

void EntityJobQueueManager::TryEnqueueWork()
{
    if (!HasThreadsForProcessing())
    {
        // No capacity
        return;
    }

    if (readyQueue_.Empty())
    {
        return;
    }

    auto pendingWorkList = readyQueue_.Top();
    if (!shouldThrottle_ || pendingWorkList->JobItems.top()->CanExecuteOnStoreThrottled)
    {
        // Enqueue the first item from the ready read only queue
        readyQueue_.Pop();
        EnqueueFromWorkList(pendingWorkList);
    }
}

IHealthJobItemSPtr EntityJobQueueManager::GetTopToExecute(SortedJobItems const & list)
{
    if (list.JobItems.empty())
    {
        return nullptr;
    }

    if (!shouldThrottle_ || list.JobItems.top()->CanExecuteOnStoreThrottled)
    {
        return list.JobItems.top();
    }

    return nullptr;
}

void EntityJobQueueManager::MoveToReadyQueueAndTryEnqueueWork(
    SortedJobItemsUPtr const & mapEntry)
{
    ASSERT_IF(mapEntry->JobItems.empty(), "{0}: There should be at least one item in the map entry", this->PartitionedReplicaId.TraceId);
    
    if (!HasThreadsForProcessing())
    {
        // No threads available so simply add to the ready queue in the appropriate location
        readyQueue_.Push(mapEntry.get());
        return;
    }

    IHealthJobItemSPtr pendingWork = GetTopToExecute(*mapEntry);
    
    IHealthJobItemSPtr topReady;
    if (!readyQueue_.Empty())
    {
        topReady = GetTopToExecute(*readyQueue_.Top());
    }

    if (pendingWork && (!topReady || (*topReady > *pendingWork)))
    {
        // The first item in the ready queue has a sequence number greater than the current item
        // or there are no other items ready
        // Thus the current item needs to be enqueued to avoid a push and a pop in the ready queue
        EnqueueFromWorkList(mapEntry.get());
    }
    else
    {
        // Enqueue the work item in the appropriate queue
        readyQueue_.Push(mapEntry.get());
        TryEnqueueWork();
    }
}

void EntityJobQueueManager::EnqueueFromWorkList(
    __inout EntityJobQueueManager::SortedJobItemsRPtr pendingWorkList)
{
    ASSERT_IF(
       pendingWorkList == NULL, 
        "{0}: EnqueueFromWorkList: Pending work cannot be empty",
        this->PartitionedReplicaId.TraceId);

    // Execute the first work
    auto work = std::move(pendingWorkList->JobItems.top());

    HealthManagerReplica::WriteNoise(
        TraceComponent,
        "EnqueueFromWorkList: execute {0} {1}, sn {2}",
        work->JobId,
        work->TypeString,
        work->SequenceNumber);

    pendingWorkList->JobItems.pop();

    if (!work->CanExecuteOnStoreThrottled)
    {
        // The work is just started, so it can be combined with next items
        int batchCount = ManagementConfig::GetConfig().MaxEntityJobItemBatchCount;
        for (int i = 0; i < batchCount && !pendingWorkList->JobItems.empty(); ++i)
        {
            if (work->CanCombine(pendingWorkList->JobItems.top()))
            {
                auto nextWork = move(pendingWorkList->JobItems.top());
                pendingWorkList->JobItems.pop();
                work->Append(move(nextWork));
            }
            else
            {
                break;
            }
        }
    }

    ++currentThreadCount_;
        
    // Enqueue the job for processing
    // Keep the replica alive, which in turn will keep its root alive
    auto root = healthManagerReplica_.CreateComponentRoot();
    Threadpool::Post([root, work, this] () { work->ProcessJob(work, this->healthManagerReplica_); });
}
