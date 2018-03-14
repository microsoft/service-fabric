// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    struct PendingNamesHierarchy
    {
    public:
        PendingNamesHierarchy();

        __declspec(property(get=get_CurrentName)) Common::NamingUri const & CurrentName;
        Common::NamingUri const & get_CurrentName() const { return tentativeNames_.back(); }
        
        __declspec(property(get=get_CurrentNameString)) std::wstring const & CurrentNameString;
        std::wstring const & get_CurrentNameString() const { return tentativeNameStrings_.back(); }
        
        __declspec(property(get=get_CurrentParentName)) Common::NamingUri const & CurrentParentName;
        Common::NamingUri const & get_CurrentParentName() const { return currentParentName_; }
        
        __declspec(property(get=get_CurrentParentNameString)) std::wstring const & CurrentParentNameString;
        std::wstring const & get_CurrentParentNameString() const { return currentParentNameString_; }
        
        __declspec(property(get=get_IsCurrentNameCompletePending)) bool IsCurrentNameCompletePending;
        bool get_IsCurrentNameCompletePending() const { return !isTentative_; }

        // Add a name to be processed and set the parent name based on it.
        void AddTentativeName(Common::NamingUri const & name, std::wstring const & nameString);

        // Move parent in the list of tentative names and set parent name to its parent
        void MarkParentAsTentative();

        // Change the state to complete
        void SetCurrentNameCompletePending();

        // Remove the last tentative name and use it as a parent.
        // Returns true if there any pending names left.
        bool CompleteNameAndTryMoveNext();

        // Gets the name of the first child of the current name, if the child is in the list.
        bool TryGetCurrentChildNameString(__out std::wstring & childNameString) const;

        // Completes the current name and replaces it with its parent. 
        // A new parent will be computed based on the new current name.
        void PromoteParentAsTentative();

    private:
        std::vector<Common::NamingUri> tentativeNames_;
        std::vector<std::wstring> tentativeNameStrings_;
        Common::NamingUri currentParentName_;
        std::wstring currentParentNameString_;
        bool isTentative_;
    };
}
