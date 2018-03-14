// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

namespace Hosting2
{
    namespace ContainerEventType
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case None: w << "None"; return;
            case Stop: w << "Stop"; return;
            case Die: w << "Die"; return;
            case Health: w << "Health"; return;
            default: w << "ContainerEventType(" << static_cast<ULONG>(e) << ')'; return;
            }
        }

        ErrorCode FromPublicApi(__in FABRIC_CONTAINER_EVENT_TYPE publicLevel, __out Enum & internalLevel)
        {
            switch (publicLevel)
            {
            case FABRIC_CONTAINER_EVENT_TYPE_NONE:
                internalLevel = None;
                return ErrorCodeValue::Success;

            case FABRIC_CONTAINER_EVENT_TYPE_STOP:
                internalLevel = Stop;
                return ErrorCodeValue::Success;

            case FABRIC_CONTAINER_EVENT_TYPE_DIE:
                internalLevel = Die;
                return ErrorCodeValue::Success;

            case FABRIC_CONTAINER_EVENT_TYPE_HEALTH:
                internalLevel = Health;
                return ErrorCodeValue::Success;

            default:
                return ErrorCodeValue::InvalidArgument;
            }
        }

        ENUM_STRUCTURED_TRACE(ContainerEventType, FirstValidEnum, LastValidEnum)
    }
}

ContainerEventDescription::ContainerEventDescription()
    : eventType_(ContainerEventType::None)
    , containerId_()
    , containerName_()
    , timeStampInSeconds_(0)
    , isHealthy_(false)
    , exitCode_(0)
{
}

void ContainerEventDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerEventDescription { ");

    w.Write("ContainerEventType={0}, ", eventType_);
    w.Write("ContainerId={0}, ", containerId_);
    w.Write("ContainerName={0}, ", containerName_);
    w.Write("TimeStampInSeconds={0}, ", timeStampInSeconds_);
    
    if (eventType_ == ContainerEventType::Health)
    {
        w.Write("IsHealthy={0}, ", isHealthy_);
    }
    else
    {
        w.Write("ExitCode={0}", exitCode_);
    }

    w.Write("}");
}

ErrorCode ContainerEventDescription::FromPublicApi(
    FABRIC_CONTAINER_EVENT_DESCRIPTION const & eventDescription)
{
    auto error = ContainerEventType::FromPublicApi(eventDescription.EventType, eventType_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(eventDescription.ContainerId, false, containerId_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(eventDescription.ContainerName, false, containerName_);
    if (!error.IsSuccess())
    {
        return error;
    }

    timeStampInSeconds_ = eventDescription.TimeStampInSeconds;
    isHealthy_ = eventDescription.IsHealthy;
    exitCode_ = eventDescription.ExitCode;

    return ErrorCode(ErrorCodeValue::Success);
}
