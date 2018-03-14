// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {                   
        class RequestManager;
        class ReplicatedStoreWrapper :
            public Common::RootedObject,  
            private Common::TextTraceComponent<Common::TraceTaskCodes::FileStoreService>
        {
            DENY_COPY(ReplicatedStoreWrapper)      

        public:
            ReplicatedStoreWrapper(RequestManager & root);
            ~ReplicatedStoreWrapper();                        

            Common::ErrorCode CreateTransaction(Common::ActivityId const & activityId, StoreTransactionUPtr & tx);
            Common::ErrorCode CreateSimpleTransaction(Common::ActivityId const & activityId, StoreTransactionUPtr & tx);            

            Common::ErrorCode ReadExact(Common::ActivityId const & activityId, __inout Store::StoreData & storeData);
            Common::ErrorCode ReadExact(Store::StoreTransaction const &, __inout Store::StoreData & storeData);

            Common::ErrorCode TryReadOrInsertIfNotFound(Common::ActivityId const & activityId, Store::StoreTransaction const &, __inout Store::StoreData & result, __out bool & readExisting);

            template <class T>
            Common::ErrorCode ReadPrefix(
                Common::ActivityId const & activityId,
                std::wstring const & prefix,
                __inout std::vector<T> & entries)
            {
                StoreTransactionUPtr tx;
                auto error = CreateTransaction(activityId, tx);

                if (error.IsSuccess())
                {
                    error = this->ReadPrefixImpl<T>(*tx, prefix, entries);

                    tx->CommitReadOnly();
                }

                return error;
            }

            template <class T>
            Common::ErrorCode ReadPrefix(
                Store::StoreTransaction const & storeTx,
                std::wstring const & prefix,
                __inout std::vector<T> & entries)
            {
                if (FileStoreServiceConfig::GetConfig().EnableTStore)
                {
                    return this->ReadPrefixImpl(storeTx, prefix, entries);
                }
                else
                {
                    // In the V1 stack, simple transactions are used to
                    // batch writes. Since simple transactions do not support
                    // reads, we need to create a separate transaction
                    // for reading.
                    //
                    return this->ReadPrefix(storeTx.ActivityId, prefix, entries);
                }
            }

        private:
            Common::ErrorCode ReadExactImpl(Store::StoreTransaction const &, __inout Store::StoreData & storeData);

            template <class T>
            Common::ErrorCode ReadPrefixImpl(
                Store::StoreTransaction const & storeTx,
                std::wstring const & prefix,
                __inout std::vector<T> & entries)
            {
                T dummy;
                return storeTx.ReadPrefix<T>(dummy.Type, prefix, entries);
            }

            RequestManager & owner_;
        };
    }
}
