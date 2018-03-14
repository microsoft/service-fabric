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
            /*
                Provides fault injection on top of ra storage layer

                The inner store is called when fault injection is disabled

                There are two features:
                - Fault injection which is enabled by EnableFaultInjection and DisableFaultInjection
                  Any store operation request received while fault injection is enabled
                  will not be forwarded to the inner store
                - Async mode where the inner store will be called immediately and then 
                  the async op will be stored in a list in the adapter
                  A caller must explicitly complete these ops
            */
            class FaultInjectionAdapter : public Storage::Api::IKeyValueStore
            {
            public:
                static const Common::ErrorCodeValue::Enum DefaultError = Common::ErrorCodeValue::GlobalLeaseLost;

                FaultInjectionAdapter(Storage::Api::IKeyValueStoreSPtr const & inner);

                __declspec(property(get = get_InnerStore)) Storage::Api::IKeyValueStoreSPtr InnerStore;
                Storage::Api::IKeyValueStoreSPtr get_InnerStore() const { return inner_; }

                __declspec(property(get = get_PersistenceResult)) Common::ErrorCodeValue::Enum PersistenceResult;
                Common::ErrorCodeValue::Enum get_PersistenceResult() const { return faultError_; }

                __declspec(property(get = get_IsFaultInjectionEnabled)) bool FaultInjectionEnabled;
                bool get_IsFaultInjectionEnabled() const { return faultError_ != Common::ErrorCodeValue::Success; }

                __declspec(property(get = get_IsAsync, put = set_IsAsync)) bool IsAsync;
                bool get_IsAsync() const
                {
                    Common::AcquireExclusiveLock grab(lock_);
                    return isAsync_;
                }
                void set_IsAsync(bool const& value)
                {
                    Common::AcquireExclusiveLock grab(lock_);
                    isAsync_ = value;
                }
                
                void EnableFaultInjection(Common::ErrorCodeValue::Enum err = DefaultError);
                void DisableFaultInjection();

                size_t FinishPendingCommits();
                
                Common::ErrorCode Enumerate(
                    Storage::Api::RowType::Enum type,
                    __out std::vector<Storage::Api::Row>& rows) override;

                Common::AsyncOperationSPtr BeginStoreOperation(
                    Storage::Api::OperationType::Enum operationType,
                    Storage::Api::RowIdentifier const & id,
                    Storage::Api::RowData && bytes,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override;

                Common::ErrorCode EndStoreOperation(Common::AsyncOperationSPtr const & operation) override;

                void Close() override;

            private:
                class FaultInjectionAsyncOperation;

                mutable Common::ExclusiveLock lock_;

                std::vector<Common::AsyncOperationSPtr> pendingCommits_;
                
                bool isAsync_;
                Common::ErrorCodeValue::Enum faultError_;
                Storage::Api::IKeyValueStoreSPtr inner_;
            };
        }
    }
}





