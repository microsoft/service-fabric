// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

INITIALIZE_SIZE_ESTIMATION( VersionRangeCollection )

VersionRangeCollection::VersionRangeCollection(int64 initialVersion)
    : versionRanges_(), initialVersion_(initialVersion)
{
}

VersionRangeCollection::VersionRangeCollection(int64 startVersion, int64 endVersion)
    : versionRanges_(), initialVersion_(1)
{
    if (startVersion < endVersion)
    {
        versionRanges_.push_back(VersionRange(startVersion, endVersion));
    }
}

VersionRangeCollection::VersionRangeCollection(VersionRangeCollection const& other)
{
    versionRanges_ = std::vector<VersionRange>(other.versionRanges_);
    initialVersion_ = other.initialVersion_;
}

VersionRangeCollection::VersionRangeCollection(vector<VersionRange> && ranges, int64 initialVersion)
    : versionRanges_(move(ranges))
    , initialVersion_(initialVersion)
{
}

int64 VersionRangeCollection::get_EndVersion() const
{
    return (versionRanges_.empty() ? initialVersion_ : versionRanges_[versionRanges_.size() - 1].EndVersion);
}

int64 VersionRangeCollection::get_StartVersion() const
{
    return (versionRanges_.empty() ? numeric_limits<int64>::max() : versionRanges_[0].StartVersion);
}

bool VersionRangeCollection::AddRange(int64 startVersion, int64 endVersion)
{
    return this->Add(VersionRange(startVersion, endVersion));
}

bool VersionRangeCollection::Add(VersionRange versionRange)
{
    bool result = false;
    int64 startVersion = versionRange.StartVersion;
    int64 endVersion = versionRange.EndVersion;

    if (startVersion < endVersion)
    {
        result = true;

        size_t startIndex;
        for (startIndex = 0; startIndex < versionRanges_.size() && versionRanges_[startIndex].EndVersion < startVersion; startIndex++);

        size_t endIndex;
        for (endIndex = startIndex; endIndex < versionRanges_.size() && versionRanges_[endIndex].StartVersion <= endVersion; endIndex++);

        if (startIndex < endIndex)
        {
            startVersion = min(startVersion, versionRanges_[startIndex].StartVersion);
            endVersion = max(endVersion, versionRanges_[endIndex - 1].EndVersion);

            if ((endIndex == startIndex + 1) &&
                (versionRanges_[startIndex].StartVersion == startVersion) &&
                (versionRanges_[startIndex].EndVersion == endVersion))
            {
                result = false;
            }
            else
            {
                VersionRange newVersionRange(startVersion, endVersion);
                versionRange = newVersionRange;
            }
        }

        if (result)
        {
            vector<VersionRange> newVersionRanges;
            for (size_t i = 0; i < startIndex; i++)
            {
                newVersionRanges.push_back(versionRanges_[i]);
            }

            newVersionRanges.push_back(versionRange);

            for (size_t i = endIndex; i < versionRanges_.size(); i++)
            {
                newVersionRanges.push_back(versionRanges_[i]);
            }

            versionRanges_ = newVersionRanges;
        }
    }

    return result;
}

bool VersionRangeCollection::Test_Add(VersionRangeCollection const& versionRangeCollection)
{
    bool result = false;

    for (VersionRange const& versionRange : versionRangeCollection.VersionRanges)
    {
        if (Add(versionRange))
        {
            result = true;
        }
    }

    return result;
}

void VersionRangeCollection::Merge(VersionRangeCollection const & sortedCollection)
{
    auto const & left = versionRanges_;
    auto const & right = sortedCollection.VersionRanges;

    if (!left.empty() && !right.empty())
    {
        // Optimize for special-case where the incoming collection is merged onto the
        // end of the existing collection. Just merge in-place to the last element
        // of the existing collection.
        //
        if (left.back().StartVersion <= right.front().StartVersion)
        {
            for (auto it=right.begin(); it != right.end(); ++it)
            {
                this->MergeBack(versionRanges_, *it);
            }

            return;
        }
    }

    vector<VersionRange> merged;

    auto leftIt = left.begin();
    auto rightIt = right.begin();

    while (leftIt != left.end() && rightIt != right.end())
    {
        if (leftIt->StartVersion <= rightIt->StartVersion)
        {
            this->MergeBack(merged, *leftIt++);
        }
        else
        {
            this->MergeBack(merged, *rightIt++);
        }
    } 

    while (leftIt != left.end())
    {
        this->MergeBack(merged, *leftIt++);
    }

    while (rightIt != right.end())
    {
        this->MergeBack(merged, *rightIt++);
    }

    versionRanges_.swap(merged);
}

void VersionRangeCollection::Merge(VersionRange const & range)
{
    this->Merge(VersionRangeCollection(range.StartVersion, range.EndVersion));
}

void VersionRangeCollection::MergeBack(vector<VersionRange> & sorted, VersionRange const & range)
{
    if (sorted.empty() || sorted.back().EndVersion < range.StartVersion)
    {
        sorted.push_back(range);
    }
    else if (sorted.back().EndVersion < range.EndVersion)
    {
        sorted.back().SetEndVersion(range.EndVersion);
    }
}

void VersionRangeCollection::Split(
    int64 endVersion, 
    __out VersionRangeCollection & remainder)
{
    vector<VersionRange> remainderRanges;
	for (auto index = versionRanges_.size(); index > 0; --index)
	{
		auto const & range = *(versionRanges_.data() + index - 1);

		if (endVersion <= range.StartVersion)
		{
			remainderRanges.push_back(range);
			versionRanges_.pop_back();
		}
	    else if (endVersion >= range.EndVersion)
        {
            break;
        }
        else
        {
            remainderRanges.push_back(VersionRange(endVersion, range.EndVersion));
            versionRanges_.pop_back();
            versionRanges_.push_back(VersionRange(range.StartVersion, endVersion));
            break;
        }
    }

    if (!remainderRanges.empty())
    {
        reverse(remainderRanges.begin(), remainderRanges.end());
    }

    remainder.Swap(VersionRangeCollection(move(remainderRanges), initialVersion_));
}

void VersionRangeCollection::Remove(int64 versionToRemove)
{
    vector<VersionRange>::iterator where;
    if (this->Contains(versionToRemove, where))
    {
        if (versionToRemove == where->StartVersion)
        {
            where->SetStartVersion(versionToRemove + 1);

            if (where->StartVersion >= where->EndVersion)
            {
                versionRanges_.erase(where);
            }
        }
        else if (versionToRemove == (where->EndVersion - 1))
        {
            where->SetEndVersion(versionToRemove);
        }
        else
        {
            VersionRange remainingRange(versionToRemove + 1, where->EndVersion);
            where = --(versionRanges_.insert(where + 1, remainingRange));

            where->SetEndVersion(versionToRemove);
        }
    }
}

void VersionRangeCollection::Remove(VersionRange const& versionRange)
{
    int64 startVersion = versionRange.StartVersion;
    int64 endVersion = versionRange.EndVersion;

    if (startVersion < endVersion)
    {
        size_t startIndex;
        for (startIndex = 0; startIndex < versionRanges_.size() && versionRanges_[startIndex].EndVersion <= startVersion; startIndex++) ;

        size_t endIndex;
        for (endIndex = startIndex; endIndex < versionRanges_.size() && versionRanges_[endIndex].StartVersion < endVersion; endIndex++) ;

        if (startIndex < endIndex)
        {
            endIndex--;
            VersionRangeCollection newVersionRangeCollection;

            if (versionRanges_[startIndex].StartVersion < startVersion)
            {
                newVersionRangeCollection.Add(VersionRange(versionRanges_[startIndex].StartVersion, startVersion));
            }

            if (versionRanges_[endIndex].EndVersion > endVersion)
            {
                newVersionRangeCollection.Add(VersionRange(endVersion, versionRanges_[endIndex].EndVersion));
            }

            versionRanges_.erase(versionRanges_.begin() + startIndex, versionRanges_.begin() + endIndex + 1);

            if (!newVersionRangeCollection.versionRanges_.empty())
            {
                this->Merge(newVersionRangeCollection);
            }
        }
    }
}

void VersionRangeCollection::Test_Remove(VersionRangeCollection const& versionRangeCollection)
{
    for (VersionRange const& versionRange : versionRangeCollection.VersionRanges)
    {
        Remove(versionRange);
    }
}

void VersionRangeCollection::Remove(VersionRangeCollection const& versionRangeCollection)
{
    auto const & left = versionRanges_;
    auto const & right = versionRangeCollection.VersionRanges;

    if (left.empty() || right.empty()) { return; }

    vector<VersionRange> complement;

    auto leftIt = left.begin();
    auto rightIt = right.begin();

    unique_ptr<VersionRange> rangeToCheck;
    unique_ptr<VersionRange> rangeRemainder;

    while (leftIt != left.end() && rightIt != right.end())
    {
        if (rangeRemainder)
        {
            rangeToCheck = make_unique<VersionRange>(
                rangeRemainder->StartVersion,
                rangeRemainder->EndVersion);

            rangeRemainder.reset();
        }
        else
        {
            rangeToCheck = make_unique<VersionRange>(
                leftIt->StartVersion,
                leftIt->EndVersion);
        }

        unique_ptr<VersionRange> rangeToKeep;

        bool advanceRangeToCheck = false;
        bool advanceRangeToRemove = false;

        this->Remove(
            *rangeToCheck, 
            *rightIt, 
            rangeToKeep, 
            rangeRemainder,
            advanceRangeToCheck,
            advanceRangeToRemove);

        if (rangeToKeep)
        {
            complement.push_back(*rangeToKeep);
        }
            
        if (advanceRangeToCheck)
        {
            ++leftIt;
        }

        if (advanceRangeToRemove)
        {
            ++rightIt;
        }
    }

    while (leftIt != left.end())
    {
        if (rangeRemainder)
        {
            complement.push_back(*rangeRemainder);
            rangeRemainder.reset();
        }
        else
        {
            complement.push_back(*leftIt);
        }

        ++leftIt;
    }

    versionRanges_.swap(complement);
}

void VersionRangeCollection::Remove(
    VersionRange const & rangeToCheck,
    VersionRange const & rangeToRemove,
    __out unique_ptr<VersionRange> & rangeToKeep,
    __out unique_ptr<VersionRange> & rangeRemainder,
    __out bool & advanceRangeToCheck,
    __out bool & advanceRangeToRemove)
{
    advanceRangeToCheck = false;
    advanceRangeToRemove = false;

    auto checkStart = rangeToCheck.StartVersion;
    auto checkEnd = rangeToCheck.EndVersion;

    auto removeStart = rangeToRemove.StartVersion;
    auto removeEnd = rangeToRemove.EndVersion;

    if (removeStart <= checkStart && removeEnd <= checkStart)
    {
        // entire remove range < entire check range
        advanceRangeToCheck = false;
        advanceRangeToRemove = true;
    }
    else if (removeStart <= checkStart && removeEnd < checkEnd)
    {
        // remove range covers beginning of (but does not consume) check range
        rangeRemainder = make_unique<VersionRange>(removeEnd, checkEnd);
        advanceRangeToCheck = false;
        advanceRangeToRemove = true;
    }
    else if (removeStart <= checkStart && removeEnd >= checkEnd)
    {
        // remove range consumes entirety of check range
        advanceRangeToCheck = true;
        advanceRangeToRemove = false;
    }
    else if (removeStart >= checkEnd && removeEnd >= checkEnd)
    {
        // entire remove range > entire check range
        rangeToKeep = make_unique<VersionRange>(checkStart, checkEnd);
        advanceRangeToCheck = true;
        advanceRangeToRemove = false;
    }
    else if (removeStart > checkStart && removeEnd >= checkEnd)
    {
        // remove range covers end of (but does not consume) check range
        rangeToKeep = make_unique<VersionRange>(checkStart, removeStart);
        advanceRangeToCheck = true;
        advanceRangeToRemove = false;
    }
    else // implicit final clause: if (removeStart > checkStart && removeEnd < checkEnd)
    {
        // check range consumes entirety of remove range
        rangeToKeep = make_unique<VersionRange>(checkStart, removeStart);
        rangeRemainder = make_unique<VersionRange>(removeEnd, checkEnd);
        advanceRangeToCheck = false;
        advanceRangeToRemove = true;
    }
}

void VersionRangeCollection::Clear()
{
    this->versionRanges_.clear();
}

void VersionRangeCollection::Swap(VersionRangeCollection && other)
{
    versionRanges_.swap(other.versionRanges_);
    initialVersion_ = other.initialVersion_;
}

bool VersionRangeCollection::Contains(int64 target)
{
    vector<VersionRange>::iterator where;
    return this->Contains(target, where);
}

bool VersionRangeCollection::Contains(int64 target, __out int64 & endVersion)
{
    vector<VersionRange>::iterator where;
    if (this->Contains(target, where))
    {
        endVersion = where->EndVersion;
        return true;
    }

    return false;
}

bool VersionRangeCollection::Contains(int64 target, __out vector<VersionRange>::iterator & where)
{
    where = versionRanges_.end();

    if (versionRanges_.empty()) 
	{ 
		return false; 
	}

    // Can binary search since version ranges are always kept sorted
    //
	size_t low = 0;
	size_t high = versionRanges_.size() - 1;
	auto startIter = versionRanges_.begin();

    while (low <= high)
    {
        auto index = (high - low) / 2 + low;
		auto indexIter = startIter + index;

        // StartVersion is inclusive, EndVersion is exclusive
        //
        if (indexIter->Contains(target))
        {
            where = indexIter;
            return true;
        }
        else if (target < indexIter->StartVersion)
        {
			if (index == 0)
			{
				return false;
			}
			else
			{
				high = index - 1;
			}
        }
        else
        {
            low = index + 1;
        }
    }

    return false;
}


void VersionRangeCollection::RemoveFirstRange()
{
    versionRanges_.erase(versionRanges_.begin());
}

void VersionRangeCollection::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << "[ ";
    for (VersionRange const& versionRange : versionRanges_)
    {
        w << versionRange << ' ';
    }
    w << "]";
}

string VersionRangeCollection::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    traceEvent.AddField<vector<VersionRange>>(name + ".ranges");
        
    return "[{0}]";
}

void VersionRangeCollection::FillEventData(TraceEventContext & context) const
{
    context.Write<vector<VersionRange>>(versionRanges_);
}
