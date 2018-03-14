// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{
    /// <summary>
    /// The base class for classes containing event data.  
    /// </summary>
    class EventArgs
    {
    public:

        EventArgs() {}
        virtual ~EventArgs() {}
    };
}

