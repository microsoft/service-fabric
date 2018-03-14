// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class VersionedCuid
    {
    public:
        VersionedCuid()
          : cuid_(ConsistencyUnitId(Common::Guid::Empty()))
          , consistencyUnitVersion_(0)
          , generation_()
        {
        }

        explicit VersionedCuid(ConsistencyUnitId const & cuid)
          : cuid_(cuid)
          , consistencyUnitVersion_(0)
          , generation_()
        {
        }

        VersionedCuid(ConsistencyUnitId const & cuid, __int64 version, GenerationNumber const & generation)
          : consistencyUnitVersion_(version)
          , cuid_(cuid)
          , generation_(generation)
        {
        }

        __declspec(property(get=get_Cuid)) ConsistencyUnitId const & Cuid;
        ConsistencyUnitId const get_Cuid() const { return cuid_; }

        __declspec(property(get=get_ConsistencyUnitVersion)) __int64 const & Version;
        __int64 const & get_ConsistencyUnitVersion() const { return consistencyUnitVersion_; }
        
        __declspec(property(get=get_Generation)) GenerationNumber const & Generation;
        GenerationNumber const & get_Generation() const { return generation_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << cuid_ << L"(" << consistencyUnitVersion_ << L", Gen:" << generation_ << ")";
        }
    
    private:
        int64 consistencyUnitVersion_;
        ConsistencyUnitId cuid_;
        GenerationNumber generation_;
    };
}
