// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

namespace Common
{
    static const StringLiteral TraceType("X509StoreLocation");

    namespace X509StoreLocation
    {
        ErrorCode PlatformValidate(Enum e)
        {
#ifdef PLATFORM_UNIX
            if (e == LocalMachine) return ErrorCodeValue::Success;

            TRACE_LEVEL_AND_TESTASSERT(Trace.WriteError, TraceType, "{0} is not supported on PLATFORM_UNIX", e);
            return ErrorCodeValue::InvalidArgument;
#else
            e;  // all valid values of X509StoreLocation::Enum are allowed
            return ErrorCodeValue::Success;
#endif
        }

        Common::ErrorCode FromPublic(::FABRIC_X509_STORE_LOCATION publicEnum, Enum & internalEnum)
        {
            switch(publicEnum)
            {
            case ::FABRIC_X509_STORE_LOCATION_CURRENTUSER:
                internalEnum = CurrentUser;
                return PlatformValidate(internalEnum); 
            case ::FABRIC_X509_STORE_LOCATION_LOCALMACHINE:
                internalEnum = LocalMachine;
                return Common::ErrorCode::Success();
            }

            return Common::ErrorCodeValue::InvalidX509StoreLocation;
        }

        ::FABRIC_X509_STORE_LOCATION ToPublic(Enum internalEnum)
        {
            switch(internalEnum)
            {
            case CurrentUser:
                PlatformValidate(internalEnum);
                return ::FABRIC_X509_STORE_LOCATION_CURRENTUSER;
            case LocalMachine:
                return ::FABRIC_X509_STORE_LOCATION_LOCALMACHINE;
            default:
                Common::Assert::CodingError("Cannot convert X509StoreLocation {0}", internalEnum);
            }
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case CurrentUser: w << L"CurrentUser"; return;
                case LocalMachine: w << L"LocalMachine"; return;
            }

            w << "X509StoreLocation(" << static_cast<int>(e) << ')';
        }

        Common::ErrorCode Parse(std::wstring const & inputString, __out Enum & result)
        {
            if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"CurrentUser"))
            {
                result = Enum::CurrentUser;
                return PlatformValidate(result); 
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(inputString, L"LocalMachine"))
            {
                result = Enum::LocalMachine;
                return Common::ErrorCode::Success();
            }

            return Common::ErrorCodeValue::InvalidX509StoreLocation;
        }
    }
}
