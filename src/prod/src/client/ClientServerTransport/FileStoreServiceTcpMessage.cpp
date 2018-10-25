// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ClientServerTransport;

GlobalWString FileStoreServiceTcpMessage::GetStagingLocationAction = make_global<wstring>(L"GetStagingLocationAction");
GlobalWString FileStoreServiceTcpMessage::GetStoreLocationAction = make_global<wstring>(L"GetStoreLocationAction");
GlobalWString FileStoreServiceTcpMessage::GetStoreLocationsAction = make_global<wstring>(L"GetStoreLocationsAction");
GlobalWString FileStoreServiceTcpMessage::UploadAction = make_global<wstring>(L"UploadAction");
GlobalWString FileStoreServiceTcpMessage::CopyAction = make_global<wstring>(L"CopyAction");
GlobalWString FileStoreServiceTcpMessage::DeleteAction = make_global<wstring>(L"DeleteAction");
GlobalWString FileStoreServiceTcpMessage::CheckExistenceAction = make_global<wstring>(L"CheckExistenceAction");
GlobalWString FileStoreServiceTcpMessage::ListAction = make_global<wstring>(L"ListAction");
GlobalWString FileStoreServiceTcpMessage::InternalListAction = make_global<wstring>(L"InternalListAction");
GlobalWString FileStoreServiceTcpMessage::ListUploadSessionAction = make_global<wstring>(L"ListUploadSessionAction");
GlobalWString FileStoreServiceTcpMessage::CreateUploadSessionAction = make_global<wstring>(L"CreateUploadSessionAction");
GlobalWString FileStoreServiceTcpMessage::DeleteUploadSessionAction = make_global<wstring>(L"DeleteUploadSessionAction");
GlobalWString FileStoreServiceTcpMessage::UploadChunkAction = make_global<wstring>(L"UploadChunkAction");
GlobalWString FileStoreServiceTcpMessage::UploadChunkContentAction = make_global<wstring>(L"UploadChunkContentAction");
GlobalWString FileStoreServiceTcpMessage::CommitUploadSessionAction = make_global<wstring>(L"CommitUploadSessionAction");

const Transport::Actor::Enum FileStoreServiceTcpMessage::actor_ = Transport::Actor::FileStoreService;
