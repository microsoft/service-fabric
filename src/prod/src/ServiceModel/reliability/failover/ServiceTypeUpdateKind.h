// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ServiceTypeUpdateKind
    {
        enum Enum
        {
            Disabled = 0,
            Enabled = 1,
            PartitionDisabled = 2,
            PartitionEnabled = 3,
            LastValidEnum = PartitionEnabled
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        DECLARE_ENUM_STRUCTURED_TRACE(ServiceTypeUpdateKind);
    }
}
