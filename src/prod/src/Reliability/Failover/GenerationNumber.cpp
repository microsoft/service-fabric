// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Reliability;
using namespace Federation;
    
GenerationNumber const GenerationNumber::Zero = GenerationNumber();

GenerationNumber::GenerationNumber()
    :   generation_(0),
        owner_(NodeId::MinNodeId)
{
}

GenerationNumber::GenerationNumber(int64 generation, NodeId const & owner)
    :   generation_(generation),
        owner_(owner)
{ 
}

GenerationNumber::GenerationNumber(GenerationNumber const & oldGeneration, NodeId const & owner)
    :   generation_(oldGeneration.generation_ + 1),
        owner_(owner)
{
}


bool GenerationNumber::operator==(GenerationNumber const & other) const
{
    return (generation_ == other.generation_ && owner_ == other.owner_);
}

bool GenerationNumber::operator!=(GenerationNumber const & other) const
{
    return !(*this == other);
}

bool GenerationNumber::operator<(GenerationNumber const & other) const
{
    return ((generation_ < other.generation_) ||
        (generation_ == other.generation_ && owner_ < other.owner_));
}

bool GenerationNumber::operator>(GenerationNumber const & other) const
{
    return ((generation_ > other.generation_) ||
        (generation_ == other.generation_ && owner_ > other.owner_));
}

bool GenerationNumber::operator<=(GenerationNumber const & other) const
{
    return !(*this > other);
}

bool GenerationNumber::operator>=(GenerationNumber const & other) const
{
    return !(*this < other);
}

std::wstring GenerationNumber::ToString() const
{
    std::wstring str;
    Common::StringWriter writer(str);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return str;
}

void GenerationNumber::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    w.Write("{0}:{1}", generation_, owner_);
}
