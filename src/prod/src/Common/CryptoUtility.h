// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct X509Default
    {
        static X509StoreLocation::Enum StoreLocation();
        static enum FABRIC_X509_STORE_LOCATION StoreLocation_Public();
        static wstring const & StoreName();
    };

    class CryptoUtility : TextTraceComponent<TraceTaskCodes::Common>
    {
    public:
        static ErrorCode GetCertificate(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            _Out_ CertContextUPtr & certContext);

        static ErrorCode GetCertificate(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            DWORD storeOpenFlag,
            std::shared_ptr<X509FindValue> const & findValue,
            _Out_ CertContextUPtr & certContext);

        static ErrorCode GetCertificate(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            _Out_ CertContexts & certContexts);

        static ErrorCode GetCertificate(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            DWORD storeOpenFlag,
            std::shared_ptr<X509FindValue> const & findValue,
            _Out_ CertContexts & certContexts);

        static ErrorCode GetCertificate(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::wstring const & findType,
            std::wstring const & findValue,
            _Out_ CertContextUPtr & certContext);

        static ErrorCode GetCertificate(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::wstring const & findType,
            std::wstring const & findValue,
            _Out_ CertContexts & certContexts);

        static ErrorCode GetCertificate(
            std::wstring const & certFilePath,
            _Out_ CertContextUPtr & certContext);

        static ErrorCode GetCertificateChain(
            PCCertContext certContext,
            _Out_ CertChainContextUPtr & certChain);

        static ErrorCode GetCertificateIssuerPubKey(
            PCCertContext certContext,
            _Out_ X509Identity::SPtr & issuerPubKey);

        static bool IsCertificateSelfSigned(PCCERT_CHAIN_CONTEXT certChain);

        static bool GetCertificateIssuerChainIndex(PCCERT_CHAIN_CONTEXT certChain, size_t & issuerCertIndex);
        static PCCertContext GetCertificateIssuer(PCCERT_CHAIN_CONTEXT certChain);

        static Common::ErrorCode GetCertificateExpiration(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode GetCertificateExpiration(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::wstring const & findType,
            std::wstring const & findValue,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode GetCertificateExpiration(
            CertContextUPtr const & cert,
            _Out_ Common::DateTime & expiration);

        static Common::ErrorCode GetCertificateCommonName(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            _Out_ std::wstring & commonName);

        static Common::ErrorCode GetCertificateCommonName(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::wstring const & findType,
            std::wstring const & findValue,
            _Out_ std::wstring & commonName);

        static Common::ErrorCode GetCertificateCommonName(
            PCCertContext certContext,
            _Out_ std::wstring & commonName);

        static Common::ErrorCode GetCertificatePrivateKeyFileName(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            std::wstring & privateKeyFileName);

        static Common::ErrorCode GetCertificatePrivateKeyFileName(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            std::vector<std::wstring> & privateKeyFileName);

        static Common::ErrorCode GetCertificateIssuerThumbprint(
            PCCERT_CONTEXT certContext,
            Thumbprint::SPtr & issuerThumbprint);

        static Common::ErrorCode CryptoUtility::GenerateExportableKey(
            wstring const & keyContainerName,
            _Out_ SecureString & key);

        static Common::ErrorCode CryptoUtility::GenerateExportableKey(
            wstring const & keyContainerName,
            bool fMachineKeyset,
            _Out_ SecureString & key);

        static Common::ErrorCode CryptoUtility::CreateCertFromKey(
            ByteBuffer const & buffer,
            wstring const & key,
            _Out_ Common::CertContextUPtr & cert);

        static Common::ErrorCode CryptoUtility::ImportCertKey(
            wstring const& keyContainerName,
            wstring const& key);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            std::wstring const & keyContainerName,
            _Out_ Common::CertContextUPtr & cert);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            std::wstring const & keyContainerName,
            bool fMachineKeyset,
            _Out_ Common::CertContextUPtr & cert);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            _Out_ Common::CertContextUPtr & cert);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            Common::DateTime expiration,
            _Out_ Common::CertContextUPtr & cert);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            const std::vector<std::wstring> *subjectAltNames,
            DateTime expiration,
            std::wstring const & keyContainerName,
            _Out_ CertContextUPtr & cert);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            const std::vector<std::wstring> *subjectAltNames,
            DateTime expiration,
            std::wstring const & keyContainerName,
            bool fMachineKeyset,
            _Out_ CertContextUPtr & cert);

        static Common::ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            Common::DateTime expiration,
            std::wstring const & keyContainerName,
            _Out_ Common::CertContextUPtr & cert);

        static Common::ErrorCode InstallCertificate(
            CertContextUPtr & certContext,
            X509StoreLocation::Enum certificateStoreLocation = X509Default::StoreLocation(),
            std::wstring const & certificateStoreName = X509Default::StoreName());

        static Common::ErrorCode UninstallCertificate(
            X509StoreLocation::Enum certStoreLocation,
            std::wstring const & certStoreName,
            std::shared_ptr<X509FindValue> const & findValue,
            std::wstring const & keyContainerFilePath = L"");

        static Common::ErrorCode EncryptText(
            std::wstring const & plainText,
            std::wstring const & certThumbPrint,
            std::wstring const & certStoreName,
            X509StoreLocation::Enum certStoreLocation,
            _In_opt_ LPCSTR algorithmOid,
            _Out_ std::wstring & encryptedText);

        static Common::ErrorCode EncryptText(
            std::wstring const & plainText,
            std::wstring const & certFilePath,
            _In_opt_ LPCSTR algorithmOid,
            _Out_ std::wstring & encryptedText);

        static Common::ErrorCode EncryptText(
            std::wstring const & plainText,
            PCCertContext certContext,
            _In_opt_ LPCSTR algorithmOid,
            _Out_ std::wstring & encryptedText);

        static Common::ErrorCode DecryptText(
            std::wstring const & encryptedText,
            _Out_ SecureString & decryptedText);

        static Common::ErrorCode DecryptText(
            std::wstring const & encryptedText,
            X509StoreLocation::Enum certStoreLocation,
            _Out_ SecureString & decryptedText);

        static Common::ErrorCode SignMessage(
            BYTE const* inputBuffer,
            size_t inputSize,
            char const* algOID,
            CertContextUPtr const & certContext,
            _Out_ ByteBuffer & signedMessage);

        static Common::ErrorCode SignMessage(
            std::wstring const & message,
            CertContextUPtr const & certContext,
            _Out_ ByteBuffer & signedMessage);

        #if !defined(PLATFORM_UNIX)
        static Common::ErrorCode VerifyEmbeddedSignature(std::wstring const & fileName);
        #endif

        static Common::ErrorCode CreateAndACLPasswordFile(std::wstring const & passwordFilePath, SecureString & password, vector<wstring> const & machineAccountNamesForACL);
        static Common::ErrorCode GenerateRandomString(_Out_ SecureString & password);
        static void Base64StringToBytes(std::wstring const & encryptedValue, ByteBuffer & buffer);
        static std::wstring BytesToBase64String(BYTE const* bytesToEncode, unsigned int length);
        static std::wstring CertToBase64String(CertContextUPtr const & cert);

    private:
        static bool IsBase64(WCHAR c);
        static Common::ErrorCode GetKnownFolderName(int csidl, std::wstring const & relativePath, std::wstring & folderName);

        #if !defined(PLATFORM_UNIX)
        static Common::ErrorCode GetCertificatePrivateKeyFileName(
            CertContextUPtr const & certContext, 
            std::wstring & privateKeyFileName);
        #endif
        static Common::ErrorCode GetCertificateByExtension(
            HCERTSTORE certStore,
            std::shared_ptr<X509FindValue> const & findValue,
            PCCertContext pPrevCertContext,
            PCCertContext & pCertContext);


        static const std::string BASE64_CHARS;

#if defined(PLATFORM_UNIX)

        static const char alphanum[];

        static ErrorCode GetAllCertificates(
            X509StoreLocation::Enum certificateStoreLocation,
            std::wstring const & certificateStoreName,
            DWORD storeOpenFlag,
            _Out_ std::vector<std::string> & certFiles);

        static bool IsMatch(
            PCCertContext pcertContext,
            std::shared_ptr<X509FindValue> const & findValue);
#else
        static PCCertContext GetCertificateFromStore(
            HCERTSTORE certStore,
            std::shared_ptr<X509FindValue> const & findValue,
            PCCertContext pPrevCertContext);
#endif
    };

    class CrlOfflineErrCache : public enable_shared_from_this<CrlOfflineErrCache>
    {
    public:
        static CrlOfflineErrCache & GetDefault();
        CrlOfflineErrCache();

        static bool CrlCheckingEnabled(uint certChainFlags);

        uint MaskX509CertChainFlags(Thumbprint const & cert, uint flags, bool shouldIgnoreCrlOffline) const;
        void AddError(Thumbprint const & cert, CertContext const* certContext, uint certChainFlagsUsed);
        void TryRemoveError(Thumbprint const & cert, uint certChainFlagsUsed);
        void Update(Thumbprint const & cert, CertContext const* certContext, uint certChainFlagsUsed, bool onlyCrlOfflineEncountered, bool matched);

        using HealthReportCallback = std::function<void(size_t errCount, std::wstring const & description)>;
        void SetHealthReportCallback(HealthReportCallback const & callback);

        struct ErrorRecord
        {
            ErrorRecord(CertContext const * cert, uint certChainFlagsUsed, StopwatchTime time);

            std::shared_ptr<const CertContext> certContext_;
            uint certChainFlagsUsed_;
            StopwatchTime failureTime_;
        };

        using CacheMap = std::map<Thumbprint, ErrorRecord>;

        CacheMap Test_GetCacheCopy() const;
        void Test_Reset();
        void Test_CheckCrl(uint certChainFlags, uint & errorStatus);

    private:
        void PurgeExpired_LockHeld(StopwatchTime now);
        void GetHealthReport_LockHeld(size_t & errCount, std::wstring & description); 
        void ReportHealth_LockHeld(size_t errCount, std::wstring const & description);
        void CleanUp();
        void ScheduleCleanup();

        void RecheckCrl(Thumbprint const & thumbprint, ErrorRecord const & errorRecord);

        mutable RwLock lock_;
        CacheMap cache_;
        bool cleanupActive_ = false;
        HealthReportCallback healthReportCallback_;

        // Test only
        StopwatchTime testStartTime_;
        StopwatchTime testEndTime_;
    };

    class InstallTestCertInScope
    {
        DENY_COPY(InstallTestCertInScope)

    public:
        InstallTestCertInScope(
            std::wstring const & subjectName = L"", // a guid common name will be generated
            const std::vector<std::wstring> *subjectAltNames = nullptr, // No Alt name extension added if it is null or empty.
            TimeSpan expiration = DefaultCertExpiry(),
            std::wstring const & storeName = X509Default::StoreName(),
            std::wstring const & keyContainerName = L"",
            X509StoreLocation::Enum storeLocation = X509Default::StoreLocation());

        InstallTestCertInScope(
            bool install,
            std::wstring const & subjectName = L"", // a guid common name will be generated
            const std::vector<std::wstring> *subjectAltNames = nullptr, // No Alt name extension added if it is null or empty.
            TimeSpan expiration = DefaultCertExpiry(),
            std::wstring const & storeName = X509Default::StoreName(),
            std::wstring const & keyContainerName = L"",
            X509StoreLocation::Enum storeLocation = X509Default::StoreLocation());

        ~InstallTestCertInScope();

        ErrorCode Install();
        void Uninstall(bool deleteKeyContainer = true);
        void DisableUninstall();

        PCCertContext CertContext() const;

#ifdef PLATFORM_UNIX
        X509Context const & X509ContextRef() const;
#endif

        Thumbprint::SPtr const & Thumbprint() const;
        std::wstring const & SubjectName() const;
        std::wstring const & StoreName() const;
        X509StoreLocation::Enum StoreLocation() const;

        CertContextUPtr DetachCertContext();
        CertContextUPtr const & GetCertContextUPtr() const;

        static TimeSpan DefaultCertExpiry();

    private:
        std::wstring const subjectName_;
        std::wstring const storeName_;
        X509StoreLocation::Enum const storeLocation_;
        CertContextUPtr certContext_;
        std::wstring keyContainerFilePath_;
        Thumbprint::SPtr thumbprint_;
        bool uninstallOnDestruction_;
    };
}
