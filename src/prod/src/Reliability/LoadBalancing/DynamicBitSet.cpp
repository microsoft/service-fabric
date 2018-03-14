// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DynamicBitSet.h"

using namespace std;
using namespace Reliability::LoadBalancingComponent;

DynamicBitSet::DynamicBitSet(size_t count)
    : maxElement_(-1),
    elementCount_(0),
    bitmapLength_(0)
{
    if (count > 0)
    {
        Grow(count);
    }
}

void DynamicBitSet::Grow(size_t count)
{
    // Handle the case when zero is added to grow enough!
    size_t newBitmapLength = count == 0 ? 1 : (count + BitmapElementSize) / BitmapElementSize;
    maxElement_ = static_cast<int64>(count);
    if (newBitmapLength > bitmapLength_)
    {
        bitmapLength_ = static_cast<int>(newBitmapLength);
        bitmap_.resize(bitmapLength_);
        bitmapPtr_ = bitmap_.data();
    }
}

DynamicBitSet::DynamicBitSet(DynamicBitSet const& other)
    : maxElement_(other.maxElement_),
      elementCount_(other.elementCount_),
      bitmapLength_(other.bitmapLength_),
      bitmap_(other.bitmap_),
      bitmapPtr_(bitmap_.data())
{
}

DynamicBitSet::DynamicBitSet(DynamicBitSet && other)
    : maxElement_(other.maxElement_),
      elementCount_(other.elementCount_),
      bitmapLength_(other.bitmapLength_),
      bitmap_(move(other.bitmap_)),
      bitmapPtr_(other.bitmapPtr_)
{
}

DynamicBitSet::DynamicBitSet(std::vector<int> const& set)
    : maxElement_(-1),
    elementCount_(0),
    bitmapLength_(0)
{
    for (auto element : set)
    {
        Add(element);
    }
}

DynamicBitSet & DynamicBitSet::operator = (DynamicBitSet && other)
{
    if (this != &other)
    {
        maxElement_ = other.maxElement_;
        elementCount_ = other.elementCount_;
        bitmapLength_ = other.bitmapLength_;
        bitmap_ = move(other.bitmap_);
        bitmapPtr_ = other.bitmapPtr_;
    }

    return *this;
}

void DynamicBitSet::Copy(DynamicBitSet const& other)
{
    maxElement_ = other.maxElement_;
    elementCount_ = other.elementCount_;
    bitmapLength_= other.bitmapLength_;
    bitmap_ = other.bitmap_;
    bitmapPtr_ = bitmap_.data();
}

void DynamicBitSet::Add(size_t element)
{
    if (static_cast<int64>(element) > maxElement_)
    {
        // Grow the container so that we can add this element.
        Grow(element);
    }
    if (!Check(element))
    {
        InternalAdd(element);
    }
}

void DynamicBitSet::Delete(size_t element)
{
    if (static_cast<int64>(element) > maxElement_)
    {
        return;
    }
    if (Check(element))
    {
        InternalDelete(element);
    }
}


void DynamicBitSet::Clear()
{
    if (elementCount_ != 0)
    {
        InnerClear();
    }
}

void DynamicBitSet::ForEach(std::function<void(size_t)> processor) const
{
    if (elementCount_ == 0)
    {
        return;
    }

    size_t currentIndex = 0;

    for (int bitmapIndex = 0; bitmapIndex < bitmapLength_; ++bitmapIndex)
    {
        BitmapElementType currentMask = 1;
        BitmapElementType currentElement = bitmap_[bitmapIndex];

        if (currentElement == 0)
        {
            currentIndex += BitmapElementSize;
            continue;
        }

        for (int smallIndex = 0; smallIndex < BitmapElementSize; ++smallIndex)
        {
            if (currentMask & currentElement)
            {
                processor(currentIndex);
            }
            currentMask <<= 1;
            currentIndex++;
        }
    }
}

void DynamicBitSet::InnerClear()
{
    elementCount_ = 0;
    if (bitmapLength_ > 0)
    {
        memset(bitmapPtr_, 0, bitmapLength_ * (BitmapElementSize / 8));
    }
}

void DynamicBitSet::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("(");
    bool first = true;
    ForEach([&](size_t element) -> void
    {
        if (!first)
        {
            writer.Write(" ");
        }
        writer.Write("{0}", element);
        first = false;
    });
    writer.Write(")");
}

