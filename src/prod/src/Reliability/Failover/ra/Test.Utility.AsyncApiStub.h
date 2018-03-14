// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// Defines for things specific to this test dll
namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            template<typename TParameters, typename TReturnValue>
            class AsyncApiStub
            {
            public:
                class AsyncOperationWithHolder : public Common::AsyncOperation
                {
                public:
                    AsyncOperationWithHolder(
                        bool completeSynchronously,
                        TParameters const & parameters,
                        TReturnValue && returnValue,
                        Common::ErrorCodeValue::Enum error,
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) : 
                        Common::AsyncOperation(callback, parent),
                        completeSynchronously_(completeSynchronously),
                        error_(error),
                        returnValue_(std::move(returnValue)),
                        parameters_(parameters)
                    {
                    }

                    AsyncOperationWithHolder(
                        bool completeSynchronously,
                        TParameters && parameters,
                        TReturnValue && returnValue,
                        Common::ErrorCodeValue::Enum error,
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) :
                        Common::AsyncOperation(callback, parent),
                        completeSynchronously_(completeSynchronously),
                        error_(error),
                        returnValue_(std::move(returnValue)),
                        parameters_(std::move(parameters))
                    {
                    }
                protected:
                    void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
                    {
                        if (completeSynchronously_)
                        {
                            TryComplete(thisSPtr, error_);
                        }
                    }

                public:
                    __declspec(property(get = get_Error)) Common::ErrorCodeValue::Enum Error;
                    Common::ErrorCodeValue::Enum get_Error() const { return error_; }

                    __declspec(property(get = get_ReturnValue)) TReturnValue const & ReturnValue;
                    TReturnValue const & get_ReturnValue() const { return returnValue_; }

                    __declspec(property(get = get_Parameters)) TParameters const & Parameters;
                    TParameters const & get_Parameters() const
                    {
                        return parameters_;
                    }

                    void GetReturnValue(__out TReturnValue & valueOut)
                    {
                        valueOut = std::move(returnValue_);
                    }

                    void SetReturnValue(TReturnValue const & returnValue)
                    {
                        returnValue_ = returnValue;
                    }

                    void SetReturnValue(TReturnValue && returnValue)
                    {
                        returnValue_ = std::move(returnValue);
                    }

                private:
                    bool completeSynchronously_;
                    Common::ErrorCodeValue::Enum error_;
                    TReturnValue returnValue_;
                    TParameters parameters_;
                };

                typedef std::shared_ptr<AsyncOperationWithHolder> AsyncOperationTSPtr;
                typedef std::vector<AsyncOperationTSPtr> ApiCallList;
                typedef std::vector<TParameters> ApiCallParameterList;

                AsyncApiStub(std::wstring const & name) : 
                    completeSynchronously_(true),
                    error_(Common::ErrorCodeValue::Success),
                    returnValue_(TReturnValue()),
                    name_(name)
                {
                }

                __declspec(property(get = get_Current)) Common::AsyncOperationSPtr Current;
                Common::AsyncOperationSPtr get_Current() const { return std::static_pointer_cast<Common::AsyncOperation>(currentOperation_); }

                __declspec(property(get = get_CallList)) ApiCallList const & CallList;
                ApiCallList const & get_CallList() const
                {
                    return apiCalls_;
                }

                void SetCompleteSynchronouslyWithSuccess(TReturnValue const & returnValue)
                {
                    completeSynchronously_ = true;
                    returnValue_ = returnValue;
                    error_ = Common::ErrorCodeValue::Success;
                }

                void SetCompleteSynchronouslyWithSuccess(TReturnValue && returnValue)
                {
                    completeSynchronously_ = true;
                    returnValue_ = std::move(returnValue);
                    error_ = Common::ErrorCodeValue::Success;
                }

                void SetCompleteSynchronouslyWithError()
                {
                    SetCompleteSynchronouslyWithError(Common::ErrorCodeValue::GlobalLeaseLost);
                }

                void SetCompleteSynchronouslyWithError(Common::ErrorCodeValue::Enum error)
                {
                    completeSynchronously_ = true;
                    error_ = error;
                }

                void SetCompleteAsynchronously()
                {
                    completeSynchronously_ = false;
                }

                Common::ErrorCode OnCallAsSynchronousApi(TParameters const & parameters)
                {
                    auto op = OnCall(parameters, [](Common::AsyncOperationSPtr const&) {}, nullptr);
                    return End(op);
                }

                Common::AsyncOperationSPtr OnCall(TParameters const & parameters, Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent)
                {
                    auto op = Common::AsyncOperation::CreateAndStart<AsyncOperationWithHolder>(
                        completeSynchronously_,
                        parameters,
                        std::move(returnValue_),
                        error_,
                        callback,
                        parent);

                    currentOperation_ = std::static_pointer_cast<AsyncOperationWithHolder>(op);

                    apiCalls_.push_back(currentOperation_);

                    return op;
                }

                Common::AsyncOperationSPtr OnCall2(TParameters && parameters, Common::AsyncCallback const & callback, Common::AsyncOperationSPtr const & parent)
                {
                    auto op = Common::AsyncOperation::CreateAndStart<AsyncOperationWithHolder>(
                        completeSynchronously_,
                        std::move(parameters),
                        std::move(returnValue_),
                        error_,
                        callback,
                        parent);

                    currentOperation_ = std::static_pointer_cast<AsyncOperationWithHolder>(op);

                    apiCalls_.push_back(currentOperation_);

                    return op;
                }

                void FinishOperationWithSuccess(TReturnValue const & returnValue)
                {
                    VerifyAsyncOperationIsRunning();
                    currentOperation_->SetReturnValue(returnValue);

                    FinishOperationWithError(Common::ErrorCodeValue::Success);
                }

                void FinishOperationWithSuccess(TReturnValue && returnValue)
                {
                    VerifyAsyncOperationIsRunning();
                    currentOperation_->SetReturnValue(std::move(returnValue));

                    FinishOperationWithError(Common::ErrorCodeValue::Success);
                }

                void FinishOperationWithError()
                {
                    FinishOperationWithError(Common::ErrorCodeValue::GlobalLeaseLost);
                }

                void FinishOperationWithError(Common::ErrorCodeValue::Enum error)
                {
                    VerifyAsyncOperationIsRunning();
                    currentOperation_->TryComplete(currentOperation_, error);
                }

                void Clear()
                {
                    currentOperation_.reset();
                    apiCalls_.clear();
                }

                void VerifyCancelRequested() const
                {
                    VerifyCancelRequested(true);
                }

                void VerifyCancelNotRequested() const
                {
                    VerifyCancelRequested(false);
                }

                void VerifyCancelRequested(bool requested) const
                {
                    VerifyAsyncOperationIsRunning();
                    Verify::AreEqualLogOnError(requested, currentOperation_->IsCancelRequested, L"Cancel Requested");
                }

                void VerifyCallCount(size_t expected) const
                {
                    Verify::AreEqualLogOnError(expected, apiCalls_.size(), Common::wformatString("CallCount {0}", name_));
                }

                void VerifyNoCalls() const
                {
                    VerifyCallCount(0);
                }

                TParameters const & GetParametersAt(size_t index) const
                {
                    if (apiCalls_.size() <= index)
                    {
                        VERIFY_FAIL(Common::wformatString("Asked to return parameters for non existant index at {0}. Size {1}. Index {2}", name_, apiCalls_.size(), index).c_str());
                    }

                    return apiCalls_[index]->Parameters;
                }

                ApiCallParameterList GetParameterList() const
                {
                    ApiCallParameterList rv;

                    for (auto const & it : apiCalls_)
                    {
                        rv.push_back(it->Parameters);
                    }

                    return rv;
                }

                static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation)
                {
                    auto casted = CastOperation(operation);
                    return casted->Error;
                }

                static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, TReturnValue & returnValue)
                {
                    auto casted = CastOperation(operation);
                    casted->GetReturnValue(returnValue);
                    return casted->Error;
                }

            private:

                static AsyncOperationTSPtr CastOperation(Common::AsyncOperationSPtr const & operation)
                {
                    return std::static_pointer_cast<AsyncOperationWithHolder>(operation);
                }

                void VerifyAsyncOperationIsRunning() const
                {
                    Verify::IsTrueLogOnError(currentOperation_ != nullptr, L"AsyncOperation must be running");
                }

                bool completeSynchronously_;
                Common::ErrorCodeValue::Enum error_;
                TReturnValue returnValue_;
                std::wstring name_;

                AsyncOperationTSPtr currentOperation_;
                ApiCallList apiCalls_;
            };
        }
    }
}
