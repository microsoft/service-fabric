// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"

#ifdef PLATFORM_UNIX
#include <utility>
#include <ext/hash_map>
#else
#include "Api/Definitions/ApiDefinitions.h"
#define SECURITY_WIN32
#include <security.h>
#include <wincrypt.h>
#include <Ntdsapi.h>
#endif

#include "FabricCommon.h"

#include "Functor.h"
#include "TransportFwd.h"
#include "ConstCharPtrLess.h"
#include "Constants.h"

#include "MessageHeaderId.h"
#include "MessageHeader.h"
#include "Actor.h"
#include "ActorHeader.h"
#include "ActionHeader.h"
#include "MessageId.h"
#include "MessageIdHeader.h"
#include "ExpectsReplyHeader.h"
#include "RelatesToHeader.h"
#include "TimeoutHeader.h"
#include "RequestInstanceHeader.h"
#include "RetryHeader.h"
#include "FabricActivityHeader.h"
#include "FaultHeader.h"
#include "UncorrelatedReplyHeader.h"
#include "ForwardMessageHeader.h"
#include "HighPriorityHeader.h"
#include "RejectFaultBody.h"

#include "ByteBique.h"
#include "BiqueChunkIterator.h"
#include "BiqueRangeStream.h"
#include "BiqueWriteStream.h"

#include "MessageHeaders.h"
#include "MessageHeadersCollection.h"
#include "MessageHeaderTrace.h"
#include "Message.h"
#include "TransportSecurity.h"
#include "SecurityUtil.h"

#include "TransportFlags.h"
#include "TransportPriority.h"
#include "ISendTarget.h"
#include "RequestTable.h"
#include "RequestAsyncOperation.h"
#include "IDatagramTransport.h"
#include "RequestReply.h"
#include "ReceiverContext.h"
#include "DuplexRequestReply.h"
#include "DemuxerT.h"
#include "Demuxer.h"
#include "IpcHeader.h"
#include "IpcReceiverContext.h"
#include "IpcDemuxer.h"
#include "IpcServer.h"
#include "IpcClient.h"

#include "NamedAddress.h"
#include "DatagramTransportFactory.h"
#include "TcpTransportUtility.h"

#include "MulticastSendTarget.h"
#include "MulticastDatagramSender.h"
