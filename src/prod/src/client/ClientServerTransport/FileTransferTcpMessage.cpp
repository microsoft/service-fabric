// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace SystemServices;
using namespace ClientServerTransport;

GlobalWString FileTransferTcpMessage::FileContentAction = make_global<wstring>(L"FileContent");
GlobalWString FileTransferTcpMessage::FileDownloadAction = make_global<wstring>(L"FileDownload");
GlobalWString FileTransferTcpMessage::ClientOperationSuccessAction = make_global<wstring>(L"ClientOperationSuccess");
GlobalWString FileTransferTcpMessage::ClientOperationFailureAction = make_global<wstring>(L"ClientOperationFailure");
GlobalWString FileTransferTcpMessage::FileCreateAction = make_global<wstring>(L"FileCreate");
GlobalWString FileTransferTcpMessage::FileUploadCommitAction = make_global<wstring>(L"FileUploadCommit");
GlobalWString FileTransferTcpMessage::FileUploadCommitAckAction = make_global<wstring>(L"FileUploadCommitAck");
GlobalWString FileTransferTcpMessage::FileUploadDeleteSessionAction = make_global<wstring>(L"FileUploadDeleteSession");

