// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace RepairManager
    {
        class RepairTask;
        class CancelRepairTaskMessageBody;
        class ApproveRepairTaskMessageBody;
        class DeleteRepairTaskMessageBody;
        class UpdateRepairTaskHealthPolicyMessageBody;
        class RepairScopeIdentifierBase;
    }
}
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationTransportSettings;
        typedef std::unique_ptr<ServiceCommunicationTransportSettings> ServiceCommunicationTransportSettingsUPtr;
        class ServiceMethodCallDispatcher;
        typedef std::shared_ptr<ServiceMethodCallDispatcher> ServiceMethodCallDispatcherSPtr;
    }
}
namespace Transport
{
    class Message;
    typedef std::unique_ptr<Message> MessageUPtr;
}
