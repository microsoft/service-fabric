// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Transport
{
    namespace ProtectionLevel
    {
        _Use_decl_annotations_
        Common::ErrorCode FromPublic(::FABRIC_PROTECTION_LEVEL publicEnum, Enum & internalEnum)
        {
            switch(publicEnum)
            {
            case ::FABRIC_PROTECTION_LEVEL_NONE:
                internalEnum = None;
                return Common::ErrorCode::Success();
            case ::FABRIC_PROTECTION_LEVEL_SIGN:
                internalEnum = Sign;
                return Common::ErrorCode::Success();
            case ::FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN:
                internalEnum = EncryptAndSign;
                return Common::ErrorCode::Success();
            }

          internalEnum = None;
          return ErrorCodeValue::InvalidProtectionLevel;
        }

        ::FABRIC_PROTECTION_LEVEL ToPublic(Enum internalEnum)
        {
            switch(internalEnum)
            {
            case None:
                return ::FABRIC_PROTECTION_LEVEL_NONE;
            case Sign:
                return ::FABRIC_PROTECTION_LEVEL_SIGN;
            case EncryptAndSign:
                return ::FABRIC_PROTECTION_LEVEL_ENCRYPTANDSIGN;
            default:
                Common::Assert::CodingError("Cannot convert ProtectionLevel {0}", internalEnum);
            }
        }

        _Use_decl_annotations_
        Common::ErrorCode Parse(wstring const & inputString, Enum & result)
        {
            if (StringUtility::AreEqualCaseInsensitive(inputString, L"None"))
            {
                result = None;
                return Common::ErrorCode::Success();
            }

            if (StringUtility::AreEqualCaseInsensitive(inputString, L"Sign"))
            {
                result = Sign;
                return Common::ErrorCode::Success();
            }

            if (StringUtility::AreEqualCaseInsensitive(inputString, L"EncryptAndSign"))
            {
                result = EncryptAndSign;
                return Common::ErrorCode::Success();
            }

            result = None;
            return ErrorCodeValue::InvalidProtectionLevel;
        }

        void WriteToTextWriter(TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case None: w << L"None"; return;
                case Sign: w << L"Sign"; return;
                case EncryptAndSign: w << L"EncryptAndSign"; return;
            }

            w << "ProtectionLevel(" << static_cast<int>(e) << ')';
        }
    }
}
