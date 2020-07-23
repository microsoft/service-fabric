// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <dirent.h>
#include <sys/stat.h>

#define GETGR_PW_R_SIZE_MAX 16384
#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>
#include <openssl/pkcs12.h>
#ifndef OPENSSL_THREADS
static_assert(false, "OPENSSL_THREADS is not supported");
#endif

#define PEM_SUFFIX ".pem"
#define PEM_PATTERN "*" PEM_SUFFIX

#define PFX_SUFFIX ".pfx"
#define PRV_SUFFIX ".prv"

using namespace Common;
using namespace std;

namespace
{
    static const StringLiteral TraceType = "LinuxCryptUtil";
    static const __int64 SECS_BETWEEN_1601_AND_1970_EPOCHS = 11644473600LL;
    static const __int64 SECS_TO_100NS = 10000000; /* 10^7 */

    /// Version to 2 for 3rd version (0 indexed, 2 means version = 3rd)
    int const X509_Version3rd = 2;

    /// This sets the serial number of our certificate to '1'.
    /// Some HTTP servers have issues with certificate with a serial number of '0'(default).
    int const X509_DefaultSerialNumber = 1;

    using BN_UPtr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;

    INIT_ONCE initOnce = INIT_ONCE_STATIC_INIT;

    Global<vector<RwLock>> openSslLocks;

    Global<RwLock> x509FolderLock;

    void OpenSslLockingFunc(int mode, int n, const char*, int)
    {
        if (mode & CRYPTO_LOCK)
        {
            (*openSslLocks)[n].AcquireExclusive();
            return;
        }
        
        (*openSslLocks)[n].ReleaseExclusive();
    }

    unsigned long OpenSslThreadIdFunc()
    {
        return pthread_self();
    }

    BOOL OpenSslStartup(PINIT_ONCE, PVOID, PVOID*)
    {
        SSL_library_init();
        SSL_load_error_strings();
        ERR_load_BIO_strings();
        OpenSSL_add_all_algorithms();

        auto lockCount = CRYPTO_num_locks();
        openSslLocks = make_global<vector<RwLock>>(lockCount);
        CRYPTO_set_locking_callback(OpenSslLockingFunc);
        CRYPTO_set_id_callback(OpenSslThreadIdFunc);

        x509FolderLock = make_global<RwLock>();

        return TRUE;
    }

    void EnsureFolder(string const & folder)
    {
        AcquireWriteLock grab(*x509FolderLock);

        if(Directory::Exists(folder)) return;

        auto err = Directory::Create2(folder); // recursion is allowed
        Invariant2(err.IsSuccess(), "EnsureFolder: failed to create folder '{0}': {1}", folder, err);
    }

    string GetX509Folder(string const & x509Folder, bool useSFUserCertPath=false)
    {
        if (x509Folder.empty())
        {
            auto folder = SecurityConfig::GetConfig().X509Folder;
            if(useSFUserCertPath)
            {
                folder = SecurityConfig::GetConfig().AppRunAsAccountGroupX509Folder;
            }
            EnsureFolder(folder);
            Trace.WriteNoise(TraceType, "GetX509Folder: input = '{0}', output = '{1}'", x509Folder, folder);
            return folder;
        }

        Trace.WriteNoise(TraceType, "GetX509Folder: input = '{0}', output = '{1}'", x509Folder, x509Folder);
        return x509Folder;
    }

    bool TryGetUserHomeFolder(string const & userName, string & homeFolder)
    {
        long buflen = GETGR_PW_R_SIZE_MAX;
        unique_ptr<char[]> buf(new char[buflen]);
        struct passwd pwbuf, *pwbufp;
        auto error = getpwnam_r(userName.c_str(), &pwbuf, buf.get(), buflen, &pwbufp);
        if (error == 0 && pwbufp)
        {
            homeFolder = pwbufp->pw_dir;
            return true;
        }

        Trace.WriteError(TraceType, "TryGetUserHomeFolder userName: '{0}' failed. Error: '{1}'", userName, error);
        return false;
    }

    bool TryGetX509CoreFxFolder(string const & userName, string & x509CoreFxFolder)
    {
        string userHomeDirectory;
        if (!TryGetUserHomeFolder(userName, userHomeDirectory))
        {
            return false;
        }

        x509CoreFxFolder = Path::Combine(userHomeDirectory, SecurityConfig::GetConfig().CoreFxX509UserCertificatesPath);
        EnsureFolder(x509CoreFxFolder);
        Trace.WriteNoise(TraceType, "TryGetX509CoreFxFolder: output = '{0}'", x509CoreFxFolder);
        return true;
    }

    string GetThumbprintFileName_NoExt(X509Context const & x509)
    {
        Thumbprint thumbprint;
        auto error = thumbprint.Initialize(x509.get());
        Invariant(error.IsSuccess());

        auto thumbprintStr = thumbprint.PrimaryToString();
        StringUtility::ToUpper(thumbprintStr);
        return StringUtility::Utf16ToUtf8(thumbprintStr);
    }

    string GetPrivKeyLoadPathWithThumbprintFileNameInCertFileFolder(X509Context const & x509)
    {
        Invariant(!x509.FilePath().empty());
        auto folder = Path::GetDirectoryName(x509.FilePath());

        auto thumbprintStr = GetThumbprintFileName_NoExt(x509); 
        auto path = Path::Combine(folder, thumbprintStr);
        Path::ChangeExtension(path, SecurityConfig::GetConfig().PrivateKeyFileExtension);
        return path;
    }

    string GetX509Path_NoExt(X509Context const & x509, string const & x509Folder = "", bool useSFUserCertPath = false)
    {
        auto thumbprintStr = GetThumbprintFileName_NoExt(x509);
        auto folder = GetX509Folder(x509Folder, useSFUserCertPath); 
        return Path::Combine(folder, thumbprintStr);
    }

    string GetX509InstallPath(X509Context const & x509, string const & x509Folder)
    {
        auto path = GetX509Path_NoExt(x509, x509Folder);
        Path::ChangeExtension(path, SecurityConfig::GetConfig().X509InstallExtension);
        return path;
    }
    
    string GetX509SFUserInstallPath(X509Context const & x509, string const & x509Folder)
    {
        auto path = GetX509Path_NoExt(x509, x509Folder, true);
        Path::ChangeExtension(path, SecurityConfig::GetConfig().X509InstallExtension);
        return path;
    }

    string GetDefaultPrivKeyLoadPath(X509Context const & x509)
    {
        auto path = GetX509Path_NoExt(x509);
        Path::ChangeExtension(path, SecurityConfig::GetConfig().PrivateKeyFileExtension);
        return path;
    }

    bool TryGetDefaultCoreFxPrivKeyLoadPath(X509Context const & x509, string const & userName, string & coreFxPrivKeyLoadPath)
    {
        auto filename = GetThumbprintFileName_NoExt(x509);
        Path::ChangeExtension(filename, SecurityConfig::GetConfig().CoreFxX509CertFileExtension);
        string folder;
        if (!TryGetX509CoreFxFolder(userName, folder))
        {
            return false;
        }

        coreFxPrivKeyLoadPath = Path::Combine(folder, filename);
        return true;
    }

    string GetDefaultPrivKeyInstallPath(X509Context const & x509)
    {
        auto path = GetX509Path_NoExt(x509);
        Path::ChangeExtension(path, SecurityConfig::GetConfig().PrivateKeyInstallExtension);
        Trace.WriteNoise(TraceType, "GetDefaultPrivKeyInstallPath: '{0}'", path); 
        return path;
    }
    
    string GetDefaultSFUserPrivKeyInstallPath(X509Context const & x509)
    {
        auto path = GetX509Path_NoExt(x509, "", true);
        Path::ChangeExtension(path, SecurityConfig::GetConfig().PrivateKeyInstallExtension);
        Trace.WriteNoise(TraceType, "GetDefaultSFUserPrivKeyInstallPath: '{0}'", path); 
        return path;
    }

    string CertPathToPrivKeyPath(string const& certFilePath)
    {
        auto path = certFilePath;
        Path::ChangeExtension(path, SecurityConfig::GetConfig().PrivateKeyFileExtension);
        return path;
    }
}

LinuxCryptUtil::LinuxCryptUtil()
{
    Startup();
}

bool LinuxCryptUtil::Startup()
{
    PVOID context = NULL;
    return ::InitOnceExecuteOnce(
        &initOnce,
        OpenSslStartup,
        NULL,
        &context) != 0 ;       
}

ErrorCode LinuxCryptUtil::GetSHA1Thumbprint(PCCertContext x, ByteBuffer & md_valueV) const
{
    static_assert(HASH_SIZE_DEFAULT <= EVP_MAX_MD_SIZE);
    unsigned int md_len;
    md_valueV = ByteBuffer(EVP_MAX_MD_SIZE);

    const EVP_MD* mdalgo = EVP_sha1();
    int error = X509_digest(x, mdalgo, md_valueV.data(), &md_len);

    if (error <= 0)
    {
        md_valueV.resize(0);
        return LogError(__FUNCTION__, " while getting digest using EVP_sha1().", ErrorCode::FromErrno());
    }

    /// resize byte buffer
    md_valueV.resize(md_len);
    if (md_len < HASH_SIZE_DEFAULT)
    {
        /// length should be 20 for sha1 hash
        LogInfo(__FUNCTION__, " : Returning thumbprint of length: " + to_string(md_len));
    }

    return ErrorCode::Success();
}

ErrorCode LinuxCryptUtil::GetCommonName(PCCertContext x, std::string& cn) const
{
    Invariant(x);

    X509_NAME * xn = X509_get_subject_name((CertContext *)x);
    if (xn == NULL)
    {
        auto err = GetOpensslErr();
        WriteWarning(TraceType, "GetCommonName: X509_get_subject_name returned null: {0}", err.Message);
        return err; 
    }

    /// get length first
    int len = X509_NAME_get_text_by_NID(xn, NID_commonName, NULL, 0);
    if (len < 0)
    {
        auto err = GetOpensslErr();
        WriteInfo(TraceType, "GetCommonName: X509_NAME_get_text_by_NID returned {0}: {1}", len, err.Message);
        return err;
    }

    /// get common name
    std::vector<char> commonName(len + 1);
    X509_NAME_get_text_by_NID(xn, NID_commonName, commonName.data(), commonName.size());

    /// return string
    cn = std::string(commonName.data(), len);
   // LogInfo(__FUNCTION__, " : Returning common name: " + cn);
    return ErrorCode::Success();
}

/// We are not supporting CERT_SIMPLE_NAME_STR
ErrorCode LinuxCryptUtil::GetKeyValPairsFromSubjectName(std::string const& sn, std::map<std::string, std::string>& snKV)
{
    std::vector<std::string> kvStrings;
    std::string comma(",");
    StringUtility::Split(sn, kvStrings, comma, false);
    bool success = true;
    for (auto &kvStr : kvStrings)
    {
        std::vector<std::string> kvPair;
        std::string eq("=");
        StringUtility::Split(kvStr, kvPair, eq, false);
        if (kvPair.size() != 2)
        {
            LogInfo(__FUNCTION__, " : kvString is not a valid key value pair.:" + kvStr);
            success = false;
            continue;
        }

        std::string & k = kvPair[0];
        std::string & v = kvPair[1];
        Common::StringUtility::TrimWhitespaces(k);
        Common::StringUtility::TrimWhitespaces(v);
        snKV[k] = v;
        if (k.empty() || v.empty())
        {
            LogInfo(__FUNCTION__, " : kvString is not a valid key value pair.:" + kvStr);
            success = false;
            continue;
        }
    }

    if (!success)
    {
        // ERROR no value just key was found
        return ErrorCodeValue::InvalidSubjectName;
    }

    return ErrorCode::Success();
}

ErrorCode LinuxCryptUtil::GetKeyValPairsFromSubjectName(std::wstring const& wsn, std::map<std::string, std::string>& snKV)
{
    return GetKeyValPairsFromSubjectName(StringUtility::Utf16ToUtf8(wsn), snKV);
}

string LinuxCryptUtil::X509NameToString(X509_NAME* xn)
{
    if (xn == NULL)
    {
        WriteError(TraceType, "X509_NAME is null");
        return string();
    }

    BioUPtr memBioUPtr(BIO_new(BIO_s_mem()));
    BIO* outbio = memBioUPtr.get();
    X509_NAME_print_ex(outbio, xn, 0, XN_FLAG_SEP_COMMA_PLUS);
    BIO_flush(outbio);
    BUF_MEM* bufMem = nullptr;
    BIO_get_mem_ptr(outbio, &bufMem);

    return string(bufMem->data, bufMem->length);
}

string LinuxCryptUtil::GetSubjectName(PCCertContext x) const
{
    X509_NAME * xn = X509_get_subject_name((CertContext *)x);
    return X509NameToString(xn);
}

string LinuxCryptUtil::GetIssuerName(PCCertContext x) const
{
    X509_NAME * xn = X509_get_issuer_name((CertContext *)x);
    return X509NameToString(xn);
}

vector<string> LinuxCryptUtil::GetSubjectAltName(PCCertContext x) const
{
    vector<string> names;
    STACK_OF(GENERAL_NAME) *altNames = NULL;
    altNames = (STACK_OF(GENERAL_NAME)*) X509_get_ext_d2i((CertContext *)x, NID_subject_alt_name, NULL, NULL);
    if (altNames == nullptr) {
        WriteInfo(TraceType, "No Subject Alternative Name available.");
        return names;
    }

    int num = sk_GENERAL_NAME_num(altNames);
    for (int i = 0; i < num; ++i) {
        const GENERAL_NAME *name = sk_GENERAL_NAME_value(altNames, i);
        if (name->type != GEN_DNS) {
            continue;
        }
        char *dnsName = (char *) ASN1_STRING_data(name->d.dNSName);
        if (ASN1_STRING_length(name->d.dNSName) != strlen(dnsName)) {
            WriteWarning(TraceType, "Invalid dns name detected in certificate.");
            continue;
        }
        names.push_back(dnsName);
    }
    sk_GENERAL_NAME_pop_free(altNames, GENERAL_NAME_free);
    return names;
}

X509Context LinuxCryptUtil::LoadCertificate(std::string const& filepath) const
{
    auto fp = fopen(filepath.c_str(), "r");    
    KFinally([=] { if (fp) fclose(fp); });
    if (fp == NULL)
    {
        auto err = ErrorCode::FromErrno();
        if (err.IsErrno(EACCES))
        {
            WriteInfo(TraceType, "LoadCertificate: skipping {0} according to permission", filepath);
        }
        else
        {
            WriteWarning(TraceType, "LoadCertificate('{0}'): fopen failed: {1}", filepath, err); 
        }
        return nullptr;
    }

    X509Context x(PEM_read_X509(fp, NULL, NULL, NULL), filepath);
    if (!x)
    {
        auto err = ERR_GET_REASON(ERR_get_error());
        WriteTrace(
            (err == PEM_R_NO_START_LINE)? LogLevel::Info : LogLevel::Warning, // is it a PEM certificate file?
            TraceType,
            "LoadCertificate('{0}'): PEM_read_X509 failed: {1}",
            filepath, GetOpensslErr(err).Message);
        return x;
    }

    auto subjectName = X509_NAME_oneline(X509_get_subject_name(x.get()), nullptr, 0);
    KFinally([=] { free(subjectName); });
    auto issuerName = X509_NAME_oneline(X509_get_issuer_name(x.get()), nullptr, 0);
    KFinally([=] { free(issuerName); });
    uint64 certSerialNo = ASN1_INTEGER_get(x->cert_info->serialNumber);

    WriteInfo(
        TraceType,
        "LoadCertificate('{0}'): subject = {1}, issuer = {2}, serial = {3:x}",
        filepath,
        subjectName,
        issuerName,
        certSerialNo);

    return x;
}

ErrorCode LinuxCryptUtil::InstallCertificate(X509Context & x509, std::wstring const & x509Folder) const
{
    if (x509.FilePath().empty())
    {
        auto x509InstallPath = GetX509InstallPath(x509, StringUtility::Utf16ToUtf8(x509Folder));
        x509.SetFilePath(x509InstallPath);
        WriteInfo(TraceType, "InstallCertificate: installing to '{0}'", x509.FilePath());
    }
    else
    {
        WriteInfo(TraceType, "InstallCertificate: ignore input path '{0}', as X509 is associated with '{1}'", x509Folder, x509.FilePath());
    }

    return WriteX509ToFile(x509);
}

ErrorCode LinuxCryptUtil::InstallCoreFxCertificate(string const & userName, X509Context const & x509) const
{
    WriteInfo(TraceType, "Start@InstallCoreFxCertificate");

    ErrorCode err;
    PrivKeyContext privKey;

    // Check if certificate already exists
    string x509InstallPath;
    
    if (!TryGetDefaultCoreFxPrivKeyLoadPath(x509, userName, x509InstallPath))
    {
        WriteError(TraceType, "Can't get default CoreFx private key load path");
        return ErrorCodeValue::OperationFailed;
    }

    err = ReadPrivateKey(x509InstallPath, privKey);
    if (err.IsSuccess())
    {
        WriteInfo(TraceType, "CoreFx certificates found");
        return err;
    }

    // Certificate needs to be installed
    // Read private key
    err = ReadPrivateKey(x509, privKey);
    if (!err.IsSuccess())
    {
        WriteInfo(TraceType, "ReadPrivateKey failed");
        return err;
    }

    auto p12 = PKCS12_create("", "", privKey.get(), x509.get(), NULL, 0, 0, 0, 0, 0);
    KFinally([=] { if (p12) PKCS12_free(p12); });
    if (!p12)
    {
        auto err = GetOpensslErr();
        WriteInfo(TraceType, "InstallCoreFxCertificate('{0}'): PKCS12_create failed: {1}", x509InstallPath, err);
        return err;
    }

    auto fd = fopen(x509InstallPath.c_str(), "wb");
    KFinally([=] { if (fd) fclose(fd); });
    if (!fd)
    {
        auto err = ErrorCode::FromErrno();
        if (err.IsErrno(EACCES))
        {
            if ((Random().Next() % 8) == 0)
            {
                auto diagInfo = File::GetFileAccessDiagnostics(x509InstallPath);
                if (!diagInfo.empty())
                {
                    WriteInfo(TraceType, "InstallCoreFxCertificate({0}) failed due to permission: {1}", x509InstallPath, diagInfo);
                    return err;
                }
            }

            WriteInfo(TraceType, "InstallCoreFxCertificate({0}) failed due to permission", x509InstallPath);
            return err;
        }

        WriteInfo(TraceType, "InstallCoreFxCertificate('{0}'): fopen failed: {1}", x509InstallPath, err);
        return err;
    }

    int success = i2d_PKCS12_fp(fd, p12);
    if (!success)
    {
        auto err = GetOpensslErr();
        WriteWarning(TraceType, "InstallCoreFxCertificate: i2d_PKCS12_fp('{0}') failed: {1}", x509InstallPath, err);
        return err;
    }

    WriteInfo(TraceType, "Success@InstallCoreFxCertificate");
    return ErrorCodeValue::Success;
}

ErrorCode LinuxCryptUtil::UninstallCertificate(X509Context const & x509, bool removeKey) const
{
    ErrorCode err;
    auto const & path = x509.FilePath();
    auto keyPath = CertPathToPrivKeyPath(x509.FilePath());

    if (removeKey)
    {
        Invariant(!path.empty());
        auto retval = unlink(path.c_str());
        if (retval < 0)
        {
            auto errNo = errno;
            WriteNoise(TraceType, "UninstallCertificate: unable to delete certificate file '{0}': {1}", path, errNo);
            err = ErrorCode::FromErrno(errNo);
        }

        struct stat buf = {};
        if (stat(keyPath.c_str(), &buf) == 0) // separate key file exists or key is in the same file as certificate?
        {
            retval = unlink(keyPath.c_str());
            if (retval < 0)
            {
                auto errNo = errno;
                WriteNoise(TraceType, "UninstallCertificate: unable to delete private key file '{0}': {1}", keyPath, errNo);
                return ErrorCode::FromErrno(errNo); 
            }
        }

        return err;
    }

    if (StringUtility::EndsWithCaseInsensitive<string>(path, PEM_SUFFIX))
    {
        auto keyPath = path;
        Path::ChangeExtension(keyPath, SecurityConfig::GetConfig().PrivateKeyFileExtension);
        struct stat buf = {};
        if (stat(keyPath.c_str(), &buf) == 0)
        {
            //key is stored in a separate file, so we can simply remove cert file
            auto retval = unlink(path.c_str());
            if (retval < 0)
            {
                auto errNo = errno;
                WriteNoise(TraceType, "UninstallCertificate: unable to delete certificate file '{0}': {1}", path, errNo);
                err = ErrorCode::FromErrno(errNo);
            }

            return err;
        }

        //rename .pem to .prv, so this certificate will not be loaded anymore.
        if (rename(path.c_str(), keyPath.c_str()) == 0) return err;

        auto errNo = errno;
        WriteNoise(TraceType, "UninstallCertificate: rename('{0}', '{1}') failed: {2}", path, keyPath, errNo);
        err = ErrorCode::FromErrno(errNo);
    }

    return err;
}

ErrorCode LinuxCryptUtil::WriteX509ToFile(X509Context const & x509) const
{
    auto & filepath = x509.FilePath();
    Invariant(!filepath.empty());
    auto fp = fopen(filepath.c_str(), "a");
    KFinally([=] { if (fp) fclose(fp); });
    if (!fp)
    {
        auto err = ErrorCode::FromErrno();
        WriteWarning(TraceType, "WriteX509ToFile: fopen('{0}') failed: {1}", filepath, err); 
        return err; 
    }

    int success = PEM_write_X509(fp, x509.get());
    if (!success)
    {
        auto err = GetOpensslErr();
        WriteWarning(TraceType, "WriteX509ToFile: PEM_write_X509('{0}') failed: {1}", filepath, err);
        return err;
    }

    return ErrorCode::Success();
}

std::vector<std::string> LinuxCryptUtil::GetCertFiles(std::wstring const& x509PathW) const
{
    vector<string> files;

    auto x509Path = StringUtility::Utf16ToUtf8(x509PathW);
    if (!x509Path.empty())
    {
        if (Path::IsRegularFile(x509Path))
        {
            WriteNoise(TraceType, "GetCertFiles: '{0}' is regular file", x509Path);
            files.emplace_back(x509Path);
            return files;
        }
    }

    WriteNoise(TraceType, "GetCertFiles: treat '{0}' as a folder", x509PathW);
    auto certFolder = GetX509Folder(x509Path); 

    auto certFileExt = "*" + SecurityConfig::GetConfig().CertFileExtension;
    files = Directory::GetFiles(certFolder, certFileExt, true, false);

    if (!StringUtility::AreEqualCaseInsensitive(SecurityConfig::GetConfig().CertFileExtension, PEM_SUFFIX))
    {
        auto files2 = Directory::GetFiles(certFolder, PEM_PATTERN, true, false);
        files.reserve(files.size() + files2.size());
        for(auto & file : files2)
        {
            files.emplace_back(move(file));
        }
    }

    return files;
}

ErrorCode LinuxCryptUtil::GetCertificateNotAfterDate(X509* x, DateTime& t) const
{
    return Asn1TimeToDateTime(X509_get_notAfter(x), t);
}

ErrorCode LinuxCryptUtil::GetCertificateNotBeforeDate(X509* x, DateTime& t) const
{
    return Asn1TimeToDateTime(X509_get_notBefore(x), t);
}

static time_t ASN1_GetTimeT(ASN1_TIME* time)
{
    struct tm t;
    const char* str = (const char*) time->data;
    size_t i = 0;

    memset(&t, 0, sizeof(t));

    if (time->type == V_ASN1_UTCTIME) /* two digit year */
    {
        t.tm_year = (str[i++] - '0') * 10 + (str[i++] - '0');
        if (t.tm_year < 70)
        t.tm_year += 100;
    }
    else if (time->type == V_ASN1_GENERALIZEDTIME) /* four digit year */
    {
        t.tm_year = (str[i++] - '0') * 1000 + (str[i++] - '0') * 100 + (str[i++] - '0') * 10 + (str[i++] - '0');
        t.tm_year -= 1900;
    }
    t.tm_mon = ((str[i++] - '0') * 10 + (str[i++] - '0')) - 1; // -1 since January is 0 not 1.
    t.tm_mday = (str[i++] - '0') * 10 + (str[i++] - '0');
    t.tm_hour = (str[i++] - '0') * 10 + (str[i++] - '0');
    t.tm_min  = (str[i++] - '0') * 10 + (str[i++] - '0');
    t.tm_sec  = (str[i++] - '0') * 10 + (str[i++] - '0');

    return mktime(&t);
}

ErrorCode LinuxCryptUtil::Asn1TimeToDateTime(ASN1_TIME *asnt, DateTime& t) const
{
#if defined (OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x010002000
    int days;
    int seconds;
    auto retval = ASN1_TIME_diff(&days, &seconds, nullptr, asnt); //diff with current time
    if (retval == 0)
    {
        auto err = GetOpensslErr();
        WriteWarning(TraceType, "Asn1TimeToDateTime: ASN1_TIME_diff failed: {0}", err.Message);
        return err;
    }
    t = DateTime::Now() + TimeSpan::FromSeconds(3600.0 * 24.0 * days + seconds); 
#else
    time_t now;
    time(&now);
    time_t end = ASN1_GetTimeT(asnt);
    double diffSeconds = difftime(end, now);
    t = DateTime::Now() + TimeSpan::FromSeconds(diffSeconds); 
#endif

    return ErrorCode::Success();
}

PrivKeyContext LinuxCryptUtil::CreatePrivateKey(int keySizeInBits, int exponent)
{
    PrivKeyContext privKeyResult(EVP_PKEY_new());
    BN_UPtr bigN(BN_new(), ::BN_free);
    RSA* keyPair = NULL;

    std::string lastActivity("");
    lastActivity = (lastActivity + __FUNCTION__ + " start");
    bool success = false;

    do{
        // Test the big number object creation
        lastActivity = "Creating BIGNUM object ";
        if (!bigN.get())
            break;

        // Create the RSA key pair object
        lastActivity = "Creating RSA key pair object ";
        keyPair = RSA_new();
        if (!keyPair)
            break;

        // Set the word
        lastActivity = "Initializing BIGNUM with exponent BN_set_word() ";
        if (!BN_set_word(bigN.get(), exponent))
            break;

        // Compute/generate random key pair using BIGNUMBER
        lastActivity = "Computing RSA key pair ";
        if (!RSA_generate_key_ex(keyPair, keySizeInBits, bigN.get(), NULL))
            break;

        // Test private key object creation
        lastActivity = "Creating EVP_PKEY object. EVP_PKEY_new() ";
        if (!privKeyResult.get())
            break;

        // Assign the key pair to the private key object
        // keyPair now belongs to EVP_PKEY, so don't clean it up separately
        lastActivity = "Initializing EVP_PKEY object with computed RSA key pairs. ";
        if (!EVP_PKEY_assign_RSA(privKeyResult.get(), keyPair))
            break;

        success = true;
    } while (false);

    // Cleanup on failure: privKeyResult
    if (!success)
    {
        LogError(__FUNCTION__, lastActivity, ErrorCode::FromErrno());
        if (privKeyResult.get())
        {
            privKeyResult.reset();// this will free RSA keys too
        }
        else if (keyPair)
        {
            // privKeyResult is already null free keyPair
            RSA_free(keyPair);
        }
    }

    // Return the private key (or NULL on failure)
    return privKeyResult;
}

ErrorCode LinuxCryptUtil::SetSubjectName(X509_NAME * name, std::wstring const & subjectName)
{
    if (name == NULL) { return ErrorCodeValue::ArgumentNull; }

    std::map<std::string, std::string> snKV;
    ErrorCode error = GetKeyValPairsFromSubjectName(subjectName, snKV);

    if (!error.IsSuccess())
    {
        return LogError(__FUNCTION__, " : ERROR: parsing subject name, calling GetKeyValPairsFromSubjectName()", error);
    }

    for (auto& kv : snKV)
    {
        int success = X509_NAME_add_entry_by_txt(
            name,
            kv.first.c_str(),
            MBSTRING_ASC,
            (unsigned char *)kv.second.c_str(),
            -1,
            -1,
            0);

        if (!success)
        {
            return LogError(__FUNCTION__, " Adding subjectEntry. key:" + kv.first, ErrorCode::FromErrno());
        }
    }

    return ErrorCode::Success();
}

/// Assuming X509 is self-signed certificate
ErrorCode LinuxCryptUtil::SetSubjectName(X509 * x, std::wstring const & subjectName)
{
    if (x == NULL) { return ErrorCodeValue::ArgumentNull; }

    /// add entry to subject name
    X509_NAME * name = X509_get_subject_name(x);
    ErrorCode error = SetSubjectName(name, subjectName);

    if (!error.IsSuccess())
    {
        return LogError(__FUNCTION__, "updating subject name", error);
    }

    /// update certificate's subject name
    if (!X509_set_issuer_name(x, name))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCode::Success();
}

ErrorCode LinuxCryptUtil::SetAltSubjectName(X509 * x, const std::vector<std::wstring> *subjectAltNames)
{
    if (x == NULL || subjectAltNames == NULL) { return ErrorCodeValue::ArgumentNull; }

    ErrorCode ret = ErrorCode::Success();
    GENERAL_NAMES *gnames = NULL;
    GENERAL_NAME *gname = NULL;
    ASN1_IA5STRING *asn = NULL;

    gnames = sk_GENERAL_NAME_new_null();
    if (gnames == NULL) {
        ret = LogError(__FUNCTION__, "calling sk_GENERAL_NAME_new_null failed", ErrorCode::FromErrno());
        goto cleanup;
    }

    for (int i = 0; i < subjectAltNames->size(); ++i) {
        const wstring& wname = (*subjectAltNames)[i];

        gname = GENERAL_NAME_new();
        if (gname == NULL) {
            ret = LogError(__FUNCTION__, "calling GENERAL_NAME_new() failed", ErrorCode::FromErrno());
            goto cleanup;
        }

        asn = ASN1_IA5STRING_new();
        if (asn == NULL) {
            ret = LogError(__FUNCTION__, "calling ASN1_IA5STRING_new() failed", ErrorCode::FromErrno());
            goto cleanup;
        }

        string name = StringUtility::Utf16ToUtf8(wname);
        if (!ASN1_STRING_set(asn, name.c_str(), -1)) {
            ret = LogError(__FUNCTION__, "calling ASN1_STRING_set failed", ErrorCode::FromErrno());
            goto cleanup;
        }
        GENERAL_NAME_set0_value(gname, GEN_DNS, asn);
        sk_GENERAL_NAME_push(gnames, gname);
        gname = NULL;
        asn = NULL;
    }

    if(!X509_add1_ext_i2d(x, NID_subject_alt_name, gnames, 0, X509V3_ADD_KEEP_EXISTING)) {
        ret = LogError(__FUNCTION__, "calling X509_add1_ext_i2d() ", ErrorCode::FromErrno());
        goto cleanup;
    }

cleanup:
    ASN1_IA5STRING_free(asn);
    GENERAL_NAME_free(gname);
    GENERAL_NAMES_free(gnames);
    return ret;
}

ErrorCode LinuxCryptUtil::SignCertificate(X509 * x, EVP_PKEY* pkey)
{
    if (x == NULL) { return ErrorCodeValue::ArgumentNull; }
    if (!X509_set_pubkey(x, pkey))
    {
        return LogError(__FUNCTION__, "calling X509_set_pubkey() ", ErrorCode::FromErrno());
    }

    int nid = 0;
    auto retval = EVP_PKEY_get_default_digest_nid(pkey, &nid);
    if (retval <= 0)
    {
        auto err = LinuxCryptUtil().GetOpensslErr();
        WriteError(
            TraceType,
            "SignCertificate: EVP_PKEY_get_default_digest_nid failed: retval = {0}, error: {1}:{2}",
            retval, err, err.Message);

        return ErrorCodeValue::OperationFailed;
    }

    WriteInfo(TraceType,
        "SignCertificate: signatureHash: nid = {0}, name = {1}",
        nid, OBJ_nid2ln(nid)); 

    auto signatureHash = EVP_get_digestbynid(nid);
    if (!X509_sign(x, pkey, signatureHash)) 
    {
        return LogError(__FUNCTION__, "calling X509_sign() ", ErrorCode::FromErrno());
    }

    return ErrorCode::Success();
}

ErrorCode LinuxCryptUtil::SetCertificateDates(X509* x, Common::DateTime const& expiration)
{
    if (x == NULL) { return ErrorCodeValue::ArgumentNull; }

    /// start time : 24 hours before now.
    time_t startTime = -1 * 24 * 3600; //LINUXTODO, support setting start time 
    if (!X509_gmtime_adj(X509_get_notBefore(x), startTime))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    /// get time_t from expiration, get diff of seconds.
    TimeSpan lifetime = expiration - DateTime::Now();
    time_t lifetime_t = lifetime.TotalSeconds();
    if (!X509_gmtime_adj(X509_get_notAfter(x), lifetime_t))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCode::Success();
}

ErrorCode LinuxCryptUtil::CreateSelfSignedCertificate(
    std::wstring const & subjectName,
    const std::vector<std::wstring> *subjectAltNames,
    Common::DateTime const& expiration,
    std::wstring const&,
    Common::X509Context & certContext) const
{
    certContext.reset();
    ErrorCode error;
    bool success = false;
    std::string lastActivity("");

    do {
        lastActivity = "Creating X509 new object";
        X509Context newCertUPtr(X509_new());
        if (!newCertUPtr)
        {
            break;
        }

        auto cert = newCertUPtr.get();

        // Set version to 2 for 3rd version (0 indexed, 2 means version = 3rd)
        X509_set_version(cert, X509_Version3rd);

        // This sets the serial number of our certificate to '1'.
        // Some HTTP servers have issues with certificate with a serial number of '0'(default).
        ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);

        // Set certificate validity period
        lastActivity = " calling SetCertificateDates() ";
        LogInfo(__FUNCTION__, lastActivity);
        error = SetCertificateDates(cert, expiration);
        if (!error.IsSuccess()) { break; }

        // Set cert subject
        lastActivity = " calling SetSubjectName() ";
        LogInfo(__FUNCTION__, lastActivity);
        error = SetSubjectName(cert, subjectName);
        if (!error.IsSuccess()) { break; }

        //LINUXTODO: FW: handle altSubjectName
        if (subjectAltNames && (!subjectAltNames->empty())) {
            SetAltSubjectName(cert, subjectAltNames);
        }

        // Generate rsa key pair and save private key to disk.
        lastActivity = " calling CreatePrivateKey() ";
        LogInfo(__FUNCTION__, lastActivity);
        auto pkey = CreatePrivateKey();
        if (!pkey.get()) { break; }

        // Finally sign the certificate
        lastActivity = " calling SignCertificate() ";
        LogInfo(__FUNCTION__, lastActivity);
        error = SignCertificate(cert, pkey.get());
        if (!error.IsSuccess()) { break; }

        lastActivity = " calling WritePrivateKey() ";
        LogInfo(__FUNCTION__, lastActivity);
        string privKeyPath, certPath;

        // sfuser does not have write access to cert store path /var/lib/sfcerts.
        // This prevents us to write Private key of selfsigned certificate to be written to cert store
        // to be consumed while setting transport security using self signed certificate.
        // This change separates cert store folders for Selfsigned Certificates for root and sfuser
        // Fabric processes running as sfuser will use /home/sfuser/sfusercerts for storing self signed cert
        // and Fabric processes running as root will use /var/lib/sfcerts for storing self signed cert
        if(0 != getuid() && 0 != geteuid())
        {
            privKeyPath = GetDefaultSFUserPrivKeyInstallPath(newCertUPtr);
            certPath = GetX509SFUserInstallPath(newCertUPtr, "");
            newCertUPtr.SetFilePath(certPath);
        }
        else
        {
            privKeyPath = GetDefaultPrivKeyInstallPath(newCertUPtr);
        }

        error = WritePrivateKey(privKeyPath, pkey.get());
        if (!error.IsSuccess()) { break; }

        pkey.SetFilePath(privKeyPath);
        

        // Success -- set output variable
        certContext = move(newCertUPtr);
        success = true;
    } while (false);

    if (!success)
    {
        return LogError(__FUNCTION__, lastActivity, error);
    }

    return error;
}

ErrorCode LinuxCryptUtil::WritePrivateKey(string const& privKeyFilePath, EVP_PKEY* pkey)
{
    if (pkey == NULL)
    {
        return LogError(__FUNCTION__, " EVP_PKEY* pkey is NULL.", ErrorCodeValue::ArgumentNull);
    }

    // important for security that we create and set permissions before writing key
    std::wstring wPrivKeyFilePath = StringUtility::Utf8ToUtf16(privKeyFilePath);
    if (File::Exists(wPrivKeyFilePath))
    {
        if (0 != chmod(privKeyFilePath.c_str(), S_IRUSR | S_IWUSR))
        {
            return LogError(__FUNCTION__, privKeyFilePath + " permissions could not be set with chmod.", ErrorCode::FromErrno());
        }
    }

    auto fd = open(privKeyFilePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        return LogError(__FUNCTION__, privKeyFilePath + " could not be opened for writing private-key.", ErrorCode::FromErrno());
    }
    
    // save pkey to file
    auto fp = fdopen(fd, "w");
    if (fp == NULL)
    {
        close(fd);
        return LogError(__FUNCTION__, privKeyFilePath + " file pointer could not be opened for writing private-key.", ErrorCode::FromErrno());
    }
    // fclose on the fp will also close the fd
    KFinally([=] { fclose(fp);});

    auto error = PEM_write_PrivateKey(
        fp,          /* filestream to be writter */
        pkey,       /* private key which is being written */
        NULL,       /* default cipher for encrypting the key-file */
        NULL,       /* password for decrypting the key-file */
        0,          /* password length*/
        NULL,       /* callback function for providing password */
        NULL        /* parameter to callback */
        );

    if (!error)
    {
        return LogError(__FUNCTION__, " during PEM_write_PrivateKey.", ErrorCode::FromErrno());
    }

    return ErrorCode::Success();
}

ErrorCode LinuxCryptUtil::SerializePkcs(PKCS7* p7, wstring & out)
{
    BioUPtr memBioUPtr(BIO_new(BIO_s_mem()));
    //LINUXTODO consider using BIO_f_base64
 //   BioUPtr base64BioUPtr(BIO_new(BIO_f_base64()));
  //  BIO* outbio = BIO_push(base64BioUPtr.get(), memBioUPtr.get());
    BIO* outbio = memBioUPtr.get();

    long ll = i2d_PKCS7_bio(outbio, p7);
    BIO_flush(outbio);

    char* chEnc = NULL;
    long len = BIO_get_mem_data(memBioUPtr.get(), &chEnc);
    out = CryptoUtility::BytesToBase64String((BYTE const*)chEnc, len);
    return ErrorCodeValue::Success;
}

ErrorCode LinuxCryptUtil::Pkcs7Encrypt(ByteBuffer const & in, STACK_OF(X509) *recips, wstring & out) const
{
    BioUPtr memBioUPtr = ByteBufferToBioMem(in);

    const EVP_CIPHER * symmetricEncAlg = EVP_aes_256_cbc();
    PKCS7* p7 = PKCS7_encrypt(recips, memBioUPtr.get(), symmetricEncAlg, 0);
    if (!p7)
    {
        auto error = GetOpensslErr();
        WriteError(TraceType, "Pkcs7Encrypt: PKCS7_encrypt failed: {0}:{1}", error, error.Message);
        return error;
    }

    SerializePkcs(p7, out);
    return ErrorCode::Success();
}

//DeriveKey implements the algorithm of CryptDeriveKey, details can be found at
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa379916(v=vs.85).aspx
ErrorCode LinuxCryptUtil::DeriveKey(ByteBuffer const & input, ByteBuffer & key) const
{
    key.resize(256/8); // LINUXTODO determize key size based on encryption algorithm

    ByteBuffer b1(64, 0x36);
    for(uint i = 0; i < std::min(input.size(), b1.size()); ++i)
    {    
        b1[i] ^= input[i];
    }

    ByteBuffer b2(64, 0x5c);
    for(uint i = 0; i < std::min(input.size(), b2.size()); ++i)
    {    
        b2[i] ^= input[i];
    }

    ByteBuffer hash1;
    auto error = ComputeHash(b1, hash1);
    if (!error.IsSuccess()) return error;

    ByteBuffer hash2;
    error = ComputeHash(b2, hash2);
    if (!error.IsSuccess()) return error;

    auto toCopy = key.size();
    Invariant(toCopy <= (hash1.size() + hash2.size()));
    if (toCopy <= hash1.size())
    {
        memcpy(key.data(), hash1.data(), toCopy);
        return error;
    }

    memcpy(key.data(), hash1.data(), hash1.size());
    toCopy -= hash1.size();
    memcpy(key.data() + hash1.size(), hash2.data(), toCopy);
    return error;
}

ErrorCode LinuxCryptUtil::ComputeHmac(ByteBuffer const& data, ByteBuffer const& key, ByteBuffer & output) const
{
    ErrorCode error;
    HMAC_CTX ctx = {};
    HMAC_CTX_init(&ctx);
    KFinally([&] { HMAC_CTX_cleanup(&ctx); });

    auto alg = EVP_sha512(); // LINUXTODO make algorithm an input parameter
    if (!HMAC_Init(&ctx, key.data(), key.size(), EVP_sha512())) 
    {
        error = GetOpensslErr();
        WriteError(TraceType, "ComputeHmac: HMAC_Init failed: {0}:{1}", error, error.Message);
        return error;
    }

    if (!HMAC_Update(&ctx, data.data(), data.size()))
    {
        error = GetOpensslErr();
        WriteError(TraceType, "ComputeHmac: HMAC_Update failed: {0}:{1}", error, error.Message);
        return error;
    }

    // LINUXTODO output size depends on hash algorithm 
    output.resize(512);
    unsigned int len = 0;
    if (!HMAC_Final(&ctx, output.data(), &len))
    {
        error = GetOpensslErr();
        WriteError(TraceType, "ComputeHmac: HMAC_Final failed: {0}:{1}", error, error.Message);
        return error;
    }

    output.resize(len);
    return error;
}

ErrorCode LinuxCryptUtil::ComputeHash(ByteBuffer const & input, ByteBuffer & output) const
{
    EVP_MD_CTX ctx = {};
    EVP_MD_CTX_init(&ctx);
    KFinally([&] { EVP_MD_CTX_cleanup(&ctx); });

    auto md = EVP_sha256(); //LINUXTODO take algorithm as an input parameter
    if (!EVP_DigestInit_ex(&ctx, md, nullptr))
    {
        auto error = GetOpensslErr();
        WriteError(TraceType, "ComputeHash: EVP_DigestInit_ex failed: {0}:{1}", error, error.Message);
        return error;
    }

    if (!EVP_DigestUpdate(&ctx, input.data(), input.size()))
    {
        auto error = GetOpensslErr();
        WriteError(TraceType, "ComputeHash: EVP_DigestUpdate failed: {0}:{1}", error, error.Message);
        return error;
    }

    unsigned int mdLen = 0;
    output.resize(EVP_MAX_MD_SIZE);
    if (!EVP_DigestFinal(&ctx, output.data(), &mdLen))
    {
        auto error = GetOpensslErr();
        WriteError(TraceType, "ComputeHash: EVP_DigestFinal failed: {0}:{1}", error, error.Message);
        return error;
    }

    output.resize(mdLen);
    return ErrorCode(); 
}

ErrorCode LinuxCryptUtil::GetOpensslErr(uint64 err) const
{
    if (err == 0) err = ERR_get_error();

    char errStr[256];
    ERR_error_string_n(err, errStr, sizeof(errStr));
    return ErrorCode(ErrorCodeValue::OperationFailed, wformatString("openssl:{0}", errStr));
}

ErrorCode LinuxCryptUtil::Pkcs7Sign(X509Context const & cert, BYTE const* input, size_t inputLen, ByteBuffer & output) const
{
    PrivKeyContext privKey;
    auto error = ReadPrivateKey(cert, privKey);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, "Pcks7Sign: failed to load private key: {0}:{1}", error, error.Message);
        return error;
    }

    BioUPtr inputBioUPtr = CreateBioMemFromBufferAndSize(input, inputLen);
    PKCS7_UPtr pkcs7(PKCS7_sign(cert.get(), privKey.get(), nullptr, inputBioUPtr.get(), 0));
    if (!pkcs7)
    {    
        error = GetOpensslErr();
        WriteError(TraceType, "Pcks7Sign: PKCS7_Sign failed: {0}:{1}", error, error.Message);
        return error;
    }
    
    BioUPtr outputBioUPtr(BIO_new(BIO_s_mem()));
    auto outbio = outputBioUPtr.get();
    int ll = i2d_PKCS7_bio(outbio, pkcs7.get());
    BIO_flush(outbio);

    output = BioMemToByteBuffer(outbio);
    return error;
}

ErrorCode LinuxCryptUtil::Pkcs7Encrypt(std::vector<PCCertContext> const& recipientCertificates, ByteBuffer const& plainTextBuffer, wstring & cipherTextBase64) const
{
    STACK_OF(X509) *recips = sk_X509_new_null();
    for (auto & nextCert : recipientCertificates)
    {
        sk_X509_push(recips, nextCert);
    }

    Pkcs7Encrypt(plainTextBuffer, recips, cipherTextBase64);
    return ErrorCodeValue::Success;
}

void LinuxCryptUtil::ReadPrivateKeyPaths(X509Context const & x509, std::vector<string> & paths) const
{
    PrivKeyContext privKey;
    auto& x509FilePath = x509.FilePath();
    WriteInfo(TraceType, "ReadPrivateKeyPaths: x509.FilePath = '{0}'", x509FilePath);

    ErrorCode err;
    if (!x509FilePath.empty())
    {
        if (StringUtility::EndsWithCaseInsensitive<string>(x509FilePath, PEM_SUFFIX))
        {
            err = ReadPrivateKey(x509FilePath, privKey);
            if (err.IsSuccess()) paths.push_back(privKey.FilePath());
        }

        auto privKeyPath = CertPathToPrivKeyPath(x509FilePath);
        err = ReadPrivateKey(x509FilePath, privKey);
        if (err.IsSuccess()) paths.push_back(privKey.FilePath());

        auto pathWithCertThumbprint = GetPrivKeyLoadPathWithThumbprintFileNameInCertFileFolder(x509);
        err = ReadPrivateKey(pathWithCertThumbprint, privKey);
        if (err.IsSuccess()) paths.push_back(privKey.FilePath());
    }
    else
    {
        auto filepath = GetDefaultPrivKeyLoadPath(x509);
        err = ReadPrivateKey(filepath, privKey);
        if (err.IsSuccess()) paths.push_back(privKey.FilePath());
    }
}

ErrorCode LinuxCryptUtil::ReadCoreFxPrivateKeyPath(X509Context const & x509, string const & userName, string & path) const
{
    PrivKeyContext privKey;
    auto& x509FilePath = x509.FilePath();
    WriteInfo(TraceType, "ReadPrivateKeyPaths: x509.FilePath = '{0}'", x509FilePath);

    string coreFxPrivKeyPath;
    if (!TryGetDefaultCoreFxPrivKeyLoadPath(x509, userName, coreFxPrivKeyPath))
    {
        WriteError(TraceType, "Can't get default CoreFx private key load path");
        return ErrorCodeValue::OperationFailed;
    }

    auto err = ReadPrivateKey(coreFxPrivKeyPath, privKey);
    if (!err.IsSuccess())
    {
        return err;
    }

    path = privKey.FilePath();
    return err;
}

ErrorCode LinuxCryptUtil::ReadPrivateKey(X509Context const & x509, PrivKeyContext & privKey) const
{
    auto& x509FilePath = x509.FilePath();
    WriteInfo(TraceType, "ReadPrivateKey: x509.FilePath = '{0}'", x509FilePath);  
    
    ErrorCode err;
    if (!x509FilePath.empty())
    {
        if (StringUtility::EndsWithCaseInsensitive<string>(x509FilePath, PEM_SUFFIX))
        {
            err = ReadPrivateKey(x509FilePath, privKey);
            if (err.IsSuccess())return err;
        }

        auto privKeyPath = CertPathToPrivKeyPath(x509FilePath);
        err = ReadPrivateKey(x509FilePath, privKey);
        if (err.IsSuccess()) return err;

        auto pathWithCertThumbprint = GetPrivKeyLoadPathWithThumbprintFileNameInCertFileFolder(x509);
        err = ReadPrivateKey(pathWithCertThumbprint, privKey);
        if (err.IsSuccess()) return err;

        return ErrorCodeValue::InvalidCredentials;
    }

    auto filepath = GetDefaultPrivKeyLoadPath(x509);
    err = ReadPrivateKey(filepath, privKey);
    if (err.IsSuccess()) return err;
            
    filepath = GetDefaultPrivKeyInstallPath(x509);
    err = ReadPrivateKey(filepath, privKey);
    if (err.IsSuccess()) return err;
    
    filepath = GetDefaultSFUserPrivKeyInstallPath(x509);
    err = ReadPrivateKey(filepath, privKey);
    return err;
}

ErrorCode LinuxCryptUtil::ReadPrivateKey(string const & privateKeyFilePath, PrivKeyContext & privKey) const
{
    auto fd = fopen(privateKeyFilePath.c_str(), "r");
    KFinally([=] { if (fd) fclose(fd); });
    if (!fd)
    {
        auto err = ErrorCode::FromErrno();
        if (err.IsErrno(EACCES))
        {
            if ((Random().Next() % 8) == 0)
            {
                auto diagInfo = File::GetFileAccessDiagnostics(privateKeyFilePath);
                if (!diagInfo.empty())
                {
                    WriteInfo(TraceType, "ReadPrivateKey({0}) failed due to permission: {1}", privateKeyFilePath, diagInfo);
                    return err;
                }
            }

            WriteInfo(TraceType, "ReadPrivateKey({0}) failed due to permission", privateKeyFilePath);
            return err;
        }

        WriteInfo(TraceType, "ReadPrivateKey('{0}'): fopen failed: {1}", privateKeyFilePath, err);
        return err; 
    }

    if (StringUtility::EndsWithCaseInsensitive<string>(privateKeyFilePath, PEM_SUFFIX)
        || StringUtility::EndsWithCaseInsensitive<string>(privateKeyFilePath, PRV_SUFFIX))
    {
        auto key = PEM_read_PrivateKey(fd, nullptr, nullptr, nullptr);
        if (!key)
        {
            auto err = GetOpensslErr();
            WriteInfo(TraceType, "ReadPrivateKey('{0}'): PEM_read_PrivateKey failed: {1}", privateKeyFilePath, err);
            return err;
        }

        privKey = PrivKeyContext(key, privateKeyFilePath);
        WriteInfo(TraceType, "ReadPrivateKey('{0}'): success", privateKeyFilePath);
        return ErrorCodeValue::Success;
    }
    else if (StringUtility::EndsWithCaseInsensitive<string>(privateKeyFilePath, PFX_SUFFIX))
    {
        EVP_PKEY *key;
        X509 *cert = NULL;
        STACK_OF(X509) *ca = NULL;
        PKCS12 * p12 = NULL;
        KFinally([=] {
            if (ca)
            {
                sk_X509_pop_free(ca, X509_free);
            }

            if (cert)
            {
                X509_free(cert);
            }

            if (p12)
            {
                PKCS12_free(p12);
            }
        });

        p12 = d2i_PKCS12_fp(fd, NULL);
        if (!p12)
        {
            auto err = GetOpensslErr();
            WriteInfo(TraceType, "ReadPrivateKey('{0}'): d2i_PKCS12_fp failed: {1}", privateKeyFilePath, err);
            return err;
        }

        if (!PKCS12_parse(p12, nullptr, &key, &cert, &ca))
        {
            auto err = GetOpensslErr();
            WriteInfo(TraceType, "ReadPrivateKey('{0}'): PKCS12_parse failed: {1}", privateKeyFilePath, err);
            return err;
        }

        privKey = PrivKeyContext(key, privateKeyFilePath);
        WriteInfo(TraceType, "ReadPrivateKey('{0}'): success", privateKeyFilePath);
        return ErrorCodeValue::Success;
    }

    WriteInfo(TraceType, "ReadPrivateKey('{0}'): unrecognized certificate file type", privateKeyFilePath);
    return ErrorCodeValue::InvalidArgument;
}

PKCS7_UPtr LinuxCryptUtil::DeserializePkcs7(wstring const & cipherTextBase64)
{
    //LINUXTODO consider using BIO_f_base64
    ByteBuffer encryptedBuf;
    CryptoUtility::Base64StringToBytes(cipherTextBase64, encryptedBuf);
    BioUPtr memBioUPtr = ByteBufferToBioMem(encryptedBuf);

    PKCS7* p7 = NULL;
    p7 = d2i_PKCS7_bio(memBioUPtr.get(), &p7);
    PKCS7_UPtr p7UPtr(p7);
    return p7UPtr;
}

ErrorCode LinuxCryptUtil::Pkcs7Decrypt(EVP_PKEY* key, PKCS7* p7, ByteBuffer &decryptedBuf)
{
    if (!p7) {
        return ErrorCodeValue::ArgumentNull;
    }

    BioUPtr outUPtr(BIO_new(BIO_s_mem()));
    if (!PKCS7_decrypt(p7, key, NULL, outUPtr.get(), 0))
    {
        return ErrorCode::FromErrno();
    }

    decryptedBuf = BioMemToByteBuffer(outUPtr.get());
    return ErrorCodeValue::Success;
}

vector<pair<X509_NAME const*, uint64>> LinuxCryptUtil::GetPkcs7Recipients(PKCS7* pkcs7)
{
    decltype(GetPkcs7Recipients(nullptr)) results;
    auto recipientInfo = pkcs7->d.enveloped->recipientinfo;

    for (int i = 0; i < sk_PKCS7_RECIP_INFO_num(recipientInfo); i++)
    {
        auto ri = sk_PKCS7_RECIP_INFO_value(recipientInfo, i);

        auto rIssuerName = X509_NAME_oneline(ri->issuer_and_serial->issuer, nullptr, 0);
        KFinally([=] { free(rIssuerName); });
        uint64 rSerialNo = ASN1_INTEGER_get(ri->issuer_and_serial->serial);
        WriteInfo(
            TraceType,
            "GetPkcs7Recipients: recipient: issuer='{0}', serialno={1:x}",
            rIssuerName,
            rSerialNo);

        results.emplace_back(ri->issuer_and_serial->issuer, rSerialNo);
    }

    return results;
}

ErrorCode LinuxCryptUtil::LoadIssuerChain(PCCertContext certContext, std::vector<X509Context> & issuerChain)
{
    issuerChain.clear();

    auto subjectCert = certContext;
    X509_check_purpose(subjectCert, -1, 0);
    if (subjectCert->ex_flags & EXFLAG_SS) //self signed?
        return ErrorCodeValue::Success; 

    vector<X509Context> allCerts;
    for (auto const& file : GetCertFiles()) 
    {
        auto cert = LoadCertificate(file);
        if (!cert.get()) continue;

        allCerts.emplace_back(move(cert));
    }

    auto name = LinuxCryptUtil::GetSubjectName(subjectCert);
    WriteInfo(TraceType, "building issuer chain for {0}", name);

    bool shouldContinue = false;
    do
    {
        shouldContinue = false;

        for (auto & issuerCandicate : allCerts)
        {
            if (issuerCandicate && X509_check_issued(issuerCandicate.get(), subjectCert) == X509_V_OK)
            {
                name = LinuxCryptUtil::GetSubjectName(issuerCandicate.get());
                WriteInfo(TraceType, "add to issuer chain: {0}", name);
                subjectCert = issuerCandicate.get(); 
                issuerChain.emplace_back(move(issuerCandicate));

                X509_check_purpose(subjectCert, -1, 0);
                if (subjectCert->ex_flags & EXFLAG_SS) //self signed?
                   return ErrorCodeValue::Success; 

                shouldContinue = true;
                break;
            }
        }
    }
    while(shouldContinue);

    WriteError(TraceType, "partial chain: {0} found, more expected", issuerChain.size());
    return ErrorCodeValue::OperationFailed;
}

ErrorCode LinuxCryptUtil::LoadDecryptionCertificate(PKCS7* p7, wstring const & certPath, X509Context & cert) const
{
    Invariant(p7);
    cert.reset();

    auto recipients = GetPkcs7Recipients(p7);

    for (auto const& file : GetCertFiles(certPath))
    {
        cert = LoadCertificate(file);
        if (!cert) continue;

        auto certIssuerName = X509_get_issuer_name(cert.get());
        auto nameToPrint = X509_NAME_oneline(certIssuerName, nullptr, 0);
        KFinally([=] { free(nameToPrint); });
        uint64 certSerialNo = ASN1_INTEGER_get(cert->cert_info->serialNumber);
        WriteInfo(
            TraceType,
            "FindMatchingCertFile: cert: issuer='{0}', serialno={1:x}",
            nameToPrint,
            certSerialNo);

        for(auto & recipient : recipients)
        {
            if (!X509_NAME_cmp(recipient.first, certIssuerName)
                && (certSerialNo == recipient.second))
            {
                /// Found matching certificate
                return ErrorCode::Success();
            }
        }
    }

    return ErrorCodeValue::NotFound;
}

ErrorCode LinuxCryptUtil::Pkcs7Decrypt(wstring const & cipherTextBase64, wstring const & certPath, ByteBuffer& decryptedBuffer) const
{
    PKCS7_UPtr p7UPtr = DeserializePkcs7(cipherTextBase64);

    if (!p7UPtr.get()) {
        return ErrorCode::FromErrno();
    }

    X509Context x509;
    auto error = LoadDecryptionCertificate(p7UPtr.get(), certPath, x509);
    if (!error.IsSuccess()) { return error; }

    PrivKeyContext privKey;
    error = ReadPrivateKey(x509, privKey);
    if (!error.IsSuccess()) { return error; }

    return Pkcs7Decrypt(privKey.get(), p7UPtr.get(), decryptedBuffer);
}

ErrorCode LinuxCryptUtil::LogError(std::string const& functionName, std::string const& activity, ErrorCode const& ec)
{
    TraceError(
        TraceTaskCodes::Common,
        TraceType,
        "{0}() Error with error code:{1} during :{2}",
        functionName,
        ec,
        activity);
    return ec;
}

void LinuxCryptUtil::LogInfo(std::string const& functionName, std::string const& message)
{
    TraceInfo(
        TraceTaskCodes::Common,
        TraceType,
        "{0}(): {1}",
        functionName,
        message);
}

int LinuxCryptUtil::CertCheckEnhancedKeyUsage(wstring const & id, X509 *cert, LPCSTR usageIdentifier) const
{
    int authType = 0;
    char* clientIdentifier[] = {szOID_PKIX_KP_CLIENT_AUTH, "clientAuth", "TLS Web Client Authentication"};
    char* serverIdentifier[] = {szOID_PKIX_KP_SERVER_AUTH, "serverAuth", "TLS Web Server Authentication"};

    int loc;
    BUF_MEM *bptr = NULL;
    loc = X509_get_ext_by_NID(cert, NID_ext_key_usage, -1);
    X509_EXTENSION *ex = X509_get_ext(cert, loc);
    BIO *bio = BIO_new(BIO_s_mem());
    if (ex && X509V3_EXT_print(bio, ex, 0, 0))
    {
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bptr);
        vector<char> buf((bptr->length + 1) * sizeof(char));
        memcpy(buf.data(), bptr->data, bptr->length);
        buf[bptr->length] = '\0';
        for (int i = 0; i < sizeof(clientIdentifier) / sizeof(char*); i++)
        {
            if (strstr(buf.data(), clientIdentifier[i]) != 0)
            {
                authType = AUTHTYPE_CLIENT; break;
            }
        }
        for (int i = 0; i < sizeof(serverIdentifier) / sizeof(char*); i++)
        {
            if (strstr(buf.data(), serverIdentifier[i]) != 0)
            {
                authType = AUTHTYPE_SERVER; break;
            }
        }
    }
    if (!authType) {
        WriteInfo(TraceType, id, "unable to get key usage ext: {0}", GetOpensslErr().Message);
    }
    BIO_free(bio);
    return authType;
}

ErrorCode LinuxCryptUtil::VerifyCertificate(
    wstring const & id,
    X509_STORE_CTX *storeContext,
    uint x509CertChainFlags,
    bool shouldIgnoreCrlOffline,
    bool remoteAuthenticatedAsPeer,
    ThumbprintSet const & certThumbprintsToMatch,
    SecurityConfig::X509NameMap const & x509NamesToMatch,
    bool traceCert,
    wstring * nameMatched,
    CertChainErrors const * certChainErrors) const
{
    return LinuxCryptUtil::VerifyCertificate(
        id,
        X509_STORE_CTX_get_chain(storeContext),
        x509CertChainFlags,
        shouldIgnoreCrlOffline,
        remoteAuthenticatedAsPeer,
        certThumbprintsToMatch,
        x509NamesToMatch,
        traceCert,
        nameMatched,
        certChainErrors);
}

ErrorCode LinuxCryptUtil::VerifyCertificate(
    wstring const & id, 
    STACK_OF(X509)* chain,
    uint inputCertChainFlags,
    bool shouldIgnoreCrlOffline,
    bool remoteAuthenticatedAsPeer,
    ThumbprintSet const & certThumbprintsToMatch,
    SecurityConfig::X509NameMap const & x509NamesToMatch,
    bool traceCert,
    wstring * nameMatched,
    CertChainErrors const * certChainErrors) const
{
    X509 *cert = sk_X509_value(chain, 0);
    Thumbprint certThumbprint;
    auto error = certThumbprint.Initialize(cert);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, id, "failed to get certificate thumbprint: {0}", error);
        return error;
    }

    auto certChainFlags = CrlOfflineErrCache::GetDefault().MaskX509CertChainFlags(certThumbprint, inputCertChainFlags, shouldIgnoreCrlOffline);
    WriteInfo(
        TraceType, id,
        "VerifyCertificate: remoteAuthenticatedAsPeer = {0}, certThumbprintsToMatch='{1}', x509NamesToMatch='{2}', certChainFlags=0x{3:x}, maskedFlags=0x{4:x} shouldIgnoreCrlOffline={5}",
        remoteAuthenticatedAsPeer,
        (certThumbprintsToMatch.Value().size() < 100) ? certThumbprintsToMatch.ToString() : L"...",
        (x509NamesToMatch.Size() < 100) ? wformatString(x509NamesToMatch) : L"...",
        inputCertChainFlags,
        certChainFlags,
        shouldIgnoreCrlOffline);

    if (!remoteAuthenticatedAsPeer && x509NamesToMatch.IsEmpty() && certThumbprintsToMatch.Value().empty())
    {
        return ErrorCodeValue::CertificateNotMatched;
    }

    auto chainLength = sk_X509_num(chain);
    if (!chainLength)
    {
        WriteWarning(TraceType, id, "cert chain size is 0");
        return ErrorCodeValue::OperationFailed; 
    }

    X509 *issuerCert = (chainLength > 1)? sk_X509_value(chain, 1) : cert;

    Thumbprint::SPtr issuerCertThumbprint;
    if (issuerCert)
    {
        //LINUXTODO also retrieve issuer public key
        auto err = Thumbprint::Create(issuerCert, issuerCertThumbprint);
        if (!err.IsSuccess())
        {
            WriteWarning(TraceType, id, "failed to get certificate thumbprint: {0}", err);
            return err;
        }
    }

    // Trace Cert
    if (traceCert)
    {
        auto subjectName = LinuxCryptUtil::GetSubjectName(cert);
        auto issuerName = LinuxCryptUtil::GetIssuerName(cert);

        DateTime notbefore, notafter;
        LinuxCryptUtil::GetCertificateNotBeforeDate(cert, notbefore);
        LinuxCryptUtil::GetCertificateNotAfterDate(cert, notafter);

        WriteInfo(
            TraceType, id,
            "incoming cert: thumbprint = {0}, subject='{1}', issuer='{2}', issuerCertThumbprint={3}, NotBefore={4}, NotAfter={5}, chainLength = {6}",
            certThumbprint, subjectName, issuerName, *issuerCertThumbprint, DateTime(notbefore), DateTime(notafter), chainLength);
    }

    int status = 1;
    uint x509Err = 0;
    if (certChainErrors) //cert chain already verified
    {
        for(auto const & cce : *certChainErrors)
        {
            if (cce.first == X509_V_ERR_UNABLE_TO_GET_CRL)
            {
                if (!CrlOfflineErrCache::CrlCheckingEnabled(certChainFlags))
                {
                    WriteInfo(TraceType, id, "ignore X509_V_ERR_UNABLE_TO_GET_CRL, certChainFlags = {0:x}", certChainFlags);
                    continue;
                }

                if (x509Err == 0)
                {
                    // set to X509_V_ERR_UNABLE_TO_GET_CRL if no other error
                    x509Err = cce.first;
                    status = 0;
                }
                continue;
            }

            if (BenignCertErrorForAuthByThumbprintOrPubKey(cce.first))
            {
                x509Err = cce.first;
                status = 0;
                continue;
            }

            Assert::CodingError("Unexpected x509 error: {0}", cce.first);
       }
    }
    else // Need to verify cert chain
    {
        X509_STORE *store = X509_STORE_new();
        KFinally([=] { X509_STORE_free(store); });
        X509_STORE_set_default_paths(store);

        X509_STORE_CTX *ctx = X509_STORE_CTX_new();
        KFinally([=] { X509_STORE_CTX_free(ctx); });

        X509_STORE_CTX_init(ctx, store, cert, chain);
        auto param = X509_STORE_CTX_get0_param(ctx);
        LinuxCryptUtil::ApplyCrlCheckingFlag(param, certChainFlags);

    // uncomment the following if it is necessary to load CA certificates from custom locations
    //    auto rootCerts = X509StackShallowUPtr(sk_X509_new_null()); //Use "Shallow" as X509 object is managed by X509Context
    //    vector<X509Context> rootCertContexts;
    //    if (!SecurityConfig::GetConfig().X509CACertFolder.empty()) {
    //        vector<string> rootCertFiles = GetCACertFiles(SecurityConfig::GetConfig().X509CACertFolder);
    //        rootCertContexts.reserve(rootCertFiles.size());
    //        for(auto const & rootCertFile : rootCertFiles) {
    //            auto rootCertUPtr = LoadCertificate(rootCertFile);
    //            if (rootCertUPtr) {
    //                sk_X509_push(rootCerts.get(), rootCertUPtr.get());
    //                rootCertContexts.emplace_back(move(rootCertUPtr));
    //            }
    //        }
    //
    //        if (sk_X509_num(rootCerts.get()) > 0) X509_STORE_CTX_trusted_stack(ctx, rootCerts.get());
    //    }

        status = X509_verify_cert(ctx);
        if (status == 1)
        {
            WriteInfo(TraceType, id, "certificate chain validation succeeded");
        }
        else
        {
            x509Err = X509_STORE_CTX_get_error(ctx);
            WriteInfo(
                TraceType, id,
                "certificate chain validation failed:{0}:{1}",
                x509Err,
                X509_verify_cert_error_string(x509Err));
        }
    }

    CrlOfflineErrCache::GetDefault().Test_CheckCrl(certChainFlags, x509Err);

    bool onlyCrlOfflineEncountered = (x509Err == X509_V_ERR_UNABLE_TO_GET_CRL);
    if (onlyCrlOfflineEncountered)
    {
        if (shouldIgnoreCrlOffline)
        {
            WriteWarning(TraceType, id, "ignore error {0}:certificate revocation list offline", x509Err);
            status = 1;
        }
    }
    else
    {
        CrlOfflineErrCache::GetDefault().TryRemoveError(certThumbprint, certChainFlags);
    }

    int certChainStatus = (status == 1) ? 0 : -1;

    if (remoteAuthenticatedAsPeer && ((certChainStatus == 0) ||(BenignCertErrorForAuthByThumbprintOrPubKey(x509Err))))
    {
        if (onlyCrlOfflineEncountered)
        {
            //We only care about CRL offline error for certificates in our allow list
            CrlOfflineErrCache::GetDefault().AddError(certThumbprint, cert, certChainFlags);
        }

        WriteInfo(TraceType, id, "VerifyCertificate: complete as remote is authenticated as peer and the chain has no fatal error");
        return ErrorCodeValue::Success;
    }

    // Match Thumbprint List
    auto match = certThumbprintsToMatch.Value().find(certThumbprint);
    if (match != certThumbprintsToMatch.Value().cend())
    {
        if (onlyCrlOfflineEncountered)
        {
            //We only care about CRL offline error for certificates in our allow list
            CrlOfflineErrCache::GetDefault().AddError(certThumbprint, cert, certChainFlags);
        }

        WriteInfo(
            TraceType, id,
            "incoming cert matched thumbprint {0}, CertChainShouldBeVerified={1}, certChainErrorStatus=0x{2:x}",
            certThumbprint, match->CertChainShouldBeVerified(), (unsigned int)certChainStatus);
        status = match->CertChainShouldBeVerified()? certChainStatus : 0;
        if (status == 0) return ErrorCodeValue::Success;
    }

    if (!onlyCrlOfflineEncountered && certChainStatus) return ErrorCodeValue::OperationFailed;

    // Match Name-Issuer List
    int authType = CertCheckEnhancedKeyUsage(id, cert, szOID_PKIX_KP_CLIENT_AUTH);
    if (authType == AUTHTYPE_CLIENT)
    {
        string certCnA;
        error = LinuxCryptUtil::GetCommonName(cert, certCnA);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, id, "GetCertificateCommonName failed: ErrorCode - {0}", error);
            return ErrorCodeValue::CertificateNotFound;
        }

        auto certCn = StringUtility::Utf8ToUtf16(certCnA);

        SecurityConfig::X509NameMapBase::const_iterator matched;
        if (x509NamesToMatch.Match(certCn, issuerCertThumbprint, matched))
        {
            if (matched->second.IsEmpty())
            {
                WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_CLIENT", matched->first);
            }
            else
            {
                WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_CLIENT, issuer pinned to {1}", matched->first, matched->second);
            }

            if (nameMatched) *nameMatched = matched->first;

            if (onlyCrlOfflineEncountered)
            {
                //We only care about CRL offline error for certificates in our allow list
                CrlOfflineErrCache::GetDefault().AddError(certThumbprint, cert, certChainFlags);
            }

            if (certChainStatus) return ErrorCodeValue::OperationFailed; 

            return ErrorCodeValue::Success;
        }
    }
    else //check as sever cert
    {
        for (auto matchTarget = x509NamesToMatch.CBegin(); matchTarget != x509NamesToMatch.CEnd(); ++matchTarget)
        {
            if (!SecurityConfig::X509NameMap::MatchIssuer(issuerCertThumbprint, matchTarget))
                continue;

            auto matchedName = StringUtility::Utf16ToUtf8(matchTarget->first);
            auto retval = X509_check_host(cert, matchedName.c_str(), matchedName.size(), 0, nullptr);
            if (retval == 1)
            {
                if (matchTarget->second.IsEmpty())
                {
                    WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_SERVER", matchTarget->first);
                }
                else
                {
                    WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_SERVER, issuer pinned to {1}", matchTarget->first, matchTarget->second);
                }

                if (nameMatched) *nameMatched = matchTarget->first;

                if (onlyCrlOfflineEncountered)
                {
                    //We only care about CRL offline error for certificates in our allow list
                    CrlOfflineErrCache::GetDefault().AddError(certThumbprint, cert, certChainFlags);
                }

                if (certChainStatus) return ErrorCodeValue::OperationFailed; 

                return ErrorCodeValue::Success;
            }
        }
    }

    return ErrorCodeValue::CertificateNotMatched;
}

void LinuxCryptUtil::ApplyCrlCheckingFlag(X509_VERIFY_PARAM* param, DWORD crlCheckingFlag)
{
    if (!CrlOfflineErrCache::CrlCheckingEnabled(crlCheckingFlag))
    {
        WriteInfo(TraceType, "crl checking is disabled");
        X509_VERIFY_PARAM_clear_flags(param, X509_V_FLAG_CRL_CHECK);
        X509_VERIFY_PARAM_clear_flags(param, X509_V_FLAG_CRL_CHECK_ALL);
        return;
    }

    WriteInfo(TraceType, "X509_V_FLAG_CRL_CHECK set");
    X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK); //LINUXTODO consider support X509_V_FLAG_CRL_CHECK_ALL?
}

bool LinuxCryptUtil::BenignCertErrorForAuthByThumbprintOrPubKey(int err)
{
    return
        (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) ||
        (err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN) ||
        (err == X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE) ||
        (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT) ||
        (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
}
