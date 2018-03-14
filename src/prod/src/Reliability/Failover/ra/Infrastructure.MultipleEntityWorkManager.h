// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            // This class manages all the MultipleEntityWork
            // When some code wants to execute work for multiple FTs it calls the Execute function on this
            class MultipleEntityWorkManager
            {
                DENY_COPY(MultipleEntityWorkManager);

            public:
                MultipleEntityWorkManager(ReconfigurationAgent & ra);

                void Execute(MultipleEntityWorkSPtr &&);

                void Execute(MultipleEntityWorkSPtr const &);

                void OnFailoverUnitJobItemExecutionCompleted(MultipleEntityWorkSPtr const & work);

                void Close();
            private:

                void EnqueueCompletionCallback(MultipleEntityWorkSPtr const & work);

                ReconfigurationAgent & ra_;
            };
        }
    }
}
