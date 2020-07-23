// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class encapsulates the network resource reservation pool and provides methods that implement
    // mechanisms for loading and refreshing the network reservation pool.
    class OverlayIPAM :
        public IOverlayIPAM,
        public Common::RootedObject,
        protected Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(OverlayIPAM)

    public:
        // OverlayIPAMClient: Construct an instance
        //
        // The arguments are:
        //  root: parent object for this instance.
        //  replenishNetworkResourcesCallback: Callback used to ask for network resources.
        //  ghostChangeCallback: Callback used to communicate reservation ids with non-existent network resources.
        //  poolRefreshRetryInterval: Duration to wait for before firing the replenish call. 
        OverlayIPAM(
            Common::ComponentRootSPtr const & root,
            InternalReplenishNetworkResourcesCallback const & replenishNetworkResourcesCallback,
            GhostChangeCallback const & ghostChangeCallback,
            __in Common::TimeSpan const & poolRefreshRetryInterval);

        virtual ~OverlayIPAM();

        // Reserve: Reserve a network resource using the supplied reservation key.  Note
        // that this function is idempotent, if an active reservation exists,
        // this will return the old network resource. If the reservation is currently a
        // ghost, it will reserve a new network resource and clean up the ghost entry.
        //
        // The arguments are:
        //  reservationId: unique key to use to identify this reservation.
        //  networkResource: the network resource allocated to this reservation.
        //
        // Returns:
        //  Success, if a network resource can be assigned; a failure value if it cannot
        //  (this includes pool exhaustion and calling before initialization)
        //
        Common::AsyncOperationSPtr BeginReserveWithRetry(
            std::wstring const & reservationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndReserveWithRetry(
            Common::AsyncOperationSPtr const & operation,
            OverlayNetworkResourceSPtr & networkResource);

        // Release: Release a previously reserved network resource using the supplied
        // reservation key.  This is idempotent, in that it will succeed if
        // no reservation is found.  If the reservation is currently a ghost,
        // this method will clean that up.
        //
        // The arguments are:
        //  reservationId: unique key to use to identify this reservation.
        //
        // Returns:
        //  Success, if the reservation is not found or can be released; 
        //  a failure value if it cannot (typically from calling before
        //  initialization completes)
        //
        Common::AsyncOperationSPtr BeginReleaseWithRetry(
            std::wstring const & reservationId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndReleaseWithRetry(
            Common::AsyncOperationSPtr const & operation);

        // GetGhostReservations: Gets the current list of reservation ids that
        // are known ghosts.
        //
        std::list<std::wstring> GetGhostReservations();

        // OnNewIpamData: This method is called when new network resources data has
        // arrived from the Network Inventory Manager. It is responsible for getting and
        // applying the updates to the current reservation pool.
        //
        // The arguments are:
        //  networkResourcesToBeAdded: The collection of overlay network resources to add.
        //  networkResourcesToBeRemoved: The collection of overlay network resources to remove.
        //
        void OnNewIpamData(std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeAdded, 
            std::unordered_set<OverlayNetworkResourceSPtr> const & networkResourcesToBeRemoved);

        // Get the number of network resources that are currently managed, and not
        // reserved.
        //
        __declspec(property(get=get_FreeCount)) int FreeCount;
        int get_FreeCount() { return this->pool_.FreeCount; };

        // Get the number of network resources that are currently reserved.
        //
        __declspec(property(get=get_ReservedCount)) int ReservedCount;
        int get_ReservedCount() { return this->pool_.ReservedCount; };

        // Get the number of network resources that are managed by this instance
        //
        __declspec(property(get=get_TotalCount)) int TotalCount;
        int get_TotalCount() { return this->pool_.TotalCount; };

        // Get a flag indicating if this object has been successfully
        // initialized.
        //
        __declspec(property(get = get_Initialized)) bool Initialized;
        bool get_Initialized() const { return this->initialized_; };

        // Get the timestamp for when the cache was last refreshed from the
        // Network Inventory Manager.
        //
        __declspec(property(get=get_LastRefreshTime)) Common::DateTime LastRefreshTime;
        Common::DateTime get_LastRefreshTime();

        // Get the number of ghost reservations.
        //
        __declspec(property(get=get_GhostCount)) int GhostCount;
        int get_GhostCount();

    protected:

        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:

        class ReserveWithRetryAsyncOperation;

        class ReleaseWithRetryAsyncOperation;

        // Reserve call used by ReserveWithRetry
        Common::ErrorCode Reserve(std::wstring const &  reservationId, OverlayNetworkResourceSPtr & networkResource);

        // Release call used by ReleaseWithRetry
        Common::ErrorCode Release(std::wstring const &reservationId);

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

        // Checks if reservation pool is exhausted and then marks the pool as
        // not initialized while refresh occurs. Refresh is performed by calling 
        // the replenish api on the Network Driver.
        void ReplenishReservationPool();

        // Sets up pool refresh timer.
        void SetupPoolRefreshTimer();

        // Cleans up pool refresh timer.
        void CleanupPoolRefreshTimer();

        // Holds the reservable pool of overlay network resources, along with their reservations, if
        // any. Note that this pool is already thread safe, so does not need
        // to be protected by the lock below.
        //
        OverlayNetworkReservationManager pool_;

        // Indicates if this instance has been initialized.
        //
        bool initialized_;

        // Holds the lock used to synchronize access to the fields following
        // it.
        //
        Common::ExclusiveLock lock_;

        // Holds the timestamp for the last successful refresh.
        //
        Common::DateTime lastRefreshTime_;

        // Holds the set of known ghost reservations. These are reservations
        // that were held when their associated network resources disappeared from the list
        // of eligible network resources. Such reservations are removed from the pool, and
        // placed in this set. They are removed when they are released or
        // they reserve a new network resource.
        //
        std::unordered_set<std::wstring> ghosts_;

        // This contains a reference to the method to call whenever the set of
        // ghost reservations changes.
        //
        GhostChangeCallback ghostChangeCallback_;

        // Overlay network driver callback to replenish network resources
        InternalReplenishNetworkResourcesCallback internalReplenishNetworkResourcesCallback_;

        // Timer used to fire callback that checks if pool needs to be replenished.
        // The pool will be locked until this refresh completes. Calls to release/acquire network
        // resources will fail with a 'Not Ready' status and need to have retry logic, in this case.
        Common::TimerSPtr updatePoolTimerSPtr_;

        // Time duration to wait for before firing the replenish pool call.
        Common::TimeSpan poolRefreshRetryInterval_ = HostingConfig::GetConfig().ReplenishOverlayNetworkReservationPoolRetryInterval;
    };
}