// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace SystemServices;

StringLiteral const TraceComponent("ExtractedEndpointList");

ExtractedEndpointList::ExtractedEndpointList()    
{
}

bool ExtractedEndpointList::FromString(std::wstring const& serializedString, ExtractedEndpointList & extractedEndpointList)
{
    ErrorCode error = JsonHelper::Deserialize(extractedEndpointList, serializedString);      
    if (!error.IsSuccess())
    {      
        wstring errorMessage = wformatString("Could not parse service location '{0}'", error);     
        Trace.WriteError(  
            TraceComponent,
            "{0}",
            errorMessage);
        Assert::TestAssert(
            "{0}",
            errorMessage);
              
        return false;
    } 

    return true;
}

bool ExtractedEndpointList::GetFirstEndpoint(wstring & firstEndpoint) const
{
    if (endPointList_.size() < 1)
    {   
        return false;
    }

    firstEndpoint = endPointList_.begin()->second;
    return true;
}


