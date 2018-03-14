// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComProxyAsyncEnumOperationData
        {
            DENY_COPY(ComProxyAsyncEnumOperationData)

        public:
            ComProxyAsyncEnumOperationData();

            explicit ComProxyAsyncEnumOperationData(
                Common::ComPointer<IFabricOperationDataStream> && asyncEnumOperationData);

            ComProxyAsyncEnumOperationData(ComProxyAsyncEnumOperationData && other);
            ComProxyAsyncEnumOperationData & operator=(ComProxyAsyncEnumOperationData && other);

            virtual ~ComProxyAsyncEnumOperationData();

            Common::AsyncOperationSPtr BeginGetNext(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state);

            Common::ErrorCode EndGetNext(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out bool & isLast,
                __out Common::ComPointer<IFabricOperationData> & operation);
            
        private:
            class GetNextAsyncOperation;
            class GetNextEmptyAsyncOperation;

            Common::ComPointer<IFabricOperationDataStream> asyncEnumOperationData_;
        };
    }
}
