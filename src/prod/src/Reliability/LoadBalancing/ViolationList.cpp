// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ViolationList.h"
#include "Checker.h"
#include "PlacementAndLoadBalancing.h"
#include "IViolation.h"
#include "PLBConfig.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ViolationList::ViolationList()
    : violations_(),
    violationPriorityList_()
{
}

ViolationList::ViolationList(ViolationList && other)
    : violations_(move(other.violations_)),
    violationPriorityList_(move(other.violationPriorityList_))
{
}

ViolationList & ViolationList::operator = (ViolationList && other)
{
    if (this != &other)
    {
        violations_ = move(other.violations_);
        violationPriorityList_ = move(other.violationPriorityList_);
    }

    return *this;
}

void ViolationList::AddViolations(int priority, IConstraint::Enum type, IViolationUPtr && violations)
{
    ASSERT_IF(violations_.find(type) != violations_.end(), "Violation already exist");
    violations_.insert(make_pair(type, move(violations)));
    if (violationPriorityList_.find(priority) == violationPriorityList_.end())
    {
        violationPriorityList_.insert(make_pair(priority, move(std::vector<IConstraint::Enum>())));
    }
    violationPriorityList_[priority].push_back(type);
}

bool ViolationList::IsEmpty() const
{
    return violations_.empty();
}

bool ViolationList::operator<=(ViolationList const& other) const
{
    bool ret = true;

    auto it1 = violations_.begin();
    auto it2 = other.violations_.begin();
    while (it1 != violations_.end() && it2 != other.violations_.end())
    {
        if (it1->first < it2->first)
        {
            ret = false;
            break;
        }
        else if (it1->first > it2->first)
        {
            ++it2;
        }
        else if (!(*(it1->second) <= *(it2->second)))
        {
            ret = false;
            break;
        }
        else
        {
            ++it1;
            ++it2;
        }
    }

    if (ret && it1 != violations_.end() && it2 == other.violations_.end())
    {
        ret = false;
    }

    return ret;
}

// Function compares two violation lists and return following values:
// -1 - If current list (this) has less violations
//  0 - If both lists have the same number of violations
//  1 - If current list (this) has more violations
// Violation list has less violations if one the following conditions are satisfied:
//   - Has lower maximal violation priority
//   - Has less violations within the same priority violation set
//   - Has the same number of violations for a specific priority,
//     and there is an violation type which has better internal state (e.g. has less replicas in violation)
//     and other violations have at least the same state (i.e. other violation types have the same or better state)
int ViolationList::CompareViolation(ViolationList& other)
{
    int cmpResult = ViolationRelation::Enum::Equal;
    auto itPriority1 = violationPriorityList_.begin();
    auto itPriority2 = other.violationPriorityList_.begin();
    while (itPriority1 != violationPriorityList_.end() && itPriority2 != other.violationPriorityList_.end())
    {
        // If current list has a violation with higher priority,
        // (i.e. smaller number), then the state is worse
        if (itPriority1->first < itPriority2->first)
        {
            cmpResult = ViolationRelation::Enum::Greater;
            break;
        }
        // If current list has lower max violation priority,
        // (i.e. higher number), then the state is better
        else if (itPriority1->first > itPriority2->first)
        {
            cmpResult = ViolationRelation::Enum::Smaller;
            break;
        }
        // If max priority violations are the same for both lists,
        // compare the individual violations
        else
        {
            size_t violationListSize1 = violationPriorityList_[itPriority1->first].size();
            size_t violationListSize2 = other.violationPriorityList_[itPriority2->first].size();

            // If current list has more violations for the same priority,
            // then the state is worse
            if (violationListSize1 > violationListSize2)
            {
                cmpResult = ViolationRelation::Enum::Greater;
                break;
            }
            // If current list has less violations for the same priority
            // then the state is better
            else if (violationListSize1 < violationListSize2)
            {
                cmpResult = ViolationRelation::Enum::Smaller;
                break;
            }
            // If current list has the same number of violations for a priority,
            // then compare the internal status of each violation type
            else
            {
                bool hasImrovement = false;
                bool hasDegradation = false;
                auto itViolationType1 = violationPriorityList_[itPriority1->first].begin();
                auto itViolationType2 = other.violationPriorityList_[itPriority2->first].begin();

                while (itViolationType1 != violationPriorityList_[itPriority1->first].end()
                        && itViolationType2 != other.violationPriorityList_[itPriority2->first].end())
                {
                    // If there is the same number of violations for a specific priority,
                    // but there are different types, the state is not better
                    // (i.e. new violation type is created)
                    if (*itViolationType1 != *itViolationType2)
                    {
                        return ViolationRelation::Enum::Greater;
                    }

                    ViolationRelation::Enum cmpStatus = violations_[(*itViolationType1)]->CompareViolation(*(other.violations_[(*itViolationType2)]));

                    // If the violation got worse
                    if (cmpStatus == ViolationRelation::Enum::Greater)
                    {
                        hasDegradation = true;
                    }
                    // If the violation got better
                    else if (cmpStatus == ViolationRelation::Enum::Smaller)
                    {
                        hasImrovement = true;
                    }
                    
                    ++itViolationType1;
                    ++itViolationType2;
                }

                // If there is a violation with better state,
                // and non of the other violations (for the same priority) had got worse
                if (hasImrovement && !hasDegradation)
                {
                    cmpResult = ViolationRelation::Enum::Smaller;
                    break;
                }
                // If there is a violation with worse state then initially,
                // overall solution is worse
                else if (hasDegradation)
                {
                    cmpResult = ViolationRelation::Enum::Greater;
                    break;
                }
            }
        }
        ++itPriority1;
        ++itPriority2;
    }

    // If one violation list is empty,
    // or one of the iterators had reached the end of the list
    if (cmpResult == ViolationRelation::Enum::Equal && 
        (itPriority1 != violationPriorityList_.end() || itPriority2 != other.violationPriorityList_.end()))
    {
        if (itPriority1 != violationPriorityList_.end())
        {
            cmpResult = ViolationRelation::Enum::Greater;
        }
        else
        {
            cmpResult = ViolationRelation::Enum::Smaller;
        }
    }

    return cmpResult;
}

void ViolationList::WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
{
    writer.Write("[");
    for (auto it = violations_.begin(); it != violations_.end(); ++it)
    {
        if (it != violations_.begin())
        {
            writer.Write(";");
        }
        writer.Write("{0}:{1}-", it->first, it->second->GetCount());

        auto violationList = it->second->GetViolationListDetail();
        int count = 0;
        int maxViolatedItemsToTrace = PLBConfig::GetConfig().MaxViolatedItemsToTrace;
        for (auto item = violationList.begin(); item != violationList.end(); ++item)
        {
            if (++count > maxViolatedItemsToTrace)
            {
                // Break when there are too many items to be printed to trace.
                writer.Write(" and more ...");
                break;
            }
            if (item != violationList.begin())
            {
                writer.Write(",");
            }
            writer.Write("({0})", *item);
        }
    }
    writer.Write("]");
}

void ViolationList::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
