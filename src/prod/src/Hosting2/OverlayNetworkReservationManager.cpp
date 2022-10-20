// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

OverlayNetworkReservationManager::OverlayNetworkReservationManager() :
    lock_(),
    knownNetworkResources_(),
    reservations_(),
    freeNetworkResources_()
{
}

OverlayNetworkReservationManager::~OverlayNetworkReservationManager()
{
    this->knownNetworkResources_.clear();
    this->reservations_.clear();
    this->freeNetworkResources_.clear();
}

void OverlayNetworkReservationManager::AddNetworkResources(
    std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeAdded,
    std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeRemoved,
    std::unordered_set<std::wstring> & conflicts,
    int & added,
    int & removed)
{
    added = 0;
    removed = 0;

    { // lock
        AcquireExclusiveLock lock(this->lock_);

        // Add network resources to known and free list.
        //
        for (auto const & nr : networkResourcesToBeAdded)
        {
            auto item = this->knownNetworkResources_.find(nr);
            if (item == this->knownNetworkResources_.end())
            {
                auto newKnownNetworkResource = make_shared<OverlayNetworkResource>(nr->IPV4Address, nr->MacAddress);
                this->knownNetworkResources_.insert(newKnownNetworkResource);
                auto newFreeNetworkResource = make_shared<OverlayNetworkResource>(nr->IPV4Address, nr->MacAddress);
                this->freeNetworkResources_.push_back(newFreeNetworkResource);
                added++;
            }
        }

        // Remove network resoures and remove any active reservations.
        //
        for (auto const & nr : networkResourcesToBeRemoved)
        {
            auto item = this->knownNetworkResources_.find(nr);
            if (item != this->knownNetworkResources_.end())
            {
                if ((*item)->ReservationId.size() > 0)
                {
                    // We have an active reservation, so record the conflicting
                    // call, and remove the reservation
                    //
                    conflicts.insert((*item)->ReservationId);
                    this->reservations_.erase((*item)->ReservationId);
                    removed++;
                }

                this->knownNetworkResources_.erase(item);
            }
        }

        // Next, remove any entries in the free list that are in network resources to be removed list.
        //
        FilterFreeIPs([&networkResourcesToBeRemoved](OverlayNetworkResourceSPtr nr)
        {
            bool retain = true;

            auto item = networkResourcesToBeRemoved.find(nr);
            if (item != networkResourcesToBeRemoved.end())
            {
                retain = false;
            }

            return retain;
        });
    }
}

bool OverlayNetworkReservationManager::Release(const wstring & reservationId)
{
    { //lock
        AcquireExclusiveLock lock(this->lock_);

        auto item = this->reservations_.find(reservationId);
        if (item != this->reservations_.end())
        {
            auto knownNetworkResource = this->knownNetworkResources_.find(item->second);
            if (knownNetworkResource != this->knownNetworkResources_.end())
            {
                this->reservations_.erase(reservationId);

                (*knownNetworkResource)->ReservationId = L"";

                auto newFreeNetworkResource = make_shared<OverlayNetworkResource>(
                    (*knownNetworkResource)->IPV4Address,
                    (*knownNetworkResource)->MacAddress);

                this->freeNetworkResources_.push_back(newFreeNetworkResource);

                return true;
            }
        }
    }

    return false;
}

bool OverlayNetworkReservationManager::Reserve(const wstring & reservationId, OverlayNetworkResourceSPtr & networkResource)
{
    { //lock
        AcquireExclusiveLock lock(this->lock_);

        // Do we already know about this reservation? If so, give that
        // network resource back.
        //
        auto item = this->reservations_.find(reservationId);
        if (item != this->reservations_.end())
        {
            networkResource = item->second;
            return true;
        }

        // We don't have an existing reservation. Make one now, if there are
        // any free network resources left in the pool.
        //
        if (this->freeNetworkResources_.size() > 0)
        {
            networkResource = this->freeNetworkResources_.front();
            this->freeNetworkResources_.pop_front();

            auto knownNetworkResource = this->knownNetworkResources_.find(networkResource);
            if (knownNetworkResource != this->knownNetworkResources_.end())
            {
                (*knownNetworkResource)->ReservationId = reservationId;

                auto newReservedNetworkResource = make_shared<OverlayNetworkResource>(
                    (*knownNetworkResource)->IPV4Address,
                    (*knownNetworkResource)->MacAddress,
                    (*knownNetworkResource)->ReservationId);
                    
                this->reservations_[(*knownNetworkResource)->ReservationId] = newReservedNetworkResource;

                return true;
            }
        }
    }

    // We were unable to reserve a network resource.
    //
    networkResource = nullptr;
    return false;
}

wstring OverlayNetworkReservationManager::DumpCurrentState()
{
    wstringstream w;

    { // lock
        AcquireExclusiveLock lock(this->lock_);

        w << L"Total: " << this->knownNetworkResources_.size()
            << L", Reserved: " << this->reservations_.size()
            << L", Free: " << this->freeNetworkResources_.size();

        for (auto const & entry : this->knownNetworkResources_)
        {
            w << endl << L"    IP: " << entry->get_IPV4Address()
                << L", MAC: " << MACHelper::Format(entry->get_MacAddress())
                << L", ResId: '" << entry->ReservationId << L"'";
        }
    }

    return w.str();
}

int OverlayNetworkReservationManager::get_FreeCount()
{
    AcquireExclusiveLock lock(this->lock_);

    return (int)this->freeNetworkResources_.size();
}

int OverlayNetworkReservationManager::get_ReservedCount()
{
    AcquireExclusiveLock lock(this->lock_);

    return (int)this->reservations_.size();
}

int OverlayNetworkReservationManager::get_TotalCount()
{
    AcquireExclusiveLock lock(this->lock_);

    return (int)this->knownNetworkResources_.size();
}

void OverlayNetworkReservationManager::FilterFreeIPs(function<bool(OverlayNetworkResourceSPtr)> retain)
{
    if (this->freeNetworkResources_.size() != 0)
    {
        // Rotate the list one full cycle, removing all entries that fail the
        // supplied retention test.
        //
        auto last = this->freeNetworkResources_.back();

        do
        {
            auto nr = this->freeNetworkResources_.front();
            this->freeNetworkResources_.pop_front();

            auto knownNetworkResource = this->knownNetworkResources_.find(nr);
            if (knownNetworkResource != this->knownNetworkResources_.end())
            {
                if (retain(*knownNetworkResource))
                {
                    this->freeNetworkResources_.push_back(nr);
                }
            }

            if (nr->IPV4Address == last->IPV4Address && nr->MacAddress == last->MacAddress)
            {
                break;
            }
        } while (this->freeNetworkResources_.size() > 0);
    }
}
