// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace X509FindType
    {
        enum Enum
        {
            FindByThumbprint = CERT_FIND_SHA1_HASH,
            FindBySubjectName = CERT_FIND_SUBJECT_NAME,
            FindByCommonName = CERT_FIND_SUBJECT_ATTR,
            FindByExtension = FABRIC_X509_FIND_TYPE_FINDBYEXTENSION,
        };

        Common::ErrorCode Parse(std::wstring const & providerString, __out Enum & result);
        Common::ErrorCode FromPublic(::FABRIC_X509_FIND_TYPE publicEnum, X509FindType::Enum & internalEnum);
        ::FABRIC_X509_FIND_TYPE ToPublic(X509FindType::Enum internalEnum);
		std::wstring EnumToString(Common::X509FindType::Enum value);
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
