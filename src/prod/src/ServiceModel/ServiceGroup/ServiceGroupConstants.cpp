// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceGroup;

StringLiteral const ServiceGroupConstants::TraceSource("ServiceGroupUtility");

GlobalWString ServiceGroupConstants::MemberServiceNameDelimiter = make_global<wstring>(L"#");
GlobalWString ServiceGroupConstants::ServiceAddressEscapedDelimiter = make_global<wstring>(L"(%)");
GlobalWString ServiceGroupConstants::ServiceAddressDoubleDelimiter = make_global<wstring>(L"%%");
GlobalWString ServiceGroupConstants::ServiceAddressDelimiter = make_global<wstring>(L"%");
