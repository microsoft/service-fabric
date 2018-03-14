// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientInternalSettings
    {
        DENY_COPY(FabricClientInternalSettings);

    public:
        explicit FabricClientInternalSettings(std::wstring const & traceId);

        FabricClientInternalSettings(
            std::wstring const & traceId,
            ServiceModel::FabricClientSettings && settings);

        FabricClientInternalSettings(
            std::wstring const & traceId,
            ULONG keepAliveIntervalInSeconds);
        
        ~FabricClientInternalSettings();

        __declspec(property(get=get_ClientFriendlyName)) std::wstring const & ClientFriendlyName;
        std::wstring const &get_ClientFriendlyName() const { return clientFriendlyName_; }
                
        __declspec(property(get=get_PartitionLocationCacheLimit)) ULONG PartitionLocationCacheLimit;
        ULONG get_PartitionLocationCacheLimit() const;
                
        __declspec(property(get=get_KeepAliveIntervalInSeconds)) ULONG KeepAliveIntervalInSeconds;
        ULONG get_KeepAliveIntervalInSeconds() const;

        __declspec(property(get=get_ConnectionIdleTimeoutInSeconds)) ULONG ConnectionIdleTimeoutInSeconds;
        ULONG get_ConnectionIdleTimeoutInSeconds() const;

        __declspec(property(get=get_KeepAliveInterval)) Common::TimeSpan const KeepAliveInterval;
        Common::TimeSpan const get_KeepAliveInterval() const;

        __declspec(property(get=get_ConnectionIdleTimeout)) Common::TimeSpan const ConnectionIdleTimeout;
        Common::TimeSpan const get_ConnectionIdleTimeout() const;

        __declspec(property(get=get_ServiceChangePollInterval)) Common::TimeSpan const ServiceChangePollInterval;
        Common::TimeSpan const get_ServiceChangePollInterval() const;

        __declspec(property(get=get_ConnectionInitializationTimeout)) Common::TimeSpan const ConnectionInitializationTimeout;
        Common::TimeSpan const get_ConnectionInitializationTimeout() const;

        __declspec(property(get=get_HealthOperationTimeout)) Common::TimeSpan const HealthOperationTimeout;
        Common::TimeSpan const get_HealthOperationTimeout() const;
                
        __declspec(property(get=get_HealthReportSendInterval)) Common::TimeSpan const HealthReportSendInterval;
        Common::TimeSpan const get_HealthReportSendInterval() const;

        __declspec(property(get=get_HealthReportRetrySendInterval)) Common::TimeSpan const HealthReportRetrySendInterval;
        Common::TimeSpan const get_HealthReportRetrySendInterval() const;

        __declspec(property(get=get_PartitionLocationCacheBucketCount)) ULONG PartitionLocationCacheBucketCount;
        ULONG get_PartitionLocationCacheBucketCount() const;
                
        __declspec(property(get=get_NotificationGatewayConnectionTimeout)) Common::TimeSpan const NotificationGatewayConnectionTimeout;
        Common::TimeSpan const get_NotificationGatewayConnectionTimeout() const;

        __declspec(property(get=get_NotificationCacheUpdateTimeout)) Common::TimeSpan const NotificationCacheUpdateTimeout;
        Common::TimeSpan const get_NotificationCacheUpdateTimeout() const;

        __declspec(property(get=get_AuthTokenBufferSize)) size_t AuthTokenBufferSize;
        size_t const get_AuthTokenBufferSize() const;

        __declspec(property(get=get_IsStale)) bool IsStale;
        bool get_IsStale() const { return isStale_.load(); }

        ServiceModel::FabricClientSettings GetSettings() const;

        bool CanDynamicallyUpdate(
            ServiceModel::FabricClientSettings const & newSettings) const;

        void MarkAsStale();
        
    private:
        std::wstring const & traceId_;
        Common::atomic_bool isStale_;

        // Many older client settings existed in ClientConfig as well.
        // For these older settings, we support both loading from
        // config and (optionally) overridding the config values via
        // API (SetSettings).
        //
        // The client configs are not that useful in practice since they apply
        // to all clients within a process, so for newer configs,
        // we only support updates via API.
        //
        bool useConfigValues_;

        // Can NOT update dynamically - no legacy configs
        //
        std::wstring clientFriendlyName_;

        // Can NOT update dynamically - legacy configs do exist
        //
        ULONG partitionLocationCacheLimit_;

        // KeepAlive can be configured through SetSettings or through 
        // a special UpdateKeepAlive API (e.g. to override this setting
        // only independent of all other settings).
        //
        bool useKeepAliveConfigValue_;
        ULONG keepAliveIntervalInSeconds_;
        ULONG connectionIdleTimeoutInSeconds_;

        // Can update dynamically after Open - legacy configs do exist
        //
        Common::TimeSpan serviceChangePollInterval_;
        Common::TimeSpan connectionInitializationTimeout_;
        Common::TimeSpan healthOperationTimeout_;
        Common::TimeSpan healthReportSendInterval_;
        Common::TimeSpan healthReportRetrySendInterval_;

        // Can update dynamically - no legacy configs
        //
        ULONG partitionLocationCacheBucketCount_;
        Common::TimeSpan notificationGatewayConnectionTimeout_;
        Common::TimeSpan notificationCacheUpdateTimeout_;

        // Buffer size to use when retrieving AAD JWT
        ULONG authTokenBufferSize_;
    };
}
