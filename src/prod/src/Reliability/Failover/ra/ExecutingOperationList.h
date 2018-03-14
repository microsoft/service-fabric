// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        /*
            Once Service Operation Manager is refactored then this doesn't need to be a template
            This class stores running async operations and their associated api call descriptions 
            (used by the monitoring component)
            There are some operations whose storage is externalized (example build)
            These operations only maintain a dummy entry in the ExecutingOperationList
        */
        class ExecutingOperationList
        {
        public:
            ExecutingOperationList() : lastOperationStartTime_(Common::DateTime::Zero)
            {
            }

            __declspec(property(get = get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return singleInstanceOperations_.empty() && multiInstanceOperations_.empty(); }

            __declspec(property(get = get_LastOperationStartTime)) Common::DateTime LastOperationStartTime;
            Common::DateTime get_LastOperationStartTime() const { return lastOperationStartTime_; }

            void AddInstance(Common::ApiMonitoring::ApiName::Enum operation)
            {
                AssertIfInSingleInstanceStore(operation);

                auto result = multiInstanceOperations_.insert(std::make_pair(operation, 1));

                if (!result.second)
                {
                    ++result.first->second;
                }
            }

            void RemoveInstance(Common::ApiMonitoring::ApiName::Enum operation)
            {
                AssertIfInSingleInstanceStore(operation);

                auto it = multiInstanceOperations_.find(operation);
                ASSERT_IF(it == multiInstanceOperations_.end(), "Did not find multi instance operation");

                if (it->second == 1)
                {
                    multiInstanceOperations_.erase(it);
                }
                else
                {
                    --it->second;
                }
            }

            void StartOperation(Common::ApiMonitoring::ApiName::Enum operation, Common::ApiMonitoring::ApiCallDescriptionSPtr const & description)
            {
                AssertIfInMultiInstanceStore(operation);

                lastOperationStartTime_ = Common::DateTime::Now();

                ExecutingOperation executingOp;
                executingOp.StartOperation(description);
                auto result = singleInstanceOperations_.insert(std::make_pair(operation, std::move(executingOp)));
                ASSERT_IF(!result.second, "Found an existing operation when starting an op");
            }

            void ContinueOperation(Common::ApiMonitoring::ApiName::Enum operation, Common::AsyncOperationSPtr const & asyncOp)
            {
                AssertIfInMultiInstanceStore(operation);

                auto it = FindSingleInstanceOperation(operation);
                it->second.ContinueOperation(asyncOp);
            }

            Common::ApiMonitoring::ApiCallDescriptionSPtr FinishOperation(Common::ApiMonitoring::ApiName::Enum operation)
            {
                auto it = FindSingleInstanceOperation(operation);
                auto rv = it->second.FinishOperation();
                singleInstanceOperations_.erase(it);
                return rv;
            }

            ExecutingOperation const * GetOperation(Common::ApiMonitoring::ApiName::Enum operation) const
            {
                auto it = singleInstanceOperations_.find(operation);
                if (it == singleInstanceOperations_.cend())
                {
                    return nullptr;
                }

                return &it->second;
            }

            std::vector<Common::ApiMonitoring::ApiName::Enum> GetRunningOperations() const
            {
                std::vector<Common::ApiMonitoring::ApiName::Enum> rv;

                for (auto const & it : singleInstanceOperations_)
                {
                    rv.push_back(it.first);
                }

                for (auto const & it : multiInstanceOperations_)
                {
                    rv.push_back(it.first);
                }

                return rv;
            }

            std::vector<Common::AsyncOperationSPtr> GetRunningAsyncOperations(std::function<bool (Common::ApiMonitoring::ApiName::Enum)> predicate)
            {
                std::vector<Common::AsyncOperationSPtr> rv;
                
                for (auto const & it : singleInstanceOperations_)
                {
                    if (it.second.AsyncOperation && predicate(it.first))
                    {
                        rv.push_back(it.second.AsyncOperation);
                    }
                }

                return rv;
            }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
            {
                for (auto const & it: singleInstanceOperations_)
                {
                    writer.Write("{0} ", it.first);
                }

                for (auto const & it : multiInstanceOperations_)
                {
                    writer.Write("{0}:{1} ", it.first, it.second);
                }
            }

        private:
            typedef std::map<Common::ApiMonitoring::ApiName::Enum, ExecutingOperation> SingleInstanceOperationStore;
            typedef std::map<Common::ApiMonitoring::ApiName::Enum, int> MultiInstanceOperationStore;

            SingleInstanceOperationStore::iterator FindSingleInstanceOperation(Common::ApiMonitoring::ApiName::Enum key)
            {
                auto it = singleInstanceOperations_.find(key);
                ASSERT_IF(it == singleInstanceOperations_.end(), "Did not find item");
                return it;
            }

            void AssertIfInSingleInstanceStore(Common::ApiMonitoring::ApiName::Enum key)
            {
                ASSERT_IF(singleInstanceOperations_.find(key) != singleInstanceOperations_.end(), "Found in single instance");
            }

            void AssertIfInMultiInstanceStore(Common::ApiMonitoring::ApiName::Enum key)
            {
                ASSERT_IF(multiInstanceOperations_.find(key) != multiInstanceOperations_.end(), "Found in multi instance");
            }

            MultiInstanceOperationStore multiInstanceOperations_;
            SingleInstanceOperationStore singleInstanceOperations_;
            Common::DateTime lastOperationStartTime_;
        };
    }
}



