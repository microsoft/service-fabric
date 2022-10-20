// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class DenyCopyAssign
    {
    public:
        DenyCopyAssign()
        {
        }

        ~DenyCopyAssign()
        {
        }

    private:
        DenyCopyAssign & operator=(DenyCopyAssign const & other);
    };
}
