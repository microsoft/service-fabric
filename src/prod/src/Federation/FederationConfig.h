// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct VoteEntryConfig
    {
        VoteEntryConfig()
        {
        }

        VoteEntryConfig(NodeId id, std::wstring const & type, std::wstring const & connectionString, std::wstring const & ringName = L"")
            : Id(id), Type(type), ConnectionString(connectionString), RingName(ringName)
        {
        }
        
        NodeId Id;
        std::wstring Type;
        std::wstring ConnectionString;
        std::wstring RingName;
    };

    struct VoteConfig : public std::vector<VoteEntryConfig>
    {
        static VoteConfig Parse(Common::StringMap const & entries);

        VoteConfig::const_iterator find(NodeId nodeId, std::wstring const & ringName = L"") const;

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
    };

    class FederationConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FederationConfig, "Federation")

        /* -------------- Routing table -------------- */

        // The number of nodes in the successor and predecessor direction, that a node keeps leases with.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", NeighborhoodSize, 4, Common::ConfigEntryUpgradePolicy::NotAllowed, Common::InRange<int>(1, 8));
        // Obsolete
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", NodeUnknownStateDuration, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);
        // The interval for a node to send EdgeProbe message when its neighborhood information is not complete.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", EdgeProbeInterval, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Static);
        // The interval for a node to send TokenProbe message when it finds a token gap.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", TokenProbeInterval, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Static);
        // The upper bound for the interval of sending TokenProbe message.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", TokenProbeIntervalUpperBound, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);
        // The number of continuous token probe allowed before throttling.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", TokenProbeThrottleThreshold, 2, Common::ConfigEntryUpgradePolicy::Static);
        // The duration node information will be included in P2P message after its last access.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", NeighborhoodExchangeTimeout, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Static);
        // The duration node information will be kept in routing table after its last access.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", NodeInformationExpirationTimeout, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Static);
        // The interval for periodic tracing
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", PeriodicTraceInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval to compact routing table to remove old entries.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", RoutingTableCompactInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static);
        // The interval to check routing table health.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", RoutingTableHealthCheckInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static);
        // The limit on number of nodes kept in routing table that will trigger a compact.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", RoutingTableCapacity, 4096, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max number of nodes to keep in the neighborhood, including shutdown ones.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", MaxNodesToKeepInNeighborhood, 128, Common::ConfigEntryUpgradePolicy::Static);
        // The max number of node information that will be included in P2P message.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", MaxNeighborhoodHeaders, 64, Common::ConfigEntryUpgradePolicy::Static);
        // The system can only handle a fixed number of gaps, once this number is exceeded, 
        // it prevents further gaps to register, causing nodes to go down. This is desirable
        // because the system has degraded. 
        INTERNAL_CONFIG_ENTRY(int, L"Federation", MaxGapsInCluster, 30, Common::ConfigEntryUpgradePolicy::NotAllowed);

        /* -------------- Messaging -------------- */

        // Timeout for connection setup on both incoming and accepting side (including security negotiation in secure mode)
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ConnectionOpenTimeout, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Static);
        // Timeout for federation layer messages.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", MessageTimeout, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Static);
        // Connection idle timeout
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ConnectionIdleTimeout, Common::TimeSpan::FromMinutes(15), Common::ConfigEntryUpgradePolicy::Static);
        // Default timeout for routing layer retry.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", RoutingRetryTimeout, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        // The wait interval to send LivenessUpdate message when there are pending incoming requests.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LivenessUpdateInterval, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Static);
        // This is deprecated because message size checking is disabled for peer-to-peer mode.
        DEPRECATED_CONFIG_ENTRY(int, L"Federation", MaxMessageSize, 64 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static);
        // Send queue size limit in bytes, per target, set to 0 to disable
        INTERNAL_CONFIG_ENTRY(uint, L"Federation", SendQueueSizeLimit, 64 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static);
        // Specifies if error checking on federation message header and body is enabled in non-secure mode
        PUBLIC_CONFIG_ENTRY(bool, L"Federation", MessageErrorCheckingEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        // The duration to keep broadcast context.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", BroadcastContextKeepDuration, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static);
        // The number of children in the broadcast spanning tree.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", BroadcastPropagationFactor, 8, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(1));
        // The max number of nodes in each child of spanning tree.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", MaxMulticastSubtreeSize, 1000, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(1));

        /* -------------- Join protocol -------------- */

        // The interval to retry sending neighborhood query.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", NeighborhoodQueryRetryInterval, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time a join lock is granted to a joining node.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", JoinLockDuration, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);
        // The interval to run join protocol state machine.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", JoinStateMachineInterval, Common::TimeSpan::FromSeconds(0.25), Common::ConfigEntryUpgradePolicy::Static);
        // The interval for non-seed node to wait before quorum of seed nodes.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", NonSeedNodeJoinWaitInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval a node is allowed to resend lock request message.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LockRequestTimeout, Common::TimeSpan::FromSeconds(0.5), Common::ConfigEntryUpgradePolicy::Static);
        // The maximum time a node is allowed to acquire its first routing token after it joins the federation.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", TokenAcquireTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
        // The timeout for join throttle.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", JoinThrottleTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The timeout for a competing joining node to be considered active.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", JoinThrottleActiveInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval to check throttle.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", JoinThrottleCheckInterval, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The threshold from which throttle is to be removed.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", JoinThrottleLowThreshold, 2, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The threshold from which throttle is to be added.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", JoinThrottleHighThreshold, 4, Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Ping and update protocols -------------- */

        // The interval to send ping messages.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", PingInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Static);
        // The interval to send update messages.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", UpdateInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Static);
        // Maximum number of update messages to send each round.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", MaxUpdateTarget, 6, Common::ConfigEntryUpgradePolicy::Static);
        // The interval to send ping messages to external rings.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ExternalRingPingInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);

        /* -------------- Bootstrap protocol -------------- */

        // A vote represents a single count towards a quorum of in a cluster.
        // A vote is assigned to a vote owner(node) by a vote authority.
        // Vote Authorities are typically nodes (seed nodes) in the cluster.
        // A cluster needs a quorum of votes present to guarantee its health and ability
        // to stay up.  Losing a quorum of votes will cause the cluster to go down.
        // An alternative to seed nodes are SQL votes, where the vote authority is not
        // a node in the Cluster, but a SQL Server instance. In this case, the node with
        // the closest Id to the SQL vote Id acts as a proxy. The votes selected through
        // configuration, must be the same across all nodes.
        // ID is a string that is parsed into a long integer that represents the ID of the vote
        // in the cluster. The Type can be one of two options: SeedNode or SqlServer,
        // depending on the vote authority used by the cluster. Depending on the type is the format
        // for the connection string. The connection string for SeedNode is the NodeEndpoint for
        // the node with the same NodeID. For type=SqlServer is a connection string to a SQL Server
        // 2008 and up.
        // An example to a SeedNode is : '0 = SeedNode,10.0.0.1:10000'.
        // An example to a SQL vote is: 'sqlvote1 = SqlServer,Provider=SQLNCLI10;Server=.\SQLEXPRESS;Database=master;Integrated Security=SSPI'
        PUBLIC_CONFIG_GROUP(VoteConfig, L"Votes", Votes, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Nodes in the cluster need to maintain a global lease with the voters.
        // Voters submit their global leases to be propagated across the cluster for this duration.
        // If the duration expires, then the lease is lost. Loss of quorum of leases causes a node
        // to abandon the cluster, by failing to receive communication with a quorum of nodes in
        // this period of time.  This value needs to be adjusted based on the size of the cluster.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTicketLeaseDuration, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static);
        // The lease interval for a bootstrap ticket.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", BootstrapTicketLeaseDuration, Common::TimeSpan::FromSeconds(100), Common::ConfigEntryUpgradePolicy::Static);
        // The lease interval for acquiring the ownership for a shared (sql) vote.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", VoteOwnershipLeaseInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);
        // The interval to send bootstrap ticket request.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", BootstrapVoteRequestInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);
        // The time to wait to send the next bootstrap ticket request after a failed attempt.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", BootstrapVoteAcquireRetryInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Static);
        // The interval to run the bootstrap state machine.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", BootstrapStateMachineInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static);
        // The interval to renew global ticket.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTicketRenewInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Static);
        // Margin makes sure we don't transfer Super Tickets that are about to expire "soon".
        // Otherwise, a super seed node may create a ring only to lose global lease soon afterwards.
        // This is the amount of time vote owners have to join a newly formed ring and
        // issue a new global lease.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", BootstrapTicketAcquireLimit, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Static);
        // The ratio to increase uncertainty interval as time elapses.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", GlobalTimeClockDriftRatio, 512, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Obsolete.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTimeUncertaintyIncreseInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval to trace global time.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTimeTraceInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // When the uncertainty interval exceeds this limit a new epoch will be issued.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTimeUncertaintyIntervalUpperBound, Common::TimeSpan::FromSeconds(0.3), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max interval to decrease per epoch.  Must be equal or smaller than GlobalTimeUncertaintyMaxIncrease.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTimeUncertaintyMaxDecrease, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max interval to increase per epoch.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTimeUncertaintyMaxIncrease, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time to wait before issing new epoch.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", GlobalTimeNewEpochWaitInterval, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval to retry locating voter store.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", VoterStoreRetryInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The time a voter store replica needs to wait before starting bootstrap phase.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", VoterStoreBootstrapWaitInterval, Common::TimeSpan::FromSeconds(3600), Common::ConfigEntryUpgradePolicy::Static);
        // The time a voter store replica pings the previous replica to check its liveness.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", VoterStoreLivenessCheckInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        /* -------------- Lease -------------- */

        // Duration that a lease lasts between a node and its neighbors.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LeaseDuration, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Duration that a lease lasts between a node and its neighbors accross data center.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LeaseDurationAcrossFaultDomain, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Max LeaseDuration allowed.  LeaseDuration and LeaseDurationAcrossFaultDomain must not exceed this value during upgrade.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", MaxLeaseDuration, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Duration that driver bugcheck if there is no ioctl call from user mode
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", UnresponsiveDuration, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Time interval that user mode is performing heartbeat
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", HeartbeatInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);
        // After a lease loss, the duration allowed for a node to continue operating before
        // a successful arbitration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LeaseSuspendTimeout, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Static);
        // The number of retry messages within a lease interval.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", LeaseRetryCount, 3, Common::ConfigEntryUpgradePolicy::Static);
        // The starting point of first renew message within a lease interval; it is interpreted as 1 over LeaseRenewBeginRatio.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", LeaseRenewBeginRatio, 6, Common::ConfigEntryUpgradePolicy::Static);
        // The TTL granted by lease driver to leasing applications.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ApplicationLeaseDuration, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static);
        // The timeout for arbitration request.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ArbitrationTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max timeout for arbitration request.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", MaxArbitrationTimeout, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The delay between successive arbitration requests starting from the 4th request.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ArbitrationRequestDelay, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval between arbitration retry requests.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ArbitrationRetryInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The amount of time arbitration records are used to reject requests from other nodes.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ArbitrationCleanupInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max duration lease is renewed indirectly; after this duration, indirect lease will be stopped.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", MaxConsecutiveIndirectLeaseDuration, Common::TimeSpan::FromSeconds(600), Common::ConfigEntryUpgradePolicy::Static);
        // The maintenance thread interval.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LeaseMaintenanceInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The process assert exit timeout.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ProcessAssertExitTimeout, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval for node implicit lease within its immediate neighbors.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ImplicitLeaseInterval, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max interval for node implicit lease within its immediate neighbors.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", MaxImplicitLeaseInterval, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The suspicion level for a weak report.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", ArbitrationWeakSuspicionLevel, 1, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The suspicion level for a strong report.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", ArbitrationStrongSuspicionLevel, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The suspicion level for a rejected report.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", ArbitrationRejectSuspicionLevel, 9, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The suspicion level threshold for a node to be considered suspicious.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", SuspicionLevelThreshold, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The number of reports for a node to become suspicious.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", SuspicionReportThreshold, 3, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The difference in suspicion level for two nodes to be considered substantially different.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", SuspicionLevelComparisonThreshold, 6, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The suspicion level threshold for a node to be kept alive by previous arbitration requests.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", KeepAliveSuspicionLevelThreshold, 9, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval for arbitration record suspicion level to expire.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ArbitrationRecordSuspicionExpireInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval for arbitrator to wait for a delayed arbitration.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", ArbitrationDelayInterval, Common::TimeSpan::FromSeconds(15), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval allowed for clock inaccuracy.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LeaseClockUncertaintyInterval, Common::TimeSpan::FromSeconds(0.03), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The disk sector size, default is 4096 bytes.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", DiskSectorSize, 4096, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The disk probe buffer sector count, default is 1, the buffer size = DiskSectorSize (4096 bytes) * DiskProbeBufferSectorCount (1) = 4096 bytes, negative number enables the sector count to be random in between [1, abs(DiskProbeBufferSectorCount)].
        INTERNAL_CONFIG_ENTRY(int, L"Federation", DiskProbeBufferSectorCount, 1, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The disk probe buffer data, default is 0xff, 0 and positive number enables the buffer to be filled with the config value, -1 enables the buffer data to be random.
        INTERNAL_CONFIG_ENTRY(int, L"Federation", DiskProbeBufferData, 255, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<int>(-1, 255));
        // The config to enable disk probe buffer data at random, default is false. This config overrides DiskProbeBufferData.
        INTERNAL_CONFIG_ENTRY(bool, L"Federation", DiskProbeReopenFile, true, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The config to set how long the delay for lease agent close.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Federation", DelayLeaseAgentCloseInterval, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The config to set the threshold to do faster pulling of the app ttl
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Federation", LeaseMonitorRenewLevelThreshold, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic);

        /* Point to point communcation*/
        // Specify max thread count for loopback job queue, default to 0, which means using processor count
        INTERNAL_CONFIG_ENTRY(int, L"Federation", LoopbackJobQueueThreadMax, 0, Common::ConfigEntryUpgradePolicy::Static, Common::NoLessThan(0));
        // Specify whether to do extra tracing for loopback job queue
        INTERNAL_CONFIG_ENTRY(bool, L"Federation", TraceLoopbackJobQueue, false, Common::ConfigEntryUpgradePolicy::Static);

        /* security */
        // Bitmask for cluster certificate chain validation, e.g. CRL checking
        // Values often used:
        // 0x80000000 CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY: disable certificate revocation list (CRL) downloading when unavailable
        // 0x00000004 CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL : disable certificate trust list (CTL) downloading on Windows machines
        // lacking Internet access. Details about CTL can be found at:
        // https://docs.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-R2-and-2012/dn265983(v=ws.11)
        // Setting to 0 disables CRL checking, but it does not disable CTL downloading.
        // Full list of supported values is documented by dwFlags of CertGetCertificateChain:
        // http://msdn.microsoft.com/en-us/library/windows/desktop/aa376078(v=vs.85).aspx
#ifdef PLATFORM_UNIX
        // Disable CRL checking on Linux until CRL installation and update issues are figured out
        INTERNAL_CONFIG_ENTRY(uint, L"Federation", X509CertChainFlags, 0, Common::ConfigEntryUpgradePolicy::Dynamic);
#else
        INTERNAL_CONFIG_ENTRY(uint, L"Federation", X509CertChainFlags, 0x40000000, Common::ConfigEntryUpgradePolicy::Dynamic);
#endif
        // Whether to ignore CRL offline error for cluster security 
        INTERNAL_CONFIG_ENTRY(bool, L"Federation", IgnoreCrlOfflineError, true, Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}
