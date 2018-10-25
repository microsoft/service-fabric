// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ClientServerTransport;

const __int64 ClientProtocolVersionHeader::CurrentMajorVersion;
const __int64 ClientProtocolVersionHeader::CurrentMinorVersion;

Global<ClientProtocolVersionHeader> ClientProtocolVersionHeader::CurrentVersionHeader = make_global<ClientProtocolVersionHeader>(ClientProtocolVersionHeader::CurrentMajorVersion, ClientProtocolVersionHeader::CurrentMinorVersion);
Global<ClientProtocolVersionHeader> ClientProtocolVersionHeader::SingleFileUploadVersionHeader = make_global<ClientProtocolVersionHeader>(1, 1);