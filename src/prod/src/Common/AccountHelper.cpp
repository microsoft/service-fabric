// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

GlobalWString AccountHelper::UPNDelimiter = make_global<wstring>(L"@");
GlobalWString AccountHelper::DLNDelimiter = make_global<wstring>(L"\\");
GlobalWString AccountHelper::GroupNameLOCAL = make_global<wstring>(L"Local");

// Use a prefix that contains multiple different characters,
// so the account password policy doesn't complain
GlobalWString AccountHelper::PasswordPrefix = make_global<wstring>(L"ABCD!abcd@#$");
DWORD const AccountHelper::RandomDigitsInPassword = 10;
size_t const AccountHelper::MaxCharactersInAccountUsername = 20;
int64 const AccountHelper::ExpiryBaselineTicks = 131592384000000000; // ticks for January 1st, 2018

StringLiteral TraceType_AccountHelper = "AccountHelper";

ErrorCode AccountHelper::GetDomainUserAccountName(wstring accountName, __out wstring & userName, __out wstring & domain, __out wstring & dlnFormatName)
{
    vector<wstring> names;
    if(StringUtility::Contains<wstring>(accountName, UPNDelimiter))
    {
        StringUtility::Split<wstring>(accountName, names, UPNDelimiter, true);

        if(names.size() != 2)
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "The AccountName format is incorrect. AccountName={0}",
                accountName);
            return ErrorCodeValue::InvalidArgument;
        }

        domain = move(names[1]);
        userName = move(names[0]);
        dlnFormatName = wformatString("{0}\\{1}", domain, userName);
    }
    else
    {
        StringUtility::Split<wstring>(accountName, names, DLNDelimiter, true);

        if(names.size() != 2)
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "The AccountName format is incorrect. AccountName={0}",
                accountName);
            return ErrorCodeValue::InvalidArgument;
        }

        domain = move(names[0]);
        userName = move(names[1]);
        dlnFormatName = accountName;
    }

    return ErrorCodeValue::Success;
}

ErrorCode AccountHelper::GetServiceAccountName(wstring accountName, __out wstring & serviceName, __out wstring & domain, __out wstring & dlnFormatName)
{
    vector<wstring> names;
    if(StringUtility::Contains<wstring>(accountName, UPNDelimiter))
    {
        StringUtility::Split<wstring>(accountName, names, UPNDelimiter, true);

        if(names.size() != 2)
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "The AccountName format is incorrect. AccountName={0}",
                accountName);
            return ErrorCodeValue::InvalidArgument;
        }

        domain = move(names[1]);
        serviceName = move(names[0]);
    }
    else
    {
        StringUtility::Split<wstring>(accountName, names, DLNDelimiter, true);

        if(names.size() != 2)
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "The AccountName format is incorrect. AccountName={0}",
                accountName);
            return ErrorCodeValue::InvalidArgument;
        }

        domain = move(names[0]);
        serviceName = move(names[1]);
    }
    if(!StringUtility::EndsWith<wstring>(serviceName, L"$"))
    {
        serviceName += L"$";
    }

    dlnFormatName = wformatString("{0}\\{1}", domain, serviceName);

    return ErrorCodeValue::Success;
}

bool AccountHelper::GroupAllowsMemberAddition(wstring const & groupName)
{
    if(StringUtility::AreEqualCaseInsensitive(groupName, AccountHelper::GroupNameLOCAL))
    {
        return false;
    }
    return true;
}

wstring AccountHelper::GetAccountName(wstring const & name, ULONG applicationPackageCounter, bool canSkipAddCounter)
{
    if (canSkipAddCounter && name.size() == MaxCharactersInAccountUsername)
    {
        return name;
    }

    wstring output;
    StringWriter(output).Write("{0}{1:x}", name, applicationPackageCounter);
    return output;
}

ErrorCode AccountHelper::GetAccountNamesWithCertificateCommonName(
    std::wstring const& commonName,
    X509StoreLocation::Enum storeLocation,
    std::wstring const & storeName,
    bool generateV1Account,
    __inout std::map<std::wstring, std::wstring> & accountNamesMap)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_AccountHelper,
        "GetAccountNamesWithCertificateCommonName: x509CommonName={0}, StoreLocation={1}, StoreName={2}",
        commonName,
        storeLocation,
        storeName);

    X509FindValue::SPtr findValue;
    auto errorCode = X509FindValue::Create(X509FindType::FindBySubjectName, commonName, findValue);
    if(!errorCode.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "GetAccountNamesWithCertificateCommonName: create findValue for common name {0} failed: {1}.",
            commonName,
            errorCode);
        return errorCode;
    }

    CertContexts certContexts;
    errorCode = CryptoUtility::GetCertificate(
        storeLocation,
        storeName,
        findValue,
        certContexts);
    if(!errorCode.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "GetAccountNamesWithCertificateCommonName: GetCertificates for common name {0} failed: {1}.",
            commonName,
            errorCode);
        return errorCode;
    }

    accountNamesMap.clear();
    for (auto const & certContext : certContexts)
    {
        Thumbprint::SPtr thumbprintObj;
        errorCode = Thumbprint::Create(certContext.get(), thumbprintObj);
        if(!errorCode.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "GetAccountNamesWithCertificateCommonName: Thumbprint::Create for common name {0} failed: {1}.",
                commonName,
                errorCode);
            return errorCode;
        }

        wstring thumbprint = wformatString(thumbprintObj);
        if (generateV1Account)
        {
            wstring accountNameV1 = AccountHelper::GenerateUserAcountNameV1(thumbprintObj);
            TraceInfo(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "GetAccountNamesWithCertificateCommonName: generate V1 user account name {0} based on thumbprint {1}.",
                accountNameV1,
                thumbprint);
            accountNamesMap.insert(make_pair(move(accountNameV1), thumbprint));
        }

        // Generate account using v2 schema
        DateTime expirationTime;
        errorCode = CryptoUtility::GetCertificateExpiration(certContext, expirationTime);
        if (!errorCode.IsSuccess())
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "GetAccountNamesWithCertificateCommonName: CryptoUtility::GetCertificateExpiration for common name {0} failed: {1}.",
                commonName,
                errorCode);
            return errorCode;
        }

        if (expirationTime < DateTime::Now())
        {
            // The certificate is expired, ignore it.
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "GetAccountNamesWithCertificateCommonName: CryptoUtility::GetCertificateExpiration for common name {0} is {1}, expired. Ignore certificate.",
                commonName,
                expirationTime);
        }
        else
        {
            wstring accountNameV2 = AccountHelper::GenerateUserAcountName(thumbprintObj, expirationTime);
            TraceInfo(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "GetAccountNamesWithCertificateCommonName: generate user account name {0} based on thumbprint {1} and expiration time {2}.",
                accountNameV2,
                thumbprint,
                expirationTime);
            accountNamesMap.insert(make_pair(move(accountNameV2), move(thumbprint)));
        }
    }

    return ErrorCode::Success();
}

void AccountHelper::ReplaceAccountInvalidChars(__inout std::wstring & accountName)
{
    // Account names invalid characters: " / \ [ ] : ; | = , + * ? < >
    // Base64 encoding returns digits, letters and /, + and =
    for (size_t i = 0; i < accountName.size(); ++i)
    {
        if (accountName[i] == L'/')
        {
            accountName[i] = L'!';
        }
        else if (accountName[i] == L'+')
        {
            accountName[i] = L'%';
        }
        else if (accountName[i] == L'=')
        {
            accountName[i] = L'.';
        }
    }
}

std::wstring AccountHelper::GenerateUserAcountName(
    Thumbprint::SPtr const & thumbprint,
    DateTime const & expirationTime)
{
    //
    // A thumbprint has 20 BYTES.
    // A user name is limited to 20 characters.
    // To avoid hitting the limit, take the first 90 bits of the thumbprint, as they provide low chance of collision.
    // To reduce the chance of collision even more,
    // append a shortened expiration time (4 characters).
    //

    BYTE const * hash = thumbprint->Hash();
    DWORD hashSizeInBytes = thumbprint->HashSizeInBytes();
    const unsigned int maxConvertByteCount = 12;
    ASSERT_IFNOT(hashSizeInBytes >= maxConvertByteCount, "GenerateUserAcountName: the certificate thumbprint has less than {0} bytes, can't create account name", maxConvertByteCount);
    
    // Encode the first 12 BYTEs to Base64, resulting in 16 chars string.
    wstring userName = CryptoUtility::BytesToBase64String(hash, maxConvertByteCount);
    
    // Compress the date time in 4 characters
    // Take the ticks for January 1, 2018
    int64 ticks = expirationTime.Ticks;
    int64 diff = ticks - ExpiryBaselineTicks;
    // If ticks is less than baseline, the certificate is expired, reject
    ASSERT_IF(
        diff < 0,
        "GenerateUserAcountName: the certificate expiration date {0} (ticks {1}) is less than baseline 1/1/2018 (ticks {2})",
        expirationTime,
        ticks,
        ExpiryBaselineTicks);

    int64 minutes = TimeSpan::FromTicks(diff).TotalRoundedMinutes();

    // Take the minutes and convert to ULONG (4 BYTES, 2^32), then discard the first BYTE.
    // The max value before overflow is 0xffffff (16777215).
    // January 1, 2018 + max allowed minutes will cover expiry dates less than 2049.
    // For expiration times greater than this value, the algorithm works but we have less precision.
    ULONG minSinceBaseline = static_cast<ULONG>(minutes);
    if (static_cast<int64>(minSinceBaseline) != minutes)
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "GenerateUserAcountName: overflow when converting minutes since baseline to ULONG: expiration time: {0}, baseline ticks {1}, use value {2} instead of {3}.",
            expirationTime,
            ExpiryBaselineTicks,
            minSinceBaseline,
            minutes);
    }

    // Check if first BYTE is not 0, and trace what we lose
    ULONG reducedMin = minSinceBaseline & 0xFFFFFF;
    if (reducedMin != minSinceBaseline)
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "GenerateUserAcountName: overflow when dropping the first BYTE of the ULONG minutes: expiration time: {0}, baseline ticks {1}, use value {2} instead of {3}.",
            expirationTime,
            ExpiryBaselineTicks,
            reducedMin,
            minSinceBaseline);
    }

    // Add the chars for the ticks since baseline, using only the last 3 BYTES.
    BYTE const* expiryBytes = reinterpret_cast<BYTE const*>(&minSinceBaseline);
    wstring expiryStr = CryptoUtility::BytesToBase64String(expiryBytes + 1, 3);
    userName.append(expiryStr);

    ReplaceAccountInvalidChars(userName);

    ASSERT_IFNOT(
        userName.size() == MaxCharactersInAccountUsername, 
        "GenerateUserAcountName: resulting name {0} doesn't have {1} characters, but {2}",
        userName,
        MaxCharactersInAccountUsername,
        userName.size());

    return userName;
}

// v1 version for generating account name based on a certificate, generating Unicode characters.
// Deprecated in favor of generation account name using the thumbprint and expiration date,
// which is more stable and has user readable characters.
std::wstring AccountHelper::GenerateUserAcountNameV1(Thumbprint::SPtr const & thumbprint)
{
    //
    // A thumbprint has 20 BYTES and when written to wstring it takes 40 chars.
    // A user name is limited to 20 characters.
    // To avoid hitting the limit, take the bytes inside the thumbprint in binary format.
    // Encode any 0 to prevent early string termination.
    // To encode, replace any 0 inside the string with 255.
    // To avoid collisions with other thumbprints where there is a 255 in the same place,
    // for each replaced 0 add another 255 at the end.
    //

    BYTE const * hash = thumbprint->Hash();
    DWORD hashSizeInBytes = thumbprint->HashSizeInBytes();

    // First, find out how many 0 entries we have in the hash, to know the final size of the BYTE *.
    DWORD bytesToReplace = 0;
    for (DWORD i = 0; i < hashSizeInBytes; ++i)
    {
        if(*(hash + i) == 0)
        {
            ++bytesToReplace;
        }
    }

    DWORD sizeInBytes = hashSizeInBytes + bytesToReplace;
    DWORD charSize = sizeof(wchar_t);
    DWORD charCount = sizeInBytes / charSize;
    DWORD charReminder = sizeInBytes % charSize;
    if(charReminder != 0)
    {
        // Round the size to be multiple of wchar_t size
        ++charCount;
        sizeInBytes += charSize - charReminder;
    }

    auto userNameData = new BYTE[sizeInBytes];

    if (bytesToReplace > 0)
    {
        static BYTE encodeByte = 255;

        memset(userNameData + hashSizeInBytes, encodeByte, bytesToReplace);
        memcpy(userNameData, hash, hashSizeInBytes);
        for (DWORD i = 0; i < hashSizeInBytes; ++i)
        {
            if (*(userNameData + i) == 0)
            {
                *(userNameData + i) = encodeByte;
            }
        }
    }
    else
    {
        memcpy(userNameData, hash, hashSizeInBytes);
    }

    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_AccountHelper,
        "GenerateUserAcountNameV1: generate account name from bits {0} after replacing {1} '0' bits.",
        userNameData,
        bytesToReplace);

    wstring userName(reinterpret_cast<wchar_t*>(userNameData), charCount);
    delete userNameData;

    return userName;
}

ErrorCode AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(
    wstring const& accountName,
    SecureString const & passwordSecret,
    X509StoreLocation::Enum storeLocation,
    wstring const & x509StoreName,
    wstring const & x509Thumbprint,
    __inout SecureString & password)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_AccountHelper,
        "GeneratePasswordForNTLMAuthenticatedUser: AccountName={0}, StoreLocation={1}, StoreName={2}, Thumbprint={3}",
        accountName,
        storeLocation,
        x509StoreName,
        x509Thumbprint);

    shared_ptr<X509FindValue> findValue;
    auto errorCode = X509FindValue::Create(X509FindType::FindByThumbprint, x509Thumbprint, findValue);
    if(!errorCode.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "The thumbprint is invalid. Thumbprint={0}, ErrorCode={1}",
            x509Thumbprint,
            errorCode);
        return errorCode;
    }

    CertContextUPtr certContext;
    errorCode = CryptoUtility::GetCertificate(
        storeLocation,
        x509StoreName,
        findValue,
        certContext);
    if(!errorCode.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "GetCertificate for {0} failed with {1}",
            x509Thumbprint,
            errorCode);
        return errorCode;
    }

    ByteBuffer signedMessageBuffer;
    errorCode = CryptoUtility::SignMessage(passwordSecret.GetPlaintext(), certContext, signedMessageBuffer);
    if(!errorCode.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "SignMessage failed with {0}. Thumbprint={1}",
            errorCode,
            x509Thumbprint);
        return errorCode;
    }

    ResourceHolder<vector<BYTE>> signedMessageBufferHolder(
        move(signedMessageBuffer),
        [](ResourceHolder<vector<BYTE>> * thisPtr)
    {
        SecureZeroMemory((void *)thisPtr->Value.data(), thisPtr->Value.size());
    });

#ifdef PLATFORM_UNIX

    ByteBuffer hashedSignedMsg;
    LinuxCryptUtil cryptoUtil;
    errorCode = cryptoUtil.ComputeHash(signedMessageBuffer, hashedSignedMsg);
    if(!errorCode.IsSuccess()) return errorCode;

    ByteBuffer derivedKey;
    errorCode = cryptoUtil.DeriveKey(hashedSignedMsg, derivedKey);
    if(!errorCode.IsSuccess()) return errorCode;

    wstring str = move(accountName + passwordSecret.GetPlaintext());
    auto dataInputForHmac = StringToByteBuffer(str);
    SecureZeroMemory((void*)str.c_str(), str.size() * sizeof(wchar_t));

    ByteBuffer hashedData;
    errorCode = cryptoUtil.ComputeHmac(dataInputForHmac, derivedKey, hashedData);
    SecureZeroMemory(dataInputForHmac.data(), dataInputForHmac.size());
    if (!errorCode.IsSuccess()) return errorCode;

#else

    HCRYPTPROV hCryptProv = NULL;
    if(!CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptAcquireContext failed with {0}",
            errorCode);
        return errorCode;
    }

    ResourceHolder<HCRYPTPROV> hCryptProvHolder(
        hCryptProv,
        [](ResourceHolder<HCRYPTPROV> * thisPtr)
    {
        CryptReleaseContext(thisPtr->Value, 0);
    });

    HCRYPTHASH hHash = NULL;
    if(!CryptCreateHash(hCryptProv, CALG_SHA_512, 0, 0, &hHash))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptCreateHash failed with {0}",
            errorCode);
        return errorCode;
    }

    ResourceHolder<HCRYPTHASH> hHashHolder(
        hHash,
        [](ResourceHolder<HCRYPTHASH> * thisPtr)
    {
        CryptDestroyHash(thisPtr->Value);
    });

    if(!CryptHashData(hHash, signedMessageBufferHolder.Value.data(), static_cast<DWORD>(signedMessageBufferHolder.Value.size()), 0))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptHashData failed with {0}",
            ErrorCode::FromWin32Error());
        return errorCode;
    }

    HCRYPTKEY hKey = NULL;
    if(!CryptDeriveKey(hCryptProv, CALG_AES_256, hHash, 0, &hKey))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptDeriveKey failed with {0}",
            errorCode);
        return errorCode;
    }

    ResourceHolder<HCRYPTKEY> hKeyHolder(
        hKey,
        [](ResourceHolder<HCRYPTKEY> * thisPtr)
    {
        CryptDestroyKey(thisPtr->Value);
    });

    HCRYPTHASH hHmacHash = NULL;
    if(!CryptCreateHash(hCryptProv, CALG_HMAC, hKey, 0, &hHmacHash))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptCreateHash failed with {0}",
            errorCode);
        return errorCode;
    }

    ResourceHolder<HCRYPTHASH> hHmacHashHolder(
        hHmacHash,
        [](ResourceHolder<HCRYPTHASH> * thisPtr)
    {
        CryptDestroyHash(thisPtr->Value);
    });

    HMAC_INFO hmacInfo;
    ::ZeroMemory(&hmacInfo, sizeof(hmacInfo));
    hmacInfo.HashAlgid = CALG_SHA_512;

    if(!CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&hmacInfo, 0))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptGetHashParam failed with {0}",
            errorCode);
        return errorCode;
    }

    wstring hashData = move(accountName + passwordSecret.GetPlaintext());

    auto result = CryptHashData(
        hHmacHash,
        (BYTE *)hashData.c_str(),
        (DWORD)hashData.size() * sizeof(WCHAR),
        0);
    SecureZeroMemory((void *)hashData.c_str(), hashData.size() * sizeof(WCHAR));

    if(!result)
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptHashData failed with {0}",
            errorCode);
        return errorCode;
    }

    DWORD dataSize = 0;
    DWORD dataSizeLength = sizeof(dataSize);
    if(!CryptGetHashParam(hHmacHash, HP_HASHSIZE, (BYTE *)&dataSize, &dataSizeLength, 0))
    {
        errorCode = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptGetHashParam failed with {0}",
            errorCode);
        return errorCode;
    }

    ASSERT_IF(dataSize == 0, "dataSize should be set to a positive value");
    vector<BYTE> hashedData(dataSize);
    if(!CryptGetHashParam(hHmacHash, HP_HASHVAL, hashedData.data(), &dataSize, 0))
    {
        Common::Assert::CodingError("Unable to retrieve hash value from the crypt framework, error: {0}", ErrorCode::FromWin32Error());
    }
#endif

    wstring value;
    StringWriter writer(value);
    writer.Write("{0}", PasswordPrefix);
    for(auto it = hashedData.begin(); it != hashedData.end(); ++it)
    {
        writer.Write("{0:x}", (*it)); //todo, length depends byte values, will this be an issue?
    }

    password = SecureString(move(value));

    return ErrorCodeValue::Success;
}

ErrorCode AccountHelper::GeneratePasswordForNTLMAuthenticatedUser(wstring const& accountName, SecureString const & passwordSecret, __inout SecureString & password)
{
#if defined(PLATFORM_UNIX)

    wstring str = move(accountName + passwordSecret.GetPlaintext());
    auto hashInput = StringToByteBuffer(str);
    SecureZeroMemory((void*)str.c_str(), str.size() * sizeof(wchar_t));

    ByteBuffer hashedData;
    auto errorCode = LinuxCryptUtil().ComputeHash(hashInput, hashedData);
    SecureZeroMemory(hashInput.data(), hashInput.size());
    if (!errorCode.IsSuccess()) return errorCode;

#else

    // TODO: improve password generation by using a secret
    HCRYPTPROV hCryptProv = NULL;
    HCRYPTHASH hHash = NULL;

    BOOL result = CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    KFinally([=] { if(hCryptProv) CryptReleaseContext(hCryptProv, 0); });
    if(result)
    {
        result = CryptCreateHash(hCryptProv, CALG_SHA_512, 0, 0, &hHash);
    }

    KFinally([=] { if (hHash) CryptDestroyHash(hHash); });

    if(result)
    {
        wstring hashData = move(accountName + passwordSecret.GetPlaintext());

        result = CryptHashData(hHash, (BYTE *)hashData.c_str(), (DWORD)hashData.size() * sizeof(WCHAR), 0);

        SecureZeroMemory((void *)hashData.c_str(), hashData.size() * sizeof(WCHAR));
    }

    DWORD dataSize = 0;
    DWORD dataSizeLength = sizeof(dataSize);
    if(result)
    {
        result = CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&dataSize, &dataSizeLength, 0);
    }

    vector<BYTE> hashedData(dataSize);
    if(result)
    {
        ASSERT_IF(dataSize == 0, "dataSize should be set to a positive value");
        result = CryptGetHashParam(hHash, HP_HASHVAL, hashedData.data(), &dataSize, 0);
        if(!result)
        {
            Common::Assert::CodingError("Unable to retrieve hash value from the crypt framework");
        }
    }
    else
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "Error generating password");
        return ErrorCodeValue::OperationFailed;
    }

#endif

    wstring value;
    StringWriter writer(value);
    writer.Write("{0}", PasswordPrefix);
    for(auto it = hashedData.begin(); it != hashedData.end(); ++it)
    {
        writer.Write("{0:x}", (*it));
    }
    password = SecureString(move(value));

    return ErrorCodeValue::Success;
}

#if !defined(PLATFORM_UNIX)
bool AccountHelper::GenerateRandomPassword(__out SecureString & password)
{
    HCRYPTPROV hCryptProv;
    BOOL result = CryptAcquireContext(&hCryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT);

    if(result)
    {
        BYTE arData[RandomDigitsInPassword];
        ::ZeroMemory(arData, RandomDigitsInPassword);
        // generating the password
        if(!::CryptGenRandom(hCryptProv, RandomDigitsInPassword, arData))
        {
            DWORD dwErr = ::GetLastError();
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_AccountHelper,
                "CryptGenRandom failed with {0}",
                dwErr);
            return false;
        }

        wstring value;
        StringWriter writer(value);
        writer.Write("{0}", PasswordPrefix);
        for(DWORD i = 0; i < RandomDigitsInPassword; ++i)
        {
            writer.Write("{0:x}", arData[i]);
        }

        password = SecureString(move(value));

        //clear out the memory - SecureZeroMemory clears the data working around compiler optimizations
        ::SecureZeroMemory(arData, RandomDigitsInPassword);
    }
    else
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_AccountHelper,
            "CryptAcquireContext failed with {0}",
            GetLastError());
        return false;
    }

    if(hCryptProv)
    {
        CryptReleaseContext(hCryptProv, 0);
    }

    return true;
}
#endif
