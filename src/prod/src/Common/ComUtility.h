// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComUtility
    {
    public:
        static ErrorCode CheckMTA();
        static void ClearErrorInfo();

        static HRESULT OnPublicApiReturn(HRESULT hr);
        static HRESULT OnPublicApiReturn(HRESULT hr, std::wstring && description);
        static HRESULT OnPublicApiReturn(Common::ErrorCode const & error);
        static HRESULT OnPublicApiReturn(Common::ErrorCode && error);
    private:
        static Common::StringLiteral const ComUtilitySource;

        static bool Initialize();
    };
}
