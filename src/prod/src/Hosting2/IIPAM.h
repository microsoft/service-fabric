// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IIPAM
    {
        DENY_COPY(IIPAM)

    public:
        IIPAM() {};
        virtual ~IIPAM() {};

        // BeginInitialize: Start the initial loading of state.  Note that
        // the class assumes that initialize is not called multiple times
        // simultaneously.
        //
        // The arguments are:
        //  timeout: time limit for http operations during load
        //  callback: completion notification routine
        //  parent: owning operation or context
        //
        // Returns:
        //  The initialization operation context.
        //
        virtual Common::AsyncOperationSPtr BeginInitialize(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        // EndInitialize: Wait for the initialization to complete.
        //
        // The arguments are:
        //  operation: the initialzation operation context
        //
        // Returns:
        //  An error code indicating if the initializaiton was successful.
        //
        virtual Common::ErrorCode EndInitialize(
            Common::AsyncOperationSPtr const & operation) = 0;

        // StartRefreshProcessing: Starts the recurring housekeeping taks that checks
        // for any updates on allocable IPs from the wireserver, and updates the
        // reservation cache if so.
        //
        // The arguments are:
        //  refreshInterval: The delta time between refresh attempts. The first
        //                   refresh attempt will not occur until this time has
        //                   passed.
        //  refreshTimeout: The time limit on http operations during a refresh.
        //  ghostChangeCallback: The method to call if the set of ghost
        //                       reservations changed.
        //
        // Returns:
        //  An error code indicating if the housekeeping operation was set up.
        //
        virtual Common::ErrorCode StartRefreshProcessing(
            Common::TimeSpan const refreshInterval,
            Common::TimeSpan const refreshTimeout,
            std::function<void(Common::DateTime)> ghostChangeCallback) = 0;

        // CancelRefreshProcessing: Cancels the recurring refresh for any updates
        // on allocable IPs from the wireserver.
        virtual void CancelRefreshProcessing() = 0;

        // Reserve: Reserve an IP using the supplied reservation key.  Note
        // that this function is idempotent, if an active reservation exists,
        // this will return the old IP.  If the reservation is currently a
        // ghost, it will reserve a new IP and clean up the ghost entry.
        //
        // The arguments are:
        //  reservationId: unique key to use to identify this reservation
        //  ip: the IP allocated by this reservation.
        //
        // Returns:
        //  Success, if an IP can be assigned; a failure value if it cannot
        //  (this includes pool exhaustion and calling before initialization)
        //
        virtual Common::ErrorCode Reserve(std::wstring const & reservationId, uint & ip) = 0;

        // Release: Release a previously reserved IP using the supplied
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
        virtual Common::ErrorCode Release(std::wstring const & reservationId) = 0;

        // GetGhostReservations: Gets the current list of reservation ids that
        // are known ghosts.
        //
        virtual std::list<std::wstring> GetGhostReservations() = 0;

        // Get the subnet(CIDR) and gateway ip address for the vnet
        virtual Common::ErrorCode GetSubnetAndGatewayIpAddress(wstring & subnetCIDR, uint & gatewayIpAddress) = 0;
    };
}
