// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::StateManager;

LONG32 Data::StateManager::Comparer::Compare(__in Metadata::SPtr const & itemOne, __in Metadata::SPtr const & itemTwo)
{
    return itemOne->Name->Get(KUriView::eRaw).Compare(itemTwo->Name->Get(KUriView::eRaw));
}

LONG32 Data::StateManager::Comparer::Compare(__in Metadata::CSPtr const & itemOne, __in Metadata::CSPtr const & itemTwo)
{
    return itemOne->Name->Get(KUriView::eRaw).Compare(itemTwo->Name->Get(KUriView::eRaw));
}

BOOLEAN Data::StateManager::Comparer::Equals(__in KUri::CSPtr const & keyOne, __in KUri::CSPtr const & keyTwo)
{
    return *keyOne == *keyTwo ? TRUE : FALSE;
}

BOOLEAN Data::StateManager::Comparer::Equals(__in KString::CSPtr const & keyOne, __in KString::CSPtr const & keyTwo)
{
    return *keyOne == *keyTwo ? TRUE : FALSE;
}
