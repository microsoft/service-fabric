// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Communication
{
    namespace NamespaceManager
    {
        class TestRequestBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            TestRequestBody() 
                : testByteCount_(0) 
            { 
            };
             
            TestRequestBody(
                size_t testByteCount)
                : testByteCount_(testByteCount) 
            { 
            }

            size_t GetTestByteCount() const { return testByteCount_; }

            FABRIC_FIELDS_01(testByteCount_);

        private:
            size_t testByteCount_;
        };

        class TestReplyBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            TestReplyBody() : data_(), error_(Common::ErrorCodeValue::Success) { }

            TestReplyBody(std::shared_ptr<std::vector<byte>> && data): data_(std::move(data)), error_(Common::ErrorCodeValue::Success) { }

            TestReplyBody(Common::ErrorCode && error): data_(), error_(std::move(error)) { }

            std::shared_ptr<std::vector<byte>> const & GetData() { return data_; }
            Common::ErrorCodeDetails & GetErrorDetails() { return error_; }

            FABRIC_FIELDS_02(data_, error_);

        private:
            std::shared_ptr<std::vector<byte>> data_;
            Common::ErrorCodeDetails error_;
        };

        class NamespaceManagerMessage : public Client::ClientServerRequestMessage
        {
        public:

            static Common::GlobalWString TestRequestAction;
            static Common::GlobalWString TestReplyAction;

            NamespaceManagerMessage(
            std::wstring const & action,
            std::unique_ptr<ServiceModel::ClientServerMessageBody> body,
            Common::ActivityId const & activityId)
            : Client::ClientServerRequestMessage(action, Transport::Actor::NM, std::move(body), activityId)
            {
            }

            static Client::ClientServerRequestMessageUPtr CreateTestRequest(
                Common::ActivityId const & activityId,
                size_t testByteCount)
            {
                return std::move(Common::make_unique<NamespaceManagerMessage>(TestRequestAction, std::move(Common::make_unique<TestRequestBody>(testByteCount)), activityId));
            }

            static Client::ClientServerRequestMessageUPtr CreateTestReply(
                Common::ActivityId const & activityId,
                TestReplyBody const & body)
            {
                return std::move(Common::make_unique<NamespaceManagerMessage>(TestReplyAction, std::move(Common::make_unique<TestReplyBody>(body)), activityId));
            }

        private:


        };
    }
}
