// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class FabricClientSettings
        : Common::TextTraceComponent<Common::TraceTaskCodes::ServiceModel>
    {
        DENY_COPY(FabricClientSettings);
        DEFAULT_MOVE_CONSTRUCTOR(FabricClientSettings);
        DEFAULT_MOVE_ASSIGNMENT(FabricClientSettings);
    public:
        static const size_t MaxClientFriendlyNameLength;
        static const ULONG DefaultPartitionLocationCacheBucketCount;
        static const ULONG DefaultNotificationGatewayConnectionTimeoutInSeconds;
        static const ULONG DefaultNotificationCacheUpdateTimeoutInSeconds;
        static const ULONG DefaultAuthTokenBufferSize;

    public:
        FabricClientSettings();

        FabricClientSettings(
            ULONG partitionLocationCacheLimit,
            ULONG keepAliveIntervalInSeconds,
            ULONG serviceChangePollIntervalInSeconds,
            ULONG connectionInitializationTimeoutInSeconds,
            ULONG healthOperationTimeoutInSeconds,
            ULONG healthReportSendIntervalInSeconds,
            ULONG connectionIdleTimeoutInSeconds);

        FabricClientSettings(
            std::wstring const & clientFriendlyName,
            ULONG partitionLocationCacheLimit,
            ULONG keepAliveIntervalInSeconds,
            ULONG serviceChangePollIntervalInSeconds,
            ULONG connectionInitializationTimeoutInSeconds,
            ULONG healthOperationTimeoutInSeconds,
            ULONG healthReportSendIntervalInSeconds,
            ULONG healthReportRetrySendIntervalInSeconds,
            ULONG partitionLocationCacheBucketCount,
            ULONG notificationGatewayConnectionTimeoutInSeconds,
            ULONG notificationCacheUpdateTimeoutInSeconds,
            ULONG authTokenBufferSize,
            ULONG connectionIdleTimeoutInSeconds);

        ~FabricClientSettings();

        __declspec(property(get=get_ClientFriendlyName)) std::wstring const & ClientFriendlyName;
        std::wstring const & get_ClientFriendlyName() const { return clientFriendlyName_; }
        void SetClientFriendlyName(std::wstring const & value) { clientFriendlyName_ = value; }

        __declspec(property(get=get_PartitionLocationCacheLimit)) ULONG PartitionLocationCacheLimit;
        ULONG get_PartitionLocationCacheLimit() const { return partitionLocationCacheLimit_; }
        void SetPartitionLocationCacheLimit(ULONG value) { partitionLocationCacheLimit_ = value; }

        __declspec(property(get=get_KeepAliveIntervalInSeconds)) ULONG KeepAliveIntervalInSeconds;
        ULONG get_KeepAliveIntervalInSeconds() const { return keepAliveIntervalInSeconds_; }

        __declspec(property(get=get_ServiceChangePollIntervalInSeconds)) ULONG ServiceChangePollIntervalInSeconds;
        ULONG get_ServiceChangePollIntervalInSeconds() const { return serviceChangePollIntervalInSeconds_; }
        void SetServiceChangePollIntervalInSeconds(ULONG value) { serviceChangePollIntervalInSeconds_ = value; }

        __declspec(property(get=get_ConnectionInitializationTimeoutInSeconds)) ULONG ConnectionInitializationTimeoutInSeconds;
        ULONG get_ConnectionInitializationTimeoutInSeconds() const { return connectionInitializationTimeoutInSeconds_; }

        __declspec(property(get=get_HealthOperationTimeoutInSeconds)) ULONG HealthOperationTimeoutInSeconds;
        ULONG get_HealthOperationTimeoutInSeconds() const { return healthOperationTimeoutInSeconds_; }

        __declspec(property(get=get_HealthReportSendIntervalInSeconds)) ULONG HealthReportSendIntervalInSeconds;
        ULONG get_HealthReportSendIntervalInSeconds() const { return healthReportSendIntervalInSeconds_; }
        void SetHealthReportSendIntervalInSeconds(ULONG value) { healthReportSendIntervalInSeconds_ = value; }

        __declspec(property(get=get_HealthReportRetrySendIntervalInSeconds)) ULONG HealthReportRetrySendIntervalInSeconds;
        ULONG get_HealthReportRetrySendIntervalInSeconds() const { return healthReportRetrySendIntervalInSeconds_; }

        __declspec(property(get=get_PartitionLocationCacheBucketCount)) ULONG PartitionLocationCacheBucketCount;
        ULONG get_PartitionLocationCacheBucketCount() const { return partitionLocationCacheBucketCount_; }
        void SetPartitionLocationCacheBucketCount(ULONG value) { partitionLocationCacheBucketCount_ = value; }

        __declspec(property(get=get_NotificationGatewayConnectionTimeoutInSeconds)) ULONG NotificationGatewayConnectionTimeoutInSeconds;
        ULONG get_NotificationGatewayConnectionTimeoutInSeconds() const { return notificationGatewayConnectionTimeoutInSeconds_; }
        void SetNotificationGatewayConnectionTimeoutInSeconds(ULONG value) { notificationGatewayConnectionTimeoutInSeconds_ = value; }

        __declspec(property(get=get_NotificationCacheUpdateTimeoutInSeconds)) ULONG NotificationCacheUpdateTimeoutInSeconds;
        ULONG get_NotificationCacheUpdateTimeoutInSeconds() const { return notificationCacheUpdateTimeoutInSeconds_; }
        void SetNotificationCacheUpdateTimeoutInSeconds(ULONG value) { notificationCacheUpdateTimeoutInSeconds_ = value; }

        __declspec(property(get=get_AuthTokenBufferSize)) ULONG AuthTokenBufferSize;
        ULONG get_AuthTokenBufferSize() const { return authTokenBufferSize_; }
        void SetAuthTokenBufferSize(ULONG value) { authTokenBufferSize_ = value; }

        __declspec(property(get=get_ConnectionIdleTimeoutInSeconds)) ULONG ConnectionIdleTimeoutInSeconds;
        ULONG get_ConnectionIdleTimeoutInSeconds() const { return connectionIdleTimeoutInSeconds_; }


        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_CLIENT_SETTINGS & settings) const;

        Common::ErrorCode FromPublicApi(
            FABRIC_CLIENT_SETTINGS const & settings);

    private:
        std::wstring clientFriendlyName_;
        ULONG partitionLocationCacheLimit_;
        ULONG keepAliveIntervalInSeconds_;
        ULONG serviceChangePollIntervalInSeconds_;
        ULONG connectionInitializationTimeoutInSeconds_;
        ULONG healthOperationTimeoutInSeconds_;
        ULONG healthReportSendIntervalInSeconds_;
        ULONG healthReportRetrySendIntervalInSeconds_;
        ULONG partitionLocationCacheBucketCount_;
        ULONG notificationGatewayConnectionTimeoutInSeconds_;
        ULONG notificationCacheUpdateTimeoutInSeconds_;
        ULONG authTokenBufferSize_;
        ULONG connectionIdleTimeoutInSeconds_;
    };
}
