// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class FileStoreServiceMessage
        {
        public:
            // Replies
            static Transport::MessageUPtr GetClientOperationSuccess(Common::ActivityId const & activityId) 
            {
                return CreateMessage(Constants::Actions::OperationSuccess, activityId);
            }

            template <class TBody>
            static Transport::MessageUPtr GetClientOperationSuccess(TBody const& body, Common::ActivityId const & activityId)
            {
                return CreateMessage(Constants::Actions::OperationSuccess, body, activityId);
            }

        private:          
            static Transport::MessageUPtr CreateMessage(std::wstring const & action, Common::ActivityId const & activityId)
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                FileStoreServiceMessage::SetHeaders(*message, action, activityId);
                return std::move(message);
            }

            template <class TBody> 
            static Transport::MessageUPtr CreateMessage(std::wstring const & action, TBody const & body, Common::ActivityId const & activityId)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
                FileStoreServiceMessage::SetHeaders(*message, action, activityId);
                return std::move(message);
            }

            static void SetHeaders(Transport::Message &, std::wstring const & action, Common::ActivityId const & activityId);
        };
    }
}
