// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        enum class KtlLoggerMode : short
        {
            Default    = 0, // choose the mode based on platform, environment, and configuration
            InProc     = 1, // choose the appropriate inproc mode (staging log, inproc shared log, in the future direct dedicated) based on platform, environment, and configuration
            OutOfProc  = 2  // choose the appropriate outofproc mode (driver, in the future daemon)
        };

        bool IsValidKtlLoggerMode(__in KtlLoggerMode const & ktlLoggerMode);
    }
}
