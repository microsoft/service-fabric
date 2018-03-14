// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Communication::TcpServiceCommunication;
using namespace std;

GlobalWString Constants::ListenAddressPathDelimiter = make_global<wstring>(L"+");
GlobalWString Constants::FabricServiceCommunicationAction = make_global<wstring>(L"FabricServiceCommunication");
GlobalWString Constants::ConnectAction = make_global<wstring>(L"Connect");
GlobalWString Constants::ConnectActorDelimiter = make_global<wstring>(L",");
