// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;
    class IQueryHandler;

    class FabricTestQueryExecutor
    {
        DENY_COPY(FabricTestQueryExecutor);

    public:
        FabricTestQueryExecutor();
        virtual ~FabricTestQueryExecutor() {}

        static void AddQueryHandler(std::wstring const& name, std::shared_ptr<IQueryHandler> const& handler)
        {
            queryHandlers_[name] = handler;
        }

        HRESULT ExecuteQuery(Query::QueryNames::Enum queryName, ServiceModel::QueryArgumentMap const & queryArgs, __out Common::ComPointer<IInternalFabricQueryResult2> & result);
        bool ExecuteQuery(Common::StringCollection const & params);
        static ComPointer<IFabricQueryClient7> CreateFabricQueryClient(__in FabricTestFederation & testFederation);

    private:

        static std::map<std::wstring, std::shared_ptr<IQueryHandler>> queryHandlers_;
        Common::Random random_;
    };
}
