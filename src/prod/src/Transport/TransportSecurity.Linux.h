// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*
 * =====================================================================================
 *
 *       Filename:  TransportSecurity.Linux.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/23/2016 10:34:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#pragma once

#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)

#define SECPKG_CRED_INBOUND         0x00000001
#define SECPKG_CRED_OUTBOUND        0x00000002

#define SEC_E_OK 0
#define SEC_I_COMPLETE_NEEDED ((HRESULT)0x00090313L)
#define SEC_I_CONTINUE_NEEDED ((HRESULT)0x00090312L)

#define ASC_REQ_MUTUAL_AUTH             0x00000002
#define ASC_REQ_REPLAY_DETECT           0x00000004
#define ASC_REQ_CONFIDENTIALITY         0x00000010
#define ASC_REQ_ALLOCATE_MEMORY         0x00000100
#define ASC_REQ_STREAM                  0x00010000
#define ASC_REQ_INTEGRITY               0x00020000

#define ISC_REQ_MUTUAL_AUTH             0x00000002
#define ISC_REQ_REPLAY_DETECT           0x00000004
#define ISC_REQ_CONFIDENTIALITY         0x00000010
#define ISC_REQ_ALLOCATE_MEMORY         0x00000100
#define ISC_REQ_STREAM                  0x00008000
#define ISC_REQ_INTEGRITY               0x00010000

#define CRYPT_E_REVOCATION_OFFLINE ((HRESULT)0x80092013L)
#define SEC_E_CERT_UNKNOWN ((HRESULT)0x80090327L)

typedef Common::PrivKeyContext CredHandle;
typedef CredHandle* PCredHandle;

inline void SecInvalidateHandle(PCredHandle)
{
}

struct SSL_Deleter
{
    void operator()(SSL* ssl) const
    {
        if (ssl) SSL_free(ssl);
    }
};

typedef std::unique_ptr<SSL, SSL_Deleter> SslUPtr; 

//typedef struct {} SecPkgContext_StreamSizes;

struct CtxtHandle
{
    CtxtHandle() : valid_(false)
    {
    }

    bool valid_; 
};

typedef CtxtHandle* PCtxtHandle; 
typedef CtxtHandle const* PCCtxtHandle; 

inline void SecInvalidateHandle(PCtxtHandle ctxtHandle)
{
    ctxtHandle->valid_ = false;
}

inline bool SecIsValidHandle(PCCtxtHandle ctxtHandle)
{
    return ctxtHandle->valid_; 
}

inline void DeleteSecurityContext(PCtxtHandle)
{
}

#define SECBUFFER_VERSION           0
#define SECBUFFER_TOKEN             2   // Security token

inline void FreeContextBuffer(char* buf)
{
    delete buf;
}

struct SecPkgContext_NegotiationInfo {};
struct SecPkgContext_StreamSizes {};
struct SecPkgContext_Sizes {};

struct GENERIC_MAPPING {};
typedef GENERIC_MAPPING * PGENERIC_MAPPING;

