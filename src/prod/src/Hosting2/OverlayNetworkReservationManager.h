// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // An instance of this class provides a pool of network resources that can be managed,
    // whether they are in use, or not, and if so, by whom.  The pool can be
    // updated with new network resources, have network resources removed, or any combination thereof.
    //
    class OverlayNetworkReservationManager
    {
        DENY_COPY(OverlayNetworkReservationManager)

    public:
        
        OverlayNetworkReservationManager();
        virtual ~OverlayNetworkReservationManager();

        // Refresh the pool of network resources with a new list of network resources to add and remove.
        //
        // The arguments are:
        //  networkResourcesToBeAdded: The set of network resources to be added to the pool.
        //  networkResourcesToBeDeleted: The set of network resources to be removed from the pool.
        //  conflicts: Filled in with the set of existing reservations that are
        //             using network resources which are no longer active.
        //  added: Updated with the number of network resources that are new with this call.
        //  removed: Updated with the number of network resources that were removed with this call.
        //
        void AddNetworkResources(
            std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeAdded,
            std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeRemoved,
            std::unordered_set<std::wstring> & conflicts, 
            int & added, 
            int & removed);

        // Release an existing reservation.
        //
        // The arguments are:
        //  reservationId: Idenfities the specific in-use network resource to release.
        //
        // Returns:
        //  true, if the reservation was found and released; false, if it was
        //  not found.
        //
        bool Release(const std::wstring & reservationId);

        // Reserve an available network resource. This is an idempotent call, in that if the
        // reservation has already been made, the associated network resource is returned.
        //
        // The arguments are:
        //  reservationId: The unique tag used to identify this request.
        //  networkResource: Updated with the network resource to use for this reservation, 
        //  or nullptr to indicate no network resource assigned.
        //
        // Returns:
        //  true, if a network resource was reserved; false, if it was not.
        //
        bool Reserve(const std::wstring & reservationId, OverlayNetworkResourceSPtr & networkResource);

        // Dump the details of the current state of this reservation pool into
        // a string.  This is to support diagnostic traces and test runs.
        //
        std::wstring DumpCurrentState();

        // Get the number of network resources that are currently managed, and not
        // reserved.
        //
        __declspec(property(get=get_FreeCount)) int FreeCount;
        int get_FreeCount();

        // Get the number of network resources that are currently reserved.
        //
        __declspec(property(get=get_ReservedCount)) int ReservedCount;
        int get_ReservedCount();

        // Get the number of network resources that are managed by this instance
        //
        __declspec(property(get=get_TotalCount)) int TotalCount;
        int get_TotalCount();

    private:
        // Remove entries from the free network resources list that do not meet the supplied
        // retention criteria.
        //
        // The arguments are:
        //  retain: lamdba that returns true if the supplied entry should be
        //          kept.
        //
        void FilterFreeIPs(function<bool(OverlayNetworkResourceSPtr)> retain);

        Common::ExclusiveLock lock_;

        // Contains the full set of network resources currently managed by this object. This
        // holds the definitive network resource definitions that the other maps reference.
        //
        std::unordered_set<OverlayNetworkResourceSPtr, OverlayNetworkResource::OverlayNetworkResourceSetHasher, OverlayNetworkResource::OverlayNetworkResourceSetComparator> knownNetworkResources_;

        // Holds the currently active reservations.
        //
        std::map<std::wstring, OverlayNetworkResourceSPtr> reservations_;

        // Holds the network resources that are currently not reserved, sorted by the order
        // that they were released.  This avoids reusing network resources any earlier than
        // necessary.
        //
        std::deque<OverlayNetworkResourceSPtr> freeNetworkResources_;
    };
}