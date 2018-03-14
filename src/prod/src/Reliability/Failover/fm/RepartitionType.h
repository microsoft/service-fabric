// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace RepartitionType
        {
            enum Enum
            {
                Invalid = 0,
                Add = 1,
                Remove = 2,

                LastValidEnum = Remove
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

            DECLARE_ENUM_STRUCTURED_TRACE(RepartitionType);
        }
    }
}
