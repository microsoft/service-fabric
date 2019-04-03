// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <vector>
#include <string>

typedef unsigned int        uint;

#ifdef PLATFORM_UNIX
#include <stdint.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
typedef int16_t int16;
typedef uint16_t uint16;

typedef X509 CertContext;
struct CERT_CHAIN_CONTEXT {};
typedef CERT_CHAIN_CONTEXT * PCERT_CHAIN_CONTEXT;
typedef CERT_CHAIN_CONTEXT const * PCCERT_CHAIN_CONTEXT;

#define CertFreeCertificateChain(context)

#else
typedef __int32             int32;
typedef unsigned __int32    uint32;
typedef __int64             int64;
typedef unsigned __int64    uint64;
typedef __int16             int16;
typedef unsigned __int16    uint16;
typedef CERT_CONTEXT CertContext;
#endif

typedef std::vector<char> cbuffer;

#ifndef __RPCNDR_H__ // rpcndr.h defines byte
    typedef unsigned __int8 byte;
#endif

namespace Common
{
    typedef std::vector<std::wstring> StringCollection;
    typedef std::map<std::wstring, std::wstring> StringMap;

    typedef std::vector<BYTE> ByteBuffer;
    typedef std::unique_ptr<std::vector<BYTE>> ByteBufferUPtr;
    typedef std::shared_ptr<std::vector<BYTE>> ByteBufferSPtr;

    bool operator == (ByteBuffer const & b1, ByteBuffer const & b2);
    bool operator != (ByteBuffer const & b1, ByteBuffer const & b2);
    bool operator < (ByteBuffer const & b1, ByteBuffer const & b2);

    ByteBuffer StringToByteBuffer(std::wstring const& str);
    std::wstring ByteBufferToString(ByteBuffer const & buffer);

    typedef CertContext* PCertContext;

#ifdef PLATFORM_UNIX

    typedef PCertContext PCCertContext;

    template <typename T>
    class CryptObjContext
    {
    public:
        CryptObjContext(T* obj = nullptr, std::string const & filePath = "");
        CryptObjContext(CryptObjContext && other);
        virtual ~CryptObjContext();

        CryptObjContext& operator=(CryptObjContext&& other);

        operator bool() const { return obj_ != nullptr; }

        T* operator->() { return obj_; }
        T const* operator->() const { return obj_; }

        T* get() const { return obj_; }
        T* release();
        void reset();

        std::string const & FilePath() const { return filePath_; }
        void SetFilePath(std::string const & filePath) { filePath_ = move(filePath); }
        
        std::string const & PrivateKeyFilePath() const { return privateKeyFilePath_; }
        void SetPrivateKeyFilePath(std::string const & privateKeyFilePath) { privateKeyFilePath_ = privateKeyFilePath; }

        static uint64 ObjCount();

    protected:
        T* obj_ = nullptr;
        void (*deleter_)(T*) = nullptr;
        std::string filePath_;
        std::string privateKeyFilePath_;
    };

    class X509Context : public CryptObjContext<X509>
    {
    public:
        X509Context(X509* obj = nullptr, std::string const & filePath = "");
    };

    using CertContextUPtr = X509Context;

    class PrivKeyContext : public CryptObjContext<EVP_PKEY>
    {
    public:
        PrivKeyContext(EVP_PKEY* obj = nullptr, std::string const & filePath = "");
    };

    bool operator == (timespec const & t1, timespec const & t2);
    bool operator != (timespec const & t1, timespec const & t2);
    bool operator <= (timespec const & t1, timespec const & t2);
    bool operator >= (timespec const & t1, timespec const & t2);
    bool operator < (timespec const & t1, timespec const & t2);
    bool operator > (timespec const & t1, timespec const & t2);
    bool timespec_is_zero(timespec const & ts);

#else
    typedef CertContext const* PCCertContext;

    struct CertContextDeleter
    {
        void operator()(PCCertContext certContext) const
        {
            if (certContext) CertFreeCertificateContext(certContext);
        }
    };

    typedef std::unique_ptr<const CertContext, CertContextDeleter> CertContextUPtr;

    struct CertStoreDisposer
    {
        void operator()(void * certStore) const
        {
            if (certStore) CertCloseStore(certStore, 0);
        }
    };

    typedef std::unique_ptr<void, CertStoreDisposer> CertStoreUPtr;

#endif
    // This type is needed due to the complexity of implementing CertContextSPtr on Linux
    typedef std::shared_ptr<CertContextUPtr> CertContextUPtrSPtr; 

    typedef std::vector<CertContextUPtr> CertContexts; // sorted by expiration in descending order

    struct CertChainContextDeleter
    {
        void operator()(CERT_CHAIN_CONTEXT const * context) const
        {
            if (context)
            {
                CertFreeCertificateChain(context);
            }
        }
    };

    typedef std::unique_ptr<const CERT_CHAIN_CONTEXT, CertChainContextDeleter> CertChainContextUPtr;

    template <class T> struct literal_holder
    {
        typedef T const * const_iterator;
        static const bool is_string_literal = 
            std::is_same<T, char>::value || std::is_same<T, wchar_t>::value ;
        static const size_t adjustment =
            is_string_literal ? 1 : 0;

        literal_holder()
            : begin_(nullptr), end_(nullptr)
        {
        }

        literal_holder(T const * begin, T const * end)
            : begin_(begin), end_(end)
        {
        }

        template <size_t N> literal_holder(T const (&a)[N])
            : begin_(a), end_(a + N - adjustment)
        {}

        T const * begin () const { return begin_; }
        T const * cbegin() const { return begin_; }
        T const * end() const { return end_; }
        T const * cend() const { return end_; }
        size_t size() const { return end() - begin(); }

        void move( T const * begin, T const * end ) { begin_ = begin; end_ = end; }

        explicit operator bool() const
        {
            return (begin_ != nullptr ? 1 : 0);
        }

    private:
        T const * begin_;
        T const * end_;
    };

    typedef literal_holder<char> StringLiteral;
    typedef literal_holder<wchar_t> WStringLiteral;

    template <class T> class is_literal_holder : public std::integral_constant<bool, false> {};
    template <class T> class is_literal_holder<literal_holder<T>> : public std::integral_constant<bool, true> {};

    namespace detail
    {
        struct NilType
        {
        };

        template <typename T>
        struct IsNilType
        {
            static const bool value = false;
        };

        template <>
        struct IsNilType<NilType>
        {
            static const bool value = true;
            typedef void True;
        };
    }
        
    template <typename T, class Enable = void>
    struct TestExists
    {
        typedef T Type;
    };
        
    template <typename T>
    struct TestExists<T, typename std::enable_if<!std::is_class<T>::value >::type>
    {
        typedef detail::NilType Type;
    };
}
