// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeFabricUpgradeReplyMessageBody : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_ASSIGNMENT(NodeFabricUpgradeReplyMessageBody)
    
    public:
        NodeFabricUpgradeReplyMessageBody()
        {
        }

        NodeFabricUpgradeReplyMessageBody(Common::FabricVersionInstance const& versionInstance)
            : versionInstance_(versionInstance)
        {
        }

        NodeFabricUpgradeReplyMessageBody(NodeFabricUpgradeReplyMessageBody && other)
            : versionInstance_(std::move(other.versionInstance_))
        {
        }

        __declspec(property(get=get_VersionInstance)) Common::FabricVersionInstance const& VersionInstance;
        Common::FabricVersionInstance const& get_VersionInstance() const { return versionInstance_; }

        void WriteToEtw(uint16 contextSequenceId) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.Write("VersionInstance: {0}", versionInstance_);
        }

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {            
            std::string format = "VersionInstance: {0}";

            size_t index = 0;
            traceEvent.AddEventField<Common::FabricVersionInstance>(format, name + ".versionInstance", index);
            
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<Common::FabricVersionInstance>(versionInstance_);

        }

        FABRIC_FIELDS_01(versionInstance_);

    private:

        Common::FabricVersionInstance versionInstance_;
    };
}
