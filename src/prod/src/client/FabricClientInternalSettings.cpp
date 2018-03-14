// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Client;

FabricClientInternalSettings::FabricClientInternalSettings(
    std::wstring const & traceId)
    : useConfigValues_(true)
    , traceId_(traceId)
    , isStale_(false)
    , clientFriendlyName_()
    , partitionLocationCacheLimit_(ClientConfig::GetConfig().PartitionLocationCacheLimit)
    , useKeepAliveConfigValue_(true)
    , keepAliveIntervalInSeconds_(0)
    , connectionIdleTimeoutInSeconds_(0)
    , serviceChangePollInterval_(TimeSpan::MinValue)
    , connectionInitializationTimeout_(TimeSpan::MinValue)
    , healthOperationTimeout_(TimeSpan::MinValue)
    , healthReportSendInterval_(TimeSpan::MinValue)
    , healthReportRetrySendInterval_(TimeSpan::MinValue)
    , partitionLocationCacheBucketCount_(FabricClientSettings::DefaultPartitionLocationCacheBucketCount)
    , notificationGatewayConnectionTimeout_(TimeSpan::FromSeconds(FabricClientSettings::DefaultNotificationGatewayConnectionTimeoutInSeconds))
    , notificationCacheUpdateTimeout_(TimeSpan::FromSeconds(FabricClientSettings::DefaultNotificationCacheUpdateTimeoutInSeconds))
    , authTokenBufferSize_(FabricClientSettings::DefaultAuthTokenBufferSize)
{
}

FabricClientInternalSettings::FabricClientInternalSettings(
    std::wstring const & traceId,
    ServiceModel::FabricClientSettings && settings)
    : useConfigValues_(false)
    , traceId_(traceId)
    , isStale_(false)
    , clientFriendlyName_(move(settings.ClientFriendlyName))
    , partitionLocationCacheLimit_(settings.PartitionLocationCacheLimit)
    , useKeepAliveConfigValue_(false)
    , keepAliveIntervalInSeconds_(settings.KeepAliveIntervalInSeconds)
    , connectionIdleTimeoutInSeconds_(settings.ConnectionIdleTimeoutInSeconds)
    , serviceChangePollInterval_(TimeSpan::FromSeconds(settings.ServiceChangePollIntervalInSeconds))
    , connectionInitializationTimeout_(TimeSpan::FromSeconds(settings.ConnectionInitializationTimeoutInSeconds))
    , healthOperationTimeout_(TimeSpan::FromSeconds(settings.HealthOperationTimeoutInSeconds))
    , healthReportSendInterval_(TimeSpan::FromSeconds(settings.HealthReportSendIntervalInSeconds))
    , healthReportRetrySendInterval_(TimeSpan::FromSeconds(settings.HealthReportRetrySendIntervalInSeconds))
    , partitionLocationCacheBucketCount_(settings.PartitionLocationCacheBucketCount)
    , notificationGatewayConnectionTimeout_(TimeSpan::FromSeconds(settings.NotificationGatewayConnectionTimeoutInSeconds))
    , notificationCacheUpdateTimeout_(TimeSpan::FromSeconds(settings.NotificationCacheUpdateTimeoutInSeconds))
    , authTokenBufferSize_(move(settings.AuthTokenBufferSize))
{
    wstring temp(L"Client settings:\n\r");
    temp.append(wformatString("ClientFriendlyName='{0}'\n\r", clientFriendlyName_));
    temp.append(wformatString("PartitionLocationCacheLimit={0}\n\r", partitionLocationCacheLimit_));
    temp.append(wformatString("ServiceChangePollInterval={0}\n\r", serviceChangePollInterval_));
    temp.append(wformatString("ConnectionInitializationTimeout={0}\n\r", connectionInitializationTimeout_));
    temp.append(wformatString("KeepAliveInterval={0}\n\r", keepAliveIntervalInSeconds_));
    temp.append(wformatString("ConnectionIdleTimeout={0}\n\r", connectionIdleTimeoutInSeconds_));
    temp.append(wformatString("HealthOperationTimeout={0}\n\r", healthOperationTimeout_));
    temp.append(wformatString("HealthReportSendInterval={0}\n\r", healthReportSendInterval_));
    temp.append(wformatString("HealthReportRetrySendInterval={0}\n\r", healthReportRetrySendInterval_));
    temp.append(wformatString("PartitionLocationCacheLimit={0}\n\r", partitionLocationCacheLimit_));
    temp.append(wformatString("NotificationGatewayConnectionTimeout={0}\n\r", notificationGatewayConnectionTimeout_));
    temp.append(wformatString("NotificationCacheUpdateTimeout={0}\n\r", notificationCacheUpdateTimeout_));
    temp.append(wformatString("AuthTokenBufferSize={0}\n\r", authTokenBufferSize_));

    FabricClientImpl::WriteInfo(Constants::FabricClientSource, traceId_, "{0}", temp);
}

FabricClientInternalSettings::FabricClientInternalSettings(
    std::wstring const & traceId,
    ULONG keepAliveIntervalInSeconds)
    : useConfigValues_(true)
    , traceId_(traceId)
    , isStale_(false)
    , clientFriendlyName_()
    , partitionLocationCacheLimit_(ClientConfig::GetConfig().PartitionLocationCacheLimit)
    , useKeepAliveConfigValue_(false)
    , keepAliveIntervalInSeconds_(keepAliveIntervalInSeconds)
    , connectionIdleTimeoutInSeconds_(0)
    , serviceChangePollInterval_(TimeSpan::MinValue)
    , connectionInitializationTimeout_(TimeSpan::MinValue)
    , healthOperationTimeout_(TimeSpan::MinValue)
    , healthReportSendInterval_(TimeSpan::MinValue)
    , healthReportRetrySendInterval_(TimeSpan::MinValue)
    , partitionLocationCacheBucketCount_(FabricClientSettings::DefaultPartitionLocationCacheBucketCount)
    , notificationGatewayConnectionTimeout_(TimeSpan::FromSeconds(FabricClientSettings::DefaultNotificationGatewayConnectionTimeoutInSeconds))
    , notificationCacheUpdateTimeout_(TimeSpan::FromSeconds(FabricClientSettings::DefaultNotificationCacheUpdateTimeoutInSeconds))
    , authTokenBufferSize_(FabricClientSettings::DefaultAuthTokenBufferSize)
{
}

FabricClientInternalSettings::~FabricClientInternalSettings()
{
}

ULONG FabricClientInternalSettings::get_KeepAliveIntervalInSeconds() const
{
    if (useKeepAliveConfigValue_)
    {
        return static_cast<ULONG>(ClientConfig::GetConfig().KeepAliveIntervalInSeconds);
    }

    return keepAliveIntervalInSeconds_;
}

ULONG FabricClientInternalSettings::get_ConnectionIdleTimeoutInSeconds() const
{
    return connectionIdleTimeoutInSeconds_;
}

TimeSpan const FabricClientInternalSettings::get_KeepAliveInterval() const
{
    return TimeSpan::FromSeconds(this->get_KeepAliveIntervalInSeconds());
}

TimeSpan const FabricClientInternalSettings::get_ConnectionIdleTimeout() const
{
    return TimeSpan::FromSeconds(this->get_ConnectionIdleTimeoutInSeconds());
}

ULONG FabricClientInternalSettings::get_PartitionLocationCacheLimit() const
{
    if (useConfigValues_)
    {
        return ClientConfig::GetConfig().PartitionLocationCacheLimit;
    }

    return partitionLocationCacheLimit_;
}

TimeSpan const FabricClientInternalSettings::get_ServiceChangePollInterval() const
{
    if (useConfigValues_)
    {
        return ClientConfig::GetConfig().ServiceChangePollInterval;
    }

    ASSERT_IF(serviceChangePollInterval_ <= TimeSpan::Zero, "serviceChangePollInterval has invalid value {0}", serviceChangePollInterval_);
    return serviceChangePollInterval_;
}

TimeSpan const FabricClientInternalSettings::get_ConnectionInitializationTimeout() const
{
    if (useConfigValues_)
    {
        return ClientConfig::GetConfig().ConnectionInitializationTimeout;
    }

    ASSERT_IF(connectionInitializationTimeout_ <= TimeSpan::Zero, "connectionInitializationTimeout has invalid value {0}", connectionInitializationTimeout_);
    return connectionInitializationTimeout_;
}

TimeSpan const FabricClientInternalSettings::get_HealthOperationTimeout() const
{
    if (useConfigValues_)
    {
        return ClientConfig::GetConfig().HealthOperationTimeout;
    }

    ASSERT_IF(healthOperationTimeout_ <= TimeSpan::Zero, "healthOperationTimeout has invalid value {0}", healthOperationTimeout_);
    return healthOperationTimeout_;
}

TimeSpan const FabricClientInternalSettings::get_HealthReportSendInterval() const
{
    if (useConfigValues_)
    {
        return ClientConfig::GetConfig().HealthReportSendInterval;
    }

    ASSERT_IF(healthReportSendInterval_ < TimeSpan::Zero, "healthReportSendInterval has invalid value {0}", healthReportSendInterval_);
    return healthReportSendInterval_;
}

TimeSpan const FabricClientInternalSettings::get_HealthReportRetrySendInterval() const
{
    if (useConfigValues_)
    {
        return ClientConfig::GetConfig().HealthReportRetrySendInterval;
    }

    ASSERT_IF(healthReportRetrySendInterval_ < TimeSpan::Zero, "healthReportRetrySendInterval has invalid value {0}", healthReportRetrySendInterval_);
    return healthReportRetrySendInterval_;
}

ULONG FabricClientInternalSettings::get_PartitionLocationCacheBucketCount() const
{
    return partitionLocationCacheBucketCount_;
}

TimeSpan const FabricClientInternalSettings::get_NotificationGatewayConnectionTimeout() const
{
    return notificationGatewayConnectionTimeout_;
}

TimeSpan const FabricClientInternalSettings::get_NotificationCacheUpdateTimeout() const
{
    return notificationCacheUpdateTimeout_;
}

size_t const FabricClientInternalSettings::get_AuthTokenBufferSize() const
{
    return authTokenBufferSize_;
}

ServiceModel::FabricClientSettings FabricClientInternalSettings::GetSettings() const
{
    return FabricClientSettings(
        this->ClientFriendlyName,
        this->PartitionLocationCacheLimit,
        this->KeepAliveIntervalInSeconds,
        static_cast<ULONG>(this->ServiceChangePollInterval.TotalSeconds()),
        static_cast<ULONG>(this->ConnectionInitializationTimeout.TotalSeconds()),
        static_cast<ULONG>(this->HealthOperationTimeout.TotalSeconds()),
        static_cast<ULONG>(this->HealthReportSendInterval.TotalSeconds()),
        static_cast<ULONG>(this->HealthReportRetrySendInterval.TotalSeconds()),
        this->PartitionLocationCacheBucketCount,
        static_cast<ULONG>(this->NotificationGatewayConnectionTimeout.TotalSeconds()),
        static_cast<ULONG>(this->NotificationCacheUpdateTimeout.TotalSeconds()),
        static_cast<ULONG>(this->AuthTokenBufferSize),
        this->ConnectionIdleTimeoutInSeconds);
}

bool FabricClientInternalSettings::CanDynamicallyUpdate(
    ServiceModel::FabricClientSettings const & newSettings) const
{
    if (newSettings.ClientFriendlyName != this->ClientFriendlyName)
    {
        FabricClientImpl::WriteWarning(
            Constants::FabricClientSource,
            traceId_,
            "Dynamically updating ClientFriendlyName not allowed: current='{0}' new='{1}'",
            this->ClientFriendlyName,
            newSettings.ClientFriendlyName);
        return false;
    }

    if (newSettings.PartitionLocationCacheLimit != this->PartitionLocationCacheLimit)
    {
        FabricClientImpl::WriteWarning(
            Constants::FabricClientSource,
            traceId_,
            "Dynamically updating PartitionLocationCacheLimit not allowed: current={0} new={1}",
            this->PartitionLocationCacheLimit,
            newSettings.PartitionLocationCacheLimit);
        return false;
    }

    if (newSettings.KeepAliveIntervalInSeconds != this->KeepAliveIntervalInSeconds)
    {
        FabricClientImpl::WriteWarning(
            Constants::FabricClientSource,
            traceId_,
            "Dynamically updating KeepAliveIntervalInSeconds not allowed: current={0} new={1}",
            this->KeepAliveIntervalInSeconds,
            newSettings.KeepAliveIntervalInSeconds);
        return false;
    }

    if (newSettings.ConnectionIdleTimeoutInSeconds != this->ConnectionIdleTimeoutInSeconds)
    {
        FabricClientImpl::WriteWarning(
            Constants::FabricClientSource,
            traceId_,
            "Dynamically updating ConnectionIdleTimeoutInSeconds not allowed: current={0} new={1}",
            this->ConnectionIdleTimeoutInSeconds,
            newSettings.ConnectionIdleTimeoutInSeconds);
        return false;
    }

    if (newSettings.PartitionLocationCacheBucketCount != this->PartitionLocationCacheBucketCount)
    {
        FabricClientImpl::WriteWarning(
            Constants::FabricClientSource,
            traceId_,
            "Dynamically updating PartitionLocationCacheBucketCount not allowed: current={0} new={1}",
            this->PartitionLocationCacheBucketCount,
            newSettings.PartitionLocationCacheBucketCount);
        return false;
    }

    return true;
}

void FabricClientInternalSettings::MarkAsStale()
{
    isStale_.store(true);
}
