// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class encapsulates the IP reservation pool and provides virtual methods which can be overridden
    // in specialized platform specific classes. The virtual methods provide mechanisms for loading and periodically
    // refreshing the IP reservation pool.
    class IPAM : 
        public IIPAM,
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(IPAM)

    public:
        // IPAMClient: Construct an instance
        //
        // The arguments are:
        //  root: parent object for this instance
        //
        IPAM(Common::ComponentRootSPtr const & root);

        virtual ~IPAM();

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
        // refreshInterval: The delta time between refresh attempts. The first
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
        void CancelRefreshProcessing();

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
        Common::ErrorCode Reserve(std::wstring const &reservationId, uint &ip);

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
        Common::ErrorCode Release(std::wstring const &reservationId);

        // GetGhostReservations: Gets the current list of reservation ids that
        // are known ghosts.
        //
        std::list<std::wstring> GetGhostReservations();

        // Get the subnet(CIDR) and gateway ip address for the vnet
        Common::ErrorCode GetSubnetAndGatewayIpAddress(wstring &subnetCIDR, uint &gatewayIpAddress);

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
        __declspec(property(get=get_LastRefreshTime)) Common::DateTime LastRefreshTime;
        Common::DateTime get_LastRefreshTime();

        // Get the number of ghost reservations.
        //
        __declspec(property(get=get_GhostCount)) int GhostCount;
        int get_GhostCount();

    protected:

        // OnNewIpamData: This method is called when new configuration data has
        // arrived from the wire server.  It is responsible for getting and
        // applying the updates to the current reservation pool.
        //
        // The arguments are:
        //  config: The configuration data to apply.
        //
        void OnNewIpamData(FlatIPConfiguration &config);

        // GetIpsFromConfig: This helper method walks through the configuration
        // data, retrieving only the IPs that are eligible for use by the local
        // reservation pool.
        //
        // The arguments are:
        //  config: The configuration data to apply.
        //
        // Returns:
        //  The IPs from the configuration that could be reserved.
        //
        list<uint> GetIpsFromConfig(FlatIPConfiguration &config);

        // GetSubnetAndGatewayIpAddressFromConfig: This helper method walks though the configuration
        // data, retrieving the subnet and gateway for the primary interface.
        //
        // The arguments are:
        // config: The configuration data to apply.
        //
        // Returns: Subnet and gateway ip address.
        void GetSubnetAndGatewayIpAddressFromConfig(FlatIPConfiguration &config, wstring &subnetCIDR, uint &gatewayIp);

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

        // Holds the lock used to synchronize access to the fields following
        // it.
        //
        Common::ExclusiveLock lock_;

        // Indicates if background refresh processing has been actived.
        //
        bool refreshInProgress_;

        // Holds the time to wait between refresh passes.
        //
        Common::TimeSpan refreshInterval_;

        // Holds the time limit for the requests to the wire server to take.
        //
        Common::TimeSpan refreshTimeout_;

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

        // Gateway ip address for the vnet
        uint gatewayIpAddress_;

        // Subnet address for the vnet in CIDR format
        wstring subnetCIDR_;

        // Is refresh processing cancelled
        bool refreshProcessingCancelled_;

        // Reference to refresh operation. 
        // This is used to cancel the operation when processing is cancelled.
        AsyncOperationSPtr refreshOperation_;
    };
}
