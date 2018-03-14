// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class FauxStore : public INamingStoreServiceRouter
    {
    public:
        FauxStore(Federation::NodeId id)
          : id_(id)
          , stuff_()
          , services_()
          , storeVersion_(0)
          , lock_()
        {
        }

        virtual ~FauxStore() {}

        __declspec(property(get=get_Id)) Federation::NodeId Id;
        Federation::NodeId get_Id() const;

        bool AddName(Common::NamingUri const & name);

        bool DeleteName(Common::NamingUri const & name);

        bool NameExists(Common::NamingUri const & name);

        bool AddProperty(Common::NamingUri const & name, std::wstring const & prop, ::FABRIC_PROPERTY_TYPE_ID typeId, std::vector<byte> && value, std::wstring const & customTypeId);

        bool RemoveProperty(Common::NamingUri const & name, std::wstring const & prop);

        bool PropertyExists(Common::NamingUri const & name, std::wstring const & prop);

        bool AddService(Common::NamingUri const & name, PartitionedServiceDescriptor const & psd);

        bool RemoveService(Common::NamingUri const & name);

        bool ServiceExists(Common::NamingUri const & name);

        __int64 GetStoreVersion();

        void IncrementStoreVersion();

        PartitionedServiceDescriptor GetService(Common::NamingUri const & name);

        virtual Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr &&, 
            Common::TimeSpan const,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndProcessRequest(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply);

        virtual Common::AsyncOperationSPtr BeginRequestReplyToPeer(
            Transport::MessageUPtr &&,
            Federation::NodeInstance const &,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) { return Common::AsyncOperationSPtr(); }
        virtual Common::ErrorCode EndRequestReplyToPeer(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &) { return Common::ErrorCodeValue::NotImplemented; }

        virtual bool ShouldAbortProcessing() { return false; }

        struct PropertyData
        {
        public:
            PropertyData() 
                : Name()
                , CustomTypeId()
                , SequenceNumber(0)
                , TypeId(::FABRIC_PROPERTY_TYPE_INVALID)
                , PropertyValue()
            {
            }
            
            PropertyData(
                std::wstring const & name, 
                std::wstring const & customTypeId,
                LONGLONG sequenceNumber,
                ::FABRIC_PROPERTY_TYPE_ID typeId,
                std::vector<byte> && propertyValue)
                : Name(name)
                , CustomTypeId(customTypeId)
                , SequenceNumber(sequenceNumber)
                , TypeId(typeId)
                , PropertyValue(move(propertyValue))
            {
            }

            std::wstring Name;
            std::wstring CustomTypeId;
            LONGLONG SequenceNumber;
            ::FABRIC_PROPERTY_TYPE_ID TypeId;
            std::vector<byte> PropertyValue;
        };

        bool TryGetProperty(Common::NamingUri const & name, std::wstring const & propName, __out PropertyData & propData);

    private:

        bool NameExistsCallerHoldsLock(Common::NamingUri const & name);

        bool PropertyExistsCallerHoldsLock(Common::NamingUri const & name, std::wstring const & prop);

        bool ServiceExistsCallerHoldsLock(Common::NamingUri const & name);

        // Node id, assigned in constructor and never changed
        Federation::NodeId id_;

        std::map<Common::NamingUri, std::vector<PropertyData>> stuff_;
        std::map<Common::NamingUri, PartitionedServiceDescriptor> services_;
        __int64 storeVersion_;

        // Lock that protects the maps and the store version
        mutable Common::RwLock lock_;

        static LONGLONG OperationSequenceNumber;
    };
}
