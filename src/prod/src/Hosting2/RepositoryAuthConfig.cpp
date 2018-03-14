// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "RepositoryAuthConfig.h"

using namespace Hosting2;
using namespace Common;

Common::WStringLiteral const RepositoryAuthConfig::AccountNameParameter(L"username");
Common::WStringLiteral const RepositoryAuthConfig::PasswordParameter(L"password");
Common::WStringLiteral const RepositoryAuthConfig::EmailParameter(L"email");

RepositoryAuthConfig::RepositoryAuthConfig() :
	AccountName(),
    Password(),
    Email()
{
}

RepositoryAuthConfig::~RepositoryAuthConfig()
{
}
