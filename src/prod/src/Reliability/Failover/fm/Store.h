// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverManagerStore
        {
            DENY_COPY(FailoverManagerStore);

        public:

            // Holds pointer to underlying IReplicatedStore in FailoverManagerReplica
            //
            explicit FailoverManagerStore(Common::RootedObjectPointer<Store::IReplicatedStore> && replicatedStore);

            // Holds pointer to no-op or unit test mock store used by FMM
            //
            explicit FailoverManagerStore(Store::IReplicatedStoreUPtr && replicatedStore);

            __declspec (property(get=get_IsThrottleNeeded)) bool IsThrottleNeeded;
            bool get_IsThrottleNeeded() const;

            Common::ErrorCode BeginTransaction(Store::IStoreBase::TransactionSPtr & txSPtr) const;
            Common::ErrorCode BeginSimpleTransaction(Store::IStoreBase::TransactionSPtr & txSPtr) const;

            template <class T>
            Common::ErrorCode LoadAll(T & data) const;

            template <class T>
            Common::ErrorCode UpdateData(T & data, __out int64 & commitDuration) const;

            template <class T>
            Common::ErrorCode UpdateData(Store::IStoreBase::TransactionSPtr const& tx, T & data) const;

            template <typename TData>
            Common::AsyncOperationSPtr BeginUpdateData(
                TData & data,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state) const;

            template <typename TData>
            Common::ErrorCode EndUpdateData(TData & data, Common::AsyncOperationSPtr const& updateOperation, __out int64 & commitDuration) const;

            Common::ErrorCode GetKeyValues(__out std::map<std::wstring, std::wstring>& keyValues) const;
            Common::ErrorCode UpdateKeyValue(std::wstring const& key, std::wstring const& value) const;

            Common::ErrorCode GetFailoverUnit(FailoverUnitId const& failoverUnitId, FailoverUnitUPtr & failoverUnit) const;

            Common::AsyncOperationSPtr BeginUpdateServiceAndFailoverUnits(
                ApplicationInfoSPtr const& applicationInfo,
                ServiceTypeSPtr const& serviceType,
                ServiceInfo & serviceInfo,
                std::vector<FailoverUnitUPtr> const& failoverUnits,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& state) const;

            Common::ErrorCode EndUpdateServiceAndFailoverUnits(
                Common::AsyncOperationSPtr const& operation,
                ApplicationInfoSPtr const& applicationInfo,
                ServiceTypeSPtr const& serviceType,
                ServiceInfo & serviceInfo,
                std::vector<FailoverUnitUPtr> const& failoverUnits,
                __out int64 & commitDuration) const;

            Common::ErrorCode DeleteInBuildFailoverUnitAndUpdateFailoverUnit(
                InBuildFailoverUnitUPtr const& inBuildFailoverUnit,
                FailoverUnitUPtr const& failoverUnit) const;

            Common::ErrorCode UpdateFabricVersionInstance(Common::FabricVersionInstance const& versionInstance) const;
            Common::ErrorCode GetFabricVersionInstance(Common::FabricVersionInstance & versionInstance) const;

            Common::ErrorCode UpdateFabricUpgrade(FabricUpgrade const& upgrade) const;
            Common::ErrorCode GetFabricUpgrade(FabricUpgradeUPtr & upgrade) const;

            Common::ErrorCode UpdateNodes(std::vector<NodeInfoSPtr> & nodes) const;

            void Dispose(bool isStoreCloseNeeded);

            // Add any new type to Test_DeleteAllData
            static std::wstring const ApplicationType;
            static std::wstring const ServiceInfoType;
            static std::wstring const ServiceTypeType;
            static std::wstring const FailoverUnitType;
            static std::wstring const NodeInfoType;
            static std::wstring const LoadInfoType;
            static std::wstring const InBuildFailoverUnitType;
            static std::wstring const FabricVersionInstanceType;
            static std::wstring const FabricUpgradeType;
            static std::wstring const StringType;

            static std::wstring const FabricVersionInstanceKey;
            static std::wstring const FabricUpgradeKey;

            static std::wstring const NetworkDefinitionType;
            static std::wstring const NetworkNodeType;
            static std::wstring const NetworkMACAddressPoolType;
            static std::wstring const NetworkIPv4AddressPoolType;

        private:
            class IReplicatedStoreHolder;

            typedef Common::RootedObjectPointer<Store::IReplicatedStore> RootedStore;
            typedef std::shared_ptr<RootedStore> RootedStoreSPtr;
            typedef std::weak_ptr<RootedStore> RootedStoreWPtr;
            typedef std::unique_ptr<Store::ILocalStore::EnumerationBase> EnumerationUPtr;

            template <class T>
            struct GetCollectionTraits
            {
                static Common::ErrorCode Add(T & collection, Store::ILocalStore::EnumerationSPtr const & enumSPtr);
                static std::wstring GetStoreType();
            };

            template<class ChildT>
            struct GetCollectionTraits<std::vector<std::unique_ptr<ChildT>>>
            {
                static Common::ErrorCode Add(std::vector<std::unique_ptr<ChildT>> & collection, Store::IStoreBase::EnumerationSPtr const & enumSPtr)
                {
                    std::unique_ptr<ChildT> data = Common::make_unique<ChildT>();
                    Common::ErrorCode error = ReadCurrentData<ChildT>(enumSPtr, *data);
                    if (error.IsSuccess())
                    {
                        collection.push_back(std::move(data));
                    }
                    return error;
                }
                static std::wstring GetStoreType()
                {
                    return ChildT::GetStoreType();
                }
            };

            template<class ChildT>
            struct GetCollectionTraits<std::vector<std::shared_ptr<ChildT>>>
            {
                static Common::ErrorCode Add(std::vector<std::shared_ptr<ChildT>> & collection, Store::IStoreBase::EnumerationSPtr const & enumSPtr)
                {
                    std::shared_ptr<ChildT> data = std::make_shared<ChildT>();
                    Common::ErrorCode error = ReadCurrentData<ChildT>(enumSPtr, *data);
                    if (error.IsSuccess())
                    {
                        collection.push_back(std::move(data));
                    }
                    return error;
                }
                static std::wstring GetStoreType()
                {
                    return ChildT::GetStoreType();
                }
            };

            static Common::ErrorCode DeleteStore(std::unique_ptr<Store::ILocalStore> & localStore);
            
            Common::ErrorCode InternalGetFailoverUnit(
                Store::IStoreBase::TransactionSPtr const& tx,
                FailoverUnitId const& failoverUnitId,
                FailoverUnitUPtr & failoverUnit) const;

            Common::ErrorCode UpdateFailoverUnits(Store::IStoreBase::TransactionSPtr const& tx, std::vector<FailoverUnitUPtr> const& failoverUnits) const;

            Common::ErrorCode GetFabricVersionInstance(Store::IStoreBase::TransactionSPtr const& tx, Common::FabricVersionInstance & versionInstance) const;

            Common::ErrorCode GetFabricUpgrade(Store::IStoreBase::TransactionSPtr const& tx, FabricUpgrade & upgrade) const;

            template <class T>
            static Common::ErrorCode ReadCurrentData(Store::IStoreBase::EnumerationSPtr const & enumSPtr, __out T & result);

            template <class T>
            Common::ErrorCode InternalGetData(
                Store::IStoreBase::TransactionSPtr const& tx,
                std::wstring const & type,
                std::wstring const & key,
                T & data) const;

            template <class T>
            Common::ErrorCode TryInsertData(
                Store::IStoreBase::TransactionSPtr const & txSPtr, 
                T const & data, 
                std::wstring const & type,
                std::wstring const & key) const;

            template <class T>
            Common::ErrorCode TryUpdateData(
                Store::IStoreBase::TransactionSPtr const & txSPtr, 
                T const & data, 
                std::wstring const & type, 
                std::wstring const & key, 
                int64 currentSequenceNumber) const;

            Common::ErrorCode TryDeleteData(
                Store::IStoreBase::TransactionSPtr const & txSPtr, 
                std::wstring const & type, 
                std::wstring const & key, 
                int64 currentSequenceNumber) const;

            Common::ErrorCode TryGetStore(__out RootedStoreSPtr &) const;

            mutable RootedStoreSPtr storeSPtr_;
            mutable RootedStoreWPtr storeWPtr_;
            static size_t const StoreDataBufferSize;
        };
    }
}
