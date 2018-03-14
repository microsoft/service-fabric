// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        namespace MustCatchup
        {
            enum Enum
            {
                Unknown = 0,
                No = 1,
                Yes = 2,

                LastValidEnum = Yes
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            bool AreEqual(Enum value, bool mustCatchup);

            DECLARE_ENUM_STRUCTURED_TRACE(MustCatchup);
        }
    }
}

