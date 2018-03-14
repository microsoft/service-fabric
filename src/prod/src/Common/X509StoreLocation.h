// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace X509StoreLocation
    {
        enum Enum
        {
            CurrentUser = CERT_SYSTEM_STORE_CURRENT_USER,
            LocalMachine = CERT_SYSTEM_STORE_LOCAL_MACHINE
        };
		
        Common::ErrorCode Parse(std::wstring const & providerString, __out Enum & result);
        Common::ErrorCode FromPublic(::FABRIC_X509_STORE_LOCATION publicEnum, X509StoreLocation::Enum & internalEnum);
        ::FABRIC_X509_STORE_LOCATION ToPublic(X509StoreLocation::Enum internalEnum);

        Common::ErrorCode PlatformValidate(Enum e);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}

#define PLATFORM_VALIDATE(e) { auto err = Common::X509StoreLocation::PlatformValidate(e); if (!err.IsSuccess()) return err; }
