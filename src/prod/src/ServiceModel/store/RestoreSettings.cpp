// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;

RestoreSettings::RestoreSettings()
    : inlineReopen_(false)
    , enableLsnCheck_(false)
{
} 

RestoreSettings::RestoreSettings(bool inlineReopen)
    : inlineReopen_(inlineReopen)
    , enableLsnCheck_(false)
{
}

RestoreSettings::RestoreSettings(bool inlineReopen, bool enableLsnCheck)
    : inlineReopen_(inlineReopen)
    , enableLsnCheck_(enableLsnCheck)
{

}

ErrorCode RestoreSettings::FromPublicApi(FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS const& publicSettings)
{
    inlineReopen_ = (publicSettings.InlineReopen == TRUE);

    if (publicSettings.Reserved == nullptr)
    {
        return ErrorCodeValue::Success;
    }

    auto castedEx1 = static_cast<FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS_EX1*>(publicSettings.Reserved);

    enableLsnCheck_ = (castedEx1->EnableLsnCheck == TRUE);

    return ErrorCodeValue::Success;
}
