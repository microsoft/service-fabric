// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

//#include "pal/pal_capi.h"

#ifdef __cplusplus
extern "C" {
#endif

// 984
typedef struct _CRYPTOAPI_BLOB {
                            DWORD   cbData;
    __field_bcount(cbData)  BYTE    *pbData;
} CRYPT_INTEGER_BLOB, *PCRYPT_INTEGER_BLOB,
CRYPT_UINT_BLOB, *PCRYPT_UINT_BLOB,
CRYPT_OBJID_BLOB, *PCRYPT_OBJID_BLOB,
CERT_NAME_BLOB, *PCERT_NAME_BLOB,
CERT_RDN_VALUE_BLOB, *PCERT_RDN_VALUE_BLOB,
CERT_BLOB, *PCERT_BLOB,
CRL_BLOB, *PCRL_BLOB,
DATA_BLOB, *PDATA_BLOB,
CRYPT_DATA_BLOB, *PCRYPT_DATA_BLOB,
CRYPT_HASH_BLOB, *PCRYPT_HASH_BLOB,
CRYPT_DIGEST_BLOB, *PCRYPT_DIGEST_BLOB,
CRYPT_DER_BLOB, *PCRYPT_DER_BLOB,
CRYPT_ATTR_BLOB, *PCRYPT_ATTR_BLOB;

// 1948
typedef struct _CRYPT_BIT_BLOB {
    DWORD   cbData;
    BYTE    *pbData;
    DWORD   cUnusedBits;
} CRYPT_BIT_BLOB, *PCRYPT_BIT_BLOB;

typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
    LPSTR               pszObjId;
    CRYPT_OBJID_BLOB    Parameters;
} CRYPT_ALGORITHM_IDENTIFIER, *PCRYPT_ALGORITHM_IDENTIFIER;

// 1990
#define szOID_RSA_SHA1RSA       "1.2.840.113549.1.1.5"

// 2213
typedef struct _CERT_EXTENSION {
    LPSTR               pszObjId;
    BOOL                fCritical;
    CRYPT_OBJID_BLOB    Value;
} CERT_EXTENSION, *PCERT_EXTENSION;

// 2256
typedef struct _CERT_RDN_ATTR {
    LPSTR                   pszObjId;
    DWORD                   dwValueType;
    CERT_RDN_VALUE_BLOB     Value;
} CERT_RDN_ATTR, *PCERT_RDN_ATTR;

// 2266
#define szOID_COMMON_NAME                   "2.5.4.3"  // case-ignore string

// 2369
#define CERT_RDN_ANY_TYPE                0

// 2458
typedef struct _CERT_PUBLIC_KEY_INFO {
    CRYPT_ALGORITHM_IDENTIFIER    Algorithm;
    CRYPT_BIT_BLOB                PublicKey;
} CERT_PUBLIC_KEY_INFO, *PCERT_PUBLIC_KEY_INFO;

// 2592
typedef struct _CERT_INFO {
    DWORD                       dwVersion;
    CRYPT_INTEGER_BLOB          SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER  SignatureAlgorithm;
    CERT_NAME_BLOB              Issuer;
    FILETIME                    NotBefore;
    FILETIME                    NotAfter;
    CERT_NAME_BLOB              Subject;
    CERT_PUBLIC_KEY_INFO        SubjectPublicKeyInfo;
    CRYPT_BIT_BLOB              IssuerUniqueId;
    CRYPT_BIT_BLOB              SubjectUniqueId;
    DWORD                       cExtension;
    PCERT_EXTENSION             rgExtension;
} CERT_INFO, *PCERT_INFO;

// 2618
#define CERT_INFO_VERSION_FLAG                      1
#define CERT_INFO_SERIAL_NUMBER_FLAG                2
#define CERT_INFO_SIGNATURE_ALGORITHM_FLAG          3
#define CERT_INFO_ISSUER_FLAG                       4
#define CERT_INFO_NOT_BEFORE_FLAG                   5
#define CERT_INFO_NOT_AFTER_FLAG                    6
#define CERT_INFO_SUBJECT_FLAG                      7
#define CERT_INFO_SUBJECT_PUBLIC_KEY_INFO_FLAG      8
#define CERT_INFO_ISSUER_UNIQUE_ID_FLAG             9
#define CERT_INFO_SUBJECT_UNIQUE_ID_FLAG            10
#define CERT_INFO_EXTENSION_FLAG                    11

// 2426
typedef struct _CERT_RDN {
    DWORD           cRDNAttr;
    PCERT_RDN_ATTR  rgRDNAttr;
} CERT_RDN, *PCERT_RDN;

// 3292
#define szOID_SUBJECT_ALT_NAME          "2.5.29.7"
#define szOID_SUBJECT_ALT_NAME2         "2.5.29.17"

// 3472
#define szOID_PKIX_KP_SERVER_AUTH       "1.3.6.1.5.5.7.3.1"

// 3475
#define szOID_PKIX_KP_CLIENT_AUTH       "1.3.6.1.5.5.7.3.2"

// 3907
typedef struct _CERT_OTHER_NAME {
    LPSTR               pszObjId;
    CRYPT_OBJID_BLOB    Value;
} CERT_OTHER_NAME, *PCERT_OTHER_NAME;

typedef struct _CERT_ALT_NAME_ENTRY {
    DWORD   dwAltNameChoice;
    union {                                             // certenrolls_skip
        PCERT_OTHER_NAME            pOtherName;         // 1
        LPWSTR                      pwszRfc822Name;     // 2  (encoded IA5)
        LPWSTR                      pwszDNSName;        // 3  (encoded IA5)
        // Not implemented          x400Address;        // 4
        CERT_NAME_BLOB              DirectoryName;      // 5
        // Not implemented          pEdiPartyName;      // 6
        LPWSTR                      pwszURL;            // 7  (encoded IA5)
        CRYPT_DATA_BLOB             IPAddress;          // 8  (Octet String)
        LPSTR                       pszRegisteredID;    // 9  (Object Identifer)
    };                                                  // certenrolls_skip
} CERT_ALT_NAME_ENTRY, *PCERT_ALT_NAME_ENTRY;

// 3929
#define CERT_ALT_NAME_OTHER_NAME         1
#define CERT_ALT_NAME_RFC822_NAME        2
#define CERT_ALT_NAME_DNS_NAME           3
#define CERT_ALT_NAME_X400_ADDRESS       4
#define CERT_ALT_NAME_DIRECTORY_NAME     5
#define CERT_ALT_NAME_EDI_PARTY_NAME     6
#define CERT_ALT_NAME_URL                7
#define CERT_ALT_NAME_IP_ADDRESS         8
#define CERT_ALT_NAME_REGISTERED_ID      9

typedef void *HCERTSTORE;

// 8712
typedef struct _CERT_CONTEXT {
    DWORD                   dwCertEncodingType;
    BYTE                    *pbCertEncoded;
    DWORD                   cbCertEncoded;
    PCERT_INFO              pCertInfo;
    HCERTSTORE              hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;

// 9047
#define CERT_STORE_NO_CRYPT_RELEASE_FLAG                0x00000001
#define CERT_STORE_SET_LOCALIZED_NAME_FLAG              0x00000002
#define CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG     0x00000004
#define CERT_STORE_DELETE_FLAG                          0x00000010
#define CERT_STORE_UNSAFE_PHYSICAL_FLAG                 0x00000020
#define CERT_STORE_SHARE_STORE_FLAG                     0x00000040
#define CERT_STORE_SHARE_CONTEXT_FLAG                   0x00000080
#define CERT_STORE_MANIFOLD_FLAG                        0x00000100
#define CERT_STORE_ENUM_ARCHIVED_FLAG                   0x00000200
#define CERT_STORE_UPDATE_KEYID_FLAG                    0x00000400
#define CERT_STORE_BACKUP_RESTORE_FLAG                  0x00000800
#define CERT_STORE_READONLY_FLAG                        0x00008000
#define CERT_STORE_OPEN_EXISTING_FLAG                   0x00004000
#define CERT_STORE_CREATE_NEW_FLAG                      0x00002000
#define CERT_STORE_MAXIMUM_ALLOWED_FLAG                 0x00001000

// By default, when the CurrentUser "Root" store is opened, any SystemRegistry
// roots not also on the protected root list are deleted from the cache before
// CertOpenStore() returns. Set the following flag to return all the roots
// in the SystemRegistry without checking the protected root list.
#define CERT_SYSTEM_STORE_UNPROTECTED_FLAG      0x40000000

// Location of the system store:
#define CERT_SYSTEM_STORE_LOCATION_MASK         0x00FF0000
#define CERT_SYSTEM_STORE_LOCATION_SHIFT        16

//  Registry: HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
#define CERT_SYSTEM_STORE_CURRENT_USER_ID       1
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_ID      2
//  Registry: HKEY_LOCAL_MACHINE\Software\Microsoft\Cryptography\Services
#define CERT_SYSTEM_STORE_CURRENT_SERVICE_ID    4
#define CERT_SYSTEM_STORE_SERVICES_ID           5
//  Registry: HKEY_USERS
#define CERT_SYSTEM_STORE_USERS_ID              6

//  Registry: HKEY_CURRENT_USER\Software\Policies\Microsoft\SystemCertificates
#define CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID    7
//  Registry: HKEY_LOCAL_MACHINE\Software\Policies\Microsoft\SystemCertificates
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID   8

//  Registry: HKEY_LOCAL_MACHINE\Software\Microsoft\EnterpriseCertificates
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID     9

#define CERT_SYSTEM_STORE_CURRENT_USER          \
    (CERT_SYSTEM_STORE_CURRENT_USER_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_LOCAL_MACHINE         \
    (CERT_SYSTEM_STORE_LOCAL_MACHINE_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_CURRENT_SERVICE       \
    (CERT_SYSTEM_STORE_CURRENT_SERVICE_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_SERVICES              \
    (CERT_SYSTEM_STORE_SERVICES_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_USERS                 \
    (CERT_SYSTEM_STORE_USERS_ID << CERT_SYSTEM_STORE_LOCATION_SHIFT)

#define CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY   \
    (CERT_SYSTEM_STORE_CURRENT_USER_GROUP_POLICY_ID << \
        CERT_SYSTEM_STORE_LOCATION_SHIFT)
#define CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY  \
    (CERT_SYSTEM_STORE_LOCAL_MACHINE_GROUP_POLICY_ID << \
        CERT_SYSTEM_STORE_LOCATION_SHIFT)

#define CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE  \
    (CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE_ID << \
        CERT_SYSTEM_STORE_LOCATION_SHIFT)

#define CERT_COMPARE_MASK           0xFFFF
#define CERT_COMPARE_SHIFT          16
#define CERT_COMPARE_ANY            0
#define CERT_COMPARE_SHA1_HASH      1
#define CERT_COMPARE_NAME           2
#define CERT_COMPARE_ATTR           3
#define CERT_COMPARE_MD5_HASH       4
#define CERT_COMPARE_PROPERTY       5
#define CERT_COMPARE_PUBLIC_KEY     6
#define CERT_COMPARE_HASH           CERT_COMPARE_SHA1_HASH
#define CERT_COMPARE_NAME_STR_A     7
#define CERT_COMPARE_NAME_STR_W     8
#define CERT_COMPARE_KEY_SPEC       9
#define CERT_COMPARE_ENHKEY_USAGE   10
#define CERT_COMPARE_CTL_USAGE      CERT_COMPARE_ENHKEY_USAGE
#define CERT_COMPARE_SUBJECT_CERT   11
#define CERT_COMPARE_ISSUER_OF      12
#define CERT_COMPARE_EXISTING       13
#define CERT_COMPARE_SIGNATURE_HASH 14
#define CERT_COMPARE_KEY_IDENTIFIER 15
#define CERT_COMPARE_CERT_ID        16
#define CERT_COMPARE_CROSS_CERT_DIST_POINTS 17

#define CERT_COMPARE_PUBKEY_MD5_HASH 18

#define CERT_COMPARE_SUBJECT_INFO_ACCESS 19

#define CERT_FIND_ANY           (CERT_COMPARE_ANY << CERT_COMPARE_SHIFT)
#define CERT_FIND_SHA1_HASH     (CERT_COMPARE_SHA1_HASH << CERT_COMPARE_SHIFT)
#define CERT_FIND_MD5_HASH      (CERT_COMPARE_MD5_HASH << CERT_COMPARE_SHIFT)
#define CERT_FIND_SIGNATURE_HASH (CERT_COMPARE_SIGNATURE_HASH << CERT_COMPARE_SHIFT)
#define CERT_FIND_KEY_IDENTIFIER (CERT_COMPARE_KEY_IDENTIFIER << CERT_COMPARE_SHIFT)
#define CERT_FIND_HASH          CERT_FIND_SHA1_HASH
#define CERT_FIND_PROPERTY      (CERT_COMPARE_PROPERTY << CERT_COMPARE_SHIFT)
#define CERT_FIND_PUBLIC_KEY    (CERT_COMPARE_PUBLIC_KEY << CERT_COMPARE_SHIFT)
#define CERT_FIND_SUBJECT_NAME  (CERT_COMPARE_NAME << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_SUBJECT_ATTR  (CERT_COMPARE_ATTR << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_ISSUER_NAME   (CERT_COMPARE_NAME << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_ISSUER_ATTR   (CERT_COMPARE_ATTR << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_SUBJECT_STR_A (CERT_COMPARE_NAME_STR_A << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_SUBJECT_STR_W (CERT_COMPARE_NAME_STR_W << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_SUBJECT_FLAG)
#define CERT_FIND_SUBJECT_STR   CERT_FIND_SUBJECT_STR_W
#define CERT_FIND_ISSUER_STR_A  (CERT_COMPARE_NAME_STR_A << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_ISSUER_STR_W  (CERT_COMPARE_NAME_STR_W << CERT_COMPARE_SHIFT | \
                                 CERT_INFO_ISSUER_FLAG)
#define CERT_FIND_ISSUER_STR    CERT_FIND_ISSUER_STR_W
#define CERT_FIND_KEY_SPEC      (CERT_COMPARE_KEY_SPEC << CERT_COMPARE_SHIFT)
#define CERT_FIND_ENHKEY_USAGE  (CERT_COMPARE_ENHKEY_USAGE << CERT_COMPARE_SHIFT)
#define CERT_FIND_CTL_USAGE     CERT_FIND_ENHKEY_USAGE

#define CERT_FIND_SUBJECT_CERT  (CERT_COMPARE_SUBJECT_CERT << CERT_COMPARE_SHIFT)
#define CERT_FIND_ISSUER_OF     (CERT_COMPARE_ISSUER_OF << CERT_COMPARE_SHIFT)
#define CERT_FIND_EXISTING      (CERT_COMPARE_EXISTING << CERT_COMPARE_SHIFT)
#define CERT_FIND_CERT_ID       (CERT_COMPARE_CERT_ID << CERT_COMPARE_SHIFT)
#define CERT_FIND_CROSS_CERT_DIST_POINTS \
                    (CERT_COMPARE_CROSS_CERT_DIST_POINTS << CERT_COMPARE_SHIFT)


#define CERT_FIND_PUBKEY_MD5_HASH \
                    (CERT_COMPARE_PUBKEY_MD5_HASH << CERT_COMPARE_SHIFT)

#define CERT_FIND_SUBJECT_INFO_ACCESS \
                    (CERT_COMPARE_SUBJECT_INFO_ACCESS << CERT_COMPARE_SHIFT)

// 13456
#define CERT_UNICODE_IS_RDN_ATTRS_FLAG              0x1

// 18020
#define CERT_CHAIN_REVOCATION_CHECK_END_CERT           0x10000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN              0x20000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x40000000
#define CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY         0x80000000

#ifdef __cplusplus
}
#endif
