// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    // std::map custom compare function class has to be public
    struct ConstCharPtrLess
    {
        bool operator()(const char* s1, const char* s2) const
        {
            return (s1 != s2) && (strcmp(s1, s2) < 0);
        }
    };
}
