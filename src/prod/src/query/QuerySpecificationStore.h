// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    // 
    // A Queryspecification defines the specification or template for a query. For each queryname
    // and argument set, the specification determines how to process the query. These specification
    // donot contain any state and need to be constructed only once. The specification store is a
    // a singleton that stores all the queryspecifications.
    //

    class QuerySpecificationStore
    {
        DENY_COPY(QuerySpecificationStore);

    public:

        QuerySpecificationStore();

        static QuerySpecificationStore& Get();

        QuerySpecificationSPtr GetSpecification(
            Query::QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs);

    private:
        static BOOL CALLBACK InitalizeSpecificationStore(PINIT_ONCE, PVOID, PVOID *);
        static INIT_ONCE initOnce_;
        static Common::Global<QuerySpecificationStore> singletonSpecificationStore_;

        void Initialize();
        std::map<std::wstring, QuerySpecificationSPtr> specifications_;
    };
}
