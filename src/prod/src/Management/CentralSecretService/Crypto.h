// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#pragma once

namespace Management
{
	namespace CentralSecretService
	{
		class Crypto
		{
		public:
			Crypto();

			ErrorCode Encrypt(std::wstring const & text, std::wstring & encryptedText);
			ErrorCode Decrypt(std::wstring const & encryptedText, SecureString & decryptedText);

		private:
			CertContextUPtr certContext_;
		};
	}
}