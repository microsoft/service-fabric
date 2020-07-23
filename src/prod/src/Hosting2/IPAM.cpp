// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StringLiteral const IPAMProvider("IPAM");

IPAM::IPAM(
    ComponentRootSPtr const & root)
    : RootedObject(*root),
      lock_(),
      initialized_(false),
      refreshInProgress_(false),
      refreshProcessingCancelled_(false),
      refreshInterval_(),
      refreshTimeout_(),
      lastRefreshTime_(DateTime::Zero),
      ghosts_(),
      pool_(),
      ghostChangeCallback_(nullptr)
{
}

IPAM::~IPAM()
{
}

ErrorCode IPAM::Reserve(wstring const &reservationId, uint &ip)
{
    if (!this->initialized_)
    {
        return ErrorCode(ErrorCodeValue::NotReady);
    }

    if (this->pool_.Reserve(reservationId, ip))
    {
        // Make this that this is not listed in the ghost list either.
        //
        RemoveGhostIf(reservationId);

        return ErrorCode(ErrorCodeValue::Success);
    }

    WriteWarning(
        IPAMProvider,
        "Failed to find an IP to reserve for id {0}, current pool counts (approx): total: {1}, available: {2}",
        reservationId,
        this->pool_.TotalCount,
        this->pool_.FreeCount);

    return ErrorCode(ErrorCodeValue::OperationFailed);
}

ErrorCode IPAM::Release(wstring const &reservationId)
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

DateTime IPAM::get_LastRefreshTime()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return this->lastRefreshTime_;
    }
}

int IPAM::get_GhostCount()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return (int) this->ghosts_.size();
    }
}

list<wstring> IPAM::GetGhostReservations()
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

void IPAM::OnNewIpamData(FlatIPConfiguration &config)
{
    int added;
    int removed;
    bool callbackNeeded = false;
    DateTime refreshTime;

    AcquireExclusiveLock lock(this->lock_);
    {
        // Traverses the configuration to get subnet(CIDR)and gateway ip address.
        this->GetSubnetAndGatewayIpAddressFromConfig(config, this->subnetCIDR_, this->gatewayIpAddress_);

        // Construct the list of IPs from the configuration
        // 
        list<uint> newIps = this->GetIpsFromConfig(config);

        // Remember the number of currently known ghosts, to use to determinte if
        // the list has changed.
        //
        // Note that this approach works for the initial call because there are
        // no existing reservations to declare ghosts, and no pre-existing ghosts
        // so the ghost list will never have changed in size.
        //
        auto count = this->ghosts_.size();

        // Form up the list of ips
        this->pool_.AddIPs(newIps, this->ghosts_, added, removed);

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
                IPAMProvider,
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
                IPAMProvider,
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

list<uint> IPAM::GetIpsFromConfig(FlatIPConfiguration &config)
{
    list<uint> ips;

    for (auto & inter : config.Interfaces)
    {
        if (inter->Primary)
        {
            for (auto & subnet : inter->Subnets)
            {
                for (auto ip : subnet->SecondaryAddresses)
                {
                    ips.push_back(ip);
                }
            }
        }
    }

    return ips;
}

void IPAM::GetSubnetAndGatewayIpAddressFromConfig(FlatIPConfiguration &config, wstring &subnetCIDR, uint &gatewayIp)
{
    for (auto & inter : config.Interfaces)
    {
        if (inter->Primary && inter->Subnets.size() > 0)
        {
            auto subnet = inter->Subnets.front();
            gatewayIp = subnet->GatewayIp;
            subnetCIDR = subnet->SubnetCIDR;
            break;
        }
    }
}

void IPAM::RemoveGhostIf(wstring const &reservationId)
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

wstring IPAM::FormatSetAsWString(unordered_set<wstring> const &entries)
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

ErrorCode IPAM::GetSubnetAndGatewayIpAddress(wstring &subnetCIDR, uint &gatewayIpAddress)
{
    if (!this->initialized_)
    {
        return ErrorCode(ErrorCodeValue::InvalidState);
    }

    gatewayIpAddress = this->gatewayIpAddress_;
    subnetCIDR = this->subnetCIDR_;

    return ErrorCode(ErrorCodeValue::Success);
}

void IPAM::CancelRefreshProcessing()
{
    // Set the refresh cancelled flag to true.
    // Invoke cancel on the refresh operation, if already initialized
    {
        AcquireExclusiveLock lock(this->lock_);

        this->refreshProcessingCancelled_ = true;

        if (this->refreshOperation_ != nullptr)
        {
            // cancel with forcecomplete=true actually cancels the internal timer
            this->refreshOperation_->Cancel(true);
        }
    }
}
