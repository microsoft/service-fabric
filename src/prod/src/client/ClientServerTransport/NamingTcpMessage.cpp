// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;

GlobalWString NamingTcpMessage::PingRequestAction = make_global<wstring>(L"PingRequest");
GlobalWString NamingTcpMessage::PingReplyAction = make_global<wstring>(L"PingReply");
GlobalWString NamingTcpMessage::DeactivateNodeAction = make_global<wstring>(L"DeactivateNodeRequest");
GlobalWString NamingTcpMessage::ActivateNodeAction = make_global<wstring>(L"ActivateNodeRequest");
GlobalWString NamingTcpMessage::NodeStateRemovedAction = make_global<wstring>(L"NodeStateRemovedRequest");
GlobalWString NamingTcpMessage::DeactivateNodesBatchRequestAction = make_global<wstring>(L"DeactivateNodesBatchRequest");
GlobalWString NamingTcpMessage::RemoveNodeDeactivationsRequestAction = make_global<wstring>(L"RemoveNodeDeactivationsRequest");
GlobalWString NamingTcpMessage::GetNodeDeactivationStatusRequestAction = make_global<wstring>(L"GetNodeDeactivationStatusRequest");
GlobalWString NamingTcpMessage::RecoverPartitionsAction = make_global<wstring>(L"RecoverPartitionsRequest");
GlobalWString NamingTcpMessage::RecoverPartitionAction = make_global<wstring>(L"RecoverPartitionRequest");
GlobalWString NamingTcpMessage::RecoverServicePartitionsAction = make_global<wstring>(L"RecoverServicePartitionsRequest");
GlobalWString NamingTcpMessage::RecoverSystemPartitionsAction = make_global<wstring>(L"RecoverSystemPartitionsRequest");
GlobalWString NamingTcpMessage::ReportFaultAction = make_global<wstring>(L"ReportFaultRequest");
GlobalWString NamingTcpMessage::CreateServiceAction = make_global<wstring>(L"CreateServiceRequest");
GlobalWString NamingTcpMessage::UpdateServiceAction = make_global<wstring>(L"UpdateServiceRequest");
GlobalWString NamingTcpMessage::DeleteServiceAction = make_global<wstring>(L"DeleteServiceRequest");
GlobalWString NamingTcpMessage::GetServiceDescriptionAction = make_global<wstring>(L"GetServiceDescriptionRequest");
GlobalWString NamingTcpMessage::CreateNameAction = make_global<wstring>(L"CreateNameRequest");
GlobalWString NamingTcpMessage::DeleteNameAction = make_global<wstring>(L"DeleteNameRequest");
GlobalWString NamingTcpMessage::NameExistsAction = make_global<wstring>(L"NameExistsRequest");
GlobalWString NamingTcpMessage::EnumerateSubNamesAction = make_global<wstring>(L"EnumerateSubNamesRequest");
GlobalWString NamingTcpMessage::EnumeratePropertiesAction = make_global<wstring>(L"EnumeratePropertiesRequest");
GlobalWString NamingTcpMessage::ResolveNameOwnerAction = make_global<wstring>(L"ResolveNameOwnerRequest");
GlobalWString NamingTcpMessage::PropertyBatchAction = make_global<wstring>(L"PropertyBatchRequest");
GlobalWString NamingTcpMessage::ResolveServiceAction = make_global<wstring>(L"ResolveServiceRequest");
GlobalWString NamingTcpMessage::ResolveSystemServiceAction = make_global<wstring>(L"ResolveSystemServiceRequest");
GlobalWString NamingTcpMessage::PrefixResolveServiceAction = make_global<wstring>(L"PrefixResolveServiceRequest");
GlobalWString NamingTcpMessage::ResolvePartitionAction = make_global<wstring>(L"ResolvePartitionRequest");
GlobalWString NamingTcpMessage::ServiceLocationChangeListenerAction = make_global<wstring>(L"ServiceLocationChangeListener");
GlobalWString NamingTcpMessage::StartNodeAction = make_global<wstring>(L"StartNodeRequest");
GlobalWString NamingTcpMessage::SetAclAction = make_global<wstring>(L"SetAcl");
GlobalWString NamingTcpMessage::GetAclAction = make_global<wstring>(L"GetAcl");
GlobalWString NamingTcpMessage::RegisterServiceNotificationFilterRequestAction = make_global<wstring>(L"RegisterServiceNotificationFilterRequest");
GlobalWString NamingTcpMessage::RegisterServiceNotificationFilterReplyAction = make_global<wstring>(L"RegisterServiceNotificationFilterReply");
GlobalWString NamingTcpMessage::UnregisterServiceNotificationFilterRequestAction = make_global<wstring>(L"UnregisterServiceNotificationFilterRequest");
GlobalWString NamingTcpMessage::UnregisterServiceNotificationFilterReplyAction = make_global<wstring>(L"UnregisterServiceNotificationFilterReply");
GlobalWString NamingTcpMessage::ServiceNotificationRequestAction = make_global<wstring>(L"ServiceNotificationRequest");
GlobalWString NamingTcpMessage::ServiceNotificationReplyAction = make_global<wstring>(L"ServiceNotificationReply");
GlobalWString NamingTcpMessage::NotificationClientConnectionRequestAction = make_global<wstring>(L"NotificationClientConnectionRequest");
GlobalWString NamingTcpMessage::NotificationClientConnectionReplyAction = make_global<wstring>(L"NotificationClientConnectionReply");
GlobalWString NamingTcpMessage::NotificationClientSynchronizationRequestAction = make_global<wstring>(L"NotificationClientSynchronizationRequest");
GlobalWString NamingTcpMessage::NotificationClientSynchronizationReplyAction = make_global<wstring>(L"NotificationClientSynchronizationReply");
GlobalWString NamingTcpMessage::CreateSystemServiceRequestAction = make_global<wstring>(L"CreateSystemServiceRequest");
GlobalWString NamingTcpMessage::DeleteSystemServiceRequestAction = make_global<wstring>(L"DeleteSystemServiceRequest");
GlobalWString NamingTcpMessage::DeleteSystemServiceReplyAction = make_global<wstring>(L"DeleteSystemServiceReply");
GlobalWString NamingTcpMessage::ResolveSystemServiceRequestAction = make_global<wstring>(L"ResolveSystemServiceRequest");
GlobalWString NamingTcpMessage::ResolveSystemServiceReplyAction = make_global<wstring>(L"ResolveSystemServiceReply");
GlobalWString NamingTcpMessage::ResetPartitionLoadAction = make_global<wstring>(L"ResetPartitionLoad");
GlobalWString NamingTcpMessage::ToggleVerboseServicePlacementHealthReportingAction = make_global<wstring>(L"ToggleVerboseServicePlacementHealthReporting");
GlobalWString NamingTcpMessage::CreateNetworkAction = make_global<wstring>(L"CreateNetwork");
GlobalWString NamingTcpMessage::DeleteNetworkAction = make_global<wstring>(L"DeleteNetwork");

GlobalWString NamingTcpMessage::ClientOperationFailureAction = make_global<wstring>(L"ClientOperationFailure");

// - these definition needs to be removed after abstracting tcp transport from client
GlobalWString NamingTcpMessage::DeleteServiceReplyAction = make_global<wstring>(L"DeleteServiceReply");

const Transport::Actor::Enum NamingTcpMessage::actor_ = Actor::NamingGateway;
