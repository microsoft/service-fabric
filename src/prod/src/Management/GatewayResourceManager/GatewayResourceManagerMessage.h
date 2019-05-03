// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace GatewayResourceManager
    {
        class GatewayResourceManagerMessage
        {
        public:

            // Reply actions
            static Common::GlobalWString OperationSuccessAction;

            // Replies
            static Transport::MessageUPtr GetClientOperationSuccess() 
            { 
                return CreateMessage(OperationSuccessAction);
            }

            template <class TBody>
            static Transport::MessageUPtr GetClientOperationSuccess(TBody const& body)
            {
                return CreateMessage(OperationSuccessAction, body);
            }
           
            static Transport::MessageUPtr GetQueryReply(ServiceModel::QueryResult replyBody)
            {
              return CreateMessage(OperationSuccessAction, replyBody);
            }
           
        private:

            static void WrapForGatewayResourceManager(__inout Transport::Message &);

            static Transport::MessageUPtr CreateMessage(std::wstring const & action)
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>();
                GatewayResourceManagerMessage::SetHeaders(*message, action);
                return std::move(message);
            }

            template <class TBody> 
            static Transport::MessageUPtr CreateRoutedRequestMessage(std::wstring const & action, TBody const & body)
            {
                auto message = CreateMessage(action, body);
                WrapForGatewayResourceManager(*message);
                return std::move(message);
            }

            template <class TBody> 
            static Transport::MessageUPtr CreateMessage(std::wstring const & action, TBody const & body)
            {
                Transport::MessageUPtr message(Common::make_unique<Transport::Message>(body));
                GatewayResourceManagerMessage::SetHeaders(*message, action);
                return std::move(message);
            }

            static void SetHeaders(Transport::Message &, std::wstring const & action);
        };
    }
}
