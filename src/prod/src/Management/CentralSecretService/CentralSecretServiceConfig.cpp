// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Management::CentralSecretService;

DEFINE_SINGLETON_COMPONENT_CONFIG(CentralSecretServiceConfig)

bool CentralSecretServiceConfig::IsCentralSecretServiceEnabled()
{
    if (!((GetConfig().IsEnabled) && (CommonConfig::GetConfig().EnableUnsupportedPreviewFeatures)))
    {
        return false;
    }

    FabricNodeConfig nodeConfig;
    wstring certThumbprint = CentralSecretServiceConfig::GetConfig().EncryptionCertificateThumbprint;
    ErrorCode error;
    wstring commonName;

    // if EncryptionCertificateThumbprint is not configured,
    // fallback to Cluster Certificate instead.
    if (certThumbprint.empty()) {
        error = CryptoUtility::GetCertificateCommonName(
            X509StoreLocation::LocalMachine,
            nodeConfig.ClusterX509StoreName,
            L"FindByThumbprint",
            nodeConfig.ClusterX509FindValue,
            commonName);
    }
    else {
        error = CryptoUtility::GetCertificateCommonName(
            X509StoreLocation::LocalMachine,
            CentralSecretServiceConfig::GetConfig().EncryptionCertificateStoreName,
            L"FindByThumbprint",
            certThumbprint,
            commonName);
    }

    if (!error.IsSuccess())
    {
        return false;
    }

    return true;
}
