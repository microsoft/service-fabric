// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/CryptProvider.h"

namespace Common
{
	CryptProvider::~CryptProvider()
	{
		CryptDestroyKey(key_);
		CryptReleaseContext(provider_, 0);
	}

	bool CryptProvider::AcquireContext(DWORD dwFlags)
	{
		if (!CryptAcquireContext(&provider_, NULL, MS_DEF_RSA_SCHANNEL_PROV, PROV_RSA_SCHANNEL, dwFlags))
		{
			return false;
		}
		else
		{
			return true;
		}	
	}

	bool CryptProvider::GenerateKey(ALG_ID algId, DWORD dwFlags)
	{
		if (!CryptGenKey(provider_, algId, dwFlags, &key_))
		{
			return false;
		}
		else
		{
			return true;
		}	
	}

}
