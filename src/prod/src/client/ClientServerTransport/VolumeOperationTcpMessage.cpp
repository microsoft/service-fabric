//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;

GlobalWString VolumeOperationTcpMessage::CreateVolumeAction = make_global<wstring>(L"CreateVolumeAction");
GlobalWString VolumeOperationTcpMessage::DeleteVolumeAction = make_global<wstring>(L"DeleteVolumeAction");

const Actor::Enum VolumeOperationTcpMessage::actor_ = Actor::CM;
