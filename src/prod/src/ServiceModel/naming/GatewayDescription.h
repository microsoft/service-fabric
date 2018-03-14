// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class GatewayDescription : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(GatewayDescription)
        DEFAULT_COPY_ASSIGNMENT(GatewayDescription)
    public:
        GatewayDescription()
            : address_()
            , nodeInstance_()
            , nodeName_()
        {
        }

        GatewayDescription(GatewayDescription && other)
            : address_(std::move(other.address_))
            , nodeInstance_(std::move(other.nodeInstance_))
            , nodeName_(std::move(other.nodeName_))
        {
        }

        GatewayDescription(
            std::wstring const & address,
            Federation::NodeInstance const & nodeInstance,
            std::wstring const & nodeName)
            : address_(address)
            , nodeInstance_(nodeInstance)
            , nodeName_(nodeName)
        {
        }

        __declspec(property(get=get_Address)) std::wstring const & Address;
        std::wstring const & get_Address() const { return address_; }

        __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
        Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }

        __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        bool operator == (GatewayDescription const & other) const
        {
            return (address_ == other.address_
                && nodeInstance_ == other.nodeInstance_
                && nodeName_ == other.nodeName_);
        }

        bool operator != (GatewayDescription const & other) const
        {
            return !(*this == other);
        }

        void Clear()
        {
            address_.clear();
            nodeInstance_ = Federation::NodeInstance(Federation::NodeId::MinNodeId, 0);
            nodeName_.clear();
        }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_GATEWAY_INFORMATION &) const;

        FABRIC_FIELDS_03(address_, nodeInstance_, nodeName_);

    private:
        std::wstring address_;
        Federation::NodeInstance nodeInstance_;
        std::wstring nodeName_;
    };

    typedef std::shared_ptr<GatewayDescription> GatewayDescriptionSPtr;
}
