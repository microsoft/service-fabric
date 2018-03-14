// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ISharedLogSettingsResult
    {
    public:
        virtual ~ISharedLogSettingsResult() {};

        virtual void ToPublicApi(__in Common::ScopedHeap &, __out KTLLOGGER_SHARED_LOG_SETTINGS &) const = 0;
    };
}
