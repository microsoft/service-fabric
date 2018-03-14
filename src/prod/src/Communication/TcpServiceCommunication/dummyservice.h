// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class DummyService : public Api::IServiceCommunicationMessageHandler, public Common::ComponentRoot
        {
        public:
            DummyService()
                : notificationHandled_()
            {
            };

            ~DummyService()
            {
            }

            virtual Common::AsyncOperationSPtr BeginProcessRequest(
                std::wstring const & clientId,
                Transport::MessageUPtr && message,
                Common::TimeSpan const  & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual Common::ErrorCode EndProcessRequest(
                Common::AsyncOperationSPtr const &  operation,
                Transport::MessageUPtr & reply);

            virtual Common::ErrorCode HandleOneWay(
                std::wstring const & clientId,
                Transport::MessageUPtr && message);

            bool WaitForNotificationRecieved(Common::TimeSpan timeout)
            {
                return notificationHandled_.WaitOne(timeout);
            }

            virtual void Initialize() {
                //no-op 
                //Needed for V2 Stack
            }
        private:
            Common::ManualResetEvent notificationHandled_;
        };
    }
}
