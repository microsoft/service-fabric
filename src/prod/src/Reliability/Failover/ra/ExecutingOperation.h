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
            This class represents any operation that is running
        */
        class ExecutingOperation
        {
            DEFAULT_COPY_ASSIGNMENT(ExecutingOperation);
            
        public:
            ExecutingOperation() {}

            ExecutingOperation(ExecutingOperation && other) :
                asyncOp_(std::move(other.asyncOp_)),
                description_(std::move(other.description_))
            {
            }

            ExecutingOperation & operator=(ExecutingOperation && other)
            {
                if (this != &other)
                {
                    asyncOp_ = std::move(other.asyncOp_);
                    description_ = std::move(other.description_);
                }

                return *this;
            }

            __declspec(property(get = get_AsyncOperation)) Common::AsyncOperationSPtr const & AsyncOperation;
            Common::AsyncOperationSPtr const & get_AsyncOperation() const { return asyncOp_;}

            __declspec(property(get = get_IsRunning)) bool IsRunning;
            bool get_IsRunning() const { return asyncOp_ != nullptr; }

            void StartOperation(Common::ApiMonitoring::ApiCallDescriptionSPtr const & description)
            {
                ASSERT_IF(description_ != nullptr, "Description must be null");
                description_ = description;
            }

            void ContinueOperation(Common::AsyncOperationSPtr const & asyncOp)
            {
                ASSERT_IF(asyncOp == nullptr, "parameter cant be null");
                ASSERT_IF(asyncOp_ != nullptr, "Parameter can't be set");
                asyncOp_ = asyncOp;
            }

            Common::ApiMonitoring::ApiCallDescriptionSPtr FinishOperation()
            {
                ASSERT_IF(description_ == nullptr, "Description cant be null");
                Common::ApiMonitoring::ApiCallDescriptionSPtr rv;
                std::swap(rv, description_);
                asyncOp_ = nullptr;
                return rv;
            }

        private:
            Common::ApiMonitoring::ApiCallDescriptionSPtr description_;
            Common::AsyncOperationSPtr asyncOp_;
        };
    }
}


