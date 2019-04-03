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
        PUBLIC_CONFIG_ENTRY(uint, L"FabricClient", MaxFileSenderThreads, 4, Common::ConfigEntryUpgradePolicy::Static);

        //
        // Chunk based FileTransfer
        //

        // The max number of file chunks that are transferred in parallel
        INTERNAL_CONFIG_ENTRY(uint, L"FabricClient", MaxFileChunkSenderThreads, 4, Common::ConfigEntryUpgradePolicy::Static);

        // The max number of file chunks that are processed in parallel by file receiver
        INTERNAL_CONFIG_ENTRY(uint, L"FabricClient", MaxFileChunkReceiverThreads, 8, Common::ConfigEntryUpgradePolicy::Static);

        // The interval to wait for create upload session message response before reporting error
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileCreateMessageResponseWaitInterval, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The interval to wait before resending unacknowledged file chunks by the file sender
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileChunkResendWaitInterval, Common::TimeSpan::FromSeconds(20), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The initial interval to wait before timing out on upload. This is set to large value since single file can be very large
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", PerFileUploadWaitTime, Common::TimeSpan::FromMinutes(100), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The initial interval to wait before resending unacknowledged file chunks by the file sender before upload protocol is selected
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileCreateMessageInitialResponseWaitInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The max number of attempts allowed for resending the unacknowledged file chunks by the file sender
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", FileChunkResendRetryAttempt, 5, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The interval for retrying of sending the file chunks
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileChunkRetryInterval, Common::TimeSpan::FromMilliseconds(2000), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The max interval to wait before retrying the sending of the file chunks
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileChunkMaxRetryInterval, Common::TimeSpan::FromMilliseconds(10000), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The interval for retrying the create upload session operation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileCreateSendRetryInterval, Common::TimeSpan::FromMilliseconds(200), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The max number of attempts made for retrying the create file upload operation to send it to the cluster
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", FileCreateSendAttempt, 225, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The  number of successful file create requests sent (after which filesender waits for response).
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", NumberOfFileCreateRequestsSent, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The max number of file chunk send attempts for retrying the operation
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", FileChunkRetryAttempt, 500, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The interval after which file upload commit message is sent again because of no response from the file store service.
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileUploadCommitRetryInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The max number of file chunk send attempts for retrying the operation
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", FileUploadCommitRetryAttempt, 200, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of times file can be tried to resend when commit message fails with missing file chunks.
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", FileUploadResendRetryAttempt, 8, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of times to try to resend the file before switching to old protocol. This should be less than FileUploadResendRetryAttempt.
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", SwitchUploadProtocolResendRetryAttempt, 3, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The back-off interval before retrying the resend file operation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileUploadResendRetryBackoffInterval, Common::TimeSpan::FromSeconds(3), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of times file action message is message is retried after getting retryable error when trying to send the action message.
        INTERNAL_CONFIG_ENTRY(int, L"FabricClient", SendMessageActionRetryAttempt, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The back-off interval before retrying sending the message action operation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", SendMessageActionRetryBackoffInterval, Common::TimeSpan::FromMilliseconds(100), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The limit under which gateway not reachable error will be treated as transient error
        INTERNAL_CONFIG_ENTRY(uint, L"FabricClient", GatewayNotReachableThresholdLimit, 25, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The time file sender waits for the job queue to finish before bringing down the process
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", MaxCloseJobQueueWaitDuration, Common::TimeSpan::FromMinutes(5), Common::ConfigEntryUpgradePolicy::Dynamic, Common::TimeSpanGreaterThan(Common::TimeSpan::FromMinutes(3)));

        // The max number of consecutive failure to establish connection to cluster before switching to non-chunk version of upload protocol.
        INTERNAL_CONFIG_ENTRY(uint, L"FabricClient", SwitchUploadProtocolThreshold, 5, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The number of chunks batched together to be sent to the fabric gateway.
        INTERNAL_CONFIG_ENTRY(uint, L"FabricClient", FileChunkBatchCount, 50, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The maximum number of chunks send operation that hasn't completed from the earlier batch before sending next batch of file chunks.
        INTERNAL_CONFIG_ENTRY(uint, L"FabricClient", MaxAllowedPendingFileChunkSendBeforeNextBatch, 10, Common::ConfigEntryUpgradePolicy::Dynamic);

        // The back-off interval before retrying the resend file operation
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricClient", FileChunkBatchUploadInterval, Common::TimeSpan::FromSeconds(2), Common::ConfigEntryUpgradePolicy::Dynamic);

        // The config to enable tracing of file chunk job queue
        INTERNAL_CONFIG_ENTRY(bool, L"FabricClient", IsEnableFileChunkQueueTracing, false, Common::ConfigEntryUpgradePolicy::Static);

    };
}
