// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS

#include "Transport/Transport.h"
#include "Common/KtlSF.Common.h"

#ifdef PLATFORM_UNIX
#include <sys/types.h>
#include <netinet/tcp.h>
#include <ifaddrs.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include "Common/CryptoUtility.Linux.h"
#include "Transport/TransportSecurity.Linux.h"
#else
#include <schannel.h>
#include <Ws2tcpip.h>
#include <Dsrole.h>
#include <unordered_map>
#include "Transport/TcpOverlappedIo.h"
#endif

#include "Transport/IdempotentHeader.h"
#include "Transport/NamedAddress.h"
#include "Transport/TcpConnectionState.h"
#include "Transport/TcpFrameHeader.h"
#include "Transport/ListenInstance.h"
#include "Transport/SecurityNegotiationHeader.h"
#include "Transport/IConnection.h"
#include "Transport/IoBuffer.h"
#include "Transport/ReceiveBuffer.h"
#include "Transport/TcpReceiveBuffer.h"
#include "Transport/SendBuffer.h"
#include "Transport/TcpSendBuffer.h"
#include "Transport/IBufferFactory.h"
#include "Transport/LTFrameHeader.h"
#include "Transport/LTMessageHeader.h"
#include "Transport/LTSendBuffer.h"
#include "Transport/LTReceiveBuffer.h"
#include "Transport/LTBufferFactory.h"
#include "Transport/TcpConnection.h"
#include "Transport/TcpSendTarget.h"
#include "Transport/IListenSocket.h"
#include "Transport/TcpBufferFactory.h"
#include "Transport/TransportConfig.h"
#include "Transport/TcpDatagramTransport.h"
#include "Transport/MemoryTransport.h"
#include "Transport/UnreliableTransport.h"
#include "Transport/PerfCounters.h"
#include "Transport/Throttle.h"
#include "Transport/ListenSocket.h"
#include "Transport/SecurityBuffers.h"
#include "Transport/SecurityCredentials.h"
#include "Transport/SecurityContext.h"
#include "Transport/SecurityContextSsl.h"
#include "Transport/SecurityContextWin.h"
#include "Transport/CredentialType.h"
#include "Transport/ClaimsMessage.h"

#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfiguration.h"
#include "Transport/UnreliableTransportConfig.h"
#include "Transport/UnreliableTransportEventSource.h"
#include "Transport/TransportEventSource.h"
#include "Transport/IpcEventSource.h"
#include "Transport/TransportTable.h"
#include "Transport/ClientRoleHeader.h"
#include "Transport/SecurityHeader.h"
#include "Transport/ClientAuthHeader.h"
#include "Transport/ServerAuthHeader.h"
#include "Transport/AcceptThrottle.h"
