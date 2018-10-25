//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Client;

CallSystemServiceResult::CallSystemServiceResult(
    wstring && result)
    : result_(move(result))
{
}

wstring const & CallSystemServiceResult::GetResult()
{
    return result_;
}
