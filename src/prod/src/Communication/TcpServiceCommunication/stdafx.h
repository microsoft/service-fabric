// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "Transport/Transport.h"
#include "api/definitions/ApiDefinitions.h"
#include "Communication/TcpServiceCommunication/TcpClientServerPointers.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationHelper.h"
#include "Communication/TcpServiceCommunication/Constants.h"
#include "Communication/TcpServiceCommunication/TcpClientIdHeader.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationErrorHeader.h"
#include "Communication/TcpServiceCommunication/IsAsyncOperationHeader.h"
#include "Communication/TcpServiceCommunication/ServiceLocationActorHeader.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationTransportSettings.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationClient.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationMessageHandlerCollection.h"
#include "Communication/TcpServiceCommunication/ServiceMethodCallDispatcher.h"
#include "Communication/TcpServiceCommunication/ClientConnectionStateInfo.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationTransport.h"
#include "Communication/TcpServiceCommunication/ClientConnection.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationListener.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationListenerFactory.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationClientFactory.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationClient.TryConnectAsyncOperation.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationClient.DisconnectAsyncOperation.h"
#include "Communication/TcpServiceCommunication/ServiceCommunicationClient.SendRequestAsyncOperation.h"
