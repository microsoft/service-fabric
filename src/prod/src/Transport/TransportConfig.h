// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class TransportConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(TransportConfig, "Transport")

        // The ceiling for the number of active activities in fabric.exe
        // Once the threshold is reached, incoming messages can be dropped.
        // setting to 0 will disable thread count throttling.
        // This is only applicable for fabric.exe.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", ThreadThrottle, 400, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Limit how many threads are allowed in testing, crash the process immediately when reaching the limit.
        // setting to 0 will disable thread count test limit
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", ThreadTestLimit, 0, Common::ConfigEntryUpgradePolicy::Static);
        // If the thread count exceeds the configured ThreadThrottle value for ThrottleTestAssertThreshold consecutive
        // performance measurements, then a TestAssert is triggered.
        // setting to 0 will disable the TestAssert.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", ThrottleTestAssertThreshold, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));
        // The process has this configurable value number of memory as a threshold.
        // Once the threshold is reached, incoming messages can be dropped.
        // setting to 0 will disable memory throttling.
        // This is only applicable for fabric.exe.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", MemoryThrottleInMB, 8192, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval to apply memory throttle, after which memory throttle limit will be increased
        // by MemoryThrottleIncrementRatio.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", MemoryThrottleInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);
        // The interval from which the stable memory consumption is calculated.  If the stable consumption average
        // is close to the threshold by the increment ratio, threshold will be increased.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", MemoryStableInterval, Common::TimeSpan::FromSeconds(1800), Common::ConfigEntryUpgradePolicy::Static);
        // The ratio to increase the memory throttle limit.
        INTERNAL_CONFIG_ENTRY(double, L"Transport", MemoryThrottleIncrementRatio, 0.1, Common::ConfigEntryUpgradePolicy::Static);
        // The upper limit of memory consumption.  If memory is above this limit for MemoryThrottleInterval,
        // the process will be restarted.  0 if no upper limit.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", MemoryThrottleUpperLimitInMB, 16384, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Incoming connections will be dropped when reaching this threshold.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", IncomingConnectionThrottle, 10000, Common::ConfigEntryUpgradePolicy::Dynamic);
        // Delay before retry accept on failure
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", AcceptRetryDelay, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Determines how FQDN are resolved.  Valid values are "unspecified/ipv4/ipv6".
        PUBLIC_CONFIG_ENTRY(std::wstring, L"Transport", ResolveOption, L"unspecified", Common::ConfigEntryUpgradePolicy::Static);

        INTERNAL_CONFIG_ENTRY(uint, L"Transport", TcpReceiveBufferSize, 64 * 1024, Common::ConfigEntryUpgradePolicy::Static, Common::UIntGreaterThan(0));

        // TCP send batch size limit in bytes
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", SendBatchSizeLimit, 16 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static, Common::UIntNoLessThan(64 * 1024));
        // Send timeout for detecting stuck connection. TCP failure reports are not reliable in some environment.
        // This may need to be adjusted according to available network bandwidth and size of outbound data (*MaxMessageSize/*SendQueueSizeLimit).
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Transport", SendTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);

        // Specify threshold for MessageHeaders::CompactIfNeeded, which compacts if the byte count of all headers marked for deletion is beyond threshold
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", MessageHeaderCompactThreshold, 512, Common::ConfigEntryUpgradePolicy::Static);

        // Receive chunk size for non-secure mode
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", DefaultReceiveChunkSize, 4*1024, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<uint>(3, 8*1024*1024));

        // Chunk size of SSL receive buffer, it must be at least twice as large as SSL record size:
        // SecPkgContext_StreamSizes{cbHeader + cbMaximumMessage + cbTrailer}
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", SslReceiveChunkSize, 64*1024, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<uint>(32*1024, 8*1024*1024));

        // Indicate how long an outgoing message can be queued until being sent or dropped, set to 0 to disable
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", DefaultOutgoingMessageExpiration, Common::TimeSpan::FromSeconds(180), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));
        // Indicate how often periodic outgoing message expiration check is done, set to 0 to disable
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", OutgoingMessageExpirationCheckInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static);

        // Interval of periodic listener state trace, set to 0 to disable.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", ListenerStateTraceInterval, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        // Default send queue size limit in bytes, per target, set to 0 to disable
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", DefaultSendQueueSizeLimit, 64 * 1024 * 1024, Common::ConfigEntryUpgradePolicy::Static, Common::InRange<uint>(0, 1024 * 1024 * 1024));

        // To make sure send queue size limit is large enough for a given send queue to hold at lease some messages at max message size.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", MinSendQueueCapacityInMessageCount, 1, Common::ConfigEntryUpgradePolicy::Static, Common::UIntGreaterThan(0));

        // When listen address has hostname and "ResolveOption" set to "unspecified", we need to listen on both
        // IPv4 0.0.0.0 and IPv6 [::], and we need to make sure the two listen ports are the same, otherwise we
        // will have to publish two separate listen addresses instead of one with hostname. The following setting
        // specify how many times we retry on finding a dynamic listen port common to IPv4 and IPv6.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", DynamicListenPortTrialMax, 3, Common::ConfigEntryUpgradePolicy::Static, Common::UIntGreaterThan(0));
        // Retry delay of choosing dynamic listen port for hostname address, see DynamicListenPortTrialMax
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", DynamicListenPortRetryDelay, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanGreaterThan(Common::TimeSpan::Zero));

        // MaxMessageSize for IPC
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", IpcMaxMessageSize, 16*1024*1024, Common::ConfigEntryUpgradePolicy::Static);
        // Connection idle timeout: connection gets closed after being inactive for a while
        // Setting to 0 or negative to disable
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", ConnectionIdleTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));
        // Maximum for the number of connection cleanup threads
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", ConnectionCleanupThreadMax, 10, Common::ConfigEntryUpgradePolicy::Static, Common::UIntGreaterThan(0));
        // When cleaning up an idle connection, if its send target still have external reference counts, the following threshold
        // will be checked. The connection is cleaned up only if the total number of send targets reaches the threshold.
        // Setting to 0 or negative value will disable such threshold checking.
        DEPRECATED_CONFIG_ENTRY(uint, L"Transport", SendTargetSoftLimit, 1000, Common::ConfigEntryUpgradePolicy::Static);
        // If no receive is posted for the threshold, something is probably stuck and an assert
        // will be fired. Setting to 0 or negative to disable.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", ReceiveMissingThreshold, Common::TimeSpan::FromSeconds(200), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));
        // Probability to fail fast when receive missing is detected
        INTERNAL_CONFIG_ENTRY(double, L"Transport", FailFastProbabilityOnReceiveMissingDetected, 0.1, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0));
        // Timeout for connection setup on both incoming and accepting side (including security negotiation in secure mode)
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"Transport", ConnectionOpenTimeout, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Static);
        // How long can messages be sent on a connection with unconfirmed instance
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Transport", ConnectionConfirmWaitLimit, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));
        // How long IpcClient waits before reconnecting, after connection failure
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", IpcReconnectDelay, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));
        // IpcClient exits process when disconnect count reaches the following limit, set to 0 to disable such process exit.
        INTERNAL_CONFIG_ENTRY(uint, L"Transport", IpcClientDisconnectLimit, 100, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time Ipc server and client connection needs to remain idle before TCP starts sending keepalive probes.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", IpcKeepaliveIdleTime, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static);

        // Default close delay for scheduled close
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Transport", DefaultCloseDelay, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        // Timeout for draining send when closing a connection
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"Transport", CloseDrainTimeout, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanNoLessThan(Common::TimeSpan::Zero));

        // Whether to disable message size checking in peer-to-peer mode, default to true, as peers trust one another completely
        INTERNAL_CONFIG_ENTRY(bool, L"Transport", MessageSizeCheckDisabledOnPeers, true, Common::ConfigEntryUpgradePolicy::Static);

        // Whether to enable TCP fast loopback
        INTERNAL_CONFIG_ENTRY(bool, L"Transport", TcpFastLoopbackEnabled, true, Common::ConfigEntryUpgradePolicy::Dynamic);

        // Whether to enable TCP_NODELAY
        INTERNAL_CONFIG_ENTRY(bool, L"Transport", TcpNoDelayEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

        // TCP listen backlog, default value 0 is interpreted as SOMAXCONN
        INTERNAL_CONFIG_ENTRY(int, L"Transport", TcpListenBacklog, 0, Common::ConfigEntryUpgradePolicy::Static, Common::NoLessThan(0));

        // Comma-separated list of actors for which per-message tracing is disabled
        INTERNAL_CONFIG_ENTRY(std::wstring, L"Transport", PerMessageTraceDisableList, L"", Common::ConfigEntryUpgradePolicy::Static);

        // Specifies if we support multi homing for non loopback addresses.
        INTERNAL_CONFIG_ENTRY(bool, L"Transport", AlwaysListenOnAnyAddress, true, Common::ConfigEntryUpgradePolicy::Static);

        // Whether to enable in-memory channel.
        TEST_CONFIG_ENTRY(bool, L"Transport", InMemoryTransportEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
        
        // Count of concurrent event loops for sockets, linux only, default to 0 to use processor current. 
        DEPRECATED_CONFIG_ENTRY(uint, L"Transport", EventLoopConcurrency, 0, Common::ConfigEntryUpgradePolicy::Static);
        // Cleanup delay for fd context used in event loop
        DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, L"Transport", EventLoopCleanupDelay, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static, Common::TimeSpanGreaterThan(Common::TimeSpan::Zero));
        // Enable support for Unreliable over IPC
        INTERNAL_CONFIG_ENTRY(bool, L"Transport", UseUnreliableForRequestReply, false, Common::ConfigEntryUpgradePolicy::Static);
        // For testing IPv6 usage.  If true, transport will fail open if the endpoint is not an IPv6 address
        TEST_CONFIG_ENTRY(bool, L"Transport", TestOnlyValidateIPv6Usage, false, Common::ConfigEntryUpgradePolicy::Static);

        // Default setting for error checking on frame header in non-secure mode, component setting overrides this
        PUBLIC_CONFIG_ENTRY(bool, L"Transport", FrameHeaderErrorCheckingEnabled, true, Common::ConfigEntryUpgradePolicy::Static);

        // Default setting for error checking on message header and body in non-secure mode, component setting overrides this
        PUBLIC_CONFIG_ENTRY(bool, L"Transport", MessageErrorCheckingEnabled, false, Common::ConfigEntryUpgradePolicy::Static);
    };
}
