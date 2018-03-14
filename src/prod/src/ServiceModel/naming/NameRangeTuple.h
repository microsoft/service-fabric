// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NameRangeTuple
    {
    public:
        NameRangeTuple(Common::NamingUri const & name, PartitionInfo const & info);

        NameRangeTuple(Common::NamingUri const & name, PartitionKey const & key);

        NameRangeTuple(Common::NamingUri && name, PartitionKey && key);

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        inline Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_Info)) PartitionInfo const & Info;
        inline PartitionInfo const & get_Info() const { return info_; }

        void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;
        
        class ContainsComparitor
        {
        public:
            bool operator ()(NameRangeTuple const & first, NameRangeTuple const & second) const
            {
                if (first.name_ != second.name_)
                {
                    return false;
                }

                return first.Info.RangeContains(second.Info);
            }
        };        

        class LessThanComparitor
        {
        public:
            bool operator ()(NameRangeTuple const & first, NameRangeTuple const & second) const
            {
                if (first.name_ != second.name_)
                {
                    return first.name_ < second.name_;
                }

                if (first.Info.PartitionScheme != second.Info.PartitionScheme)
                {
                    return first.Info.PartitionScheme < second.Info.PartitionScheme;
                }

                switch (first.Info.PartitionScheme)
                {
                case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_UNIFORM_INT64_RANGE:
                    return first.info_ < second.info_;                    
                case FABRIC_PARTITION_SCHEME::FABRIC_PARTITION_SCHEME_NAMED:
                    return first.info_.PartitionName < second.info_.PartitionName;
                default:
                    return false;
                }
            }           
        };

    private:
        Common::NamingUri name_;
        PartitionInfo info_;
    };

    typedef std::unique_ptr<NameRangeTuple> NameRangeTupleUPtr;
}
