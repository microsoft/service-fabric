// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class LocalStorageInitializer
        {
            DENY_COPY(LocalStorageInitializer);
        public:
            LocalStorageInitializer(
                ReconfigurationAgent & ra,
                Reliability::NodeUpOperationFactory & factory);

            Common::ErrorCode Open();

        private:
            bool UpdateStateOnLFUMLoadProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase & context) const;

            Common::ErrorCode LoadLFUM();
            Common::ErrorCode UpdateStateOnLFUMLoad(
                Infrastructure::EntityEntryBaseList const & ftIds);

            Common::ErrorCode UpdateStateOnLoad(
                Infrastructure::EntityEntryBaseList const & entities,
                Infrastructure::MultipleEntityWork::FactoryFunctionPtr factory);         

            Common::ErrorCode DeserializeLfum(
                std::vector<Storage::Api::Row> & rows,
                __out Infrastructure::LocalFailoverUnitMap::InMemoryState & lfumState,
                __out Infrastructure::EntityEntryBaseList & entries);


            ReconfigurationAgent & ra_;
            Reliability::NodeUpOperationFactory & factory_;
        };
    }
}
