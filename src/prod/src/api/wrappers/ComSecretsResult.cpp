//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Management::CentralSecretService;

ComSecretsResult::ComSecretsResult(IFabricSecretsResultPtr const & resultPtr)
	: heap_()
	, secrets_()
{
	
}

ComSecretsResult::ComSecretsResult(SecretsDescription const & secretDescription)
	: heap_()
	, secrets_()
{
	secretDescription.ToPublicApi(this->heap_, *this->secrets_);
}

const FABRIC_SECRET_LIST * STDMETHODCALLTYPE ComSecretsResult::get_Secrets()
{
	return this->secrets_.GetRawPointer();
}
