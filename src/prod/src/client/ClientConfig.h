// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class ClientConfig : Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(ClientConfig, "Client")

        //A collection of addresses (connection strings) on different nodes that can be used to communicate with the Naming Service. 
        //Initially the Client connects selecting one of the addresses randomly.  
        //If more than one connection string is supplied and a connection fails because of a communication or timeout error, the Client switches to use the next address sequentially.  
        //See the Naming Service Address retry section for details on retries semantics.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"FabricClient", NodeAddresses, L"", Common::ConfigEntryUpgradePolicy::Static);
        //Connection timeout interval for each time client tries to open a connection to the gateway
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", ConnectionInitializationTimeout, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Number of partitions cached for service resolution (set to 0 for no limit).
        PUBLIC_CONFIG_ENTRY(int, L"FabricClient", PartitionLocationCacheLimit, 100000, Common::ConfigEntryUpgradePolicy::Static);
        //The interval between consecutive polls for service changes from the client to the gateway for registered service change notifications callbacks
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", ServiceChangePollInterval, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The interval at which the FabricClient transport sends keep-alive messages to the gateway. For 0, keepAlive is disabled. Must be a positive value
        PUBLIC_CONFIG_ENTRY(int, L"FabricClient", KeepAliveIntervalInSeconds, 20, Common::ConfigEntryUpgradePolicy::Static, Common::GreaterThan(-1));
        // The maximum number of requests that can be packaged together in a poll request. Used to test the notification paging of the requests from client to gateway.
        // Default is 0, which shows that the feature is disabled.
        TEST_CONFIG_ENTRY(int, L"FabricClient", MaxServiceChangePollBatchedRequests, 0, Common::ConfigEntryUpgradePolicy::Dynamic); 
        // Expiration for non-retryable error cached by client
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", NonRetryableErrorExpiration, Common::TimeSpan::FromMinutes(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // Health 
        //

        // The timeout for a report message sent to Health Manager.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", HealthOperationTimeout, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The interval at which reporting component sends accumulated health reports to Health Manager.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", HealthReportSendInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // Whether to enable health reporting
        INTERNAL_CONFIG_ENTRY(bool, L"FabricClient", IsHealthReportingEnabled, true, Common::ConfigEntryUpgradePolicy::NotAllowed);
        // The maximum number of reports that can be batched in the message sent to Health Manager.
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", MaxNumberOfHealthReportsPerMessage, 500, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The maximum number of health reports that can be queued for processing by a reporting component.
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", MaxNumberOfHealthReports, 10000, Common::ConfigEntryUpgradePolicy::Static);
        // The interval at which reporting component re-sends accumulated health reports to Health Manager.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", HealthReportRetrySendInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The back off interval step added at each successive ServiceTooBusy reply from Health Manager, up to HealthReportSendMaxBackOffInterval.
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", HealthReportSendBackOffStepInSeconds, 5, Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max back off interval that can be applied when Health Manager returns ServiceTooBusy.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", HealthReportSendMaxBackOffInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        //
        // FileTransfer
        //

        // The back-off interval before retrying the operation
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", RetryBackoffInterval, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Dynamic);
        // The max number of files that are transferred in parallel
        PUBLIC_CONFIG_ENTRY(uint, L"FabricClient", MaxFileSenderThreads, 10, Common::ConfigEntryUpgradePolicy::Static);
    };
}
