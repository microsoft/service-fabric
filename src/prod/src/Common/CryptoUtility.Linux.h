// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    struct PKCS7_Deleter
    {
        void operator()(PKCS7* pkcs7) const
        {
            if (pkcs7) PKCS7_free(pkcs7);
        }
    };

    typedef std::unique_ptr<PKCS7, PKCS7_Deleter> PKCS7_UPtr;

    struct X509StackDeleter
    {
        void operator()(STACK_OF(X509)* x509Stack) const
        {
            if (x509Stack) sk_X509_pop_free(x509Stack, X509_free);
        }
    };

    typedef std::unique_ptr<STACK_OF(X509), X509StackDeleter> X509StackUPtr;

    struct X509StackShallowDeleter //only free stack, not objects in stack
    {
        void operator()(STACK_OF(X509)* x509Stack) const
        {
            if (x509Stack) sk_X509_free(x509Stack); 
        }
    };

    typedef std::unique_ptr<STACK_OF(X509), X509StackShallowDeleter> X509StackShallowUPtr;

    class LinuxCryptUtil : TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    public:
        LinuxCryptUtil();

        ErrorCode InstallCertificate(X509Context & x509, wstring const & filepath) const;
        ErrorCode UninstallCertificate(X509Context const & x509, bool removeKey) const;
        ErrorCode InstallCoreFxCertificate(std::string const & userName, X509Context const & x509) const;
        X509Context LoadCertificate(std::string const& filepath) const;
        ErrorCode LoadIssuerChain(PCCertContext certContext, std::vector<X509Context> & issuerChain);

        std::vector<std::string> GetCertFiles(std::wstring const& x509Path = L"") const;

        ErrorCode ReadPrivateKey(X509Context const & x509, PrivKeyContext & privKey) const;
        ErrorCode ReadPrivateKey(std::string const & privKeyFilePath, PrivKeyContext & privKey) const;
        void ReadPrivateKeyPaths(X509Context const & x509, std::vector<string> & paths) const;
        ErrorCode ReadCoreFxPrivateKeyPath(X509Context const & x509, std::string const & userName, string & path) const;

        ErrorCode GetCommonName(PCCertContext x, std::string& cn) const;
        ErrorCode GetSHA1Thumbprint(PCCertContext x, ByteBuffer &md_valueV) const;
        std::string GetSubjectName(PCCertContext x) const;
        std::string GetIssuerName(PCCertContext x) const;
        std::vector<std::string> GetSubjectAltName(PCCertContext x) const;

        static ErrorCode GetKeyValPairsFromSubjectName(std::string const& sn, std::map<std::string, std::string> &snKV);
        static ErrorCode GetKeyValPairsFromSubjectName(std::wstring const& sn, std::map<std::string, std::string> &snKV);

        ErrorCode GetCertificateNotAfterDate(X509* x, DateTime& t) const;
        ErrorCode GetCertificateNotBeforeDate(X509* x, DateTime& t) const;

        // Creating new certificate
        ErrorCode CreateSelfSignedCertificate(
            std::wstring const & subjectName,
            const std::vector<std::wstring> *subjectAltNames,
            Common::DateTime const& expiration,
            std::wstring const & keyContainerName,
            Common::X509Context & cert) const;

        template <typename TK, typename TV>
        static bool MapCompare(const std::map<TK, TV>& lhs, const std::map<TK, TV>& rhs)
        {
            if (lhs.size() != rhs.size()) { return false; }

            return std::equal(lhs.begin(), lhs.end(), rhs.begin());
        }

        // PKCS7 
        ErrorCode Pkcs7Encrypt(std::vector<PCCertContext> const& recipientCertificates, ByteBuffer const& plainTextBuffer, std::wstring & cipherTextBase64) const;
        ErrorCode Pkcs7Decrypt(std::wstring const & cipherTextBase64, std::wstring const & certPath, ByteBuffer& decryptedTextBuffer) const;
        ErrorCode Pkcs7Sign(X509Context const & cert, BYTE const* input, size_t inputLen, ByteBuffer & output) const;

        // Hashing
        // LINUXTODO support hash algorithm parameter
        ErrorCode ComputeHash(ByteBuffer const & input, ByteBuffer & output) const;
        ErrorCode ComputeHmac(ByteBuffer const & data, ByteBuffer const & key, ByteBuffer & output) const;

        ErrorCode DeriveKey(ByteBuffer const & input, ByteBuffer & key) const;

        ErrorCode GetOpensslErr(uint64 err = 0) const;

        static const int AUTHTYPE_CLIENT = 1;
        static const int AUTHTYPE_SERVER = 2;
        int CertCheckEnhancedKeyUsage(std::wstring const & traceId, X509* certContext, LPCSTR usageIdentifier) const;

        using CertChainErrors = std::vector<std::pair<uint/* cert chain error */, uint/* depth */>>;

        ErrorCode VerifyCertificate(
            std::wstring const & id,
            X509_STORE_CTX * storeContext,
            uint x509CertChainFlags,
            bool shouldIgnoreCrlOffline,
            bool remoteAuthenticatedAsPeer,
            ThumbprintSet const & thumbprintsToMatch,
            SecurityConfig::X509NameMap const & x509NamesToMatch,
            bool traceCert,
            wstring * nameMatched,
            CertChainErrors const * certChainErrors = nullptr) const;

        ErrorCode VerifyCertificate(
            std::wstring const & id,
            STACK_OF(X509)* x509Chain,
            uint x509CertChainFlags,
            bool shouldIgnoreCrlOffline,
            bool remoteAuthenticatedAsPeer,
            ThumbprintSet const & thumbprintsToMatch,
            SecurityConfig::X509NameMap const & x509NamesToMatch,
            bool traceCert,
            wstring * nameMatched,
            CertChainErrors const * certChainErrors = nullptr) const;

        static void ApplyCrlCheckingFlag(X509_VERIFY_PARAM* param, uint crlCheckingFlag);

        static bool BenignCertErrorForAuthByThumbprintOrPubKey(int err);

    private:
        static bool Startup();

        ErrorCode Asn1TimeToDateTime(ASN1_TIME *asnt, DateTime& t) const;

        static std::string X509NameToString(X509_NAME* xn);
        static ErrorCode SetSubjectName(X509_NAME * xn, std::wstring const & subjectName);
        static ErrorCode SetSubjectName(X509 * x, std::wstring const & subjectName);
        static ErrorCode SetAltSubjectName(X509 * x, const std::vector<std::wstring> *subjectAltNames);
        static ErrorCode SignCertificate(X509 * x, EVP_PKEY* pkey);
        static ErrorCode SetCertificateDates(X509* cert, Common::DateTime const& expiration);

        ErrorCode WriteX509ToFile(X509Context const & x509) const;

        static PrivKeyContext CreatePrivateKey(int keySizeInBits = 2048, int exponent = 65537);
        static ErrorCode WritePrivateKey(std::string const & privKeyFilePath, EVP_PKEY* pkey);

        static ErrorCode SerializePkcs(PKCS7* p7, std::wstring & out);
        static PKCS7_UPtr DeserializePkcs7(std::wstring const & cipherTextBase64);
        ErrorCode Pkcs7Encrypt(ByteBuffer const& in, STACK_OF(X509) *recips, std::wstring &out) const;
        static ErrorCode Pkcs7Decrypt(EVP_PKEY* key, PKCS7* p7, ByteBuffer &decryptedBuf);
        ErrorCode LoadDecryptionCertificate(PKCS7* p7, std::wstring const & certPath, X509Context & x509) const;
        static std::vector<std::pair<X509_NAME const*, uint64>> GetPkcs7Recipients(PKCS7* pkcs7);

        static ErrorCode LogError(std::string const& functionName, std::string const& activity, ErrorCode const& ec);
        static void LogInfo(std::string const& functionName, std::string const& message);

        static int GetDataFromP7(PKCS7* p7, ByteBuffer & out);

        static void EnsureFolder(std::wstring const & folderw);
        static void EnsureFolder(std::string const & folder);
    };
}
