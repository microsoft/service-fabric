// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

namespace Transport
{
    namespace CredentialType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case None: w << L"None"; return;
                case X509: w << L"X509"; return;
                case Windows: w << L"Windows"; return;
            }

            w << "CredentialType(" << static_cast<int>(e) << ')';
        }

        bool TryParse(std::wstring const & inputString, __out Enum & result)
        {
            const wchar_t * providerPtr = inputString.c_str();
            if (Common::StringUtility::AreEqualCaseInsensitive(providerPtr, L"None"))
            {
                result = Enum::None;
                return true;
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(providerPtr, L"X509"))
            {
                result = Enum::X509;
                return true;
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(providerPtr, L"Windows"))
            {
                result = Enum::Windows;
                return true;
            }

            return false;
        }
    }
}
