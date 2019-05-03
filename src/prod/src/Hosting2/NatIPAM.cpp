// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const NatIPAMProvider("NatIPAM");

// ********************************************************************************************************************
// NatIPAM::InitializeAsyncOperation Implementation
// ********************************************************************************************************************
class NatIPAM::InitializeAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::Hosting>
{
    DENY_COPY(InitializeAsyncOperation)

public:
    InitializeAsyncOperation(
        NatIPAM & owner,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout)
    {
    }

    virtual ~InitializeAsyncOperation()
    {
    }

    static ErrorCode InitializeAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<InitializeAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        UINT baseIP = 0;
        int maskNum = 0;
        if (!IPHelper::TryParseCIDR(owner_.networkRange_, baseIP, maskNum))
        {
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }

        // Convert the mask number to a bitmask.
        uint mask = 0;
        for (int i = 0; i < maskNum; i++)
        {
            mask >>= 1;
            mask |= 0x80000000;
        }

        uint size = (sizeof(uint) * 8) - (maskNum);
        uint totalIpCount = 1;
        for (uint i = 0; i < size; i++)
        {
            totalIpCount <<= 1;
        }

        // Calculate IP addresses.
        // Skip the first since that is the gateway ip.
        list<uint> ipaddresses;
        for (uint i = 1; i < totalIpCount; i++)
        {
            uint startIP = (baseIP & mask) + i + 1;
            ipaddresses.push_back(startIP);
        }

        // Populate reservation pool.
        owner_.OnNewIpamData(ipaddresses);

        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

private:
    NatIPAM & owner_;
    TimeoutHelper timeoutHelper_;
};

NatIPAM::NatIPAM(
    ComponentRootSPtr const & root,
    wstring const & networkRange)
    : RootedObject(*root),
      lock_(),
      initialized_(false),
      lastRefreshTime_(DateTime::Zero),
      ghosts_(),
      networkRange_(networkRange),
      pool_(),
      ghostChangeCallback_(nullptr)
{
}

NatIPAM::~NatIPAM()
{
}

AsyncOperationSPtr NatIPAM::BeginInitialize(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<InitializeAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode NatIPAM::EndInitialize(AsyncOperationSPtr const & operation)
{
    return InitializeAsyncOperation::End(operation);
}

ErrorCode NatIPAM::Reserve(wstring const &reservationId, uint &ip)
{
    if (!this->initialized_)
    {
        return ErrorCode(ErrorCodeValue::NotReady);
    }

    if (this->pool_.Reserve(reservationId, ip))
    {
        // Make sure that this is not listed in the ghost list either.
        //
        RemoveGhostIf(reservationId);

        return ErrorCode(ErrorCodeValue::Success);
    }

    WriteWarning(
        NatIPAMProvider,
        "Failed to find an IP to reserve for id {0}, current pool counts (approx): total: {1}, available: {2}",
        reservationId,
        this->pool_.TotalCount,
        this->pool_.FreeCount);

    return ErrorCode(ErrorCodeValue::OperationFailed);
}

ErrorCode NatIPAM::Release(wstring const &reservationId)
{
    if (!this->initialized_)
    {
        return ErrorCode(ErrorCodeValue::NotReady);
    }

    if (!this->pool_.Release(reservationId))
    {
        // It may be a release of a ghost.  Check that here, and remove it if
        // it is.  If it is still not found, that is also fine.
        //
        RemoveGhostIf(reservationId);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

DateTime NatIPAM::get_LastRefreshTime()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return this->lastRefreshTime_;
    }
}

int NatIPAM::get_GhostCount()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return (int) this->ghosts_.size();
    }
}

list<wstring> NatIPAM::GetGhostReservations()
{
    list<wstring> ghosts;

    AcquireExclusiveLock lock(this->lock_);
    {
        for (auto item : this->ghosts_)
        {
            ghosts.push_back(item);
        }
    }

    return ghosts;
}

void NatIPAM::OnNewIpamData(list<uint> ipAddresses)
{
    int added;
    int removed;
    bool callbackNeeded = false;
    DateTime refreshTime;

    AcquireExclusiveLock lock(this->lock_);
    {
        // Remember the number of currently known ghosts, to use to determinte if
        // the list has changed.
        //
        // Note that this approach works for the initial call because there are
        // no existing reservations to declare ghosts, and no pre-existing ghosts
        // so the ghost list will never have changed in size.
        //
        auto count = this->ghosts_.size();

        // Form up the list of ips
        this->pool_.AddIPs(ipAddresses, this->ghosts_, added, removed);

        // if the new conflict list is any different from the existing ghost list
        // then flag to issue an update notification
        //
        callbackNeeded = this->ghosts_.size() != count;

        // Update the refresh time.
        refreshTime = DateTime::Now();
        this->lastRefreshTime_ = refreshTime;

        if (added > 0 || removed > 0 || callbackNeeded)
        {
            // We have a change in the pool, so dump much detailed information
            // about the resulting state.
            //
            WriteInfo(
                NatIPAMProvider,
                "New data loaded, IPs added: {0}, removed: {1}, new ghosts: {2}, known ghosts:\n{3}\nCurrent Pool:\n{4}",
                added,
                removed,
                callbackNeeded,
                FormatSetAsWString(this->ghosts_),
                this->pool_.DumpCurrentState());
        }
        else
        {
            WriteInfo(
                NatIPAMProvider,
                "New data loaded, no changes found");
        }

        // We have successfully loaded the pool, so no matter what, we are now
        // initialized.
        //
        this->initialized_ = true;
    }

    // Finally, if we need to notify the caller that the ghost list has changed,
    // do so now.
    if (callbackNeeded)
    {
        this->ghostChangeCallback_(refreshTime);
    }
}

void NatIPAM::RemoveGhostIf(wstring const &reservationId)
{
    AcquireExclusiveLock lock(this->lock_);
    {
        auto item = this->ghosts_.find(reservationId);
        if (item != this->ghosts_.end())
        {
            this->ghosts_.erase(item);
        }
    }
}

wstring NatIPAM::FormatSetAsWString(unordered_set<wstring> const &entries)
{
    bool first = true;
    wstringstream ss;

    for (auto & item : entries)
    {
        if (!first)
        {
            ss << endl;
        }

        ss << item;
        first = false;
    }

    return ss.str();
}
