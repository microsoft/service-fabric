//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Management::CentralSecretService;

ComSecretReferencesResult::ComSecretReferencesResult(SecretReferencesDescription const & secretReferencesDescription)
	: heap_()
	, secretReferences_()
{
	secretReferencesDescription.ToPublicApi(this->heap_, *this->secretReferences_);
}

const FABRIC_SECRET_REFERENCE_LIST * STDMETHODCALLTYPE ComSecretReferencesResult::get_SecretReferences()
{
	return this->secretReferences_.GetRawPointer();
}