// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Query;
using namespace Common;
using namespace ServiceModel;

INIT_ONCE QuerySpecificationStore::initOnce_;
Global<QuerySpecificationStore> QuerySpecificationStore::singletonSpecificationStore_;

QuerySpecificationStore::QuerySpecificationStore()
{
    Initialize();
}

void QuerySpecificationStore::Initialize()
{
    auto specificationsVector = move(QuerySpecification::CreateSpecifications());

    for(auto i = 0; i < specificationsVector.size(); ++i)
    {
        ASSERT_IF(
            specifications_.find(specificationsVector[i]->QuerySpecificationId) != specifications_.end(),
            "QuerySpecificationId {0} is specified for more than one query specification",
            specificationsVector[i]->QuerySpecificationId);

        specifications_[specificationsVector[i]->QuerySpecificationId] = specificationsVector[i];
    }
}

BOOL CALLBACK QuerySpecificationStore::InitalizeSpecificationStore(PINIT_ONCE, PVOID, PVOID *)
{
    singletonSpecificationStore_ = make_global<QuerySpecificationStore>();
    return TRUE;
}

QuerySpecificationStore& QuerySpecificationStore::Get()
{
    PVOID lpContext = NULL;
    BOOL result = InitOnceExecuteOnce(
        &initOnce_,
        InitalizeSpecificationStore,
        NULL,
        &lpContext);

    ASSERT_IF(!result, "Failed to initialize QuerySpecificationStore");

    return *(singletonSpecificationStore_);
}

QuerySpecificationSPtr QuerySpecificationStore::GetSpecification(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs)
{
    auto specificationId = QuerySpecification::GetSpecificationId(queryName, queryArgs);

    auto iter = specifications_.find(specificationId);
    if (iter == specifications_.end())
    {
        return nullptr;
    }

    return iter->second;
}
