// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    StringLiteral const TraceComponent("Enumeration");

    ReplicatedStore::Enumeration::Enumeration(
        TransactionBaseSPtr const & transaction,
        IStoreBase::EnumerationSPtr && innerEnumeration,
        ErrorCodeValue::Enum const creationError,
        int enumerationPerfTraceThreshold)
        : txWPtr_(transaction)
        , innerEnumeration_(move(innerEnumeration))
        , creationError_(creationError)
        , enumerationPerfTraceThreshold_(enumerationPerfTraceThreshold)
        , perfDetails_()
        , stopwatch_()
        , itemCount_(0)
        , totalBytes_(0)
        , totalElapsed_(TimeSpan::Zero)
    {
        auto txSPtr = txWPtr_.lock();
        if (txSPtr)
        {
            txSPtr->OnConstructEnumeration();

            if (creationError_ == ErrorCodeValue::Success)
            {
                if (!innerEnumeration_)
                {
                    Assert::CodingError(
                        "{0} inner enumeration must be Non-NULL for valid enumerations",
                        txSPtr->TraceId); 
                }
            }
            else if (innerEnumeration_)
            {
                Assert::CodingError(
                    "{0} inner enumeration must be NULL for invalid enumerations: error = {1}", 
                    txSPtr->TraceId, 
                    creationError_);
            }
        }

        if (this->IsPerfDetailsEnabled())
        {
            this->InitializePerfDetails();
        }
    }

    ReplicatedStore::Enumeration::~Enumeration()
    {
        auto txSPtr = txWPtr_.lock();
        if (txSPtr)
        {
            txSPtr->OnDestructEnumeration();

            if (this->IsPerfDetailsEnabled())
            {
                this->TracePerfDetails(txSPtr);
            }
        }
    }

    bool ReplicatedStore::Enumeration::IsPerfDetailsEnabled()
    {
        return (enumerationPerfTraceThreshold_ > 0);
    }

    void ReplicatedStore::Enumeration::InitializePerfDetails()
    {
        this->InitializePerfDetails(L"CheckAccess");
        this->InitializePerfDetails(L"MoveNext");
        this->InitializePerfDetails(L"CurrentOperationLSN");
        this->InitializePerfDetails(L"CurrentLastModifiedFILETIME");
        this->InitializePerfDetails(L"CurrentType");
        this->InitializePerfDetails(L"CurrentKey");
        this->InitializePerfDetails(L"CurrentValue");
        this->InitializePerfDetails(L"CurrentValueSize");
        this->InitializePerfDetails(L"CurrentLastModifiedOnPrimaryFILETIME");
    }

    void ReplicatedStore::Enumeration::InitializePerfDetails(wstring const & name)
    {
        perfDetails_.push_back(make_pair(name, TimeSpan::Zero));
    }

    void ReplicatedStore::Enumeration::TrackElapsedTime(TransactionBaseSPtr const & txSPtr, size_t opIndex)
    {
        if (opIndex >= perfDetails_.size())
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} perf index out of bounds: {1} >= {2}", 
                txSPtr->ActivityId,
                opIndex, 
                perfDetails_.size());
        }
        else
        {
            auto elapsed = stopwatch_.Elapsed;
            auto & opElapsed = perfDetails_[opIndex].second;

            opElapsed = opElapsed.AddWithMaxAndMinValueCheck(elapsed);
            totalElapsed_ = totalElapsed_.AddWithMaxAndMinValueCheck(elapsed);

            stopwatch_.Restart();
        }
    }

    void ReplicatedStore::Enumeration::TracePerfDetails(TransactionBaseSPtr const & txSPtr)
    {
        if (totalElapsed_ < TimeSpan::FromSeconds(enumerationPerfTraceThreshold_)) { return; }

        auto msg = wformatString("{0}: count={1} bytes={2}: ", txSPtr->ActivityId, itemCount_, totalBytes_);

        for (auto const & item : perfDetails_)
        {
            msg.append(wformatString("{0}:{1} ", item.first, item.second));
        }

        WriteInfo(TraceComponent, "{0}total:{1}", msg, totalElapsed_);
    }

    ErrorCode ReplicatedStore::Enumeration::MoveNext()
    {
        return this->CheckAndPerform([this]() 
            { 
                auto error = innerEnumeration_->MoveNext(); 

                if (this->IsPerfDetailsEnabled() && error.IsSuccess())
                {
                    ++itemCount_;

                    size_t size;
                    error = innerEnumeration_->CurrentValueSize(size);
                    if (error.IsSuccess())
                    {
                        totalBytes_ += size;
                    }
                }

                return error;
            }, 1);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentOperationLSN(__out _int64 & lsn)
    {
        return this->CheckAndPerform([this, &lsn]() { return innerEnumeration_->CurrentOperationLSN(lsn); }, 2);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentLastModifiedFILETIME(__out FILETIME & fileTime)
    {
        return this->CheckAndPerform([this, &fileTime]() { return innerEnumeration_->CurrentLastModifiedFILETIME(fileTime); }, 3);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentType(__out wstring & buffer)
    {
        return this->CheckAndPerform([this, &buffer]() { return innerEnumeration_->CurrentType(buffer); }, 4);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentKey(__out wstring & buffer)
    {
        return this->CheckAndPerform([this, &buffer]() { return innerEnumeration_->CurrentKey(buffer); }, 5);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentValue(__out vector<byte> & buffer)
    {
        return this->CheckAndPerform([this, &buffer]() { return innerEnumeration_->CurrentValue(buffer); }, 6);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentValueSize(__out size_t & size)
    {
        return this->CheckAndPerform([this, &size]() { return innerEnumeration_->CurrentValueSize(size); }, 7);
    }

    ErrorCode ReplicatedStore::Enumeration::CurrentLastModifiedOnPrimaryFILETIME(__out FILETIME & fileTime)
    {
        return this->CheckAndPerform([this, &fileTime]() { return innerEnumeration_->CurrentLastModifiedOnPrimaryFILETIME(fileTime); }, 8);
    }

    ErrorCode ReplicatedStore::Enumeration::CheckAndPerform(EnumerationOperationFunc const & operation, size_t opIndex)
    {
        if (this->IsPerfDetailsEnabled())
        {
            stopwatch_.Restart();
        }

        TransactionBaseSPtr txSPtr;
        ErrorCode error = this->CheckReadAccess(txSPtr);

        if (this->IsPerfDetailsEnabled() && error.IsSuccess())
        {
            this->TrackElapsedTime(txSPtr, 0);
        }

        if (error.IsSuccess())
        {
            error = operation();

            if (this->IsPerfDetailsEnabled() && error.IsSuccess())
            {
                this->TrackElapsedTime(txSPtr, opIndex);
            }

            this->AbortOnError(txSPtr, error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::Enumeration::CheckReadAccess(__out TransactionBaseSPtr & txSPtr)
    {
        if (creationError_ != ErrorCodeValue::Success)
        {
            return creationError_;
        }

        txSPtr = txWPtr_.lock();
        if (txSPtr)
        {
            return txSPtr->ReplicatedStore.CheckReadAccess(txSPtr, L"");
        }
        else
        {
            return ErrorCodeValue::StoreTransactionNotActive;
        }
    }

    void ReplicatedStore::Enumeration::AbortOnError(TransactionBaseSPtr const & txSPtr, ErrorCode const & error)
    {
        if (!error.IsSuccess() && !error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            txSPtr->Abort();
        }
    }
}
