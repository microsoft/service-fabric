// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/CertStore.h"

namespace Common
{
    CertStore::~CertStore()
    {
        CertCloseStore(store_, 0);
    }

    bool CertStore::OpenPhysicalStore(DWORD dwFlags, wstring storeName)
    {
        store_ = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, 0, dwFlags, storeName.c_str());
        if (!store_)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    bool CertStore::OpenMemoryStore()
    {
        store_ = CertOpenStore(CERT_STORE_PROV_MEMORY, 0, NULL, 0, NULL);
        if (!store_)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    bool CertStore::AddCertToStore(PCCERT_CONTEXT cert, DWORD dwAddDisposition)
    {
        if (!CertAddCertificateContextToStore(store_, cert, dwAddDisposition, 0))
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    bool CertStore::AddCertToStore(PCCERT_CONTEXT cert, DWORD dwAddDisposition, HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hPrivKey)
    {
        PCCERT_CONTEXT pCert = NULL;
        KFinally([pCert]() {
            if (pCert != NULL) {
                CertFreeCertificateContext(pCert);
            }
        });

        if (!CertAddCertificateContextToStore(store_, cert, dwAddDisposition, &pCert))
        {
            return false;
        }

        if (hPrivKey != NULL) {
            if (!CertSetCertificateContextProperty(pCert, CERT_HCRYPTPROV_OR_NCRYPT_KEY_HANDLE_PROP_ID, 0, &hPrivKey)) {
                return false;
            }

            if (!CertAddCertificateContextToStore(store_, pCert, CERT_STORE_ADD_REPLACE_EXISTING, NULL)) {
                return false;
            }
        }

        return true;
    }

    bool CertStore::checkPrefix(wstring mainStr, wstring prefixStr)
    {
        return mainStr.substr(0, prefixStr.size()) == prefixStr;
    }

    bool CertStore::DeleteFromStore(wstring certName, bool exactMatch)
    {
        PCCERT_CONTEXT pTargetCert = NULL;
        pTargetCert = CertEnumCertificatesInStore(store_, pTargetCert);
        while (pTargetCert)
        {
            DWORD cbSize;
            cbSize = CertGetNameString(pTargetCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0);
            if (!cbSize)
            {
                return false;
            }
            vector<wchar_t> buffer(cbSize, 0);
            wchar_t *pszName = buffer.data();
            if (!CertGetNameString(pTargetCert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, pszName, cbSize))
            {
                return false;
            }
            if (exactMatch)
            {
                if (certName.compare(pszName) == 0)
                {
                    CertDuplicateCertificateContext(pTargetCert);
                    if (!CertDeleteCertificateFromStore(pTargetCert))
                    {
                        return false;
                    }
                }
            }
            else
            {
                if (checkPrefix(wstring(pszName), certName))
                {
                    CertDuplicateCertificateContext(pTargetCert);
                    if (!CertDeleteCertificateFromStore(pTargetCert))
                    {
                        return false;
                    }
                }
            }
            pTargetCert = CertEnumCertificatesInStore(store_, pTargetCert);
        }
        CertFreeCertificateContext(pTargetCert);
        return true;
    }

    bool CertStore::PFXExport(SecureString pwd)
    {
        pfxBlob_.pbData = NULL;
        pfxBlob_.cbData = 0;
        if (!PFXExportCertStoreEx(store_, &pfxBlob_, pwd.GetPlaintext().c_str(), NULL, EXPORT_PRIVATE_KEYS))
        {
            return false;
        }
        buffer_.resize(pfxBlob_.cbData, 0);
        pfxBlob_.pbData = buffer_.data();
        if (!PFXExportCertStoreEx(store_, &pfxBlob_, pwd.GetPlaintext().c_str(), NULL, EXPORT_PRIVATE_KEYS))
        {
            return false;
        }
        return true;
    }

}
