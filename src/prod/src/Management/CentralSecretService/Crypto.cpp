// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::CentralSecretService;

Crypto::Crypto()
    : certContext_()
{
    FabricNodeConfig nodeConfig;
    wstring certThumbprint = CentralSecretServiceConfig::GetConfig().EncryptionCertificateThumbprint;
    ErrorCode error;

    // if EncryptionCertificateThumbprint is not configured,
    // use Cluster Certificate instead.
    if (certThumbprint.empty()) {
        error = CryptoUtility::GetCertificate(
            X509StoreLocation::LocalMachine,
            nodeConfig.ClusterX509StoreName,
            L"FindByThumbprint",
            nodeConfig.ClusterX509FindValue,
            this->certContext_);
    }
    else {
        error = CryptoUtility::GetCertificate(
            X509StoreLocation::LocalMachine,
            CentralSecretServiceConfig::GetConfig().EncryptionCertificateStoreName,
            L"FindByThumbprint",
            certThumbprint,
            this->certContext_);
    }

    if (!error.IsSuccess())
    {
        // Log
        this->certContext_.reset();
    }
}

ErrorCode Crypto::Encrypt(std::wstring const & text, std::wstring & encryptedText) const
{
    if (!this->certContext_)
    {
        return ErrorCode(ErrorCodeValue::InvalidState, L"Certificate Context is not initialized.");
    }

    return CryptoUtility::EncryptText(text, this->certContext_.get(), nullptr, encryptedText);
}

ErrorCode Crypto::Decrypt(std::wstring const & encryptedText, SecureString & decryptedText) const
{
    return CryptoUtility::DecryptText(encryptedText, decryptedText);
}
