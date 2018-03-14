// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    HierarchyNameData::HierarchyNameData() 
        : nameState_(HierarchyNameState::Invalid)
        , serviceState_(UserServiceState::Invalid)
        , subnamesVersion_(0)
        , flags_()
    { 
    }

    HierarchyNameData::HierarchyNameData(
        HierarchyNameState::Enum nameState,
        UserServiceState::Enum serviceState,
        __int64 subnamesVersion)
        : nameState_(nameState)
        , serviceState_(serviceState)
        , subnamesVersion_(subnamesVersion)
        , flags_()
    {
    }
    
    // Inner class

    HierarchyNameData::Flags::Flags() 
        : value_(0) 
    {
    }

    bool HierarchyNameData::Flags::IsImplicit() const 
    { 
        return ((value_ == None) || (value_ & Implicit) != 0); 
    }
    
    bool HierarchyNameData::Flags::IsExplicit() const 
    { 
        return ((value_ & Explicit) != 0); 
    }
            
    void HierarchyNameData::Flags::SetExplicitState(bool isExplicit) 
    {
        if (isExplicit)
        {
            value_ |= Explicit;
        }
        else
        {
            value_ |= Implicit;
        }
    }

    void HierarchyNameData::Flags::WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const
    {
        writer << (IsImplicit() ? "I" : "E");
    }
}
