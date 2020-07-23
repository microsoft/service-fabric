// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Query;


GlobalWString QueryAddresses::FMAddressSegment      = make_global<std::wstring>(L"FM");
GlobalWString QueryAddresses::FMMAddressSegment     = make_global<std::wstring>(L"FMM");
GlobalWString QueryAddresses::CMAddressSegment      = make_global<std::wstring>(L"CM");
GlobalWString QueryAddresses::HMAddressSegment      = make_global<std::wstring>(L"HM");
GlobalWString QueryAddresses::HostingAddressSegment = make_global<std::wstring>(L"Hosting");
GlobalWString QueryAddresses::TestabilityAddressSegment = make_global<std::wstring>(L"TestabilityAgent");
GlobalWString QueryAddresses::RAAddressSegment      = make_global<std::wstring>(L"RA");
GlobalWString QueryAddresses::RMAddressSegment      = make_global<std::wstring>(L"RM");
GlobalWString QueryAddresses::TSAddressSegment      = make_global<std::wstring>(L"TS");
GlobalWString QueryAddresses::UOSAddressSegment     = make_global<std::wstring>(L"UOS");
GlobalWString QueryAddresses::GRMAddressSegment     = make_global<std::wstring>(L"GRM");

QueryAddressGenerator QueryAddresses::GetGateway()
{ return Query::QueryAddressGenerator("/"); }

QueryAddressGenerator QueryAddresses::GetHosting() 
{ return Query::QueryAddressGenerator("/Hosting[{0}]", QueryResourceProperties::Node::Name); }
        
QueryAddressGenerator QueryAddresses::GetFM() 
{ return Query::QueryAddressGenerator("/FM"); }

QueryAddressGenerator QueryAddresses::GetFMM() 
{ return Query::QueryAddressGenerator("/FMM"); }

QueryAddressGenerator QueryAddresses::GetCM() 
{ return Query::QueryAddressGenerator("/CM"); }

QueryAddressGenerator QueryAddresses::GetHM() 
{ return Query::QueryAddressGenerator("/HM"); }

QueryAddressGenerator QueryAddresses::GetHMViaCM() 
{ return Query::QueryAddressGenerator("/CM/HM"); }

QueryAddressGenerator QueryAddresses::GetRA() 
{ return Query::QueryAddressGenerator("/RA[{0}]", QueryResourceProperties::Node::Name); }

QueryAddressGenerator QueryAddresses::GetRM()
{ return Query::QueryAddressGenerator("/RM"); }

QueryAddressGenerator QueryAddresses::GetTS()
{ return Query::QueryAddressGenerator("/TS"); }

QueryAddressGenerator QueryAddresses::GetUOS()
{ return Query::QueryAddressGenerator("/UOS"); }

QueryAddressGenerator QueryAddresses::GetTestability()
{ return Query::QueryAddressGenerator("/TestabilityAgent[{0}]", QueryResourceProperties::Node::Name); }

QueryAddressGenerator QueryAddresses::GetGRM()
{ return Query::QueryAddressGenerator("/GRM"); } // Gateway Resource Manager
