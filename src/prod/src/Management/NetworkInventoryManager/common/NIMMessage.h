// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        class NIMMessage
        {
        public:
            NIMMessage(
                Transport::Actor::Enum actor,
                std::wstring const & action)			
                : actor_(actor),
                actionHeader_(action),
                idempotent_(true)
            {
            }

            static NIMMessage const & GetCreateNetwork() { return CreateNetwork; }
            static NIMMessage const & GetDeleteNetwork() { return DeleteNetwork; }
            static NIMMessage const & GetValidateNetwork() { return ValidateNetwork; }
            
            // Creates a message with the specified body
            template <class T>
            Transport::MessageUPtr CreateMessage(T const& t) const
            {
                Transport::MessageUPtr message = Common::make_unique<Transport::Message>(t);
                AddHeaders(*message);

                return message;
            }

            __declspec (property(get = get_Action)) const std::wstring & Action;
            std::wstring const& get_Action() const { return actionHeader_.Action; };

            static void AddActivityHeader(Transport::Message & message, Transport::FabricActivityHeader const & activityHeader);
            static Common::ActivityId NIMMessage::GetActivityId(Transport::Message & message);

            static Transport::Actor::Enum const NIMActor;

        private:
            static Common::Global<NIMMessage> CreateCommonToNIMMessage(std::wstring const & action)
            {
                return Common::make_global<NIMMessage>(NIMActor, action);
            }

            void AddHeaders(Transport::Message & message) const;

            static Common::Global<NIMMessage> CreateNetwork;
            static Common::Global<NIMMessage> DeleteNetwork;
            static Common::Global<NIMMessage> ValidateNetwork;

            Transport::Actor::Enum actor_;
            Transport::ActionHeader actionHeader_;
            bool idempotent_;
        };
    }
}
