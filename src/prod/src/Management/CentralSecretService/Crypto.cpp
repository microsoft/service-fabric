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
	auto error =
		CryptoUtility::GetCertificate(
			X509StoreLocation::LocalMachine,
			nodeConfig.ClusterX509StoreName,
			nodeConfig.ClusterX509FindType,
			nodeConfig.ClusterX509FindValue,
			this->certContext_);

	if (!error.IsSuccess())
	{
		// Log
		this->certContext_.reset();
	}
}

ErrorCode Crypto::Encrypt(std::wstring const & text, std::wstring & encryptedText)
{
	if (!this->certContext_)
	{
		return ErrorCode(ErrorCodeValue::InvalidState, L"Certificate Context is not initialized.");
	}

	return CryptoUtility::EncryptText(text, this->certContext_.get(), nullptr, encryptedText);
}

ErrorCode Crypto::Decrypt(std::wstring const & encryptedText, SecureString & decryptedText)
{
	return CryptoUtility::DecryptText(encryptedText, decryptedText);
}
