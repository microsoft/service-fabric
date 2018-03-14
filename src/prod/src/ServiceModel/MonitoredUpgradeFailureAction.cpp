// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
    namespace MonitoredUpgradeFailureAction
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case Invalid:
                w << L"Invalid";
                return;
            case Rollback:
                w << L"Rollback";
                return;
            case Manual:
                w << L"Manual";
                return;
            default:
                w << wformatString("Unknown MonitoredUpgradeFailureAction value {0}", static_cast<int>(val));
            }
        }

        ENUM_STRUCTURED_TRACE(MonitoredUpgradeFailureAction, FirstValidEnum, LastValidEnum)

        Enum FromPublicApi(FABRIC_MONITORED_UPGRADE_FAILURE_ACTION const & action)
        {
            switch (action)
            {
            case FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK:
                return Rollback;

            case FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL:
                return Manual;

            default:
                return Invalid;
            }
        }

        FABRIC_MONITORED_UPGRADE_FAILURE_ACTION ToPublicApi(Enum action)
        {
            switch (action)
            {
            case Rollback:
                return FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK;

            case Manual:
                return FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL;

            default:
                return FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_INVALID;
            }
        }
    }
}
