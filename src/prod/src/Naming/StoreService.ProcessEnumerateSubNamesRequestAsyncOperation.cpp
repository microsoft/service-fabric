// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace Store;

    StringLiteral const TraceComponent("ProcessEnumerateSubNames");

    StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::ProcessEnumerateSubNamesRequestAsyncOperation(
        MessageUPtr && request,
        __in NamingStore & namingStore,
        StoreServiceProperties & properties,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ProcessRequestAsyncOperation(
            std::move(request),
            namingStore,
            properties,
            timeout,
            callback,
            root)
        , body_()
        , allSubnames_()
        , tentativeSubnames_()
        , pendingNameReads_(0)
        , failedTentativeSubnames_()
        , hasFailedPendingNameReads_(false)
        , tentativeSubnamesLock_()
        , lastEnumeratedName_()
        , enumerationStatus_(FABRIC_ENUMERATION_INVALID)
        , subnamesVersion_(0)
        , replySizeThreshold_(0)
        , replyItemsSizeEstimate_(0)
        , replyTotalSizeEstimate_(0)
    {
    }

    ErrorCode StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::HarvestRequestMessage(Transport::MessageUPtr && request)
    {
        if (request->GetBody(body_))
        {
            this->SetName(body_.NamingUri);

            replySizeThreshold_ = NamingUtility::GetMessageContentThreshold();

            size_t maxReplySizeEstimate = EnumerateSubNamesResult::EstimateSerializedSize(CommonConfig::GetConfig().MaxNamingUriLength);

            if (!NamingUtility::CheckEstimatedSize(maxReplySizeEstimate, replySizeThreshold_).IsSuccess())
            {
                TRACE_WARNING_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: (MaxMessageSize * MessageContentBufferRatio) is too small: allowed = {1} max reply estimate = {2}",
                    this->TraceId,
                    replySizeThreshold_,
                    maxReplySizeEstimate);
            }

            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InvalidMessage;
        }
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::PerformRequest(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceComponent,
            "{0}: Enumerating subnames at '{1}': continuation token = {2}",
            this->TraceId,
            this->NameString,
            body_.ContinuationToken);

        TransactionSPtr txSPtr;
        EnumerationSPtr enumSPtr;
        ErrorCode error = Store.CreateTransaction(this->ActivityHeader, txSPtr);

        if (error.IsSuccess())
        {
            error = InitializeEnumeration(txSPtr, enumSPtr);
        }

        if (error.IsSuccess())
        {
            error = EnumerateSubnames(move(enumSPtr));
        }
        else if (error.IsError(ErrorCodeValue::OperationCanceled))
        {
            NamingStore::CommitReadOnly(move(txSPtr));

            // The parent name is still tentative:
            //
            // could alternatively schedule a blind retry for PerformRequest() here, but
            // acquiring the corresponding named lock first increases our chances of success
            // upon retrying
            //
            auto inner = this->Store.BeginAcquireNamedLock(
                this->Name,
                this->TraceId,
                this->GetRemainingTime(),
                [this] (AsyncOperationSPtr const & operation) { this->OnNamedLockAcquiredForParentName(operation, false); },
                thisSPtr);
            this->OnNamedLockAcquiredForParentName(inner, true);

            return;
        }

        NamingStore::CommitReadOnly(move(txSPtr));

        if (error.IsSuccess())
        {
            StartResolveTentativeSubnames(thisSPtr); 
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: subname enumeration failed with {1}",
                this->TraceId,
                error);

            this->TryComplete(thisSPtr, error);
        }
    }

    ErrorCode StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::InitializeEnumeration(
        TransactionSPtr const & txSPtr,
        __out EnumerationSPtr & enumSPtr)
    {
        HierarchyNameData nameData;
        ErrorCode error = this->ReadHierarchyNameData(txSPtr, this->Name, nameData);

        if (!error.IsSuccess())
        {
            return error;
        }

        switch (nameData.NameState)
        {
            case HierarchyNameState::Created:
                this->SetConsistentFlag(true);
                break;

            case HierarchyNameState::Deleting:
            case HierarchyNameState::Creating:
                //
                // We must wait for the name to be consistent since it may or may not actually exist. For example,
                // DeleteName could set the tentative flag on the Authority Owner but fail at the Name Owner due
                // to NameNotEmpty from named properties existing.
                //
                WriteInfo(
                    TraceComponent,
                    "{0}: Name {1} is still tentative ({2})",
                    this->TraceId,
                    this->NameString,
                    nameData.NameState);
                return ErrorCodeValue::OperationCanceled;

            default:
                TRACE_WARNING_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: Name {1} is in invalid state {2}",
                    this->TraceId,
                    this->NameString,
                    nameData.NameState);
                return ErrorCodeValue::OperationFailed;
        }

        WriteNoise(
            TraceComponent,
            "{0}: subnames version = {1}",
            this->TraceId,
            nameData.SubnamesVersion);

        if (body_.ContinuationToken.IsValid)
        {
            // Versions must stay the same across multiple continuations for the enumeration
            // to be consistent
            //
            subnamesVersion_ = body_.ContinuationToken.SubnamesVersion;
            
            // The enumeration consistency status is only affected by subname writes at this name.
            // Changes to properties or services do not affect the enumeration status.
            //
            if (nameData.SubnamesVersion != body_.ContinuationToken.SubnamesVersion)
            {
                this->SetConsistentFlag(false);
            }
            
            // Start from the last subname we returned in previous enumeration request
            //
            error  = Store.CreateEnumeration(
                txSPtr,
                Constants::HierarchyNameEntryType,
                body_.ContinuationToken.LastEnumeratedName.ToString(),
                enumSPtr);
        }
        else
        {
            // Initialize versions using starting point of enumeration
            //
            subnamesVersion_ = nameData.SubnamesVersion;

            // Start at parent name
            //
            error = Store.CreateEnumeration(
                txSPtr,
                Constants::HierarchyNameEntryType,
                this->NameString,
                enumSPtr);
        }

        return error;
    }

    ErrorCode StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::EnumerateSubnames(EnumerationSPtr && enumSPtr)
    {
        allSubnames_.clear();
        tentativeSubnames_.clear();
        
        lastEnumeratedName_ = this->Name;

        ErrorCode error;

        //
        // Names are stored in the database in lexicographic order of their string representations.
        // Subname enumeration is implemented by starting at the entry corresponding to the root name of
        // the enumeration operation and scanning successive entries in lexicographic order.
        // Note that all children in the tree below an enumeration root must appear after the enumeration root
        // entry itself, but the children may not necessarily be contiguous. For example:
        //
        // fabric:/a/b
        // fabric:/a/b.c
        // fabric:/a/b/c
        //
        // This is the ordering of these three names stored in the database. However, fabric:/a/b.c
        // is NOT a child of fabric:/a/b, whereas fabric:/a/b/c is a child. This can happen whenever
        // a character that lexicographically precedes '/' appears in the segment of a name.
        //
        // Fragments are considered children. Otherwise, there would be no way to enumerate the fragments
        // on a name.
        //
        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            HierarchyNameData subnameData;
            if (!(error = Store.ReadCurrentData<HierarchyNameData>(enumSPtr, subnameData)).IsSuccess())
            {
                return error;
            }
         
            wstring subnameStr;
            if (!(error = enumSPtr->CurrentKey(subnameStr)).IsSuccess())
            {
                return error;
            }

            NamingUri subname;
            if (!NamingUri::TryParse(subnameStr, subname))
            {
                TRACE_INFO_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: Could not parse subname '{1}' into a Naming name",
                    this->TraceId,
                    subnameStr);
                continue;
            }

            // Skip parent name and last enumerated name (if any)
            //
            if (subname == this->Name || (body_.ContinuationToken.IsValid && subname == body_.ContinuationToken.LastEnumeratedName))
            {
                continue;
            }

            if (Name.IsPrefixOf(subname) && (body_.DoRecursive || Name == subname.GetParentName()))
            {
                bool addedToReply = false;

                switch (subnameData.NameState)
                {
                case HierarchyNameState::Created:
                    if ((addedToReply = FitsInReplyMessage(subname)) == true)
                    {
                        allSubnames_.push_back(subnameStr);
                    }
                    break;

                case HierarchyNameState::Creating:
                case HierarchyNameState::Deleting:
                    if ((addedToReply = FitsInReplyMessage(subname)) == true)
                    {
                        WriteNoise(
                            TraceComponent,
                            "{0}: adding tentative '{1}' ({2})",
                            TraceId,
                            subnameStr,
                            subnameData.NameState);

                        allSubnames_.push_back(subnameStr);
                        tentativeSubnames_.insert(make_pair(subname, subnameData));
                    }
                    break;

                default:
                    TRACE_WARNING_AND_TESTASSERT(
                        TraceComponent,
                        "{0}: Name {1} is in invalid state {2}",
                        this->TraceId,
                        subnameStr,
                        subnameData.NameState);
                    continue;
                }

                if (addedToReply)
                {
                    lastEnumeratedName_ = move(subname);
                }
                else
                {
                    // Next subName does not fit in reply message
                    //
                    this->SetFinishedFlag(false);
                    break;
                }
            }
            else if (StringUtility::StartsWith(subnameStr, this->NameString))
            {
                // Do not add this name, but keep scanning until we reach a name for
                // which the root name is not a prefix
                //
                WriteNoise(
                    TraceComponent,
                    "{0}: Subname enumeration skipping '{1}'",
                    TraceId,
                    subnameStr);
            }
            else
            {
                WriteNoise(
                    TraceComponent,
                    "{0}: Subname enumeration stopping at '{1}'",
                    TraceId,
                    subnameStr);

                this->SetFinishedFlag(true);
                break;
            }
        } // end while movenext
        
        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            this->SetFinishedFlag(true);

            error = ErrorCodeValue::Success;
        }

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: enumeration complete: status = {1} all count = {2} tentative count = {3} estimated size = {4}",
                TraceId,
                static_cast<int>(enumerationStatus_),
                allSubnames_.size(),
                tentativeSubnames_.size(),
                replyTotalSizeEstimate_);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: Subname enumeration returned error {1}",
                TraceId,
                error);
        }

        return error;
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::StartResolveTentativeSubnames(AsyncOperationSPtr const & thisSPtr)
    {
        // Enumeration scans through all subnames maintaining a list of names that are in the tentative state (if any)
        // during the scan. Once the enumeration is complete for this batch, we acquire the corresponding named
        // locks and check the tentative names again under the lock to determine their final status.
        //
        if (tentativeSubnames_.empty())
        {
            this->SetReplyMessage();

            this->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
        else
        {
            pendingNameReads_.store(static_cast<LONG>(tentativeSubnames_.size()));
            
            for (auto iter = tentativeSubnames_.begin(); iter != tentativeSubnames_.end(); ++iter)
            {
                NamingUri const & subname = iter->first;
                HierarchyNameData const & tentativeNameData = iter->second;

                WriteNoise(
                    TraceComponent,
                    "{0}: Acquiring lock for tentative subname[{1}]",
                    TraceId,
                    subname);

                auto inner = Store.BeginAcquireNamedLock(
                    subname, 
                    this->TraceId,
                    GetRemainingTime(),
                    [this, subname, tentativeNameData] (AsyncOperationSPtr const & lockOperation)
                    {
                        this->OnNamedLockAcquiredForTentativeSubname(subname, tentativeNameData, lockOperation, false);
                    },
                    thisSPtr);
                this->OnNamedLockAcquiredForTentativeSubname(subname, tentativeNameData, inner, true); 
            }
        }
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::OnNamedLockAcquiredForParentName(
        AsyncOperationSPtr const & lockOperation,
        bool expectedCompletedSynchronously)
    {
        if (lockOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = lockOperation->Parent;

        ErrorCode error = Store.EndAcquireNamedLock(this->Name, this->TraceId, lockOperation);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: lock acquired for {1} during enumeration.",
                TraceId,
                this->Name);

            Store.ReleaseNamedLock(this->Name, this->TraceId, thisSPtr);

            this->PerformRequest(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::OnNamedLockAcquiredForTentativeSubname(
        NamingUri const & subName,
        HierarchyNameData const & tentativeNameData,
        AsyncOperationSPtr const & lockOperation,
        bool expectedCompletedSynchronously)
    {
        if (lockOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = lockOperation->Parent;

        ErrorCode error = Store.EndAcquireNamedLock(subName, this->TraceId, lockOperation);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
        else if (this->IsCompleted)
        {
            // Nothing to do since we already completed with an error acquiring some other named lock
            //
            Store.ReleaseNamedLock(subName, this->TraceId, thisSPtr);
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0}: lock acquired for subname {1} during enumeration.",
                TraceId,
                subName);

            TransactionSPtr txSPtr;

            error = Store.CreateTransaction(this->ActivityHeader, txSPtr);

            HierarchyNameData currentNameData;

            if (error.IsSuccess())
            {
                error = this->ReadHierarchyNameData(txSPtr, subName, currentNameData);
            }
            
            NamingStore::CommitReadOnly(move(txSPtr));

            Store.ReleaseNamedLock(subName, this->TraceId, thisSPtr);

            if (error.IsError(ErrorCodeValue::NameNotFound))
            {
                {
                    AcquireExclusiveLock lock(tentativeSubnamesLock_);

                    auto it = find(allSubnames_.begin(), allSubnames_.end(), subName.ToString());

                    if (it != allSubnames_.end())
                    {
                        allSubnames_.erase(it);
                    }
                }

                switch (tentativeNameData.NameState)
                {
                    case HierarchyNameState::Creating:
                        // Nothing to do since name failed creation
                        break;

                    case HierarchyNameState::Deleting:
                        // The subname version will have changed on successful deletion of the new name
                        this->SetConsistentFlag(false);
                        break;

                    default:
                        TRACE_WARNING_AND_TESTASSERT(
                            TraceComponent,
                            "{0}: tentative subname {1} is in state {2}",
                            this->TraceId,
                            subName.ToString(),
                            tentativeNameData.NameState);
                        break;
                }

                error = ErrorCodeValue::Success;
            }
            else if (error.IsSuccess())
            {
                switch (currentNameData.NameState)
                {
                    case HierarchyNameState::Created:
                        if (tentativeNameData.NameState == HierarchyNameState::Creating)
                        {
                            this->SetConsistentFlag(false);
                        }

                        break;

                    default:
                        WriteInfo(
                            TraceComponent,
                            "{0}: expected consistent subname {1}: ({2}). Retry needed",
                            this->TraceId,
                            subName,
                            currentNameData.NameState);

                        // There are valid cases where the subname is in inconsistent state - example, the replicator is closing down
                        // but the replicated store wasn't yet closed.
                        // Set a flag to indicate that retry is needed.
                        // Also remember the failed tentative name for retry.
                        //
                        hasFailedPendingNameReads_.store(true);

                        {
                            AcquireExclusiveLock lock(tentativeSubnamesLock_);
                            failedTentativeSubnames_.insert(make_pair(subName, tentativeNameData));
                        }

                        break;
                }
            } // if read current name success 

            if (error.IsSuccess())
            {
                if (--pendingNameReads_ == 0)
                {    
                    if (this->IsCompleted)
                    {
                        return;
                    }

                    if (hasFailedPendingNameReads_.load())
                    {
                        // Schedule retry, there are still tentative names to resolve
                        {
                            AcquireExclusiveLock lock(tentativeSubnamesLock_);
                            tentativeSubnames_ = move(failedTentativeSubnames_);
                        }

                        hasFailedPendingNameReads_.store(false);
            
                        this->TimeoutOrScheduleRetry(thisSPtr,
                            [this](AsyncOperationSPtr const & thisSPtr) { this->StartResolveTentativeSubnames(thisSPtr); });
                        return;
                    }
                    
                    if (this->TryStartComplete())
                    {
                        this->SetReplyMessage();

                        this->FinishComplete(thisSPtr, error);
                    }
                }
            }
            else
            {
                this->TryComplete(thisSPtr, error);
            }
        } // if acquire lock success
    }

    ErrorCode StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::ReadHierarchyNameData(
        TransactionSPtr const & txSPtr,
        NamingUri const & name,
        __out HierarchyNameData & nameData)
    {
        wstring nameString = name.ToString();

        _int64 nameSequenceNumber = -1;
        EnumerationSPtr nameEnumSPtr;

        ErrorCode error = Store.TryGetCurrentSequenceNumber(
            txSPtr, 
            Constants::HierarchyNameEntryType, 
            nameString,
            nameSequenceNumber, 
            nameEnumSPtr);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: could not read Name {1}: error = {2}",
                this->TraceId,
                nameString,
                error);

            return (error.IsError(ErrorCodeValue::NotFound) ? ErrorCodeValue::NameNotFound : error);
        }

        error = Store.ReadCurrentData<HierarchyNameData>(nameEnumSPtr, nameData);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: could not deserialize Name {1}: error = {2}",
                this->TraceId,
                nameString,
                error);
        }

        return error;
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::SetConsistentFlag(bool isConsistent)
    {
        if (enumerationStatus_ & FABRIC_ENUMERATION_FINISHED_MASK)
        {
            enumerationStatus_ = (isConsistent ? FABRIC_ENUMERATION_CONSISTENT_FINISHED : FABRIC_ENUMERATION_BEST_EFFORT_FINISHED);
        }
        else
        {
            enumerationStatus_ = (isConsistent ? FABRIC_ENUMERATION_CONSISTENT_MORE_DATA : FABRIC_ENUMERATION_BEST_EFFORT_MORE_DATA);
        }
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::SetFinishedFlag(bool isFinished)
    {
        if (enumerationStatus_ & FABRIC_ENUMERATION_CONSISTENT_MASK)
        {
            enumerationStatus_ = (isFinished ? FABRIC_ENUMERATION_CONSISTENT_FINISHED : FABRIC_ENUMERATION_CONSISTENT_MORE_DATA);
        }
        else
        {
            enumerationStatus_ = (isFinished ? FABRIC_ENUMERATION_BEST_EFFORT_FINISHED : FABRIC_ENUMERATION_BEST_EFFORT_MORE_DATA);
        }
    }

    bool StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::FitsInReplyMessage(NamingUri const & subName)
    {
        size_t estimate = EnumerateSubNamesResult::EstimateSerializedSize(this->Name, subName);

        size_t finalEstimate = replyItemsSizeEstimate_ + estimate;

        if (finalEstimate <= replySizeThreshold_)
        {
            WriteNoise(
                TraceComponent,
                "{0}: fitting '{1}': size = {2} bytes",
                this->TraceId,
                subName,
                finalEstimate);

            // +1 to account for wstring metadata for each vector item (i.e. size field)
            replyItemsSizeEstimate_ += ((subName.ToString().size() + 1) * sizeof(wchar_t));
            replyTotalSizeEstimate_ = finalEstimate;

            return true;
        }
        else
        {
            WriteNoise(
                TraceComponent,
                "{0}: stopping at '{1}': size = {2} limit = {3} (bytes)",
                this->TraceId,
                subName,
                finalEstimate,
                replySizeThreshold_);

            return false;
        }
    }

    void StoreService::ProcessEnumerateSubNamesRequestAsyncOperation::SetReplyMessage()
    {
        auto replyBody = EnumerateSubNamesResult(
            Name,
            enumerationStatus_,
            move(allSubnames_),
            EnumerateSubNamesToken(lastEnumeratedName_, subnamesVersion_));

        Reply = NamingMessage::GetPeerEnumerateSubNamesReply(replyBody);
    }
}
