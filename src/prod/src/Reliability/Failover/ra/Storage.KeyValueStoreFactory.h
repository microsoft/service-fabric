// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Storage
        {
            class KeyValueStoreFactory
            {
            public:
                struct Parameters
                {
                    Parameters() :
                        IsFaultInjectionEnabled(false),
                        IsInMemory(false)
                    {
                    }

                    // The suffix appended to the instance (usually node id)
                    std::wstring InstanceSuffix;

                    std::wstring WorkingDirectory;

                    bool IsInMemory;

                    bool IsFaultInjectionEnabled;

                    Store::IStoreFactorySPtr StoreFactory;

                    InMemoryKeyValueStoreStateSPtr InMemoryKeyValueStoreState;

                    KtlLogger::KtlLoggerNodeSPtr KtlLoggerInstance;
                };

                Common::ErrorCode TryCreate(
                    Parameters const & parameters,
                    ReconfigurationAgent * ra,
                    __out Storage::Api::IKeyValueStoreSPtr & kvs);

            private:
                Common::ErrorCode TryCreateInternal(
                    Parameters const & parameters,
                    ReconfigurationAgent * ra,
                    __out Storage::Api::IKeyValueStoreSPtr & kvs);
            };
        }
    }
}


