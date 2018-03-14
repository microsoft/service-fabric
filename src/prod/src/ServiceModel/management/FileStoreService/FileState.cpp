// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

void FileState::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    w << ToString(val);
}

wstring FileState::ToString(Enum const & val)
{
    switch(val)
    {
    case FileState::Available_V1:
        return L"Available_V1";
    case FileState::Updating:
        return L"Updating";
    case FileState::Deleting:
        return L"Deleting";    
    case FileState::Replicating:
        return L"Replicating";    
    case FileState::Committed:
        return L"Committed";    
    default:
        return wformatString("{0}", static_cast<int>(val));
    }
}

bool FileState::IsStable(Enum const & val)
{
    switch(val)
    {
    case FileState::Available_V1:
    case FileState::Committed:
        return true;

    default:
        return false;
    }
}
