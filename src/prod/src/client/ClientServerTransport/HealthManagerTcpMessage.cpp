// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ClientServerTransport;

GlobalWString HealthManagerTcpMessage::ReportHealthAction = make_global<wstring>(L"ReportHealth");
GlobalWString HealthManagerTcpMessage::GetSequenceStreamProgressAction = make_global<wstring>(L"GetSequenceStreamProgress");

const Transport::Actor::Enum HealthManagerTcpMessage::actor_ = Transport::Actor::HM;
