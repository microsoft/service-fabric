// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // An instance of this class provides a pool of ips that can be managed,
    // whether they are in use, or not, and if so, by whom.  The pool can be
    // updated with new ips, have ips removed, or any combination thereof.
    //
    class ReservationManager
    {
        // An instance of this support class represents a single ip in a pool,
        // including its current reservation state.
        //
        class IPEntry
        {
        public:
            IPEntry(uint address, bool mark) :
                IPV4Address_(address),
                reservationId_(),
                marked_(mark)
            { };

            virtual ~IPEntry() {};

            __declspec(property(get = get_Address)) uint Address;
            uint get_Address() const { return this->IPV4Address_; }

            __declspec(property(get = get_ReservationId, put = set_ReservationId)) const std::wstring &ReservationId;
            const std::wstring &get_ReservationId() const { return this->reservationId_; }
            void set_ReservationId(const std::wstring &reservation) { this->reservationId_ = reservation; }

            __declspec(property(get = get_Marked, put = set_Marked)) bool Marked;
            bool get_Marked() const { return this->marked_; }
            void set_Marked(bool marked) { this->marked_ = marked; }

        private:
            uint IPV4Address_;
            std::wstring reservationId_;
            bool marked_;
        };

        DENY_COPY(ReservationManager)

    public:
        // Defines a reserved (unusable) IP address as a specific invalid marker.
        //
        static const uint INVALID_IP = 0xFFFFFFFF;

        ReservationManager();
        virtual ~ReservationManager();

        // Refresh the pool of IPs with a new list of IPs to manage.  This may be the same as the
        // old list, have new IPs added, have IPs removed, or any combination thereof.
        //
        // The arguments are:
        //  newIPs: The updated set of IPs for this instance to manage.
        //  conflicts: Filled in with the set of existing reservations that are
        //             using IPs which are no longer active.
        //  added: Updated with the number of IPs that are new with this call.
        //  removed: Updated with the number of IPs that were removed with this call.
        //
        void AddIPs(
            std::list<uint> &newIPs, 
            std::unordered_set<std::wstring> &conflicts, 
            int &added, 
            int &removed);

        // Release an existing reservation.
        //
        // The arguments are:
        //  reservationId: Idenfities the specific in-use IP to release.
        //
        // Returns:
        //  true, if the reservation was found and released; false, if it was
        //  not found.
        //
        bool Release(const std::wstring &reservationId);

        // Reserve an available IP.  This is an idempotent call, in that if the
        // reservation has already been made, the associated ip is returned.
        //
        // The arguments are:
        //  reservationId: The unique tag used to identify this request.
        //  ip: Updated with the ip to use for this reservation, or the
        //      INVALID_IP value to indicate no ip assigned.
        //
        // Returns:
        //  true, if an ip was reserved; false, if it was not.
        //
        bool Reserve(const std::wstring &reservationId, uint &ip);

        // Reserve a specific IP.  This is an idempotent call, in that if the
        // reservation with a matching IP already exists, the call simply
        // returns with success.  Note that if the specified ip is already in use by
        // another reservation, this call will fail and the existing reservation
        // will be untouched.
        //
        // The arguments are:
        //  reservationId: The unique tag used to identify this request.
        //  ip: The specific ip to use for this reservation.
        //
        // Returns:
        //  true, if an ip was reserved; false, if it was not.
        //
        bool ReserveSpecific(const std::wstring &reservationId, uint ip);

        // Dump the details of the current state of this reservation pool into
        // a string.  This is to support diagnostic traces and test runs.
        //
        std::wstring DumpCurrentState();

        // Get the number of ips that are currently managed, and not
        // reserved.
        //
        __declspec(property(get=get_FreeCount)) int FreeCount;
        int get_FreeCount();

        // Get the number of ips that are currently reserved.
        //
        __declspec(property(get=get_ReservedCount)) int ReservedCount;
        int get_ReservedCount();

        // Get the number of ips that managed by this instance
        //
        __declspec(property(get=get_TotalCount)) int TotalCount;
        int get_TotalCount();

    private:
        // Remove entries from the free IP list that do not meet the supplied
        // retention criteria.
        //
        // The arguments are:
        //  retain: lamdba that returns true if the supplied entry should be
        //          kept.
        //
        void FilterFreeIPs(function<bool(IPEntry *)> retain);

        Common::ExclusiveLock lock_;

        // Contains the full set of IPs currently managed by this object.  This
        // holds the definitive IP definitions that the other maps reference.
        //
        std::map<uint, IPEntry *> knownIPs_;

        // Holds the currently active reservations.
        //
        std::map<std::wstring, uint> reservations_;

        // Holds the IPs that are currently not reserved, sorted by the order
        // that they were released.  This avoids reusing IPs any earlier than
        // necessary.
        //
        std::deque<uint> freeIPs_;
    };
}
