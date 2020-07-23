// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/CertificateManager.h"

using namespace std;

namespace Common
{
    StringLiteral const TraceSource("CertificateManager");

    CertificateManager::CertificateManager(wstring subName)
        : certContext_(NULL), subjectName_(subName), DNS_(L"")
    {
        expirationDate_ = 
            DateTime::Now()
                .AddWithMaxValueCheck(TimeSpan::FromTicks(TimeSpan::TicksPerYear))
                .GetSystemTime();
    }

    CertificateManager::CertificateManager(wstring subName, wstring DNS)
        : certContext_(NULL), subjectName_(subName), DNS_(DNS)
    {
        expirationDate_ = 
            DateTime::Now()
                .AddWithMaxValueCheck(TimeSpan::FromTicks(TimeSpan::TicksPerYear))
                .GetSystemTime();
    }

    CertificateManager::CertificateManager(wstring subName, DateTime expireDate)
        : certContext_(NULL), subjectName_(subName), expirationDate_(expireDate.GetSystemTime()), DNS_(L"")
    {}


    CertificateManager::CertificateManager(wstring subName, wstring DNS, DateTime expireDate)
        : certContext_(NULL), subjectName_(subName), DNS_(DNS), expirationDate_(expireDate.GetSystemTime())
    {}

    CertificateManager::~CertificateManager()
    {
        CertFreeCertificateContext(certContext_);
    }

    ErrorCode CertificateManager::CheckContainerAndGenerateKey()
    {
        // Acquire key container
        CryptProvider cryptProvider;
        if (!cryptProvider.AcquireContext(CRYPT_MACHINE_KEYSET))
        {
            if (!cryptProvider.AcquireContext(CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET))
            {
                Trace.WriteError(TraceSource, L"CheckContainerAndGenerateKey",
                    "Failed to create a new container: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }
        }
        if (!cryptProvider.GenerateKey(AT_KEYEXCHANGE, RSA1024BIT_KEY | CRYPT_EXPORTABLE))
        {
            Trace.WriteError(TraceSource, L"CheckContainerAndGenerateKey",
                "Failed to create a new key: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        return ErrorCodeValue::Success;
    }

    ErrorCode CertificateManager::ImportToStore(wstring storeName, wstring profile)
    {
        ErrorCode error = GenerateSelfSignedCertificate();
        if (error.IsSuccess())
        {
            CertStore certStore;
            bool result = false;
            if (profile == L"U")
                result = certStore.OpenPhysicalStore(CERT_SYSTEM_STORE_CURRENT_USER, storeName);
            else if (profile == L"M")
                result = certStore.OpenPhysicalStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, storeName);

            if (!result)
            {
                Trace.WriteError(TraceSource, L"ImportToStore",
                    "Failed to open store: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }

            if (!certStore.AddCertToStore(certContext_, CERT_STORE_ADD_REPLACE_EXISTING))
            {
                Trace.WriteError(TraceSource, L"ImportToStore",
                    "Failed to add certificate to store: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }
            return ErrorCodeValue::Success;
        }
        else
        {
            return error;
        }
    }

    ErrorCode CertificateManager::SaveAsPFX(wstring fileName, SecureString pwd)
    {
        ErrorCode error = GenerateSelfSignedCertificate();
        if (error.IsSuccess())
        {
            CertStore certStore;
            if (!certStore.OpenMemoryStore())
            {
                Trace.WriteError(TraceSource, L"SaveToPFX",
                    "Failed to open memory store: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }
            if (!certStore.AddCertToStore(certContext_, CERT_STORE_ADD_REPLACE_EXISTING))
            {
                Trace.WriteError(TraceSource, L"SaveToPFX",
                    "Failed to add certificate to the memory store: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }

            if (!certStore.PFXExport(pwd))
            {
                Trace.WriteError(TraceSource, L"SaveToPFX",
                    "Failed to export as PFX: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }
            CRYPT_DATA_BLOB pfxBlob = certStore.GetPFXBlob();

            FileWriter file;
            error = file.TryOpen(fileName);
            if (!error.IsSuccess())
            {
                Trace.WriteError(TraceSource, L"SaveToPFX",
                    "Error while opening file to save as PFX: error={0}", ::GetLastError());
                file.Close();
                return ErrorCode::FromWin32Error(::GetLastError());
            }
            wstring wstr(reinterpret_cast<wchar_t*>(pfxBlob.pbData), pfxBlob.cbData / sizeof(wchar_t));
            file.WriteUnicodeBuffer(wstr.c_str(), wstr.size());
            file.Close();

            return ErrorCodeValue::Success;
        }
        else
        {
            return error;
        }
    }

    ErrorCode CertificateManager::GetSidOfCurrentUser(wstring &sid)
    {
        HANDLE htoken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &htoken))
        {
            Trace.WriteError(TraceSource, L"GetSidOfCurrentUser",
                "Failed to open Process Token: error={0}", ::GetLastError());
            CloseHandle(htoken);
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        DWORD dwBufferSize = 0;
        if (!GetTokenInformation(htoken, TokenUser, NULL, 0, &dwBufferSize))
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                Trace.WriteError(TraceSource, L"GetSidOfCurrentUser",
                    "Failed to get Token Information: error={0}", ::GetLastError());
                CloseHandle(htoken);
                return ErrorCode::FromWin32Error(::GetLastError());
            }
        }
        vector<BYTE> buffer(dwBufferSize, 0);
        PTOKEN_USER pTokenUser = reinterpret_cast<PTOKEN_USER>(buffer.data());
        if (!GetTokenInformation(htoken, TokenUser, pTokenUser, dwBufferSize, &dwBufferSize))
        {
            Trace.WriteError(TraceSource, L"GetSidOfCurrentUser",
                "Failed to get Token Information: error={0}", ::GetLastError());
            CloseHandle(htoken);
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        CloseHandle(htoken);
        if (!IsValidSid(pTokenUser->User.Sid))
        {
            Trace.WriteError(TraceSource, L"GetSidOfCurrentUser",
                "Failed to validate SID of current user: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        wchar_t *sidC = const_cast<wchar_t*>(sid.c_str());
        if (!ConvertSidToStringSid(pTokenUser->User.Sid, &sidC))
        {
            Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                "Failed to convert SID to string: error={0}", ::GetLastError());
            LocalFree(sidC);
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        wstring wSid(sidC);
        LocalFree(sidC);
        sid = wSid;
        return ErrorCodeValue::Success;
    }

    ErrorCode CertificateManager::GenerateSelfSignedCertificate()
    {
        ErrorCode error = CheckContainerAndGenerateKey();
        if (error.IsSuccess())
        {
            // Encode certificate Subject				
            wstring wcSubject = L"CN = " + this->subjectName_ ;
            BYTE *pbEncoded = NULL;
            DWORD cbEncoded = 0;
            if (!CertStrToName(X509_ASN_ENCODING, wcSubject.c_str(), CERT_NAME_STR_DISABLE_UTF8_DIR_STR_FLAG, NULL, pbEncoded, &cbEncoded, NULL))
            {
                Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                    "Failed to encode subject name: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }
            vector<BYTE> buffer(cbEncoded, 0);
            pbEncoded = buffer.data();
            if (!CertStrToName(X509_ASN_ENCODING, wcSubject.c_str(), CERT_NAME_STR_DISABLE_UTF8_DIR_STR_FLAG, NULL, pbEncoded, &cbEncoded, NULL))
            {
                Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                    "Failed to encode subject name: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }

            // Prepare certificate Subject for self-signed certificate
            CERT_NAME_BLOB SubjectIssuerBlob;
            memset(&SubjectIssuerBlob, 0, sizeof(SubjectIssuerBlob));
            SubjectIssuerBlob.cbData = cbEncoded;
            SubjectIssuerBlob.pbData = pbEncoded;

            //Set security descriptor
            wstring sid = L"";
            if (!GetSidOfCurrentUser(sid).IsSuccess())
            {
                Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                    "Failed to return SID of current user: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }

            //Security Descriptor picked from the CertSetup scripts in Fabric SDK's.
            wstring secDesc = wformatString(L"D:PAI(A;;0xd01f01ff;;;SY)(A;;0xd01f01ff;;;BA)(A;;0xd01f01ff;;;NS)(A;;0xd01f01ff;;;{0})", sid);
            BYTE *sd = reinterpret_cast<BYTE*>(&secDesc);

            //Prepare key parametric information for self-signed certificate
            CRYPT_KEY_PROV_PARAM KeyParamInfo[1];
            KeyParamInfo[0].dwParam = PP_KEYSET_SEC_DESCR;
            KeyParamInfo[0].pbData = sd;
            KeyParamInfo[0].cbData = sizeof(KeyParamInfo[0].pbData);

            // Prepare key provider structure for self-signed certificate
            CRYPT_KEY_PROV_INFO KeyProvInfo;
            memset(&KeyProvInfo, 0, sizeof(KeyProvInfo));
            KeyProvInfo.pwszContainerName = NULL;
            KeyProvInfo.pwszProvName = MS_DEF_RSA_SCHANNEL_PROV;
            KeyProvInfo.dwProvType = PROV_RSA_SCHANNEL;
            KeyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET;
            KeyProvInfo.cProvParam = sizeof(KeyParamInfo)/sizeof(KeyParamInfo[0]);
            KeyProvInfo.rgProvParam = KeyParamInfo;
            KeyProvInfo.dwKeySpec = AT_KEYEXCHANGE;

            // Prepare algorithm structure for self-signed certificate
            CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
            memset(&SignatureAlgorithm, 0, sizeof(SignatureAlgorithm));
            SignatureAlgorithm.pszObjId = szOID_RSA_SHA1RSA;

            //Prepare Certificate Extensions for Key Usage
            BYTE key_usage_value = CERT_DIGITAL_SIGNATURE_KEY_USAGE | CERT_KEY_ENCIPHERMENT_KEY_USAGE;
            CERT_KEY_USAGE_RESTRICTION_INFO keyUsage = {
                0,
                NULL,
                { sizeof(key_usage_value) ,&key_usage_value }
            };
            BYTE *pbEncode = NULL;
            DWORD cbEncode = 0;
            if (!CryptEncodeObject(X509_ASN_ENCODING, szOID_KEY_USAGE_RESTRICTION, &keyUsage, pbEncode, &cbEncode))
            {
                Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                    "Failed to encode object for Key Usage: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }
            {
                vector<BYTE> buff(cbEncode, 0);
                pbEncode = buff.data();
                if (!CryptEncodeObject(X509_ASN_ENCODING, szOID_KEY_USAGE_RESTRICTION, &keyUsage, pbEncode, &cbEncode))
                {
                    Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                        "Failed to encode object for Key Usage: error={0}", ::GetLastError());
                    return ErrorCode::FromWin32Error(::GetLastError());
                }
            }

            CERT_EXTENSIONS certExtensions;
            CERT_EXTENSION certExt[2];
            certExt[0] = { szOID_KEY_USAGE_RESTRICTION,
                            TRUE,
                            { cbEncode, pbEncode }
            };

            //Prepare DNS Name
            if (!DNS_.empty())
            {
                CERT_ALT_NAME_ENTRY AltNames[1];
                CERT_ALT_NAME_INFO AltNameInfo = { 1, AltNames };
                AltNames[0].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
                AltNames[0].pwszDNSName = (LPWSTR)DNS_.c_str();

                BYTE *pb = NULL;
                DWORD cb = 0;
                if (!CryptEncodeObject(X509_ASN_ENCODING, szOID_SUBJECT_ALT_NAME, &AltNameInfo, pb, &cb))
                {
                    Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                        "Failed to encode object for Key Usage: error={0}", ::GetLastError());
                    return ErrorCode::FromWin32Error(::GetLastError());
                }
                {
                    vector<BYTE> buff(cb, 0);
                    pb = buff.data();
                    if (!CryptEncodeObject(X509_ASN_ENCODING, szOID_SUBJECT_ALT_NAME, &AltNameInfo, pb, &cb))
                    {
                        Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                            "Failed to encode object for Key Usage: error={0}", ::GetLastError());
                        return ErrorCode::FromWin32Error(::GetLastError());
                    }
                }
                certExt[1] = { szOID_SUBJECT_ALT_NAME,
                                TRUE,
                                { cb, pb }
                };
                certExtensions.cExtension = 2;
                certExtensions.rgExtension = certExt;
            }
            else
            {
                certExtensions.cExtension = 1;
                certExtensions.rgExtension = certExt;
            }

            // Create self-signed certificate
            certContext_ = CertCreateSelfSignCertificate(NULL, &SubjectIssuerBlob, 0, &KeyProvInfo, &SignatureAlgorithm, 0, &expirationDate_, &certExtensions);
            if (!certContext_)
            {
                Trace.WriteError(TraceSource, L"GenerateSelfSignedCertificate",
                    "Failed to create self signed certificate: error={0}", ::GetLastError());
                return ErrorCode::FromWin32Error(::GetLastError());
            }

            CertAddEnhancedKeyUsageIdentifier(certContext_, szOID_PKIX_KP_SERVER_AUTH);
            CertAddEnhancedKeyUsageIdentifier(certContext_, szOID_PKIX_KP_CLIENT_AUTH);

            return ErrorCodeValue::Success;
        }
        else
        {
            return error;
        }
    }

    /*exactMatch = true will delete only those certificates that exactly match certName
                 = false will delete all certificates which have the prefix certName*/

    ErrorCode CertificateManager::DeleteCertFromStore(wstring certName, wstring storeName, wstring profile, bool exactMatch)
    {
        CertStore hs;
        bool result = false;
        if (profile == L"U")
            result = hs.OpenPhysicalStore(CERT_SYSTEM_STORE_CURRENT_USER, storeName);
        else if (profile == L"M")
            result = hs.OpenPhysicalStore(CERT_SYSTEM_STORE_LOCAL_MACHINE, storeName);

        if (!result)
        {
            Trace.WriteError(TraceSource, L"DeleteCertFromStore",
                "Failed to open store: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        if (!hs.DeleteFromStore(certName, exactMatch))
        {
            Trace.WriteError(TraceSource, L"DeleteCertFromStore",
                "Failed to delete cert from store: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }
        return ErrorCodeValue::Success;
    }

    ErrorCode CertificateManager::GenerateAndACLPFXFromCer(
        X509StoreLocation::Enum cerStoreLocation,
        wstring x509StoreName,
        wstring x509Thumbprint,
        wstring pfxFilePath,
        SecureString password,
        vector<wstring> machineAccountNamesForACL)
    {
        shared_ptr<X509FindValue> findValue;
        auto error = X509FindValue::Create(X509FindType::FindByThumbprint, x509Thumbprint, findValue);
        if (!error.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceSource,
                "The thumbprint is invalid. Thumbprint={0}, ErrorCode={1}",
                x509Thumbprint,
                error);
            return error;
        }

        CertContextUPtr certContext;
        error = CryptoUtility::GetCertificate(
            cerStoreLocation,
            x509StoreName,
            findValue,
            certContext);
        if (!error.IsSuccess())
        {
            Trace.WriteError(
                TraceSource,
                L"GeneratePFX",
                "GetCertificate for {0} failed with {1}",
                x509Thumbprint,
                error);
            return error;
        }

        CertStore certStore;
        if (!certStore.OpenMemoryStore())
        {
            Trace.WriteError(TraceSource, L"GeneratePFX",
                "Failed to open memory store: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        auto pCertContext = certContext.get();
        HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hPrivateKey = NULL;
        DWORD pdwKeySpec = 0;
        BOOL pfCallerFreeProv = false;
        if (!CryptAcquireCertificatePrivateKey(
            pCertContext,
            NULL,
            NULL,
            &hPrivateKey,
            &pdwKeySpec,
            &pfCallerFreeProv))
        {
            Trace.WriteInfo(TraceSource, "Failed to CryptAcquireCertificatePrivateKey: error={0}", ::GetLastError());
            hPrivateKey = NULL;
        }

        if (!certStore.AddCertToStore(pCertContext, CERT_STORE_ADD_REPLACE_EXISTING, hPrivateKey))
        {
            if (pfCallerFreeProv)
            {
                CryptReleaseContext(hPrivateKey, 0);
            }

            Trace.WriteError(TraceSource, L"GeneratePFX",
                "Failed to add certificate to the memory store: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        if (pfCallerFreeProv)
        {
            CryptReleaseContext(hPrivateKey, 0);
        }

        if (!certStore.PFXExport(password))
        {
            Trace.WriteError(TraceSource, L"GeneratePFX",
                "Failed to export as PFX: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        CRYPT_DATA_BLOB pfxBlob = certStore.GetPFXBlob();
        if (!PFXIsPFXBlob(&pfxBlob))
        {
            Trace.WriteError(TraceSource, L"GeneratePFX", "PFX blob is invalid: error={0}", ::GetLastError());
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        error = CertificateManager::SavePFXBlobInFile(pfxBlob, pfxFilePath);
        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceSource, "GeneratePFX: Failed to SavePFXandPwd : {0}", error);
            return error;
        }

        error = CertificateManager::ACLPFXFile(machineAccountNamesForACL, pfxFilePath);
        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceSource, "GeneratePFX: Failed to ACLPFXandPwdFiles : {0}", error);
            return error;
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode CertificateManager::SavePFXBlobInFile(CRYPT_DATA_BLOB pfxBlob, wstring PFXfileName)
    {
        FileWriter file;
        auto error = file.TryOpen(PFXfileName);
        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceSource, L"SavePFXandPwd",
                "Error while opening file to save as PFX: error={0}", ::GetLastError());
            file.Close();
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        file.WriteBuffer(reinterpret_cast<char*>(pfxBlob.pbData), pfxBlob.cbData);
        file.Close();

        return ErrorCodeValue::Success;
    }

    ErrorCode CertificateManager::ACLPFXFile(vector<wstring> machineAccountNamesForACL, wstring PFXfileName)
    {
        vector<SidSPtr> sidVector;

        for (auto machineAccountNameForACL : machineAccountNamesForACL)
        {
            Common::SidSPtr sid;
            auto error = BufferedSid::CreateSPtr(machineAccountNameForACL, sid);
            if (!error.IsSuccess())
            {
                Trace.WriteError(TraceSource, "ACLPFXandPwdFiles: Failed to convert {0} to SID: {1}", machineAccountNameForACL, error);
                return error;
            }

            sidVector.push_back(sid);
        }

        auto error = SecurityUtility::OverwriteFileAcl(
            sidVector,
            PFXfileName,
            GENERIC_READ | GENERIC_WRITE,
            TimeSpan::MaxValue);

        if (!error.IsSuccess())
        {
            Trace.WriteError(TraceSource, "ACLPFXandPwdFiles: Failed to OverwriteFileAcl for {0} : {1}", PFXfileName, error);
            return error;
        }

        return ErrorCodeValue::Success;
    }
}


