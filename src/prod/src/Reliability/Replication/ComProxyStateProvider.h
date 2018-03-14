// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class ComProxyStateProvider
        {
        public:
            explicit ComProxyStateProvider(
                Common::ComPointer<IFabricStateProvider> && stateProvider);

            ComProxyStateProvider(ComProxyStateProvider const & other);
            ComProxyStateProvider & operator=(ComProxyStateProvider const & other);

            ComProxyStateProvider(ComProxyStateProvider && other);
            ComProxyStateProvider & operator=(ComProxyStateProvider && other);

            __declspec (property(get=get_SupportsCopyUntilLatestLsn)) bool SupportsCopyUntilLatestLsn;
            bool get_SupportsCopyUntilLatestLsn() const { return supportsCopyUntilLatestLsn_; }
                        
            __declspec (property(get=get_InnerStateProvider)) IFabricStateProvider * const InnerStateProvider;
            IFabricStateProvider * const get_InnerStateProvider() const { return stateProvider_.GetRawPointer(); }

            Common::ErrorCode GetLastCommittedSequenceNumber( 
                __out FABRIC_SEQUENCE_NUMBER & sequenceNumber) const;

            Common::AsyncOperationSPtr BeginUpdateEpoch( 
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state) const;

            Common::ErrorCode EndUpdateEpoch(
                Common::AsyncOperationSPtr const & asyncOperation) const;

            Common::ErrorCode GetCopyContext(
                __out ComProxyAsyncEnumOperationData & asyncEnumOperationData) const;

            Common::ErrorCode GetCopyOperations(
                Common::ComPointer<IFabricOperationDataStream> && context, 
                FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
                __out ComProxyAsyncEnumOperationData & asyncEnumOperationData) const;

            Common::AsyncOperationSPtr BeginOnDataLoss(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & state) const;

            Common::ErrorCode EndOnDataLoss(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out BOOLEAN & isStateChanged) const;
            
        private:
            class OnDataLossAsyncOperation;
            class UpdateEpochAsyncOperation;
    
            Common::ComPointer<IFabricStateProvider> stateProvider_;
            bool supportsCopyUntilLatestLsn_;
        };
    }
}
