// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    typedef std::shared_ptr<Store::ILocalStore::TransactionBase> TransactionSPtr;
    typedef std::shared_ptr<Store::ILocalStore::EnumerationBase> EnumerationSPtr;
}
