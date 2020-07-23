// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class CryptProvider
    {
    public:
       
		CryptProvider() : provider_(NULL), key_(NULL) {}
		~CryptProvider();
		bool AcquireContext(DWORD dwFlags);
		bool GenerateKey(ALG_ID algid, DWORD dwFlags);

		__declspec(property(get = GetProvider)) HCRYPTPROV Provider;
		HCRYPTPROV GetProvider() { return provider_; }

		__declspec(property(get = GetKey)) HCRYPTKEY Key;
		HCRYPTKEY GetKey() { return key_; }
		
    private:
		DENY_COPY(CryptProvider);
		HCRYPTPROV provider_;
		HCRYPTKEY key_;
        
    };
}
