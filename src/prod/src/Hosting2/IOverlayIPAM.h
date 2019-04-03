// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IOverlayIPAM :
        public Common::FabricComponent
    {
        DENY_COPY(IOverlayIPAM)

    public:
        IOverlayIPAM() {};
        virtual ~IOverlayIPAM() {};

        // Reserve: Reserve a network resource using the supplied reservation key.  Note
        // that this function is idempotent, if an active reservation exists,
        // this will return the old network resource. If the reservation is currently a
        // ghost, it will reserve a new network resource and clean up the ghost entry.
        //
        // The arguments are:
        //  reservationId: unique key to use to identify this reservation
        //  networkResource: the network resource allocated to this reservation.
        //
        // Returns:
        //  Success, if a network resource can be assigned; a failure value if it cannot
        //  (this includes pool exhaustion and calling before initialization)
        //
        virtual Common::AsyncOperationSPtr BeginReserveWithRetry(
            std::wstring const & reservationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndReserveWithRetry(
            Common::AsyncOperationSPtr const & operation,
            OverlayNetworkResourceSPtr & networkResource) = 0;

        // Release: Release a previously reserved network resource using the supplied
        // reservation key.  This is idempotent, in that it will succeed if
        // no reservation is found.  If the reservation is currently a ghost,
        // this method will clean that up.
        //
        // The arguments are:
        //  reservationId: unique key to use to identify this reservation
        //
        // Returns:
        //  Success, if the reservation is not found or can be released; 
        //  a failure value if it cannot (typically from calling before
        //  initialization completes)
        //
        virtual Common::AsyncOperationSPtr BeginReleaseWithRetry(
            std::wstring const & reservationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndReleaseWithRetry(
            Common::AsyncOperationSPtr const & operation) = 0;

        // GetGhostReservations: Gets the current list of reservation ids that
        // are known ghosts.
        //
        virtual std::list<std::wstring> GetGhostReservations() = 0;

        // OnNewIpamData: This method is called when new network resources data has
        // arrived from the Network Inventory Manager. It is responsible for getting and
        // applying the updates to the current reservation pool.
        //
        // The arguments are:
        //  networkResourcesToBeAdded: The collection of overlay network resources to add.
        //  networkResourcesToBeRemoved: The collection of overlay network resources to remove.
        //
        virtual void OnNewIpamData(std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeAdded,
            std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeRemoved) = 0;

    protected:

        virtual Common::ErrorCode OnOpen() = 0;
        virtual Common::ErrorCode OnClose() = 0;
        virtual void OnAbort() = 0;
    };
}
