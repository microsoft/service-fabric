// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class encapsulates the Nat IP reservation pool.
    class NatIPAM : 
        public INatIPAM,
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(NatIPAM)

    public:

        NatIPAM(
            Common::ComponentRootSPtr const & root,
            std::wstring const & networkRange);

        ~NatIPAM();

        Common::AsyncOperationSPtr BeginInitialize(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndInitialize(Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode Reserve(std::wstring const &reservationId, uint &ip);

        Common::ErrorCode Release(std::wstring const &reservationId);

        // GetGhostReservations: Gets the current list of reservation ids that
        // are known ghosts.
        //
        std::list<std::wstring> GetGhostReservations();

        // OnNewIpamData: This method is called when new configuration data 
        // is available. It is responsible for getting and
        // applying the updates to the current reservation pool.
        //
        // The arguments are:
        //  config: The configuration data to apply.
        //
        void OnNewIpamData(list<uint> ipAddresses);

        // Get the number of ips that are currently managed, and not
        // reserved.
        //
        __declspec(property(get=get_FreeCount)) int FreeCount;
        int get_FreeCount() { return this->pool_.FreeCount; };

        // Get the number of ips that are currently reserved.
        //
        __declspec(property(get=get_ReservedCount)) int ReservedCount;
        int get_ReservedCount() { return this->pool_.ReservedCount; };

        // Get the number of ips that managed by this instance
        //
        __declspec(property(get=get_TotalCount)) int TotalCount;
        int get_TotalCount() { return this->pool_.TotalCount; };

        // Get a flag indicating if this object has been successfully
        // initialized.
        //
        __declspec(property(get = get_Initialized)) bool Initialized;
        bool get_Initialized() const { return this->initialized_; };

        // Get the timestamp for when the cache was last refreshed from the
        // wire server.
        //
        __declspec(property(get = get_LastRefreshTime)) Common::DateTime LastRefreshTime;
        Common::DateTime get_LastRefreshTime();

        // Get the number of ghost reservations.
        //
        __declspec(property(get=get_GhostCount)) int GhostCount;
        int get_GhostCount();

    protected:

        class InitializeAsyncOperation;

        // RemoveGhostIf: This helper method removes the supplied reservation
        // from the set of ghost reservations, if it was present.  Otherwise,
        // it silently does nothing.
        //
        // The arguments are:
        //  reservationId: the reservation key to remove.
        //
        void RemoveGhostIf(wstring const &reservationId);

        // FormatSetAsWString: This helper method returns the entries in the
        // incoming set into a new line separated string.
        //
        // The arguments are:
        //  entries: The set of entries to put into the string.
        //
        // Returns:
        //  Resulting formatted string
        //
        static std::wstring FormatSetAsWString(std::unordered_set<std::wstring> const &entries);

        // Holds the reservable pool of IPs, along with their reservations, if
        // any.  Note that this pool is already thread safe, so does not need
        // to be protected by the lock below.
        //
        ReservationManager pool_;

        // Indicates if this instance has been initialized.
        //
        bool initialized_;

        // Network range in CIDR format
        std::wstring networkRange_;

        // Holds the lock used to synchronize access to the fields following
        // it.
        //
        Common::ExclusiveLock lock_;

        // Holds the timestamp for the last successful refresh.
        //
        Common::DateTime lastRefreshTime_;

        // Holds the set of known ghost reservations.  These are reservations
        // that were held when their associated IPs disappeared from the list
        // of eligible IPs.  Such reservations are removed from the pool, and
        // placed in this set.  They are removed when they are released or
        // they reserve a new IP.
        //
        std::unordered_set<std::wstring> ghosts_;

        // This contains a reference to the method to call whenever the set of
        // ghost reservations changes.
        //
        std::function<void(Common::DateTime)> ghostChangeCallback_;
    };
}
