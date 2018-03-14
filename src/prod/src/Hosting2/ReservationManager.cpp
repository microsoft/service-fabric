// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ReservationManager::ReservationManager() :
    lock_(),
    knownIPs_(),
    reservations_(),
    freeIPs_()
{
}

ReservationManager::~ReservationManager()
{
    for (auto & entry : this->knownIPs_)
    {
        delete entry.second;
    }

    this->knownIPs_.clear();
    this->reservations_.clear();
    this->freeIPs_.clear();
}

void ReservationManager::AddIPs(list<uint> &newIPs, unordered_set<wstring> &conflicts, int &added, int &removed)
{
    added = 0;
    removed = 0;

    AcquireExclusiveLock lock(this->lock_);
    {
        // Start by marking all existing IPs are unprocessed
        //
        for (auto & entry : this->knownIPs_)
        {
            entry.second->Marked = false;
        }

        // Next, go over each IP, adding in those that were not previously
        // known, or marking as seen those that were.
        //
        for (auto ip : newIPs)
        {
            auto item = this->knownIPs_.find(ip);
            if (item == this->knownIPs_.end())
            {
                this->knownIPs_[ip] = new IPEntry(ip, true);
                this->freeIPs_.push_back(ip);
                added++;
            }
            else
            {
                item->second->Marked = true;
            }
        }

        // Next, go over each known IP, looking for those that were not in
        // the new IP list.  Record them for deletion, and remove any active
        // reservations.
        //
        list<uint> toBeRemoved;
        for (auto entry : this->knownIPs_)
        {
            if (!entry.second->Marked)
            {
                if (entry.second->ReservationId.size() > 0)
                {
                    // We have an active reservation, so record the conflicting
                    // call, and remove the reservation
                    //
                    conflicts.insert(entry.second->ReservationId);
                    this->reservations_.erase(entry.second->ReservationId);
                }

                toBeRemoved.push_back(entry.first);
                removed++;
            }
        }

        // Next, remove any entries in the free list that are for IPs that
        // were not included in the updated IP list.
        //
        FilterFreeIPs([](IPEntry *entry)
        {
            return entry->Marked;
        });

        // Finally, delete all base IP entry items for each IP that has
        // disappeared in the updated IP list.
        //
        for (auto & ip : toBeRemoved)
        {
            delete this->knownIPs_[ip];
            this->knownIPs_.erase(ip);
        }
    }
}

bool ReservationManager::Release(const wstring &reservationId)
{
    AcquireExclusiveLock lock(this->lock_);
    {
        auto item = this->reservations_.find(reservationId);
        if (item != this->reservations_.end())
        {
            auto entry = this->knownIPs_[item->second];

            this->reservations_.erase(reservationId);
            entry->ReservationId = L"";
            this->freeIPs_.push_back(entry->Address);

            return true;
        }
    }

    return false;
}

bool ReservationManager::Reserve(const wstring &reservationId, uint &ip)
{
    AcquireExclusiveLock lock(this->lock_);
    {
        // Do we already know about this reservation?  If so, give that
        // IP back.
        //
        auto item = this->reservations_.find(reservationId);
        if (item != this->reservations_.end())
        {
            ip = item->second;
            return true;
        }

        // We don't have an existing reservation.  Make one now, if there are
        // any free IPs left in the pool.
        //
        if (this->freeIPs_.size() > 0)
        {
            ip = this->freeIPs_.front();
            this->freeIPs_.pop_front();

            auto entry = this->knownIPs_[ip];
            entry->ReservationId = reservationId;
            this->reservations_[entry->ReservationId] = ip;
            return true;
        }
    }

    // We were unable to reserve an IP.
    //
    ip = INVALID_IP;
    return false;
}

bool ReservationManager::ReserveSpecific(const wstring &reservationId, uint ip)
{
    AcquireExclusiveLock lock(this->lock_);
    {
        // First, find out if the reservation already exists.  If it does, then
        // return success or failure based on whether it matches the ip that is
        // now requested.
        //
        auto item = this->reservations_.find(reservationId);
        if (item != this->reservations_.end())
        {
            return item->second == ip;
        }

        // Next, see if this ip is free, and fail if it isn't.
        //
        auto entry = this->knownIPs_.find(ip);
        if (entry == this->knownIPs_.end() || 
            !entry->second->ReservationId.empty())
        {
            return false;
        }

        // We can reserve this IP.  Add the reservation and pull from the free
        // IP list.
        //
        entry->second->ReservationId = reservationId;
        this->reservations_[entry->second->ReservationId] = ip;

        FilterFreeIPs([ip](IPEntry *entry)
        {
            return entry->Address != ip;
        });

        return true;
    }

    return false;
}

wstring ReservationManager::DumpCurrentState()
{
    wstringstream w;

    AcquireExclusiveLock lock(this->lock_);
    {
        w << L"Total: " << this->knownIPs_.size() 
          << L", Reserved: " << this->reservations_.size() 
          << L", Free: " << this->freeIPs_.size();

        for (auto & entry : this->knownIPs_)
        {
            w << endl << L"    Key: " << entry.first 
              << L", IP: " << IPHelper::Format(entry.second->Address) 
              << L", Marked: " << entry.second->Marked
              << L", ResId: '" << entry.second->ReservationId << L"'";
        }

        return w.str();
    }
}

int ReservationManager::get_FreeCount()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return (int)this->freeIPs_.size();
    }
}

int ReservationManager::get_ReservedCount()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return (int)this->reservations_.size();
    }
}

int ReservationManager::get_TotalCount()
{
    AcquireExclusiveLock lock(this->lock_);
    {
        return (int)this->knownIPs_.size();
    }
}

void ReservationManager::FilterFreeIPs(function<bool(IPEntry *)> retain)
{
    if (this->freeIPs_.size() != 0)
    {
        // Rotate the list one full cycle, removing all entries that fail the
        // supplied retention test.
        //
        auto last = this->freeIPs_.back();

        do
        {
            uint ip = this->freeIPs_.front();
            this->freeIPs_.pop_front();

            auto entry = this->knownIPs_[ip];
            if (retain(entry))
            {
                this->freeIPs_.push_back(ip);
            }

            if (ip == last)
            {
                break;
            }
        } while (this->freeIPs_.size() > 0);
    }
}
