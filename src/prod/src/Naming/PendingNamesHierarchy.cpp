// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Federation;
    using namespace Reliability;
    using namespace std;
    using namespace Store;

    PendingNamesHierarchy::PendingNamesHierarchy()
        : tentativeNames_()
        , tentativeNameStrings_()
        , currentParentName_()
        , currentParentNameString_()
        , isTentative_(true)
    {
    }

    void PendingNamesHierarchy::AddTentativeName(
        NamingUri const & name, 
        std::wstring const & nameString)
    {
        tentativeNames_.push_back(name);
        tentativeNameStrings_.push_back(nameString);
        currentParentName_ = name.GetParentName();
        currentParentNameString_ = currentParentName_.ToString();
        isTentative_ = true;
    }

    void PendingNamesHierarchy::SetCurrentNameCompletePending()
    {
        isTentative_ = false;
    }

    void PendingNamesHierarchy::PromoteParentAsTentative()
    {
        // Discard current name, move directly to the parent
        tentativeNames_.pop_back();
        tentativeNameStrings_.pop_back();
        MarkParentAsTentative();
    }

    void PendingNamesHierarchy::MarkParentAsTentative()
    {
        tentativeNameStrings_.push_back(move(currentParentNameString_));
        tentativeNames_.push_back(move(currentParentName_));
        currentParentName_ = tentativeNames_.back().GetParentName();
        currentParentNameString_ = currentParentName_.ToString();
        isTentative_ = true;
    }

    bool PendingNamesHierarchy::CompleteNameAndTryMoveNext()
    {
        ASSERT_IF(tentativeNames_.empty(), "CompleteNameAndTryMoveNext: tentativeNames_ should not be empty");
        currentParentName_ = move(tentativeNames_.back());
        currentParentNameString_ = move(tentativeNameStrings_.back());

        tentativeNames_.pop_back();
        tentativeNameStrings_.pop_back();

        if (tentativeNames_.empty())
        {
            // No more names
            return false;
        }
        else
        {
            isTentative_ = true;
            return true;
        }
    }

    bool PendingNamesHierarchy::TryGetCurrentChildNameString(__out std::wstring & childName) const
    {
        if (tentativeNames_.size() > 1)
        {
            childName = tentativeNameStrings_[tentativeNameStrings_.size() - 2];
            return true;
        }

        return false;
    }
}
