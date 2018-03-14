// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;

StateProviderInfo::StateProviderInfo() noexcept
    : typeName_()
    , name_()
    , initializationParameters_()
{
}

StateProviderInfo::StateProviderInfo(
    __in KString const & typeName,
    __in KUri const & name,
    __in_opt Data::Utilities::OperationData const * const initializationParameters) noexcept
    : typeName_(&typeName)
    , name_(&name)
    , initializationParameters_(initializationParameters)
{
}

StateProviderInfo::~StateProviderInfo()
{
}
