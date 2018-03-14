// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Data
{
    namespace Interop
    {
        enum AsyncMode
        {
            None,
            Async,
            Random
        };

        struct TestSettings
        {
            bool enabled_;
            AsyncMode asyncMode_;
            DWORD delayInMS;

            TestSettings()
            {
                enabled_ = false;
            }

            bool forceAsync()
            {
                if (!enabled_)
                    return false;
                return  asyncMode_ == AsyncMode::Async || (asyncMode_ == AsyncMode::Random && rand() % 2 == 1);
            }

            void conditionalDelayBeforeCallback(bool delay)
            {
                if (!enabled_ || !delay)
                    return;
                Sleep(delayInMS);
            }

        };

        extern TestSettings testSettings;

        // FABRICSTORE_ASYNCMODE 0 - 2
        // FABRICSTORE_DELAYBEFORECALLBACK time in ms

    }
}

