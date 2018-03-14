// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ToggleVerboseServicePlacementHealthReportingMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        ToggleVerboseServicePlacementHealthReportingMessageBody()
        {
        }

        ToggleVerboseServicePlacementHealthReportingMessageBody(bool enabled)
            : enabled_(enabled)
        {
        }

        __declspec (property(get=get_Enabled)) bool Enabled;
        bool get_Enabled() const { return enabled_; }

        FABRIC_FIELDS_01(enabled_);

    private:

        bool enabled_;
    };
}
