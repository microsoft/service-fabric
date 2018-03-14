// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    using namespace Common;
    using namespace std;

    namespace SecurityProvider
    {
        Common::ErrorCode FromPublic(::FABRIC_SECURITY_CREDENTIAL_KIND publicEnum, SecurityProvider::Enum & internalEnum)
        {
            switch(publicEnum)
            {
                case ::FABRIC_SECURITY_CREDENTIAL_KIND_NONE:
                    internalEnum = SecurityProvider::None;
                    return Common::ErrorCode::Success();
                case ::FABRIC_SECURITY_CREDENTIAL_KIND_X509:
                case ::FABRIC_SECURITY_CREDENTIAL_KIND_X509_2:
                    internalEnum = SecurityProvider::Ssl;
                    return Common::ErrorCode::Success();
                case ::FABRIC_SECURITY_CREDENTIAL_KIND_WINDOWS:
                    internalEnum = SecurityConfig::GetConfig().NegotiateForWindowsSecurity ? SecurityProvider::Negotiate : SecurityProvider::Kerberos;
                    return Common::ErrorCode::Success();
                case ::FABRIC_SECURITY_CREDENTIAL_KIND_CLAIMS:
                    internalEnum = SecurityProvider::Claims;
                    return Common::ErrorCode::Success();
            }

            return Common::ErrorCode(Common::ErrorCodeValue::InvalidCredentialType);
        }

        ::FABRIC_SECURITY_CREDENTIAL_KIND ToPublic(SecurityProvider::Enum internalEnum, ::FABRIC_SECURITY_CREDENTIAL_KIND convertedFrom)
        {
            if (convertedFrom != FABRIC_SECURITY_CREDENTIAL_KIND_INVALID)
                return convertedFrom;

            switch(internalEnum)
            {
                case None:
                    return ::FABRIC_SECURITY_CREDENTIAL_KIND_NONE;

                case Ssl:
                    return ::FABRIC_SECURITY_CREDENTIAL_KIND_X509;

                case Negotiate:
                case Kerberos:
                    return ::FABRIC_SECURITY_CREDENTIAL_KIND_WINDOWS;

                case Claims:
                    return ::FABRIC_SECURITY_CREDENTIAL_KIND_CLAIMS;

                default:
                    Common::Assert::CodingError("Cannot convert SecurityProvider {0} to public api", internalEnum);
            }
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case None: w << L"None"; return;
                case Negotiate: w << NEGOSSP_NAME; return;
                case Kerberos: w << MICROSOFT_KERBEROS_NAME; return;
                case Ssl: w << L"SSL"; return;
                case Claims: w << L"Claims"; return;
            }

            w << "SecurityProvider(" << static_cast<int>(e) << ')';
        }

        byte EnumToMask(Enum e, byte inputMask)
        {
            if (e == None) return inputMask;

            Invariant((int)e <= 8);
            byte newMask = 1 << ((int)e - 1);
            return inputMask | newMask; 
        }

        byte EnumsToMask(vector<Enum> es)
        {
            byte mask = 0;
            for(auto const & e : es)
            {
                mask = EnumToMask(e, mask);
            }
            return mask;
        }

        vector<Enum> MaskToEnums(byte mask)
        {
            vector<Enum> es;

            //None will not be included, as None is supported by all capability-wise, whether it is supported is determined by local config 
            if (mask & 1) es.push_back(Ssl);
            if (mask & 2) es.push_back(Kerberos);
            if (mask & 4) es.push_back(Negotiate);
            if (mask & 8) es.push_back(Claims); // can we save a bit on non-transport-security provider? e.g. Claims

            return es;
        }

        void WriteMaskToTextWriter(Common::TextWriter & w, byte mask)
        {
            if (mask == SecurityProvider::None)
            {
                w << mask;
                return;
            }

            std::wstring buffer;
            StringWriter sw(buffer);
            for(auto e : MaskToEnums(mask))
            {
               if (!buffer.empty()) sw << L'|';
                sw << e;
            }

            w << buffer;
        }

        wchar_t * GetSspiProviderName(Enum e)
        {
            switch (e)
            {
                case Negotiate: return NEGOSSP_NAME;
                case Kerberos: return MICROSOFT_KERBEROS_NAME;
                case Ssl:
                case Claims:
                    return UNISP_NAME;
            }

            Common::Assert::CodingError("Unknown security provider: {0}", static_cast<int>(e));
        }

        Common::ErrorCode FromCredentialType(std::wstring const & providerString, Enum & result)
        {
            // Only None and X509 are supported 
            if (Common::StringUtility::AreEqualCaseInsensitive(providerString, L"None"))
            {
                result = Enum::None;
                return Common::ErrorCode::Success();
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(providerString, L"X509") ||
                    Common::StringUtility::AreEqualCaseInsensitive(providerString, L"SSL"))
            {
                result = Enum::Ssl;
                return Common::ErrorCode::Success();
            }

            if (StringUtility::AreEqualCaseInsensitive(providerString, L"Windows") ||
                StringUtility::AreEqualCaseInsensitive(providerString, L"Negotiate"))
            {
                result = SecurityConfig::GetConfig().NegotiateForWindowsSecurity ? Enum::Negotiate : Enum::Kerberos;
                return Common::ErrorCode::Success();
            }

            if (Common::StringUtility::AreEqualCaseInsensitive(providerString, L"Claims"))
            {
                result = Enum::Claims;
                return Common::ErrorCode::Success();
            }

            return Common::ErrorCode(Common::ErrorCodeValue::InvalidCredentialType);
        }

#include "SecurityProviderEnum.cpp"

        BEGIN_ENUM_STRUCTURED_TRACE(SecurityProvider)
            ADD_CASTED_ENUM_MAP_VALUE(SecurityProvider, None)
            ADD_CASTED_ENUM_MAP_VALUE(SecurityProvider, Ssl)
            ADD_CASTED_ENUM_MAP_VALUE(SecurityProvider, Kerberos)
            ADD_CASTED_ENUM_MAP_VALUE(SecurityProvider, Negotiate)
            ADD_CASTED_ENUM_MAP_VALUE(SecurityProvider, Claims)
        END_ENUM_STRUCTURED_TRACE(SecurityProvider)
    }
}
