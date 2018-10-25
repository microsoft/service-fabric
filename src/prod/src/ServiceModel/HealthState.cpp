//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
    namespace HealthState
    {
        wstring ToString(Enum const & val)
        {
            wstring healthStatus;
            StringWriter(healthStatus).Write(val);
            return healthStatus;
        }

        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Invalid:
                w << L"Invalid";
                return;
            case Ok:
                w << L"Ok";
                return;
            case Warning:
                w << L"Warning";
                return;
            case Error:
                w << L"Error";
                return;
            case Unknown:
                w << L"Unknown";
                return;
            default:
                w << L"Unknown Health Status {0}" << static_cast<int>(val);
                return;
            }
        }

        ::FABRIC_HEALTH_STATE ConvertToPublicHealthState(Enum toConvert)
        {
            switch (toConvert)
            {
            case Ok:
                return ::FABRIC_HEALTH_STATE_OK;
            case Warning:
                return ::FABRIC_HEALTH_STATE_WARNING;
            case Error:
                return ::FABRIC_HEALTH_STATE_ERROR;
            case Unknown:
                return ::FABRIC_HEALTH_STATE_UNKNOWN;
            case Invalid:
            default:
                return ::FABRIC_HEALTH_STATE_INVALID;
            }
        }

        Enum ConvertToServiceModelHealthState(FABRIC_HEALTH_STATE toConvert)
        {
            switch (toConvert)
            {
            case FABRIC_HEALTH_STATE_OK:
                return Enum::Ok;
            case FABRIC_HEALTH_STATE_WARNING:
                return Enum::Warning;
            case FABRIC_HEALTH_STATE_ERROR:
                return Enum::Error;
            case FABRIC_HEALTH_STATE_UNKNOWN:
                return Enum::Unknown;
            case FABRIC_HEALTH_STATE_INVALID:
            default:
                return Enum::Invalid;
            }
        }

        BEGIN_ENUM_STRUCTURED_TRACE(HealthState)

            ADD_ENUM_MAP_VALUE(HealthState, Enum::Invalid)
            ADD_ENUM_MAP_VALUE(HealthState, Enum::Ok)
            ADD_ENUM_MAP_VALUE(HealthState, Enum::Warning)
            ADD_ENUM_MAP_VALUE(HealthState, Enum::Error)
            ADD_ENUM_MAP_VALUE(HealthState, Enum::Unknown)

        END_ENUM_STRUCTURED_TRACE(HealthState)
    }
}
