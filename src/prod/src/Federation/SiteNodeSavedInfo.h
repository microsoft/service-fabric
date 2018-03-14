// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class SiteNodeSavedInfo  : public Serialization::FabricSerializable
    {    
    public:
        explicit SiteNodeSavedInfo(uint64 inVal):instanceId_(inVal){};

        __declspec (property(get=getInstanceId,put=setInstanceId)) uint64 InstanceId;            

        uint64 getInstanceId() const { return instanceId_; }
        void setInstanceId(uint64 inInsId) { instanceId_ = inInsId; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0}", instanceId_ );
        };
        
        FABRIC_FIELDS_01(instanceId_);    

    private:
        uint64 instanceId_;
    }; 
}
