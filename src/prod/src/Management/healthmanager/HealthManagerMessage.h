// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {    
        class HealthManagerMessage
        {
        public:
            // Reply actions
            static Common::GlobalWString OperationSuccessAction;

            // Replies
            static Transport::MessageUPtr GetClientOperationSuccess(
                Common::ActivityId const & activityId) 
            { 
                return CreateMessage(OperationSuccessAction, activityId);
            }

            template <class TBody>
            static Transport::MessageUPtr GetClientOperationSuccess(
                TBody const & body,
                Common::ActivityId const & activityId) 
            {
                return CreateMessage(OperationSuccessAction, body, activityId);
            }

            static Transport::MessageUPtr GetQueryReply(
                ServiceModel::QueryResult && replyBody,
                Common::ActivityId const & activityId) 
            { 
                return CreateMessage(OperationSuccessAction, replyBody, activityId);
            }

        private:

            template <class TBody> 
            static Transport::MessageUPtr CreateMessage(
                std::wstring const & action, 
                TBody const & body,
                Common::ActivityId const & activityId)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
                HealthManagerMessage::SetHeaders(*message, action, activityId);
                return message;
            }

            static Transport::MessageUPtr CreateMessage(
                std::wstring const & action,
                Common::ActivityId const & activityId);

            static void SetHeaders(
                __in Transport::Message & message, 
                std::wstring const & action,
                Common::ActivityId const & activityId);
        };
    }
}
