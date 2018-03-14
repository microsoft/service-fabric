// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CertStore
    {

    public:
        CertStore() : store_(NULL) {}
        ~CertStore();
        bool OpenPhysicalStore(DWORD dwFlags, wstring storeName);
        bool OpenMemoryStore();
        bool AddCertToStore(PCCERT_CONTEXT cert, DWORD dwAddDisposition);
        bool AddCertToStore(PCCERT_CONTEXT cert, DWORD dwAddDisposition, HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hPrivKey);
        bool PFXExport(SecureString pwd);
        bool DeleteFromStore(wstring certName, bool exactMatch);

        __declspec(property(get = GetStore)) HCERTSTORE Store;
        HCERTSTORE GetStore() { return store_; }

        __declspec(property(get = GetPFXBlob)) CRYPT_DATA_BLOB PFXBlob;
        CRYPT_DATA_BLOB GetPFXBlob() { return pfxBlob_; }

    private:
        DENY_COPY(CertStore);
        HCERTSTORE store_;
        CRYPT_DATA_BLOB pfxBlob_;
        vector<BYTE> buffer_;
        bool checkPrefix(wstring mainStr, wstring prefixStr);
    };
}
