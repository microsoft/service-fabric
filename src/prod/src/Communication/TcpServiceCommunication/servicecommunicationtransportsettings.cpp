// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using  namespace Communication::TcpServiceCommunication;

static const StringLiteral TraceType("ServiceCommunicationTransportSettings");

ErrorCode ServiceCommunicationTransportSettings::FromPublicApi(
    __in FABRIC_SERVICE_TRANSPORT_SETTINGS const & settings,
    __out ServiceCommunicationTransportSettingsUPtr & output)
{
    auto transportSettings = make_unique<ServiceCommunicationTransportSettings>();

    // Store the transport security settings if it exists
    if (settings.SecurityCredentials) {
        auto error = Transport::SecuritySettings::FromPublicApi(*settings.SecurityCredentials, transportSettings->securitySettings_);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, "{0}", "Error From  Security Settings:FromPublicApi", error);
            return error;
        }
    }

    if (settings.MaxMessageSize != 0)
    {
        transportSettings->maxMessageSize_ = static_cast<DWORD>(settings.MaxMessageSize);
    }

    if (settings.OperationTimeoutInSeconds != 0)
    {
        transportSettings->operationTimeout_ = TimeSpan::FromSeconds(static_cast<DWORD>(settings.OperationTimeoutInSeconds));
    }
    else
    {
        transportSettings->operationTimeout_ = TimeSpan::MaxValue;
    }

    if (settings.KeepAliveTimeoutInSeconds != 0)
    {
        transportSettings->keepAliveTimeout_ = TimeSpan::FromSeconds(static_cast<DWORD>(settings.KeepAliveTimeoutInSeconds));
    }
    else
    {
        transportSettings->keepAliveTimeout_ = TimeSpan::Zero;
    }

    if (settings.MaxConcurrentCalls != 0)
    {
        transportSettings->maxConcurrentCalls_ = static_cast<DWORD>(settings.MaxConcurrentCalls);
    }

    if (settings.MaxQueueSize != 0)
    {
        transportSettings->maxQueueSize_ = static_cast<DWORD>(settings.MaxQueueSize);
    }

    if(settings.Reserved)
    {
        auto ex1 = reinterpret_cast <FABRIC_SERVICE_TRANSPORT_SETTINGS_EX1* > (settings.Reserved);
       if(ex1->ConnectTimeoutInMilliseconds!=0)
       {
           transportSettings->connectTimeout_ = TimeSpan::FromMilliseconds(static_cast<DWORD>(ex1->ConnectTimeoutInMilliseconds));
       }        
    }

    auto errorCode = Validate(transportSettings);
    if (errorCode.IsSuccess())
    {
        output = move(transportSettings);
    }

    return errorCode;
}


ErrorCode ServiceCommunicationTransportSettings::FromPublicApi(
    __in FABRIC_TRANSPORT_SETTINGS const & settings,
    __out ServiceCommunicationTransportSettingsUPtr & output)
{
    auto transportSettings = make_unique<ServiceCommunicationTransportSettings>();

    // Store the transport security settings if it exists
    if (settings.SecurityCredentials) {
        auto error = Transport::SecuritySettings::FromPublicApi(*settings.SecurityCredentials, transportSettings->securitySettings_);

        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, "{0}", "Error From  Security Settings:FromPublicApi", error);
            return error;
        }
    }

    if (settings.MaxMessageSize != 0)
    {
        transportSettings->maxMessageSize_ = static_cast<DWORD>(settings.MaxMessageSize);
    }

    if (settings.OperationTimeoutInSeconds != 0)
    {
        transportSettings->operationTimeout_ = TimeSpan::FromSeconds(static_cast<DWORD>(settings.OperationTimeoutInSeconds));
    }
    else
    {
        transportSettings->operationTimeout_ = TimeSpan::MaxValue;
    }

    if (settings.KeepAliveTimeoutInSeconds != 0)
    {
        transportSettings->keepAliveTimeout_ = TimeSpan::FromSeconds(static_cast<DWORD>(settings.KeepAliveTimeoutInSeconds));
    }
    else
    {
        transportSettings->keepAliveTimeout_ = TimeSpan::Zero;
    }

    if (settings.MaxConcurrentCalls != 0)
    {
        transportSettings->maxConcurrentCalls_ = static_cast<DWORD>(settings.MaxConcurrentCalls);
    }

    if (settings.MaxQueueSize != 0)
    {
        transportSettings->maxQueueSize_ = static_cast<DWORD>(settings.MaxQueueSize);
    }

    transportSettings->connectTimeout_ = TimeSpan::Zero;

    auto errorCode = Validate(transportSettings);
    if (errorCode.IsSuccess())
    {
        output = move(transportSettings);
    }

    return errorCode;
}


ErrorCode ServiceCommunicationTransportSettings::Validate(
    __in ServiceCommunicationTransportSettingsUPtr & toBeValidated)
{
    if (toBeValidated->MaxMessageSize <= 0)
    {
        WriteWarning(TraceType, "{0}", L"Max Message Size is less than and equal to Zero", toBeValidated->MaxMessageSize);
        return ErrorCodeValue::InvalidArgument;
    }

    if (toBeValidated->OperationTimeout.TotalSeconds() < 0)
    {
        WriteWarning(TraceType, "{0}", L"OperationTimeout is less than Zero Seconds", toBeValidated->OperationTimeout.TotalSeconds());
        return ErrorCodeValue::InvalidArgument;
    }

    if (toBeValidated->KeepAliveTimeout.TotalSeconds() < 0)
    {
        WriteWarning(TraceType, "{0}", L"OperationTimeout is less than Zero Seconds", toBeValidated->KeepAliveTimeout.TotalSeconds());
        return ErrorCodeValue::InvalidArgument;
    }

    if (toBeValidated->MaxConcurrentCalls < 0)
    {
        WriteWarning(TraceType, "{0}", L"MaxConcurrentCalls is less than Zero", toBeValidated->MaxConcurrentCalls);
        return ErrorCodeValue::InvalidArgument;
    }

    if (toBeValidated->MaxQueueSize <= 0)
    {
        WriteWarning(TraceType, "{0}", L"MaxQueueSize is less than And equal To Zero", toBeValidated->MaxQueueSize);
        return ErrorCodeValue::InvalidArgument;
    }

    if (toBeValidated->ConnectTimeout.TotalSeconds() < 0)
    {
        WriteWarning(TraceType, "{0}", L"ConnectTimeout is less than Zero Seconds", toBeValidated->ConnectTimeout.TotalSeconds());
        return ErrorCodeValue::InvalidArgument;
    }



    return ErrorCodeValue::Success;
}

bool ServiceCommunicationTransportSettings::operator == (ServiceCommunicationTransportSettings const & otherSettings) const
{
    if (maxMessageSize_ != otherSettings.MaxMessageSize){
        return false;
    }
    if (operationTimeout_ != otherSettings.OperationTimeout){
        return false;
    }
    if (securitySettings_ != otherSettings.SecuritySetting){
        return false;
    }
    if (maxConcurrentCalls_ != otherSettings.MaxConcurrentCalls){
        return false;
    }
    if (maxQueueSize_ != otherSettings.MaxQueueSize){
        return false;
    }
    return true;
}

bool ServiceCommunicationTransportSettings::operator != (ServiceCommunicationTransportSettings const & rhs) const
{
    return !(*this == rhs);
}
