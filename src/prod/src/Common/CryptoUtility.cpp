// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define AZURE_SERVICE_FABRIC_KEY_CONTAINER_DEFAULT L"AzureServiceFabricDefault"
#define AZURE_SERVICE_FABRIC_CRYPT_RPOVIDER_TYPE PROV_RSA_FULL
#define BUFFER_SIZE 128
#define REGKEY32BUILD L"SOFTWARE\\Wow6432Node\\Microsoft\\StrongName\\Verification\\*,31bf3856ad364e35"
#define REGKEYBUILD L"SOFTWARE\\Microsoft\\StrongName\\Verification\\*,31bf3856ad364e35"
#define RANDOM_STRING_LENGTH 25

using namespace Common;
using namespace std;

namespace
{
    StringLiteral TraceType_CryptoUtility = "CryptoUtility";
    auto initTrigger = X509Default::StoreName();

    namespace CrlOffline
    {
        StringLiteral Trace = "CrlOfflineErrCache";

        INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;
        Global<shared_ptr<CrlOfflineErrCache>> defaultCache;

        BOOL InitFunction(PINIT_ONCE, PVOID, PVOID*)
        {
            defaultCache = make_global<shared_ptr<CrlOfflineErrCache>>();
            *defaultCache = make_shared<CrlOfflineErrCache>();
            return TRUE;
        }
    }

    using CertContextMap = multimap<DateTime, CertContextUPtr>;

    void CertContextMapToList(CertContextMap && contextMap, CertContexts & contextList) // list is sorted by cert expiration in descending order
    {
        contextList.clear();

        for (auto iter = contextMap.rbegin(); iter != contextMap.rend(); ++iter)
        {
            contextList.emplace_back(move(iter->second));
        }

        contextMap.clear();
    }
}

enum FABRIC_X509_STORE_LOCATION X509Default::StoreLocation_Public()
{
    return X509StoreLocation::ToPublic(StoreLocation()); 
}

X509StoreLocation::Enum X509Default::StoreLocation()
{
    return X509StoreLocation::LocalMachine;
}

CrlOfflineErrCache::ErrorRecord::ErrorRecord(CertContext const * cert, uint certChainFlagsUsed, StopwatchTime time)
    : certChainFlagsUsed_(certChainFlagsUsed)
    , failureTime_(time)
#ifndef PLATFORM_UNIX
    , certContext_(CertDuplicateCertificateContext(cert), [] (CertContext const* cert) { CertFreeCertificateContext(cert); })
#endif
{
}

CrlOfflineErrCache& CrlOfflineErrCache::GetDefault()
{
    Invariant(::InitOnceExecuteOnce(
        &CrlOffline::initOnce,
        CrlOffline::InitFunction,
        nullptr,
        nullptr));

    return *(*CrlOffline::defaultCache);
}

CrlOfflineErrCache::CrlOfflineErrCache()
    : testStartTime_(Stopwatch::Now() + SecurityConfig::GetConfig().CrlTestStartDelay)
    , testEndTime_(testStartTime_ + SecurityConfig::GetConfig().CrlTestPeriod)
{
}

void CrlOfflineErrCache::PurgeExpired_LockHeld(StopwatchTime now)
{
#ifdef PLATFORM_UNIX
    for(auto iter = cache_.cbegin(); iter != cache_.cend();)
    {
        if (!(iter->second.certContext_) /* should recheck */ &&
            ((now - iter->second.failureTime_) >= SecurityConfig::GetConfig().CrlDisablePeriod) &&
            ((now - iter->second.failureTime_) >= SecurityConfig::GetConfig().CrlOfflineHealthReportTtl))
        {
            Trace.WriteInfo(CrlOffline::Trace, "remove {0}, {1}", iter->first, iter->second.failureTime_);
            iter = cache_.erase(iter);
            continue;
        }

        ++iter;
    }
#else
    now; // It's RecheckCrl's job to remove
#endif
}

void CrlOfflineErrCache::SetHealthReportCallback(HealthReportCallback const & callback)
{
    AcquireWriteLock grab(lock_);

    if (healthReportCallback_)
    {
        Trace.WriteNoise(CrlOffline::Trace, "healthReportCallback_ already set");
        return;
    }

    healthReportCallback_ = callback;
    Trace.WriteInfo(CrlOffline::Trace, "healthReportCallback_ set");
}

void CrlOfflineErrCache::ReportHealth_LockHeld(size_t errCount, std::wstring const & description)
{
    if (!healthReportCallback_)
    {
        Trace.WriteInfo(CrlOffline::Trace, "ReportHealth: null callback");
        return;
    }

    Trace.WriteInfo(
        CrlOffline::Trace,
        "ReportHealth: posting report to thread pool, errCount = {0}, description = '{1}'",
        errCount, description);
    Threadpool::Post([callback = healthReportCallback_, errCount, description] { callback(errCount, description); });
}

void CrlOfflineErrCache::GetHealthReport_LockHeld(size_t & cacheSize, std::wstring & description)
{
    cacheSize = cache_.size();
    description.clear();
    if (cacheSize == 0) return;

    description.clear();
    StringWriter sw(description);
    sw.Write("CRL offline encountered for certificates: ");
    uint count = 0;

    for(auto const & e : cache_)
    {
        if (count > 0)
        {
            sw.Write(", ");
        }

        sw.Write("{0}", e.first);
        if (++count > 30)
        {
            sw.Write(" ...");
            break;
        }
    }

    sw.WriteLine();
    sw.Write("Please ensure the reporting machine has access to 'CRL Distribution Point' at ALL levels in the certificate chain. 'CRL Distribution Point' is an extension in the certificate.");
}

void CrlOfflineErrCache::TryRemoveError(Thumbprint const & cert, uint certChainFlagsUsed)
{
    auto enabled = CrlCheckingEnabled(certChainFlagsUsed);
    Trace.WriteTrace(
        enabled? LogLevel::Info : LogLevel::Noise,
        CrlOffline::Trace,
        "remove {0}, certChainFlagsUsed = {1:x}, CrlCheckingEnabled = {2}",
        cert, certChainFlagsUsed, enabled); 

    if (!enabled) return;

    AcquireWriteLock grab(lock_);

    if (cache_.erase(cert) && cache_.empty())
    {
        ReportHealth_LockHeld(0, wstring());
    }
}

bool CrlOfflineErrCache::CrlCheckingEnabled(uint certChainFlags)
{
    //CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY is unrelated to CRL offline error
    return
        (certChainFlags & CERT_CHAIN_REVOCATION_CHECK_END_CERT) ||
        (certChainFlags & CERT_CHAIN_REVOCATION_CHECK_CHAIN) ||
        (certChainFlags & CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT);
}

void CrlOfflineErrCache::Update(Thumbprint const & cert, CertContext const * certContext, uint certChainFlagsUsed, bool onlyCrlOfflineEncountered, bool matched)
{
    if (onlyCrlOfflineEncountered)
    {
        if (matched)
        {
            // We only care about CRL offline error for certificates in our allow list
            CrlOfflineErrCache::GetDefault().AddError(cert, certContext, certChainFlagsUsed);
        }
        return;
    }

    CrlOfflineErrCache::GetDefault().TryRemoveError(cert, certChainFlagsUsed);
}

void CrlOfflineErrCache::AddError(Thumbprint const & cert, CertContext const* certContext, uint certChainFlagsUsed)
{
    auto enabled = CrlCheckingEnabled(certChainFlagsUsed);
    Trace.WriteInfo(
        CrlOffline::Trace,
        "add {0}, certChainFlagsUsed = {1:x}, CrlCheckingEnabled = {2}",
        cert, certChainFlagsUsed, enabled); 

    Invariant(enabled);

    size_t cacheSize = 0;
    wstring report;
    bool shouldReport = true;
    bool shouldScheduleCleanup = false;
    auto& config = SecurityConfig::GetConfig();
    {
        AcquireWriteLock grab(lock_);

        auto now = Stopwatch::Now(); //get current time under lock to ensure failureTime_ never goes back
        {
            auto iter = cache_.find(cert);
            if (iter == cache_.end())
            {
                cache_.emplace(make_pair(cert, ErrorRecord(certContext, certChainFlagsUsed, now))); 
            }
            else
            {
                iter->second.certChainFlagsUsed_ = certChainFlagsUsed;
                Invariant(now >= iter->second.failureTime_);
                //throttle reporting?
                shouldReport = (now - iter->second.failureTime_) >= min(config.CrlOfflineReportThreshold, config.CrlDisablePeriod);
                iter->second.failureTime_ = now; 
            }
        }

        if (cache_.size() > config.CrlOfflineErrCacheCapacity)
        {
            PurgeExpired_LockHeld(now);

            if (cache_.size() > config.CrlOfflineErrCacheCapacity)
            {
                shouldReport = (Random().NextDouble() < 0.1); //throttle on reporting

                auto oldest = cache_.cbegin();
                auto it = ++(cache_.cbegin());
                for(; it != cache_.cend(); ++ it)
                {
                    if (it->second.failureTime_ < oldest->second.failureTime_)
                    {
                        oldest = it;
                    }
                }

                Trace.WriteInfo(CrlOffline::Trace, "remove oldest {0}", oldest->first);
                cache_.erase(oldest);
            }
        }

        if (shouldReport)
        {
            GetHealthReport_LockHeld(cacheSize, report);
            ReportHealth_LockHeld(cacheSize, report);
        }

        if (!cleanupActive_)
        {
            shouldScheduleCleanup = true; 
            cleanupActive_ = true;
        }
    }

    if (shouldScheduleCleanup)
    {
        ScheduleCleanup();
    }
}

void CrlOfflineErrCache::ScheduleCleanup()
{
    // Schedule cleanup ahead of expiry, in hope that CrlRecheck will successfully retrieve CRL
    // before next "real" check in actual certificate verification for secure session setup 
    auto delay = TimeSpan::FromMilliseconds(
        SecurityConfig::GetConfig().CrlDisablePeriod.TotalPositiveMilliseconds() * 2.0 / 3.0);

    Trace.WriteInfo(CrlOffline::Trace, "schedule cleanup in {0}", delay);
    weak_ptr<CrlOfflineErrCache> thisWPtr = shared_from_this();
    Threadpool::Post(
        [thisWPtr]
        {
            if (auto thisSPtr = thisWPtr.lock())
            {
                thisSPtr->CleanUp();
            }
        },
        delay);
}

void CrlOfflineErrCache::CleanUp()
{
    Trace.WriteInfo(CrlOffline::Trace, "Cleanup begins");
    KFinally([] { Trace.WriteInfo(CrlOffline::Trace, "Cleanup ends"); });

    CacheMap cacheCopy;
    auto now = Stopwatch::Now();
    {
        AcquireWriteLock grab(lock_);

        if (cache_.empty())
        {
            // no need to report, should have been reported by TryRemoveError
            cleanupActive_ = false;
            return;
        }

        PurgeExpired_LockHeld(now);

        if (cache_.empty())
        {
            cleanupActive_ = false;
            ReportHealth_LockHeld(0, wstring());
            return;
        }

        cacheCopy = cache_;
    }

    for(auto const & e : cacheCopy)
    {
        RecheckCrl(e.first, e.second);
    }

    auto shouldScheduleCleanup = false;
    {
        AcquireWriteLock grab(lock_);

        cleanupActive_ = !cache_.empty();
        shouldScheduleCleanup = cleanupActive_;
    }

    if (shouldScheduleCleanup)
    {
        ScheduleCleanup();
    }
}

void CrlOfflineErrCache::RecheckCrl(Thumbprint const & thumbprint, ErrorRecord const & errorRecord)
{
    Invariant(CrlCheckingEnabled(errorRecord.certChainFlagsUsed_));

    auto certContext = errorRecord.certContext_.get();
    auto& config = SecurityConfig::GetConfig();
    if (config.CrlTestEnabled)
    {
        auto now = Stopwatch::Now();
        Trace.WriteInfo(CrlOffline::Trace, "RecheckCrl: now = {0}, testEndTime_ = {1}", now, testEndTime_);
        if ((testStartTime_ <= now) && (now <= testEndTime_))
        {
            if (config.CrlOfflineProbability <= Random().NextDouble())
            {
                TryRemoveError(thumbprint, errorRecord.certChainFlagsUsed_);
                return;
            }

            AddError(thumbprint, certContext, errorRecord.certChainFlagsUsed_);
            return;
        }
    }

#ifdef PLATFORM_UNIX

    return; // LINUXTODO implement CRL recheck and remove PurgeExpired_LockHeld

#else

    CERT_CHAIN_PARA chainPara = {};
    chainPara.cbSize = sizeof(CERT_CHAIN_PARA);

    PCCERT_CHAIN_CONTEXT certChainContextPtr = nullptr;
    if(!::CertGetCertificateChain(
        nullptr,
        certContext,
        nullptr,
        certContext->hCertStore,
        &chainPara,
        errorRecord.certChainFlagsUsed_ | CERT_CHAIN_CACHE_END_CERT | CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT,
        nullptr,
        &certChainContextPtr))
    {
        DWORD lastError = ::GetLastError();
        Trace.WriteWarning(CrlOffline::Trace, "RecheckCrl: CertGetCertificateChain failed: 0x{0:x}", lastError);
        return;
    }

    CertChainContextUPtr certChainContext(certChainContextPtr);
    Trace.WriteInfo(
        CrlOffline::Trace,
        "RecheckCrl: cert chain trust status: info = {0:x}, error = {1:x}",
        certChainContext->TrustStatus.dwInfoStatus,
        certChainContext->TrustStatus.dwErrorStatus);

    //client-vs-server key usage does not matter in checking CRL offline 
    auto status = (certChainContext->TrustStatus.dwErrorStatus) & (~CERT_TRUST_IS_NOT_VALID_FOR_USAGE);
    if (status == CERT_TRUST_IS_OFFLINE_REVOCATION)
    {
        // Hit CRL offline again, no other error encountered
        AddError(thumbprint, certContext, errorRecord.certChainFlagsUsed_);
        return;
    }

    // CRL recovered or other error encountered
    TryRemoveError(thumbprint, errorRecord.certChainFlagsUsed_);

#endif
}

uint CrlOfflineErrCache::MaskX509CertChainFlags(Thumbprint const & cert, uint flags, bool shouldIgnoreCrlOffline) const
{
    if (!shouldIgnoreCrlOffline)
    {
        return flags;
    }

    AcquireReadLock grab(lock_);

    auto iter = cache_.find(cert);
    if (iter == cache_.cend()) return flags;

    if ((Stopwatch::Now() - iter->second.failureTime_) >= SecurityConfig::GetConfig().CrlDisablePeriod)
    {
        return flags;
    }

    auto maskedFlags = flags &
        (~CERT_CHAIN_REVOCATION_CHECK_END_CERT) &
        (~CERT_CHAIN_REVOCATION_CHECK_CHAIN) &
        (~CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT);

    // just in case CRL was retrieved successfully by other processes somehow
    maskedFlags |= CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    //Trace.WriteNoise(CrlOffline::Trace, "input flags = {0:x}, masked = {1:x}", flags, maskedFlags); 
    return maskedFlags;
}

void CrlOfflineErrCache::Test_CheckCrl(uint certChainFlags, uint & errorStatus)
{
    if (errorStatus || !CrlCheckingEnabled(certChainFlags)) return;

    auto& config = SecurityConfig::GetConfig();
    if (!config.CrlTestEnabled) return;

    auto now = Stopwatch::Now();
    Trace.WriteInfo(CrlOffline::Trace, "Test_CheckCrl: now = {0}, testStartTime_ ={1}, testEndTime_ = {2}", now, testStartTime_, testEndTime_);

    if ((now < testStartTime_) || (testEndTime_ < now))
    {
        Trace.WriteInfo(CrlOffline::Trace, "Test_CheckCrl: not in CRL test time range");
        return;
    }

    if (Random().NextDouble() < config.CrlOfflineProbability)
    {
#ifdef PLATFORM_UNIX
        errorStatus = X509_V_ERR_UNABLE_TO_GET_CRL;
#else
        errorStatus = (uint)CRYPT_E_REVOCATION_OFFLINE; 
#endif
    }
}

CrlOfflineErrCache::CacheMap CrlOfflineErrCache::Test_GetCacheCopy() const
{
    AcquireReadLock grab(lock_);
    return cache_;
}

void CrlOfflineErrCache::Test_Reset()
{
    AcquireWriteLock grab(lock_);

    cache_.clear();
    healthReportCallback_ = nullptr;
}

#ifdef PLATFORM_UNIX

wstring const & X509Default::StoreName()
{
    static auto defaultStoreName = make_global<wstring>(L""); // SecurityConfig::X509Folder will be used
    return *defaultStoreName;
}

#else

wstring const & X509Default::StoreName()
{
    static auto defaultStoreName = make_global<wstring>(L"My");
    return *defaultStoreName;
}

#endif

const string CryptoUtility::BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const string TestSignedSubstring = "testsigningYes";
const string BcdeditCommand = "bcdedit.exe";
const string AccessDeniedString = "Accessisdenied";

Common::ErrorCode CryptoUtility::SignMessage(
    wstring const & message,
    CertContextUPtr const & certContext,
    _Out_ ByteBuffer & signedMessage)
{
    return SignMessage(
        (BYTE const*)message.c_str(),
        message.size() * sizeof(wchar_t),
        szOID_RSA_SHA1RSA,
        certContext,
        signedMessage);
}

ErrorCode CryptoUtility::SignMessage(
    BYTE const* inputBuffer,
    size_t inputSize,
    char const* algOID,
    CertContextUPtr const & certContext,
    ByteBuffer & signedMessage)
{
    if (inputSize > std::numeric_limits<uint>::max())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "SignMessage: inputSize is beyond uint range: {0}",
            inputSize);

        return ErrorCodeValue::InvalidArgument;
    }

    auto inputSizeUInt = (uint)inputSize;

#if defined(PLATFORM_UNIX)
    return LinuxCryptUtil().Pkcs7Sign(certContext, inputBuffer, inputSize, signedMessage);
#else
    // Initialize the signature structure.
    auto pCertContext = certContext.get();
    CRYPT_SIGN_MESSAGE_PARA  signParameters;    
    signParameters.cbSize = sizeof(CRYPT_SIGN_MESSAGE_PARA);
    signParameters.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    signParameters.pSigningCert = pCertContext;
    signParameters.HashAlgorithm.pszObjId = (LPSTR)algOID;
    signParameters.HashAlgorithm.Parameters.cbData = NULL;
    signParameters.cMsgCert = 1;
    signParameters.rgpMsgCert = &pCertContext;
    signParameters.cAuthAttr = 0;
    signParameters.dwInnerContentType = 0;
    signParameters.cMsgCrl = 0;
    signParameters.cUnauthAttr = 0;
    signParameters.dwFlags = 0;
    signParameters.pvHashAuxInfo = NULL;
    signParameters.rgAuthAttr = NULL;

    const BYTE* messageArray[] = { inputBuffer };
    DWORD messageSizeArray[1];
    messageSizeArray[0] = inputSizeUInt;

    // First, get the size of the signed BLOB.
    DWORD signedMessageSize;
    if(!CryptSignMessage(
        &signParameters,
        FALSE,
        1,
        messageArray,
        messageSizeArray,
        NULL,
        &signedMessageSize))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptSignMessage to get size failed. Error:{0}",
            error);

        return error;
    }

    signedMessage.resize(signedMessageSize);

    if(!CryptSignMessage(
        &signParameters,
        FALSE,
        1,
        messageArray,
        messageSizeArray,
        signedMessage.data(),
        &signedMessageSize))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptSignMessage failed. Error:{0}",
            error);

        return error;
    }

    return ErrorCodeValue::Success;
#endif
}

#ifndef PLATFORM_UNIX

ErrorCode CryptoUtility::GetCertificatePrivateKeyFileName(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    shared_ptr<X509FindValue> const & findValue,
    wstring & privateKeyFileName)
{
    PLATFORM_VALIDATE(certificateStoreLocation)

        CertContextUPtr certContext;
	    ErrorCode error = GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findValue,
        certContext);

    if (!error.IsSuccess())
    {
        return error;
    }

    return CryptoUtility::GetCertificatePrivateKeyFileName(certContext, privateKeyFileName);
}

ErrorCode CryptoUtility::GetCertificatePrivateKeyFileName(
    X509StoreLocation::Enum certificateStoreLocation,
    std::wstring const & certificateStoreName,
    std::shared_ptr<X509FindValue> const & findValue,
    std::vector<std::wstring> & privateKeyFileNames)
{
    PLATFORM_VALIDATE(certificateStoreLocation)

    CertContexts certContexts;
    ErrorCode error = GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findValue,
        certContexts);

    if (!error.IsSuccess())
    {
        return error;
    }

	bool succeeded = false;
	if (certContexts.empty())
	{
		succeeded = true;
	}
	else
	{
		for (auto certContextIter = certContexts.begin(); certContextIter != certContexts.end(); ++certContextIter)
		{
			wstring  privateKeyFileName;
			error = CryptoUtility::GetCertificatePrivateKeyFileName(*certContextIter, privateKeyFileName);
			if (!error.IsSuccess())
			{
				TraceWarning(
					TraceTaskCodes::Common,
					TraceType_CryptoUtility,
					"Can't get private key filename for certificate. Error: {0}",
					error);
			}
			else
			{
				succeeded = true;
				privateKeyFileNames.push_back(privateKeyFileName);
			}
		}
	}

	if (!succeeded)
	{
		TraceWarning(
			TraceTaskCodes::Common,
			TraceType_CryptoUtility,
			"All tries to get private key filename failed.");
	}

	return succeeded ? error.Success() : ErrorCode::FromHResult(E_FAIL);
}

ErrorCode CryptoUtility::GetCertificatePrivateKeyFileName(CertContextUPtr const & certContext, _Out_ wstring & privateKeyFileName)
{
    ErrorCode error = ErrorCode::Success();

    HCRYPTPROV hCryptProv;
    DWORD keySpec;
    BOOL shouldFreeProv;

    if (!CryptAcquireCertificatePrivateKey(
        certContext.get(),
        CRYPT_ACQUIRE_COMPARE_KEY_FLAG | CRYPT_ACQUIRE_CACHE_FLAG,
        NULL,
        &hCryptProv,
        &keySpec,
        &shouldFreeProv))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptAcquireCertificatePrivateKey failed. Error:{0}",
            error);

        return error;
    }

    DWORD fileNameSize;
    if (!CryptGetProvParam(
        hCryptProv,
        PP_UNIQUE_CONTAINER,
        NULL,
        &fileNameSize,
        NULL))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptGetProvParam failed while getting data size. Error:{0}",
            error);

        return error;
    }

    ByteBuffer fileNameByteBuffer(fileNameSize);
    if (!CryptGetProvParam(
        hCryptProv,
        PP_UNIQUE_CONTAINER,
        fileNameByteBuffer.data(),
        &fileNameSize,
        0))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptGetProvParam failed. Error:{0}",
            error);

        return error;
    }

    //fileNameSize counts null terminator
    vector<wchar_t> fileNameStringBuffer(fileNameSize);
    size_t converted = 0;
    auto retval = mbstowcs_s(&converted, fileNameStringBuffer.data(), fileNameSize, (char *)fileNameByteBuffer.data(), fileNameSize);
    TextTraceComponent<TraceTaskCodes::Common>::WriteTrace(
        retval ? LogLevel::Error : LogLevel::Noise,
        TraceType_CryptoUtility,
        "mbstowcs_s: fileNameSize = {0}, fileNameByteBuffer = {1}, converted = {2}, retval = {3}",
        fileNameSize,
        fileNameByteBuffer,
        converted,
        retval);

    if (retval)
    {
        error = ErrorCode::FromWin32Error(retval);
        return error;
    }

    wstring fileName = fileNameStringBuffer.data();

    vector<wstring> matchingFiles;

    wstring commonAppDataKeyStore;
    error = GetKnownFolderName(CSIDL_COMMON_APPDATA, L"Microsoft\\Crypto\\RSA\\MachineKeys", commonAppDataKeyStore);
    if (error.IsSuccess())
    {
        matchingFiles = Directory::Find(commonAppDataKeyStore, fileName, 1, true);
    }

    if (matchingFiles.empty())
    {
        wstring appDataKeyStore;
        error = GetKnownFolderName(CSIDL_APPDATA, L"Microsoft\\Crypto\\RSA", appDataKeyStore);
        if (error.IsSuccess())
        {
            matchingFiles = Directory::Find(appDataKeyStore, fileName, 1, true);
        }
        if (matchingFiles.empty())
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "The private key {0} could not be found in {1} and {2}.",
                fileName,
                commonAppDataKeyStore,
                appDataKeyStore);

            return ErrorCode(ErrorCodeValue::FileNotFound);
        }
    }

    privateKeyFileName = matchingFiles[0];
    return ErrorCode(ErrorCodeValue::Success);
}

#endif

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificateExpiration(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    wstring const & findType,
    wstring const & findValue,
    DateTime & expiration)
{
    expiration = DateTime::Zero;
    CertContextUPtr cert;
    auto error = GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findType,
        findValue,
        cert);

    if (!error.IsSuccess())
    {
        return error;
    }

    return GetCertificateExpiration(cert, expiration);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificateExpiration(
    CertContextUPtr const & cert,
    DateTime & expiration)
{
    expiration = DateTime::Zero;
    
#if defined(PLATFORM_UNIX)
    return LinuxCryptUtil().GetCertificateNotAfterDate((X509*)cert.get(), expiration);
#else
    expiration = DateTime(cert->pCertInfo->NotAfter);
    return ErrorCode::Success();
#endif
}

_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificateExpiration(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    shared_ptr<X509FindValue> const & findValue,
    DateTime & expiration)
{
    expiration = DateTime::Zero;
    CertContextUPtr cert;
    auto error = GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findValue,
        cert);

    if (!error.IsSuccess())
    {
        return error;
    }

    return GetCertificateExpiration(cert, expiration);
}

_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificate(
X509StoreLocation::Enum certificateStoreLocation,
wstring const & certificateStoreName,
wstring const & findType,
wstring const & findValue,
CertContextUPtr & certContext)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "GetCertificate({0}, {1}, {2}, {3})",
        certificateStoreLocation,
        certificateStoreName,
        findType,
        findValue);

    certContext.reset();

    X509FindType::Enum findTypeEnum;
    auto error = X509FindType::Parse(findType, findTypeEnum);
    if (!error.IsSuccess()) return error;

    X509FindValue::SPtr findValueSPtr;
    error = X509FindValue::Create(findTypeEnum, findValue, findValueSPtr);
    if (!error.IsSuccess()) return error;

    return GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findValueSPtr,
        certContext);
}

_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificate(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    wstring const & findType,
    wstring const & findValue,
    CertContexts & certContexts)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "GetCertificate({0}, {1}, {2}, {3})",
        certificateStoreLocation,
        certificateStoreName,
        findType,
        findValue);

    X509FindType::Enum findTypeEnum;
    auto error = X509FindType::Parse(findType, findTypeEnum);
    if (!error.IsSuccess()) return error;

    X509FindValue::SPtr findValueSPtr;
    error = X509FindValue::Create(findTypeEnum, findValue, findValueSPtr);
    if (!error.IsSuccess()) return error;

    return GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findValueSPtr,
        certContexts);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificate(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    shared_ptr<X509FindValue> const & findValue,
    CertContextUPtr & certContext)
{
    return GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        CERT_STORE_READONLY_FLAG,
        findValue,
        certContext);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificate(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    shared_ptr<X509FindValue> const & findValue,
    CertContexts & certContexts)
{
    return GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        CERT_STORE_READONLY_FLAG,
        findValue,
        certContexts);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificate(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    DWORD storeOpenFlag,
    shared_ptr<X509FindValue> const & findValue,
    CertContextUPtr & certContext)
{
    CertContexts certContexts;
    auto error = GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        storeOpenFlag,
        findValue,
        certContexts);

    certContext = certContexts.empty() ? nullptr : move(certContexts.front());
    return error;
}

/// Unix implementation
#if defined(PLATFORM_UNIX)

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificate(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    DWORD storeOpenFlag,
    shared_ptr<X509FindValue> const & findValue,
    CertContexts & certContexts)
{
    certContexts.clear();
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "GetCertificate({0}, {1}, {2})",
        certificateStoreLocation,
        certificateStoreName,
        findValue);

    PLATFORM_VALIDATE(certificateStoreLocation)

        std::vector<std::string> allCerts;
    ErrorCode error = GetAllCertificates(
        certificateStoreLocation,
        certificateStoreName,
        storeOpenFlag,
        allCerts);

    // store certs in order of it's expiration date
    CertContextMap contextMap;

    // try create and insert certContexts for each file
    LinuxCryptUtil cryptUtil;
    for (auto const & certFile : allCerts)
    {
        CertContextUPtr nextCert = cryptUtil.LoadCertificate(certFile);
        if (nextCert.get() == NULL)
        {
            continue;
        }

        auto x = (CertContext *)nextCert.get();
        if (IsMatch((PCCertContext)x, findValue))
        {
            DateTime expirationDate;
            cryptUtil.GetCertificateNotAfterDate(x, expirationDate);

            contextMap.emplace(pair<DateTime, CertContextUPtr>(expirationDate, move(nextCert)));
        }
    }

    CertContextMapToList(move(contextMap), certContexts);

    if (certContexts.empty()) { return ErrorCodeValue::CertificateNotFound; }
    return ErrorCodeValue::Success;
}

_Use_decl_annotations_ ErrorCode CryptoUtility::GetAllCertificates(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certStorePath,
    DWORD storeOpenFlag,
    std::vector<std::string> & certFiles)
{
    if (StringUtility::AreEqualCaseInsensitive(certStorePath, L"My"))
    {
        WriteInfo(
            TraceType_CryptoUtility,
            "certificate file/folder path '{0}' is treated as empty path on PLATFORM_UNIX",
            certStorePath);

        certFiles = LinuxCryptUtil().GetCertFiles(L"");
        return ErrorCodeValue::Success;
    }

    certFiles = LinuxCryptUtil().GetCertFiles(certStorePath);
    return ErrorCodeValue::Success;
}

_Use_decl_annotations_ bool CryptoUtility::IsMatch(
    PCCertContext pcertContext,
    shared_ptr<X509FindValue> const & findValue)
{
    if(pcertContext == NULL) {return false;}

    if (!findValue) return true; // match all

    switch (findValue->Type())
    {
        // Thumbprint
        case X509FindType::FindByThumbprint:
        {
            Thumbprint const & queryThumbprint = (Thumbprint const &)(*findValue);
            Thumbprint newMatchThumbprint;
            ErrorCode error = newMatchThumbprint.Initialize(pcertContext);
            if (!error.IsSuccess()) {return false;}

            return queryThumbprint.PrimaryValueEqualsTo(newMatchThumbprint);
        }

        //SubjectName
        case X509FindType::FindBySubjectName:
        {
            SubjectName const & querySubjectName = (SubjectName const &)(*findValue);

            std::string sn = LinuxCryptUtil().GetSubjectName(pcertContext);
            wstring wsn;
            StringUtility::Utf8ToUtf16(sn, wsn);
            SubjectName newMatchSubjectName(wsn);
            auto error = newMatchSubjectName.Initialize();
            if (!error.IsSuccess()) {return false;}

            return querySubjectName.PrimaryValueEqualsTo(newMatchSubjectName);
        }

        // CommonName
        case X509FindType::FindByCommonName:
        {
            CommonName const & queryCommonName = (CommonName const &)(*findValue);

            std::string cn;
            ErrorCode error =  LinuxCryptUtil().GetCommonName((X509*)pcertContext, cn);
            if (!error.IsSuccess()) {return false;}
        
            std::wstring wcn(cn.begin(), cn.end());
            CommonName newMatchCommonName(wcn);
            return queryCommonName.PrimaryValueEqualsTo(newMatchCommonName);
        }

        case X509FindType::FindByExtension:
        {
            // Check if the findValue represents supported extension. Currently only SubjectAltName is supported extension.
            X509FindSubjectAltName* subjectAltName = dynamic_cast<X509FindSubjectAltName*>(findValue.get());
            if (!subjectAltName)
            {
                return false;
            }
            vector<string> names = LinuxCryptUtil().GetSubjectAltName(pcertContext);
            for (int i = 0; i < names.size(); ++i) {
                wstring wname;
                StringUtility::Utf8ToUtf16(names[i], wname);
                LPWSTR dnsName = ((CERT_ALT_NAME_ENTRY*)subjectAltName->Value())->pwszDNSName;
                if(StringUtility::AreEqualCaseInsensitive(wname, dnsName)) {
                    return true;
                }
            }
            return false;
        }

        default:
        {
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "IsMatch(): Unknown FindType '{0}'.",
                findValue->Type());
            return false;
        }
    }
}

/// Windows implementation
#else
_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificate(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    DWORD storeOpenFlag,
    shared_ptr<X509FindValue> const & findValue,
    CertContexts & certContexts)
{
    certContexts.clear();
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "GetCertificate({0}, {1}, {2})",
        certificateStoreLocation,
        certificateStoreName,
        findValue);

    PLATFORM_VALIDATE(certificateStoreLocation)

    CertStoreUPtr certStore(CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        0,
        NULL,
        certificateStoreLocation | storeOpenFlag,
        certificateStoreName.c_str()));

    ErrorCode error;
    if (!certStore.get())
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Failed to open store '{0}' at {1}: {2}",
            certificateStoreName,
            certificateStoreLocation,
            error);

        return error;
    }

    // Find first match
    auto certContext = CertContextUPtr(GetCertificateFromStore(certStore.get(), findValue, nullptr));

    if (certContext)
    {
        DateTime matchExpiration = DateTime(certContext->pCertInfo->NotAfter);
        Thumbprint thumbprint;
        error = thumbprint.Initialize(certContext.get());
        if (!error.IsSuccess()) return error;

        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "GetCertificate: match found: thumbprint = {0}, expiration = {1}",
            thumbprint,
            DateTime(certContext->pCertInfo->NotAfter));

        CertContextUPtr certContextDup;
        if (findValue == NULL || findValue->Type() != X509FindType::FindByThumbprint)
        {
            // Need to duplicate as certContext will be moved to contextMap
            certContextDup.reset(CertDuplicateCertificateContext(certContext.get()));
        }

        CertContextMap contextMap;
        contextMap.emplace(pair<DateTime, CertContextUPtr>(matchExpiration, move(certContext)));

        if (findValue == NULL || findValue->Type() != X509FindType::FindByThumbprint)
        {
            // There may be multiple matches.
            // Release to avoid double free, as CertFindCertificateInStore frees pPrevCertContext
            Invariant(certContextDup);
            PCCertContext contextPtr = certContextDup.release();
            while ((contextPtr = GetCertificateFromStore(certStore.get(), findValue, contextPtr)) != nullptr)
            {
                DateTime newMatchExpiration = DateTime(contextPtr->pCertInfo->NotAfter);
                shared_ptr<Thumbprint> newMatchThumbprint = make_shared<Thumbprint>();
                error = newMatchThumbprint->Initialize(contextPtr);
                if (!error.IsSuccess())
                {
                    return error;
                }

                TraceInfo(
                    TraceTaskCodes::Common,
                    TraceType_CryptoUtility,
                    "GetCertificate: match found: thumbprint = {0}, expiration = {1}",
                    newMatchThumbprint,
                    newMatchExpiration);

                CertContextUPtr newMatch = CertContextUPtr(::CertDuplicateCertificateContext(contextPtr));
                contextMap.emplace(pair<DateTime, CertContextUPtr>(newMatchExpiration, move(newMatch)));
            }
        }

        CertContextMapToList(move(contextMap), certContexts);
    }
    else
    {        
        auto winError = ErrorCode::FromWin32Error();
        error.Overwrite(ErrorCodeValue::CertificateNotFound);
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Failed to find certificate matching {0} in store {1} at {2}: {3}. Returning {4}",
            findValue,
            certificateStoreName,
            certificateStoreLocation,
            winError,
            error);
    }

    if (certContexts.empty() && findValue != NULL && findValue->Type() == X509FindType::FindBySubjectName)
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "GetCertificate: failed to search with {0}, will retry as common name",
            findValue);

        SubjectName const & subjectNameFindValue = (SubjectName const &)(*findValue);
        CommonName::SPtr commonNameFindValue;
        if (CommonName::Create(subjectNameFindValue.Name(), commonNameFindValue).IsSuccess())
        {
            error.Overwrite(
                GetCertificate(
                    certificateStoreLocation,
                    certificateStoreName,
                    storeOpenFlag,
                    commonNameFindValue,
                    certContexts));
        }
    }

    return error;
}
#endif

_Use_decl_annotations_
ErrorCode CryptoUtility::DecryptText(wstring const & encryptedText, SecureString & decryptedText)
{
    return DecryptText(encryptedText, X509Default::StoreLocation(), decryptedText);
}

#if defined(PLATFORM_UNIX)
ErrorCode CryptoUtility::DecryptText(wstring const & encryptedText, X509StoreLocation::Enum certStoreLocation, SecureString & decryptedText)
{
    PLATFORM_VALIDATE(certStoreLocation)

    ByteBuffer decryptedTextBuffer;
    ErrorCode error = LinuxCryptUtil().Pkcs7Decrypt(
        encryptedText,
        SecurityConfig::GetConfig().SettingsX509StoreName,
        decryptedTextBuffer);
    if (!error.IsSuccess()) return error;

    vector<WCHAR> alignedBuffer(decryptedTextBuffer.size()/sizeof(WCHAR) + 1);
    memcpy(alignedBuffer.data(), decryptedTextBuffer.data(), decryptedTextBuffer.size());
    decryptedText = SecureString(alignedBuffer.data()); 
    return error;
}
#else
_Use_decl_annotations_
ErrorCode CryptoUtility::DecryptText(wstring const & encryptedText, X509StoreLocation::Enum certStoreLocation, SecureString & decryptedText)
{
    PLATFORM_VALIDATE(certStoreLocation)

    ErrorCode error;
    decryptedText = SecureString(L"");
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "DecryptText called with encryptedText:{0}",
        encryptedText);

    wstring storeName = SecurityConfig::GetConfig().SettingsX509StoreName;
    CertStoreUPtr certStore(CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        NULL,
        certStoreLocation | CERT_STORE_READONLY_FLAG,
        storeName.c_str()));

    if (!certStore.get())
    {
        error = ErrorCode::FromWin32Error();
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Failed to open store '{0}' at {1}: {2}",
            storeName,
            certStoreLocation,
            error);

        return error;
    }

    ByteBuffer byteBuffer;
    Base64StringToBytes(encryptedText, byteBuffer);

    HCERTSTORE certStoreArray[] = { certStore.get() };

    CRYPT_DECRYPT_MESSAGE_PARA decryptParams;
    DWORD  decryptParamsSize = sizeof(decryptParams);
    memset(&decryptParams, 0, decryptParamsSize);
    decryptParams.cbSize = decryptParamsSize;
    decryptParams.dwMsgAndCertEncodingType = X509_ASN_ENCODING | PKCS_7_ASN_ENCODING;
    decryptParams.cCertStore = 1;
    decryptParams.rghCertStore = certStoreArray;

    DWORD decryptedMessageSize;
    if (!CryptDecryptMessage(
        &decryptParams,
        byteBuffer.data(),
        (DWORD)byteBuffer.size(),
        NULL,
        &decryptedMessageSize,
        NULL))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptDecryptMessage failed when trying to get decryptedMessageSize: {0}",
            error);

        return error;
    }

    ByteBuffer decryptedMessageBuffer(decryptedMessageSize);
    if (!CryptDecryptMessage(
        &decryptParams,
        byteBuffer.data(),
        (DWORD)byteBuffer.size(),
        decryptedMessageBuffer.data(),
        &decryptedMessageSize,
        NULL))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptDecryptMessage failed when trying to decrypted message: {0}",
            error);

        return error;
    }

    decryptedText = SecureString(reinterpret_cast<WCHAR *>(decryptedMessageBuffer.data()));

    SecureZeroMemory((void *)decryptedMessageBuffer.data(), decryptedMessageBuffer.size());
    return error;
}
#endif

#if defined(PLATFORM_UNIX)

_Use_decl_annotations_
ErrorCode CryptoUtility::EncryptText(
wstring const & plainText,
PCCertContext certContext,
LPCSTR algorithmOid, // algorithmOid is ignored. //LINUXTODO implement other algorithms.
wstring & encryptedText)
{
  std::vector<PCCertContext> recipientCertificates;
  recipientCertificates.push_back(certContext);
  const BYTE * byteBuffer = reinterpret_cast<const BYTE *>(plainText.c_str());
  DWORD plainTextSize = (DWORD)plainText.size() * sizeof(WCHAR);
  ByteBuffer plainTextBuffer(byteBuffer, byteBuffer + plainTextSize);
  return LinuxCryptUtil().Pkcs7Encrypt(recipientCertificates, plainTextBuffer, encryptedText);
}

#else
_Use_decl_annotations_
ErrorCode CryptoUtility::EncryptText(
wstring const & plainText,
PCCertContext certContext,
LPCSTR algorithmOid,
wstring & encryptedText)
{
    PCCertContext recipientCertificateArray[] = { certContext };

    CRYPT_ALGORITHM_IDENTIFIER encryptAlgorithm = {};
    encryptAlgorithm.pszObjId = algorithmOid ? (LPSTR)algorithmOid : (LPSTR)szOID_NIST_AES256_CBC;

    CRYPT_ENCRYPT_MESSAGE_PARA encryptParams = {};
    DWORD encryptParamsSize = sizeof(encryptParams);
    encryptParams.cbSize = encryptParamsSize;
    encryptParams.dwMsgEncodingType = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    encryptParams.ContentEncryptionAlgorithm = encryptAlgorithm;

    ErrorCode error;
    DWORD cipherTextSize;
    if (!CryptEncryptMessage(
        &encryptParams,
        1,
        recipientCertificateArray,
        reinterpret_cast<const BYTE *>(plainText.c_str()),
        (DWORD)plainText.size() * sizeof(WCHAR),
        NULL,
        &cipherTextSize))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptEncryptMessage({0}) failed when trying to get cipherTextSize: {1}",
            encryptAlgorithm.pszObjId,
            error);

        encryptedText.clear();
        return error;
    }

    ByteBuffer cipherTextBuffer(cipherTextSize);
    if (!CryptEncryptMessage(
        &encryptParams,
        1,
        recipientCertificateArray,
        reinterpret_cast<const BYTE *>(plainText.c_str()),
        (DWORD)plainText.size() * sizeof(WCHAR),
        cipherTextBuffer.data(),
        &cipherTextSize))
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptEncryptMessage({0}) failed when trying to encrypt message: {1}",
            encryptAlgorithm.pszObjId,
            error);

        encryptedText.clear();
        return error;
    }

    encryptedText = BytesToBase64String(cipherTextBuffer.data(), (unsigned int)cipherTextSize);
    return error;
}
#endif

_Use_decl_annotations_
ErrorCode CryptoUtility::EncryptText(
    wstring const & plainText,
    wstring const & certThumbPrint,
    wstring const & certStoreName,
    X509StoreLocation::Enum certStoreLocation,
    LPCSTR algorithmOid,
    wstring & encryptedText)
{
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "EncryptText called with certThumbprint:{0} certStoreName:{1} certStoreLocation:{2}",
        certThumbPrint,
        certStoreName,
        certStoreLocation);

    PLATFORM_VALIDATE(certStoreLocation)

    Thumbprint::SPtr thumbprintObj;
    auto error = Thumbprint::Create(certThumbPrint, thumbprintObj);
    if (!error.IsSuccess())
    {
        TraceInfo(TraceTaskCodes::Common, TraceType_CryptoUtility, "Failed to parse thumbprint '{0}': {1}", certThumbPrint, error);
        encryptedText.clear();
        return error;
    }

    CertContextUPtr certContext;
    error = CryptoUtility::GetCertificate(
        certStoreLocation,
        certStoreName,
        thumbprintObj,
        certContext);

    if (!error.IsSuccess())
    {
        encryptedText.clear();
        return error;
    }

    return EncryptText(plainText, certContext.get(), algorithmOid, encryptedText);
}

_Use_decl_annotations_
ErrorCode CryptoUtility::EncryptText(
wstring const & plainText,
wstring const & certFilePath,
LPCSTR algorithmOid,
wstring & encryptedText)
{
    TraceNoise(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "EncryptText called with certFilePath:{0}",
        certFilePath);

    CertContextUPtr certContext;
    auto error = GetCertificate(certFilePath, certContext);
    if (!error.IsSuccess())
    {
        encryptedText.clear();
        return error;
    }

    return EncryptText(plainText, certContext.get(), algorithmOid, encryptedText);
}


#if defined(PLATFORM_UNIX)
//LINUXTODO
#else
ErrorCode CryptoUtility::GetKnownFolderName(int csidl, wstring const & relativePath, wstring & folderName)
{
    vector<wchar_t> folderNameBuffer(MAX_PATH);

    HRESULT result = SHGetFolderPathW(NULL, csidl, NULL, 0, folderNameBuffer.data());

    ErrorCode errorCode = ErrorCode::FromHResult(result);
    if (errorCode.IsSuccess())
    {
        folderName = folderNameBuffer.data();
        if (!relativePath.empty())
        {
            folderName = Path::Combine(folderName, relativePath);
        }
    }

    return errorCode;
}
#endif

bool CryptoUtility::IsBase64(WCHAR c)
{
    return (isalnum(c) || (c == L'+') || (c == L'/'));
}

void CryptoUtility::Base64StringToBytes(wstring const& encodedString, ByteBuffer & decodedBytes)
{
    size_t indexLength = encodedString.size();
    size_t i = 0, j = 0;
    int index = 0;
    BYTE array_4[4], array_3[3];

    while (indexLength-- && encodedString[index] != '=' && IsBase64(encodedString[index]))
    {
        array_4[i++] = static_cast<BYTE>(encodedString[index]);
        index++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
            {
                array_4[i] = static_cast<BYTE>(BASE64_CHARS.find(array_4[i]));
            }

            array_3[0] = (array_4[0] << 2) + ((array_4[1] & 0x30) >> 4);
            array_3[1] = ((array_4[1] & 0xf) << 4) + ((array_4[2] & 0x3c) >> 2);
            array_3[2] = ((array_4[2] & 0x3) << 6) + array_4[3];

            for (i = 0; (i < 3); i++)
            {
                decodedBytes.push_back(array_3[i]);
            }

            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
        {
            array_4[j] = 0;
        }

        for (j = 0; j < 4; j++)
        {
            array_4[j] = static_cast<BYTE>(BASE64_CHARS.find(array_4[j]));
        }
        array_3[0] = (array_4[0] << 2) + ((array_4[1] & 0x30) >> 4);
        array_3[1] = ((array_4[1] & 0xf) << 4) + ((array_4[2] & 0x3c) >> 2);
        array_3[2] = ((array_4[2] & 0x3) << 6) + array_4[3];

        for (j = 0; (j < i - 1); j++)
        {
            decodedBytes.push_back(array_3[j]);
        }
    }
}

wstring CryptoUtility::BytesToBase64String(BYTE const* bytesToEncode, unsigned int length) {
    wstring encodedString;
    int i = 0, j = 0;
    BYTE array_3[3];
    BYTE array_4[4];

    while (length--)
    {
        array_3[i++] = *(bytesToEncode++);
        if (i == 3)
        {
            array_4[0] = (array_3[0] & 0xfc) >> 2;
            array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
            array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
            array_4[3] = array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++)
            {
                encodedString += BASE64_CHARS[array_4[i]];
            }

            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
        {
            array_3[j] = '\0';
        }
        array_4[0] = (array_3[0] & 0xfc) >> 2;
        array_4[1] = ((array_3[0] & 0x03) << 4) + ((array_3[1] & 0xf0) >> 4);
        array_4[2] = ((array_3[1] & 0x0f) << 2) + ((array_3[2] & 0xc0) >> 6);
        array_4[3] = array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            encodedString += BASE64_CHARS[array_4[j]];

        while ((i++ < 3))
            encodedString += '=';
    }

    return encodedString;
}

#if defined(PLATFORM_UNIX)
//LINUXTODO
#else
ErrorCode CryptoUtility::GetCertificateByExtension(
    HCERTSTORE certStore,
    shared_ptr<X509FindValue> const & findValue,
    PCCertContext pPrevCertContext,
    PCCertContext & pCertContext)
{
    // Check if the findValue represents supported extension. Currently only SubjectAltName is supported extension.
    X509FindSubjectAltName* subjectAltName = dynamic_cast<X509FindSubjectAltName*>(findValue.get());
    if (!subjectAltName)
    {
        return ErrorCode::FromNtStatus(STATUS_NOT_SUPPORTED);
    }

    // Enumerate certs in store, looking for the right match.
    CERT_ENHKEY_USAGE usage = { 0 };
    usage.cUsageIdentifier = 1;
    char* serverUsage = szOID_PKIX_KP_SERVER_AUTH;

    usage.rgpszUsageIdentifier = &serverUsage;
    PCCertContext prevCert = pPrevCertContext;
    pCertContext = NULL;
    PCCertContext currCert = NULL;

    bool goodMatchFound = false;

    do
    {
        currCert = CertFindCertificateInStore(
            certStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG,
            CERT_FIND_ENHKEY_USAGE,
            &usage,
            prevCert);

        if (currCert == NULL)
        {
            return ErrorCode::FromWin32Error();
        }

        prevCert = currCert;

        // Verify the cert.
        bool certFound = false;

        for (ULONG i = 0; i < currCert->pCertInfo->cExtension; ++i)
        {
            PCERT_EXTENSION certExt = &currCert->pCertInfo->rgExtension[i];
            if ((_stricmp(certExt->pszObjId, szOID_SUBJECT_ALT_NAME) != 0) &&
                (_stricmp(certExt->pszObjId, szOID_SUBJECT_ALT_NAME2) != 0))
            {
                continue;
            }

            ULONG size = 0;
            BOOL success = CryptDecodeObjectEx(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                certExt->pszObjId,
                certExt->Value.pbData,
                certExt->Value.cbData,
                0,
                NULL,
                NULL,
                &size);
            if (!success)
            {
                auto error = ErrorCode::FromWin32Error();
                if (!error.IsWin32Error(ERROR_MORE_DATA))
                {
                    // CryptDecodeObjectEx failed
                    TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "GetCertificateByExtension - CryptDecodeObjectEx failed with error: {0}.", error);
                    return error;
                }
            }

            std::unique_ptr<BYTE[]> buffer(new BYTE[size]);
            success = CryptDecodeObjectEx(
                X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                certExt->pszObjId,
                certExt->Value.pbData,
                certExt->Value.cbData,
                0,
                NULL,
                buffer.get(),
                &size);
            if (!success)
            {
                auto error = ErrorCode::FromWin32Error();
                TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "GetCertificateByExtension - CryptDecodeObjectEx failed with error: {0}.", error);
                return error;
            }

            PCERT_ALT_NAME_INFO altNameArray = reinterpret_cast<PCERT_ALT_NAME_INFO>(buffer.get());
            for (ULONG j = 0; j < altNameArray->cAltEntry; ++j)
            {
                PCERT_ALT_NAME_ENTRY altName = &altNameArray->rgAltEntry[j];
                if (altName->dwAltNameChoice != CERT_ALT_NAME_DNS_NAME)
                {
                    continue;
                }

                if (CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, altName->pwszDNSName, -1, ((CERT_ALT_NAME_ENTRY*)subjectAltName->Value())->pwszDNSName, -1) == CSTR_EQUAL)
                {
                    // Match found.
                    certFound = true;
                    break;
                }
            }

            break;
        }

        if (!certFound)
        {
            continue;
        }

        // Lookup found good cert
        goodMatchFound = true;
    } while (!goodMatchFound);

    pCertContext = currCert;
    return ErrorCode::Success();
}
#endif

#ifndef PLATFORM_UNIX
PCCertContext CryptoUtility::GetCertificateFromStore(
    HCERTSTORE certStore,
    std::shared_ptr<X509FindValue> const & findValue,
    PCCertContext pPrevCertContext)
{
    PCCertContext pCertContext = nullptr;

    if (findValue == NULL)
    {
        // Find first cert in store
        pCertContext = CertEnumCertificatesInStore(certStore, pPrevCertContext);
    }
    else if (findValue->Type() == X509FindType::FindByExtension)
    {
        ErrorCode error = GetCertificateByExtension(
            certStore,
            findValue,
            pPrevCertContext,
            pCertContext);
    }
    else
    {
        // Find first match
        pCertContext = CertFindCertificateInStore(
            certStore,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            findValue->Flag(),
            findValue->Type(),
            findValue->Value(),
            pPrevCertContext);
    }

    return pCertContext;
}
#endif

#if defined(PLATFORM_UNIX)
_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificate(wstring const & certFilePath, CertContextUPtr & certContext)
{
    certContext.reset();
    std::string certFilePath2;
    StringUtility::Utf16ToUtf8(certFilePath, certFilePath2);
    certContext = LinuxCryptUtil().LoadCertificate(certFilePath2);
    if(certContext.get() == NULL)
    {
         ErrorCode::FromErrno();
    }

    return ErrorCodeValue::Success;
}
#else
_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificate(wstring const & certFilePath, CertContextUPtr & certContext)
{
    certContext.reset();

    File certFile;
    ErrorCode error = certFile.TryOpen(certFilePath);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "GetCertificate: failed to open cert file {0}: {1}",
            certFilePath,
            error);

        return error;
    }

    int64 fileSize;
    error = certFile.GetSize(fileSize);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "GetCertificate: failed to get file size of {0}: {1}",
            certFilePath,
            error);

        return error;
    }

    ByteBuffer buffer(fileSize);
    DWORD bytesRead = 0;
    error = certFile.TryRead2(buffer.data(), (int)buffer.size(), bytesRead);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "GetCertificate: failed to read from cert file {0}: {1}",
            certFilePath,
            error);

        return error;
    }

    if (bytesRead != buffer.size())
    {
        error = ErrorCodeValue::OperationFailed;
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "GetCertificate: failed to read all bytes from cert file {0}: size={1}, read={2}",
            certFilePath,
            buffer.size(),
            bytesRead);

        return error;
    }

    PCCertContext contextPtr = ::CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        buffer.data(),
        (DWORD)buffer.size());

    if (!contextPtr)
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CertCreateCertificateContext: failed on certificate read from {0}: {1}",
            certFilePath,
            error);

        return error;
    }

    certContext.reset(contextPtr);
    return error;
}
#endif

ErrorCode CryptoUtility::GetCertificateCommonName(
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName,
    wstring const & findType,
    wstring const & findValue,
    wstring & commonName)
{
    commonName.clear();

    CertContextUPtr certContext;
    auto error = GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findType,
        findValue,
        certContext);

    if (!error.IsSuccess()) return error;

    return GetCertificateCommonName(certContext.get(), commonName);
}

_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificateCommonName(PCCertContext certContext, wstring & commonName)
{
#if defined(PLATFORM_UNIX)
    if(certContext == NULL)
    {
        return ErrorCodeValue::ArgumentNull;
    }

    std::string cn;
    auto error = LinuxCryptUtil().GetCommonName((X509*)certContext, cn);
    if (!error.IsSuccess() || cn.empty())
    {
        return error;
    }

    StringUtility::Utf8ToUtf16(cn, commonName);
    return ErrorCode::Success();
#else
    DWORD length = 0;
    length = CertGetNameString(
        certContext,
        CERT_NAME_ATTR_TYPE,
        0,
        (void*)szOID_COMMON_NAME,
        nullptr,
        length);

    if (length == 1)
    {
        ErrorCode error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "CertGetNameString() error = {0}", error);
        return error;
    }

    vector<WCHAR> nameBuffer(length);
    if (!CertGetNameString(
        certContext,
        CERT_NAME_ATTR_TYPE,
        0,
        (void*)szOID_COMMON_NAME,
        nameBuffer.data(),
        length))
    {
        ErrorCode error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "CertGetNameString() error = {0}", error);
        return error;
    }

    commonName = &nameBuffer.front();
    return ErrorCode::Success();
#endif
}

_Use_decl_annotations_
ErrorCode CryptoUtility::GetCertificateCommonName(
X509StoreLocation::Enum certificateStoreLocation,
wstring const & certificateStoreName,
shared_ptr<X509FindValue> const & findValue,
wstring & commonName)
{
    commonName.clear();

    CertContextUPtr certificate;
    auto error = CryptoUtility::GetCertificate(
        certificateStoreLocation,
        certificateStoreName,
        findValue,
        certificate);

    if (!error.IsSuccess())
    {
        return error;
    }

    return GetCertificateCommonName(certificate.get(), commonName);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    CertContextUPtr & cert)
{
    return CreateSelfSignedCertificate(
        subjectName,
        DateTime::Now() + TimeSpan::FromMinutes(60.0 * 24 * 365 * 10), //aka.ms/sre-codescan/disable
        cert);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    wstring const & keyContainerName,
    CertContextUPtr & cert)
{
    return CreateSelfSignedCertificate(
        subjectName,
        DateTime::Now() + TimeSpan::FromMinutes(60.0 * 24 * 365 * 10),//aka.ms/sre-codescan/disable
        keyContainerName,
        cert);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    wstring const & keyContainerName,
    bool fMachineKeyset,
    CertContextUPtr & cert)
{
    return CreateSelfSignedCertificate(
        subjectName,
        nullptr,
        DateTime::Now() + TimeSpan::FromMinutes(60.0 * 24 * 365 * 10),//aka.ms/sre-codescan/disable
        keyContainerName,
        fMachineKeyset,
        cert);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    DateTime expiration,
    CertContextUPtr & cert)
{
    return CreateSelfSignedCertificate(
        subjectName,
        expiration,
        AZURE_SERVICE_FABRIC_KEY_CONTAINER_DEFAULT,
        cert);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    DateTime expiration,
    wstring const & keyContainerName,
    CertContextUPtr & cert)
{
    return CreateSelfSignedCertificate(
        subjectName,
        nullptr,
        expiration,
        keyContainerName,
        cert);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    const vector<wstring> * subjectAltNames,
    DateTime expiration,
    wstring const & keyContainerName,
    CertContextUPtr & cert)
{
    return CreateSelfSignedCertificate(
        subjectName,
        subjectAltNames,
        expiration,
        keyContainerName,
        true,
        cert);
}

_Use_decl_annotations_ ErrorCode CryptoUtility::CreateSelfSignedCertificate(
    wstring const & subjectName,
    const vector<wstring> * subjectAltNames,
    DateTime expiration,
    wstring const & keyContainerName,
    bool fMachineKeyset,
    CertContextUPtr & cert)
{
    if (subjectAltNames)
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CreateSelfSignedCertificate: subjectName='{0}', expiration={1}, keyContainerName='{2}', fMachineKeyset='{3}', subjectAltNames='{4}'",
            subjectName,
            expiration,
            keyContainerName,
            fMachineKeyset,
            *subjectAltNames);
    }
    else
    {
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CreateSelfSignedCertificate: subjectName='{0}', expiration={1}, keyContainerName='{2}', fMachineKeyset='{3}'",
            subjectName,
            expiration,
            keyContainerName,
            fMachineKeyset);
    }

    // Initialize out parameters
    cert.reset();

#if defined(PLATFORM_UNIX)
    std::string sn1;
    StringUtility::Utf16ToUtf8(subjectName, sn1);
    return LinuxCryptUtil().CreateSelfSignedCertificate(
        subjectName,
        subjectAltNames,
        expiration,
        keyContainerName,
        cert);
#else

    SubjectName certSubjectName(subjectName);
    auto err = certSubjectName.Initialize();
    if (!err.IsSuccess()) return err;

    CRYPT_KEY_PROV_INFO keyProvInfo = {};
    keyProvInfo.pwszContainerName = (LPWSTR)(keyContainerName.c_str());
    keyProvInfo.pwszProvName = MS_ENHANCED_PROV_W;
    keyProvInfo.dwProvType = AZURE_SERVICE_FABRIC_CRYPT_RPOVIDER_TYPE;
    if (fMachineKeyset)
    {
        keyProvInfo.dwFlags = CRYPT_MACHINE_KEYSET;
    }
    keyProvInfo.dwKeySpec = AT_KEYEXCHANGE;

    //CRYPT_ALGORITHM_IDENTIFIER signatureAlgorithm = {};
    //signatureAlgorithm.pszObjId = szOID_RSA_SHA512RSA;

    DateTime startTimeAsDateTime = DateTime::Now() - TimeSpan::FromMinutes(60 * 24) /* adjust backward by a day for possible timezone issue */;
    FILETIME startTimeAsFileTime = startTimeAsDateTime.AsFileTime;
    SYSTEMTIME certStartTime = {};
    if (::FileTimeToSystemTime(&startTimeAsFileTime, &certStartTime) == FALSE)
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "FileTimeToSystemTime failed on start time {0}: {1}", startTimeAsDateTime, error);
        return error;
    }

    FILETIME expiryAsFileTime = expiration.AsFileTime;
    SYSTEMTIME certExpiry = {};
    if (::FileTimeToSystemTime(&expiryAsFileTime, &certExpiry) == FALSE)
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "FileTimeToSystemTime failed on expiration time {0}: {1}", expiration, error);
        return error;
    }

    PCERT_EXTENSIONS pcertExtensions = NULL;

    // This code is adding single extension only. i.e. SubjectAltName.
    CERT_EXTENSION certExtensionArray[1];
    CERT_EXTENSIONS certExtensions = { 1, certExtensionArray };
    CRYPT_DATA_BLOB extBlob;

    if (subjectAltNames && (!subjectAltNames->empty()))
    {
        // Subject Alt Name.
        std::unique_ptr<CERT_ALT_NAME_ENTRY[]> altNames(new CERT_ALT_NAME_ENTRY[subjectAltNames->size()]);
        CERT_ALT_NAME_INFO altNameInfo = { (DWORD)subjectAltNames->size(), altNames.get() };

        for (int i = 0; i < subjectAltNames->size(); i++)
        {
            altNames[i].dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
            altNames[i].pwszDNSName = (LPWSTR)((*subjectAltNames)[i].c_str());
        }

        auto retCode = CryptEncodeObjectEx(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            X509_ALTERNATE_NAME,
            &altNameInfo,
            CRYPT_ENCODE_ALLOC_FLAG,
            NULL,
            &extBlob.pbData,
            &extBlob.cbData);

        if (!retCode)
        {
            auto error = ErrorCode::FromWin32Error();
            TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "CryptEncodeObjectEx failed: {0}", error);
            return error;
        }

        certExtensionArray[0].fCritical = false;
        certExtensionArray[0].pszObjId = szOID_SUBJECT_ALT_NAME2;
        certExtensionArray[0].Value = extBlob;

        pcertExtensions = &certExtensions;
    }

    auto pCertContext = CertCreateSelfSignCertificate(NULL, (PCERT_NAME_BLOB)certSubjectName.Value(), 0, &keyProvInfo, NULL, &certStartTime, &certExpiry, pcertExtensions);
    if (!pCertContext)
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "CertCreateSelfSignCertificate failed: {0}", error);
        return error;
    }

    cert.reset(pCertContext);
    return ErrorCodeValue::Success;
#endif
}

ErrorCode CryptoUtility::InstallCertificate(
    CertContextUPtr & certContext,
    X509StoreLocation::Enum certificateStoreLocation,
    wstring const & certificateStoreName)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "InstallCertificate to '{0}' at {1}",
        certificateStoreName,
        certificateStoreLocation);

    PLATFORM_VALIDATE(certificateStoreLocation)

    ErrorCode error;
    BOOL added = TRUE;

    auto x509Folder = certificateStoreName;
#if defined(PLATFORM_UNIX)
    if (StringUtility::AreEqualCaseInsensitive(certificateStoreName, L"My"))
    {
        WriteInfo(
            TraceType_CryptoUtility,
            "certificate file/folder path '{0}' is treated as empty path on PLATFORM_UNIX",
            certificateStoreName);
        x509Folder = L"";
    }

    error = LinuxCryptUtil().InstallCertificate(certContext, x509Folder);
    if(!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CertAddCertificateContextToStore failed: {0}",
            error);

        return error;
    }
#else
    CertStoreUPtr certStore(CertOpenStore(
        CERT_STORE_PROV_SYSTEM,
        0,
        NULL,
        certificateStoreLocation,
        certificateStoreName.c_str()));

    if (!certStore.get())
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Failed to open store '{0}' at {1}: {2}",
            certificateStoreName,
            certificateStoreLocation,
            error);

        return error;
    }

    added = ::CertAddCertificateContextToStore(
        certStore.get(),
        certContext.get(),
        CERT_STORE_ADD_ALWAYS,
        nullptr);

#endif
    if (added == FALSE)
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CertAddCertificateContextToStore failed: {0}",
            error);

        return error;
    }

    return error;
}

ErrorCode CryptoUtility::UninstallCertificate(
    X509StoreLocation::Enum certStoreLocation,
    wstring const & certStoreName,
    shared_ptr<X509FindValue> const & findValue,
    wstring const & keyContainerFilePath)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "UninstallCertificate {0} from {1} at {2} keyContainerFilePath = '{3}'",
        findValue,
        certStoreName,
        certStoreLocation,
        keyContainerFilePath);

#if defined(PLATFORM_UNIX)
    CertContextUPtr cert;
    auto error = GetCertificate(
        certStoreLocation,
        certStoreName,
        0,
        findValue,
        cert);

    if (!error.IsSuccess()) return error;

    return LinuxCryptUtil().UninstallCertificate(cert, !keyContainerFilePath.empty());
#else
    bool deletedSome = false;

    ErrorCode savedError;
    for (;;)
    {
        CertContextUPtr cert;
        auto err = GetCertificate(
            certStoreLocation,
            certStoreName,
            0,
            findValue,
            cert);

        if (!err.IsSuccess())
        {
            savedError = err;
            break;
        }

        auto certContextPtr = cert.release();
        auto deletedThisMatch = ::CertDeleteCertificateFromStore(certContextPtr);
        if (deletedThisMatch == FALSE)
        {
            auto error = ErrorCode::FromWin32Error();
            TraceWarning(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "CertDeleteCertificateFromStore failed: {0}",
                error);

            return error;
        }

        deletedSome = true;
    }

    if (!keyContainerFilePath.empty())
    {
        // todo, leikong, find out how to solve racing of deleting private key file from different tests, using guid container name?
        //auto error = File::Delete2(keyContainerFilePath, true);
        //if (!error.IsSuccess())
        //{
        //    TraceWarning(
        //        TraceTaskCodes::Common,
        //        TraceType_CryptoUtility,
        //        "failed to delete private key file {0}: {1}",
        //        keyContainerFilePath,
        //        error);
        //}
    }

    return deletedSome ? ErrorCodeValue::Success : savedError;
#endif
}

InstallTestCertInScope::InstallTestCertInScope(
    wstring const & subjectName,
    const vector<wstring> *subjectAltNames,
    TimeSpan expiration,
    wstring const & storeName,
    wstring const & keyContainerName,
    X509StoreLocation::Enum storeLocation)
    : InstallTestCertInScope(true, subjectName, subjectAltNames, expiration, storeName, keyContainerName, storeLocation)
{
}


InstallTestCertInScope::InstallTestCertInScope(
    bool install,
    wstring const & subjectName,
    const vector<wstring> *subjectAltNames,
    TimeSpan expiration,
    wstring const & storeName,
    wstring const & keyContainerName,
    X509StoreLocation::Enum storeLocation)
    : subjectName_(subjectName.empty() ? (L"CN=" + Guid::NewGuid().ToString()) : subjectName)
    , storeName_(storeName)
    , storeLocation_(storeLocation)
    , uninstallOnDestruction_(true)
{
    auto error = X509StoreLocation::PlatformValidate(storeLocation_);
    Invariant(error.IsSuccess());

    error = CryptoUtility::CreateSelfSignedCertificate(
        subjectName_,
        subjectAltNames,
        DateTime::Now() + expiration,
        keyContainerName.empty() ? AZURE_SERVICE_FABRIC_KEY_CONTAINER_DEFAULT : keyContainerName,
        certContext_);

    Invariant(error.IsSuccess());

    error = Thumbprint::Create(certContext_.get(), thumbprint_);
    Invariant(error.IsSuccess());

    if (install)
    {
        error = CryptoUtility::InstallCertificate(certContext_, storeLocation_, storeName_);
        Invariant(error.IsSuccess());
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "test cert installed: '{0}' thumbprint={1}, expiration={2}",
            subjectName_, thumbprint_, expiration);

#ifdef PLATFORM_UNIX
        keyContainerFilePath_ = L"DummyValueIgnoredOnLinux";
#else
        error = CryptoUtility::GetCertificatePrivateKeyFileName(
            storeLocation_,
            storeName_,
            thumbprint_,
            keyContainerFilePath_);

        Invariant(error.IsSuccess());
        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "key container file path: {0}",
            keyContainerFilePath_);

        BOOL fCallerFreeProvOrNCryptKey = FALSE;
        HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hCryptProvOrNCryptKey = NULL;
        KFinally([=] {
            if ((fCallerFreeProvOrNCryptKey == TRUE) && (hCryptProvOrNCryptKey != NULL))
                ::CryptReleaseContext(hCryptProvOrNCryptKey, 0);
        });

        DWORD dwKeySpec;
        if (!::CryptAcquireCertificatePrivateKey(
            certContext_.get(),
            0,
            NULL,
            &hCryptProvOrNCryptKey,
            &dwKeySpec,
            &fCallerFreeProvOrNCryptKey))
        {
            auto err = ErrorCode::FromWin32Error();
            TraceError(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "Failed to load private key: {0}",
                err);
        }

        TraceInfo(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Verified private key can be loaded");

#endif
    }
}

InstallTestCertInScope::~InstallTestCertInScope()
{
    if (uninstallOnDestruction_) Uninstall();
}

ErrorCode InstallTestCertInScope::Install()
{
    auto error = CryptoUtility::InstallCertificate(certContext_, storeLocation_, storeName_);
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType_CryptoUtility,
        "test cert installed: subjectName='{0}', thumbprint={1}",
        subjectName_, thumbprint_);
    return error;
}

void InstallTestCertInScope::Uninstall(bool deleteKeyContainer)
{
    auto error = CryptoUtility::UninstallCertificate(
        storeLocation_,
        storeName_,
        thumbprint_,
        deleteKeyContainer ? keyContainerFilePath_ : L"");
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "UninstallCertificate failed: {0}",
            error);
    }

    // verify the certificate is indeed removed
    CertContextUPtr cert;
    error = CryptoUtility::GetCertificate(
        storeLocation_,
        storeName_,
        thumbprint_,
        cert);

    if (error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "certificate still can be loaded after uinstall");
    }
}

void InstallTestCertInScope::DisableUninstall()
{
    uninstallOnDestruction_ = false;
}

PCCertContext InstallTestCertInScope::CertContext() const
{
    return certContext_.get();
}

CertContextUPtr const & InstallTestCertInScope::GetCertContextUPtr() const
{
    return certContext_;
}

CertContextUPtr InstallTestCertInScope::DetachCertContext()
{
    return move(certContext_);
}

Thumbprint::SPtr const & InstallTestCertInScope::Thumbprint() const
{
    return thumbprint_;
}

wstring const & InstallTestCertInScope::SubjectName() const
{
    return subjectName_;
}

wstring const & InstallTestCertInScope::StoreName() const
{
    return storeName_;
}

X509StoreLocation::Enum InstallTestCertInScope::StoreLocation() const
{
    return storeLocation_;
}

TimeSpan InstallTestCertInScope::DefaultCertExpiry()
{
    return TimeSpan::FromMinutes(60 * 24 * 7 * 20); //20 weeks, avoid using global, as TimSpan static members have initialization order issues
}

#ifdef PLATFORM_UNIX

X509Context const & InstallTestCertInScope::X509ContextRef() const
{
    return certContext_;
}

const char CryptoUtility::alphanum[] = "0123456789"
                                       "!@#$^&*"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz";

ErrorCode CryptoUtility::GenerateRandomString(_Out_ SecureString & password)
{    
    wstring pass;
    for (auto i = 0; i < RANDOM_STRING_LENGTH; ++i)
    {
        pass += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    password = SecureString(move(pass));
    return ErrorCodeValue::Success;
}

#else

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificateChain(
    PCCertContext certContext,
    CertChainContextUPtr & certChain)
{
    certChain.reset();
    CERT_CHAIN_PARA chainPara = {};
    chainPara.cbSize = sizeof(CERT_CHAIN_PARA);

    PCCERT_CHAIN_CONTEXT certChainContextPtr = nullptr;
    if (!::CertGetCertificateChain(
        nullptr,
        certContext,
        nullptr,
        certContext->hCertStore,
        &chainPara,
        CERT_CHAIN_CACHE_END_CERT,
        nullptr,
        &certChainContextPtr))
    {
        auto err = ErrorCode::FromWin32Error();
        WriteWarning(TraceType_CryptoUtility, "CertGetCertificateChain failed: {0}", err);
        return err;
    }

    certChain.reset(certChainContextPtr);

    WriteInfo(
        TraceType_CryptoUtility,
        "cert chain trust status: info = {0:x}, error = {1:x}, chainSize = {2}",
        certChain->TrustStatus.dwInfoStatus,
        certChain->TrustStatus.dwErrorStatus,
        certChain->rgpChain[0]->cElement);

    if (certChain->TrustStatus.dwErrorStatus & CERT_TRUST_IS_PARTIAL_CHAIN) // partial chain?
    {
        WriteError(TraceType_CryptoUtility, "partial chain");
        return ErrorCodeValue::OperationFailed;
    }

    return ErrorCodeValue::Success;
}

_Use_decl_annotations_ ErrorCode CryptoUtility::GetCertificateIssuerPubKey(
    PCCertContext certContext,
    X509Identity::SPtr & issuerPubKey)
{
    CertChainContextUPtr certChain;
    auto err = GetCertificateChain(certContext, certChain);
    if (!err.IsSuccess()) return err;

    auto issuerCert = GetCertificateIssuer(certChain.get());
    if (!issuerCert)
    {
        err = ErrorCodeValue::OperationFailed;
        return err;
    }

    issuerPubKey = make_shared<X509PubKey>(issuerCert);
    return err;
}

bool CryptoUtility::IsCertificateSelfSigned(PCCERT_CHAIN_CONTEXT certChain)
{
    return (certChain->rgpChain[0]->rgpElement[0]->TrustStatus.dwInfoStatus & CERT_TRUST_IS_SELF_SIGNED) == CERT_TRUST_IS_SELF_SIGNED;
}

bool CryptoUtility::GetCertificateIssuerChainIndex(PCCERT_CHAIN_CONTEXT certChain, size_t & issuerCertIndex)
{
    if (IsCertificateSelfSigned(certChain))
    {
        issuerCertIndex = 0; // self signed
        return true;
    }

    if (1 < certChain->rgpChain[0]->cElement)
    {
        issuerCertIndex =  1; // CA signed
        return true;
    }

    issuerCertIndex = numeric_limits<size_t>::max();
    WriteWarning(
        TraceType_CryptoUtility,
        "partial chain? certChain->rgpChain[0]->cElement = {0}, certChain->rgpChain[0]->rgpElement[0]->TrustStatus.dwInfoStatus = {1:x}",
        certChain->rgpChain[0]->cElement,
        certChain->rgpChain[0]->rgpElement[0]->TrustStatus.dwInfoStatus);

    return false;
}

PCCertContext CryptoUtility::GetCertificateIssuer(PCCERT_CHAIN_CONTEXT certChain)
{
    size_t issuerCertIndex = 0;
    if (!GetCertificateIssuerChainIndex(certChain, issuerCertIndex))
    {
        return nullptr;
    }

    return certChain->rgpChain[0]->rgpElement[issuerCertIndex]->pCertContext;
}

ErrorCode CryptoUtility::GetCertificateIssuerThumbprint(
    PCCERT_CONTEXT certContext,
    Thumbprint::SPtr & issuerThumbprint)
{
    CertChainContextUPtr certChainContext;
    auto error = GetCertificateChain(certContext, certChainContext);
    if (!error.IsSuccess())
    {
        return ErrorCodeValue::OperationFailed;
    }

    auto issuerCert = GetCertificateIssuer(certChainContext.get());
    if (!issuerCert)
    {
        error = ErrorCodeValue::OperationFailed;
        return error;
    }

    error = Thumbprint::Create(issuerCert, issuerThumbprint);
    if (!error.IsSuccess())
    {
        WriteError(TraceType_CryptoUtility, "failed to get issuer thumbprint: {0}", error);
    }

    return error;
}

string Execute(const char* cmd) 
{
    char buffer[BUFFER_SIZE];
    string result = "";
    shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe)
    {
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "popen() failed. Command cannot be executed");
        return result;
    }

    while (!feof(pipe.get())) 
   {
        if (fgets(buffer, 128, pipe.get()) != NULL)
        {
            result += buffer;
        }
   }

    return result;
}

bool CheckMachineModeAndRegKeys()
{
    string result = Execute(BcdeditCommand.c_str());
    result.erase(remove_if(result.begin(), result.end(), isspace), result.end());
    if (result.find(TestSignedSubstring) == string::npos && result.find(AccessDeniedString) == string::npos)
    {
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "Test signing is not enabled on machine.");
        return false;
    }
    HKEY hKey;
    LONG regKey32Build = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEY32BUILD, 0, KEY_READ, &hKey);
    LONG regKeyBuild = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGKEYBUILD, 0, KEY_READ, &hKey);
    if (regKey32Build != ERROR_SUCCESS && regKeyBuild != ERROR_SUCCESS)
    {
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "Registry keys required for strong name validation not present");
        return false;
    }
    return true;
}

ErrorCode CryptoUtility::VerifyEmbeddedSignature(wstring const & fileName)
{
    LONG lStatus;
    DWORD dwLastError;

    // Initialize the WINTRUST_FILE_INFO structure.
    WINTRUST_FILE_INFO FileData;
    memset(&FileData, 0, sizeof(FileData));
    FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
    FileData.pcwszFilePath = fileName.c_str();
    FileData.hFile = NULL;
    FileData.pgKnownSubject = NULL;

    /*
    WVTPolicyGUID specifies the policy to apply on the file
    WINTRUST_ACTION_GENERIC_VERIFY_V2 policy checks:

    1) The certificate used to sign the file chains up to a root
    certificate located in the trusted root certificate store. This
    implies that the identity of the publisher has been verified by
    a certification authority.

    2) In cases where user interface is displayed (which this example
    does not do), WinVerifyTrust will check for whether the
    end entity certificate is stored in the trusted publisher store,
    implying that the user trusts content from this publisher.

    3) The end entity certificate has sufficient permission to sign
    code, as indicated by the presence of a code signing EKU or no
    EKU.
    */

    GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    WINTRUST_DATA WinTrustData;

    // Initialize the WinVerifyTrust input data structure.    
    memset(&WinTrustData, 0, sizeof(WinTrustData)); // Default all fields to 0.
    WinTrustData.cbStruct = sizeof(WinTrustData); 
    WinTrustData.pPolicyCallbackData = NULL; // Use default code signing EKU.
    WinTrustData.pSIPClientData = NULL; // No data to pass to SIP
    WinTrustData.dwUIChoice = WTD_UI_NONE; // Disable WVT UI.
    WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE; // No revocation checking.
    WinTrustData.dwUnionChoice = WTD_CHOICE_FILE; // Verify an embedded signature on a file.
    WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY; // Verify action
    WinTrustData.hWVTStateData = NULL; // Verification sets this value.
    WinTrustData.pwszURLReference = NULL; // Not used.
    WinTrustData.dwUIContext = 0; // This is not applicable if there is no UI because it changes the UI to accommodate running applications instead of installing applications.
    WinTrustData.pFile = &FileData; // Set pFile.

    // WinVerifyTrust verifies signatures as specified by the GUID 
    // and Wintrust_Data.
    lStatus = ::WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

    ErrorCode error = ErrorCodeValue::Success;
    switch (lStatus)
    {
    case ERROR_SUCCESS:
        TraceInfo(
            TraceTaskCodes::Common, 
            TraceType_CryptoUtility, 
            "Signed file: "
            "- Hash that represents the subject is trusted."
            "- Trusted publisher without any verification errors."
            "- UI was disabled in dwUIChoice.No publisher or time stamp chain errors.");
        break;
    case TRUST_E_NOSIGNATURE:
        // The file was not signed or had a signature 
        // that was not valid.

        // Get the reason for no signature.
        dwLastError = ::GetLastError();
        if (TRUST_E_NOSIGNATURE == dwLastError ||
            TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError ||
            TRUST_E_PROVIDER_UNKNOWN == dwLastError)
        {
            if (CheckMachineModeAndRegKeys())
            {
                error = ErrorCodeValue::Success;
                break;
            }
            TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "The file \"{0}\" is not signed.\n", fileName);		
        }
        else
        {
            TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "Error {0} occurred trying to verify the signature of the \"{1}\" file.\n", dwLastError, fileName);
        }
        error = ErrorCode::FromWin32Error(dwLastError);
        break;

    case TRUST_E_EXPLICIT_DISTRUST:
        // The hash that represents the subject or the publisher 
        // is not allowed by the admin or user.
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "The signature is present, but specifically disallowed.\n");
        dwLastError = ::GetLastError();
        error = ErrorCode::FromWin32Error(dwLastError);
        break;

    case TRUST_E_SUBJECT_NOT_TRUSTED:
        // The user clicked "No" when asked to install and run.
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "The signature is present, but not trusted.\n");
        dwLastError = ::GetLastError();
        error = ErrorCode::FromWin32Error(dwLastError);
        break;

    case CRYPT_E_SECURITY_SETTINGS:
        /*
        The hash that represents the subject or the publisher
        was not explicitly trusted by the admin and the
        admin policy has disabled user trust. No signature,
        publisher or time stamp errors.
        */
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "CRYPT_E_SECURITY_SETTINGS - The hash representing the subject or the publisher wasn't explicitly trusted by the admin and admin policy has disabled user trust. No signature, publisher or timestamp errors.\n");
        dwLastError = ::GetLastError();
        error = ErrorCode::FromWin32Error(dwLastError);
        break;

    case TRUST_E_SUBJECT_FORM_UNKNOWN:
        //The subject contains an unknown form
        dwLastError = ::GetLastError();
        if (CheckMachineModeAndRegKeys())
        {
            error = ErrorCodeValue::Success;
            break;
        }
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "The file \"{0}\" contains an unknown subject form.\n", fileName);
        error = ErrorCode::FromWin32Error(dwLastError);
        break;

    case CRYPT_E_FILE_ERROR:
        //The file cannot be read.
        dwLastError = ::GetLastError();
        if (CheckMachineModeAndRegKeys())
        {
            error = ErrorCodeValue::Success;
            break;
        }
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "The file \"{0}\" cannot be read.\n", fileName);
        error = ErrorCode::FromWin32Error(dwLastError);
        break;

    default:
        // The UI was disabled in dwUIChoice or the admin policy 
        // has disabled user trust. lStatus contains the 
        // publisher or time stamp chain error.
        TraceError(TraceTaskCodes::Common, TraceType_CryptoUtility, "Error is : \"{0}\".\n",lStatus);
        dwLastError = ::GetLastError();
        error = ErrorCode::FromWin32Error(dwLastError);
        break;
    }

    // Any hWVTStateData must be released by a call with close.
    WinTrustData.dwStateAction = WTD_STATEACTION_CLOSE;

    lStatus = ::WinVerifyTrust(
        NULL,
        &WVTPolicyGUID,
        &WinTrustData);

    return error;
}

ErrorCode CryptoUtility::GenerateRandomString(_Out_ SecureString & password)
{
    HCRYPTPROV hCryptProv;

    if (!CryptAcquireContext(&hCryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptAcquireContext failed with {0}",
            ::GetLastError());
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    BYTE pwdByte[RANDOM_STRING_LENGTH];
    ::ZeroMemory(pwdByte, RANDOM_STRING_LENGTH);

    if (!::CryptGenRandom(hCryptProv, RANDOM_STRING_LENGTH, pwdByte))
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptGenRandom failed with {0}",
            ::GetLastError());

        return ErrorCode::FromWin32Error(::GetLastError());
    }

    wstring pass;
    StringWriter writer(pass);
    for (DWORD i = 0; i < RANDOM_STRING_LENGTH; ++i)
    {
        writer.Write("{0:x}", pwdByte[i]);
    }

    password = SecureString(move(pass));
    return ErrorCodeValue::Success;
}

#endif

ErrorCode CryptoUtility::CreateCertFromKey(
    ByteBuffer const & buffer,
    wstring const & certKey,
    CertContextUPtr & certContext)
{
#ifdef PLATFORM_UNIX
    return ErrorCodeValue::NotImplemented; // LINUXTODO implement create cert from key.
#else

    PCCertContext contextPtr  = ::CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        buffer.data(),
        (DWORD)buffer.size());

    ErrorCode error;
    if (!contextPtr)
    {
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CertCreateCertificateContext: failed to create cert from bytes: {0}",
            error);

        return error;
    }

    certContext.reset(contextPtr);

    auto containerName = Guid::NewGuid().ToString();
    error = CryptoUtility::ImportCertKey(containerName, certKey);

    if (!error.IsSuccess())  { return error; }

    CRYPT_KEY_PROV_INFO kpi;
    ZeroMemory(&kpi, sizeof(kpi));
    kpi.pwszContainerName =  (LPWSTR)(containerName.c_str());
    kpi.dwProvType = PROV_RSA_FULL;
    kpi.dwKeySpec = AT_KEYEXCHANGE;
    kpi.dwFlags = CRYPT_MACHINE_KEYSET;
    if (!::CertSetCertificateContextProperty(contextPtr, CERT_KEY_PROV_INFO_PROP_ID, 0, &kpi))
    {
        certContext.reset();
        error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
                "CertSetCertificateContextProperty failed with {0}",
                error);

        return error;
    }

    return ErrorCodeValue::Success;
#endif
}

ErrorCode CryptoUtility::ImportCertKey(wstring const& keyContainerName, wstring const& key)
{
#ifdef PLATFORM_UNIX
    return ErrorCodeValue::NotImplemented; // LINUXTODO implement create cert from key.
#else
    
    HCRYPTPROV hProv;
    HCRYPTKEY hKey;

    if (!::CryptAcquireContext(
        &hProv,
        (LPWSTR)(keyContainerName.c_str()),
        NULL,
        PROV_RSA_FULL,
        CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptAcquireContext failed with {0}",
            error);

        return error;
    }

    ByteBuffer keyBuffer;
    CryptoUtility::Base64StringToBytes(key, keyBuffer);

    if (!::CryptImportKey(
        hProv,
        keyBuffer.data(),
        (DWORD)keyBuffer.size(),
        0,
        CRYPT_EXPORTABLE,
        &hKey))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptImportKey failed with {0}",
            error);

        return error;
    }

    return ErrorCodeValue::Success;

#endif
}

ErrorCode CryptoUtility::GenerateExportableKey(wstring const& keyContainerName, _Out_ SecureString & key)
{
    return GenerateExportableKey(keyContainerName, true, key);
}

ErrorCode CryptoUtility::GenerateExportableKey(wstring const& keyContainerName, bool fMachineKeyset, _Out_ SecureString & key)
{
#ifdef PLATFORM_UNIX
        return ErrorCodeValue::NotImplemented; // LINUXTODO implement create cert from key.
#else

    HCRYPTPROV hCryptProv;
    DWORD dwFlags = CRYPT_NEWKEYSET;

    if (fMachineKeyset)
    {
        dwFlags |= CRYPT_MACHINE_KEYSET;
    }

    if (!::CryptAcquireContext(
        &hCryptProv,
        (LPWSTR)(keyContainerName.c_str()),
        NULL,
        PROV_RSA_FULL,
        dwFlags))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptAcquireContext failed with {0}",
            error);

        return error;
    }

    HCRYPTKEY hKey;

    if (!::CryptGenKey(
        hCryptProv,
        AT_KEYEXCHANGE,
        0x08000000 | CRYPT_EXPORTABLE, // 0x08000000 to specify a 2048-bit RSA key
        &hKey))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "CryptGenKey failed with {0}",
            error);
        
        return error;
    }

    ByteBuffer keyBlob;
    DWORD dwBlobLen; 
    
    if (!::CryptExportKey(
        hKey,
        0,
        PRIVATEKEYBLOB,
        0,
        NULL,
        &dwBlobLen))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "CryptExportKey failed with {0}",
                error);
        return error;
    }

    keyBlob.resize(dwBlobLen);

    if (!CryptExportKey(
        hKey,
        0,
        PRIVATEKEYBLOB,
        0,
        keyBlob.data(),
        &dwBlobLen))
    {
        auto error = ErrorCode::FromWin32Error();
        TraceError(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "CryptExportKey failed with {0}",
                error);
        return error;
    }

    auto encodedKey = CryptoUtility::BytesToBase64String(keyBlob.data(), dwBlobLen);

    if (hKey)
    {
        CryptDestroyKey(hKey);
    }

    key = SecureString(move(encodedKey));
    return ErrorCodeValue::Success;

#endif
}

wstring CryptoUtility::CertToBase64String(CertContextUPtr const & cert)
{
#ifdef PLATFORM_UNIX
        return L""; // LINUXTODO implement create cert from key.
#else

    return CryptoUtility::BytesToBase64String(cert->pbCertEncoded, cert->cbCertEncoded);

#endif
}

ErrorCode CryptoUtility::CreateAndACLPasswordFile(wstring const & passwordFilePath, SecureString & password, vector<wstring> const & machineAccountNamesForACL)
{
    FileWriter passwordFile;
    auto error = passwordFile.TryOpen(passwordFilePath);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Error while opening file to save password: error={0}",
            ::GetLastError());

        passwordFile.Close();
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    passwordFile.WriteUnicodeBuffer(password.GetPlaintext().c_str(), password.GetPlaintext().size());
    passwordFile.Close();

    vector<SidSPtr> sidVector;
    for (auto machineAccountNameForACL : machineAccountNamesForACL)
    {
        Common::SidSPtr sid;
        error = BufferedSid::CreateSPtr(machineAccountNameForACL, sid);
        if (!error.IsSuccess())
        {
            TraceError(
                TraceTaskCodes::Common,
                TraceType_CryptoUtility,
                "Failed to convert {0} to SID: error={1}",
                machineAccountNameForACL,
                error);
            return error;
        }

        sidVector.push_back(sid);
    }

    error = SecurityUtility::OverwriteFileAcl(
        sidVector,
        passwordFilePath,
        GENERIC_READ | GENERIC_WRITE,
        TimeSpan::MaxValue);

    if (!error.IsSuccess())
    {
       TraceError(
            TraceTaskCodes::Common,
            TraceType_CryptoUtility,
            "Failed to OverwriteFileAcl for {0} : {1}",
            passwordFilePath,
            error);
       return error;
    }

    return ErrorCodeValue::Success;
}

