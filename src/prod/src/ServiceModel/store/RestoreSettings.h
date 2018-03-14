// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class RestoreSettings
    {
    public:
        RestoreSettings();

        RestoreSettings(bool inlineReopen);

        RestoreSettings(bool inlineReopen, bool enableLsnCheck);

        __declspec(property(get=get_InlineReopen)) bool InlineReopen;
        bool get_InlineReopen() const { return inlineReopen_; }

        __declspec(property(get = get_EnableLsnCheck)) bool EnableLsnCheck;
        bool get_EnableLsnCheck() const { return enableLsnCheck_; }

        Common::ErrorCode FromPublicApi(FABRIC_KEY_VALUE_STORE_RESTORE_SETTINGS const&);

    private:
        bool inlineReopen_;
        bool enableLsnCheck_;
    };
}

