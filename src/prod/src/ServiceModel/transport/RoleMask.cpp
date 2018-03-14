// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Transport
{
    namespace RoleMask
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case None: w << L"None"; return;
                case User: w << L"User"; return;
                case Admin: w << L"Admin"; return;
                case All: w << L"All"; return;
                default:
                    w.Write("{0:x}", (unsigned int)e);
            }
        }

        ErrorCode TryParse(std::wstring const & stringInput, _Out_ Enum & roleMask)
        {
            roleMask = None;

            if (StringUtility::AreEqualCaseInsensitive(stringInput, L"None"))
            {
                return ErrorCode();
            }

            if (StringUtility::AreEqualCaseInsensitive(stringInput, L"User"))
            {
                roleMask = User;
                return ErrorCode();
            }

            if (StringUtility::AreEqualCaseInsensitive(stringInput, L"Admin"))
            {
                roleMask = Admin;
                return ErrorCode();
            }

            if (StringUtility::AreEqualCaseInsensitive(stringInput, L"All"))
            {
                roleMask = All;
                return ErrorCode();
            }

            return ErrorCodeValue::InvalidArgument;
        }

        ErrorCode TryParse(FABRIC_CLIENT_ROLE clientRole, _Out_ Enum & roleMask)
        {
            roleMask = None;

            if (clientRole == FABRIC_CLIENT_ROLE_USER)
            {
                roleMask = User;
                return ErrorCode();
            }

            if (clientRole == FABRIC_CLIENT_ROLE_ADMIN)
            {
                roleMask = Admin;
                return ErrorCode();
            }

            return ErrorCodeValue::InvalidArgument;
        }

        std::wstring EnumToString(Transport::RoleMask::Enum e)
        {
            switch (e)
            {
            case None:
                return L"None";
            case User:
                return L"User";
            case Admin:
                return L"Admin";
            case All:
                return L"All";
            default:
                return wformatString("{0:x}", (unsigned int)e);
            }
        }
    }
}
