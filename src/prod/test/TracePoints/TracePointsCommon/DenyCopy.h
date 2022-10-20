// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    class DenyCopy : private DenyCopyAssign, private DenyCopyConstruct
    {
    public:
        DenyCopy()
        {
        }

        ~DenyCopy()
        {
        }
    };
}
