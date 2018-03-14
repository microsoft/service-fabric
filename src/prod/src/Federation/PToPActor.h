// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    namespace PToPActor
    {
        // Disable the warning about Enum base type, since it is already part of C++0X.
        #pragma warning( push )
        #pragma warning( disable : 4480 )

        // This enum must end with UpperBound.
        enum Enum
        {
            Direct = 0,
            Federation = 1,
            Routing = 2,
            Broadcast = 3,
            UpperBound = 4, // This MUST be the last item in the enum
        };

        #pragma warning( pop )

        void WriteToTextWriter(Common::TextWriter & w, Enum const & enumValue);
    }
}
