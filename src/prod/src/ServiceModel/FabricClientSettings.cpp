// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const FabricClientSettingsTrace("FabricClientSettings");

const size_t FabricClientSettings::MaxClientFriendlyNameLength = 256;
const ULONG FabricClientSettings::DefaultPartitionLocationCacheBucketCount = 1024;
const ULONG FabricClientSettings::DefaultNotificationGatewayConnectionTimeoutInSeconds = 30;
const ULONG FabricClientSettings::DefaultNotificationCacheUpdateTimeoutInSeconds = 30;
const ULONG FabricClientSettings::DefaultAuthTokenBufferSize = 4096;

FabricClientSettings::FabricClientSettings()
    : clientFriendlyName_()
    , partitionLocationCacheLimit_(0)
    , serviceChangePollIntervalInSeconds_(0)
    , connectionInitializationTimeoutInSeconds_(0)
    , keepAliveIntervalInSeconds_(0)
    , healthOperationTimeoutInSeconds_(0)
    , healthReportSendIntervalInSeconds_(0)
    , healthReportRetrySendIntervalInSeconds_(0)
    , partitionLocationCacheBucketCount_(DefaultPartitionLocationCacheBucketCount)
    , notificationGatewayConnectionTimeoutInSeconds_(DefaultNotificationGatewayConnectionTimeoutInSeconds)
    , notificationCacheUpdateTimeoutInSeconds_(DefaultNotificationCacheUpdateTimeoutInSeconds)
    , authTokenBufferSize_(DefaultAuthTokenBufferSize)
    , connectionIdleTimeoutInSeconds_(0)
{
}

FabricClientSettings::FabricClientSettings(
    ULONG partitionLocationCacheLimit,
    ULONG keepAliveIntervalInSeconds,
    ULONG serviceChangePollIntervalInSeconds,
    ULONG connectionInitializationTimeoutInSeconds,
    ULONG healthOperationTimeoutInSeconds,
    ULONG healthReportSendIntervalInSeconds,
    ULONG connectionIdleTimeoutInSeconds)
    : clientFriendlyName_()
    , partitionLocationCacheLimit_(partitionLocationCacheLimit)
    , serviceChangePollIntervalInSeconds_(serviceChangePollIntervalInSeconds)
    , connectionInitializationTimeoutInSeconds_(connectionInitializationTimeoutInSeconds)
    , keepAliveIntervalInSeconds_(keepAliveIntervalInSeconds)
    , healthOperationTimeoutInSeconds_(healthOperationTimeoutInSeconds)
    , healthReportSendIntervalInSeconds_(healthReportSendIntervalInSeconds)
    , healthReportRetrySendIntervalInSeconds_(0)
    , partitionLocationCacheBucketCount_(DefaultPartitionLocationCacheBucketCount)
    , notificationGatewayConnectionTimeoutInSeconds_(DefaultNotificationGatewayConnectionTimeoutInSeconds)
    , notificationCacheUpdateTimeoutInSeconds_(DefaultNotificationCacheUpdateTimeoutInSeconds)
    , authTokenBufferSize_(DefaultAuthTokenBufferSize)
    , connectionIdleTimeoutInSeconds_(connectionIdleTimeoutInSeconds)
{
}

FabricClientSettings::FabricClientSettings(
    wstring const & clientFriendlyName,
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
    ULONG connectionIdleTimeoutInSeconds)
    : clientFriendlyName_(clientFriendlyName)
    , partitionLocationCacheLimit_(partitionLocationCacheLimit)
    , serviceChangePollIntervalInSeconds_(serviceChangePollIntervalInSeconds)
    , connectionInitializationTimeoutInSeconds_(connectionInitializationTimeoutInSeconds)
    , keepAliveIntervalInSeconds_(keepAliveIntervalInSeconds)
    , healthOperationTimeoutInSeconds_(healthOperationTimeoutInSeconds)
    , healthReportSendIntervalInSeconds_(healthReportSendIntervalInSeconds)
    , healthReportRetrySendIntervalInSeconds_(healthReportRetrySendIntervalInSeconds)
    , partitionLocationCacheBucketCount_(partitionLocationCacheBucketCount)
    , notificationGatewayConnectionTimeoutInSeconds_(notificationGatewayConnectionTimeoutInSeconds)
    , notificationCacheUpdateTimeoutInSeconds_(notificationCacheUpdateTimeoutInSeconds)
    , authTokenBufferSize_(authTokenBufferSize)
    , connectionIdleTimeoutInSeconds_(connectionIdleTimeoutInSeconds)
{
}

FabricClientSettings::~FabricClientSettings()
{
}

ErrorCode FabricClientSettings::FromPublicApi(
    FABRIC_CLIENT_SETTINGS const & settings)
{
    if (settings.PartitionLocationCacheLimit == 0)
    {
        WriteError(
            FabricClientSettingsTrace,
            "PartitionLocationCacheLimit {0} should be greater than 0",
            settings.PartitionLocationCacheLimit);
            
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    if (settings.ServiceChangePollIntervalInSeconds == 0)
    {
        WriteError(
            FabricClientSettingsTrace,
            "ServiceChangePollIntervalInSeconds {0} should be greater than 0",
            settings.ServiceChangePollIntervalInSeconds);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    if (settings.ConnectionInitializationTimeoutInSeconds == 0)
    {
        WriteError(
            FabricClientSettingsTrace,
            "ConnectionInitializationTimeout {0} should be greater than 0",
            settings.ConnectionInitializationTimeoutInSeconds);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    if (settings.HealthOperationTimeoutInSeconds == 0)
    {
        WriteError(
            FabricClientSettingsTrace,
            "HealthOperationTimeoutInSeconds {0} should be greater than 0",
            settings.HealthOperationTimeoutInSeconds);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }
    
    if (settings.ConnectionInitializationTimeoutInSeconds >= settings.ServiceChangePollIntervalInSeconds)
    {
        WriteError(
            FabricClientSettingsTrace,
            "ConnectionInitializationTimeout {0} should be less than ServiceChangePollInterval {1}",
            settings.ConnectionInitializationTimeoutInSeconds,
            settings.ServiceChangePollIntervalInSeconds);
        return ErrorCode::FromHResult(E_INVALIDARG);
    }

    partitionLocationCacheLimit_ = settings.PartitionLocationCacheLimit;
    serviceChangePollIntervalInSeconds_ = settings.ServiceChangePollIntervalInSeconds;
    connectionInitializationTimeoutInSeconds_ = settings.ConnectionInitializationTimeoutInSeconds;
    keepAliveIntervalInSeconds_ = settings.KeepAliveIntervalInSeconds;
    healthOperationTimeoutInSeconds_ = settings.HealthOperationTimeoutInSeconds;
    healthReportSendIntervalInSeconds_ = settings.HealthReportSendIntervalInSeconds;

    if (settings.Reserved != NULL)
    {
        auto ex1 = reinterpret_cast<FABRIC_CLIENT_SETTINGS_EX1*>(settings.Reserved);

        auto hr = StringUtility::LpcwstrToWstring(
            ex1->ClientFriendlyName, 
            true,  // null input translates to empty wstring
            ParameterValidator::MinStringSize,
            MaxClientFriendlyNameLength,
            clientFriendlyName_);
        if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

        if (ex1->PartitionLocationCacheBucketCount > 0)
        {
            partitionLocationCacheBucketCount_ = ex1->PartitionLocationCacheBucketCount;
        }

        healthReportRetrySendIntervalInSeconds_ = ex1->HealthReportRetrySendIntervalInSeconds;

        if (ex1->Reserved != NULL)
        {
            auto ex2 = reinterpret_cast<FABRIC_CLIENT_SETTINGS_EX2*>(ex1->Reserved);

            if (ex2->NotificationGatewayConnectionTimeoutInSeconds == 0)
            {
                WriteError(
                    FabricClientSettingsTrace,
                    "NotificationGatewayConnectionTimeoutInSeconds {0} should be greater than 0",
                    ex2->NotificationGatewayConnectionTimeoutInSeconds);
                return ErrorCode::FromHResult(E_INVALIDARG);
            }

            if (ex2->NotificationCacheUpdateTimeoutInSeconds == 0)
            {
                WriteError(
                    FabricClientSettingsTrace,
                    "NotificationCacheUpdateTimeoutInSeconds {0} should be greater than 0",
                    ex2->NotificationCacheUpdateTimeoutInSeconds);
                return ErrorCode::FromHResult(E_INVALIDARG);
            }

            notificationGatewayConnectionTimeoutInSeconds_ = ex2->NotificationGatewayConnectionTimeoutInSeconds;
            notificationCacheUpdateTimeoutInSeconds_ = ex2->NotificationCacheUpdateTimeoutInSeconds;

            if (ex2->Reserved != NULL)
            {
                auto ex3 = reinterpret_cast<FABRIC_CLIENT_SETTINGS_EX3*>(ex2->Reserved);
            
                authTokenBufferSize_ = ex3->AuthTokenBufferSize;

                if (ex3->Reserved != NULL)
                {
                    auto ex4 = reinterpret_cast<FABRIC_CLIENT_SETTINGS_EX4*>(ex3->Reserved);
                
                    if (ex4->ConnectionIdleTimeoutInSeconds != 0)
                    {
                        WriteError(
                            FabricClientSettingsTrace,
                            "ConnectionIdleTimeoutInSeconds parameter has been deprecated. This is set t o a non zero value - {0}, and will be ignored",
                            ex4->ConnectionIdleTimeoutInSeconds);

                    }
                    connectionIdleTimeoutInSeconds_ = 0;

                }
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void FabricClientSettings::ToPublicApi(
    __in Common::ScopedHeap & heap, 
    __out FABRIC_CLIENT_SETTINGS & settings) const
{
    memset(&settings, 0, sizeof(settings));

    settings.PartitionLocationCacheLimit = partitionLocationCacheLimit_;
    settings.ServiceChangePollIntervalInSeconds = serviceChangePollIntervalInSeconds_;
    settings.ConnectionInitializationTimeoutInSeconds = connectionInitializationTimeoutInSeconds_;
    settings.KeepAliveIntervalInSeconds = keepAliveIntervalInSeconds_;
    settings.HealthReportSendIntervalInSeconds = healthReportSendIntervalInSeconds_;
    settings.HealthOperationTimeoutInSeconds = healthOperationTimeoutInSeconds_;

    auto ex1 = heap.AddItem<FABRIC_CLIENT_SETTINGS_EX1>();

    if (!clientFriendlyName_.empty())
    {
        ex1->ClientFriendlyName = heap.AddString(clientFriendlyName_);
    }

    ex1->PartitionLocationCacheBucketCount = partitionLocationCacheBucketCount_;
    ex1->HealthReportRetrySendIntervalInSeconds = healthReportRetrySendIntervalInSeconds_;

    settings.Reserved = ex1.GetRawPointer();

    auto ex2 = heap.AddItem<FABRIC_CLIENT_SETTINGS_EX2>();

    ex2->NotificationGatewayConnectionTimeoutInSeconds = notificationGatewayConnectionTimeoutInSeconds_;
    ex2->NotificationCacheUpdateTimeoutInSeconds = notificationCacheUpdateTimeoutInSeconds_;

    ex1->Reserved = ex2.GetRawPointer();

    auto ex3 = heap.AddItem<FABRIC_CLIENT_SETTINGS_EX3>();

    ex3->AuthTokenBufferSize = authTokenBufferSize_;

    ex2->Reserved = ex3.GetRawPointer();

    auto ex4 = heap.AddItem<FABRIC_CLIENT_SETTINGS_EX4>();

    ex4->ConnectionIdleTimeoutInSeconds = connectionIdleTimeoutInSeconds_;

    ex3->Reserved = ex4.GetRawPointer();

}
