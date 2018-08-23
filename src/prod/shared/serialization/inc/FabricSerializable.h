// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    class FabricSerializable : public IFabricSerializable, public KObject<FabricSerializable>
    {
    public:

        FabricSerializable();

        virtual ~FabricSerializable();

        FabricSerializable(__in FabricSerializable const & serializable);

        FabricSerializable(__in FabricSerializable && serializable);

        FabricSerializable & operator=(__in FabricSerializable && serializable);

        FabricSerializable & operator=(__in FabricSerializable  const & serializable);

        bool operator==(__in FabricSerializable const & serializable) const;

        virtual NTSTATUS GetTypeInformation(__out FabricTypeInformation & typeInformation) const;

        virtual NTSTATUS Write(__in IFabricSerializableStream * stream);

        virtual NTSTATUS Read(__in IFabricSerializableStream * stream);

        virtual NTSTATUS GetUnknownData(__in ULONG scope, __out FabricIOBuffer & data);

        virtual NTSTATUS SetUnknownData(__in ULONG scope, __in FabricIOBuffer buffer);

        virtual void ClearUnknownData()
        {
            if (unknownScopeData_ != nullptr)
            {                                             
                this->unknownScopeData_->Clear();
            }
        }

    private:

        struct DataContext : public KObject<DataContext>
        {
        public:
            DataContext()
                : _length(0)
            {
            }            

            DataContext(DataContext && context);
            
            DataContext& operator=(DataContext const & context);
            
            DataContext& operator=(DataContext && context);
            
            bool operator==(DataContext & context);         

            DataContext(ULONG length, KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> && data)
                : _length(length)
                , _data(Ktl::Move(data))
            {
            }

            ULONG _length;
            KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> _data;
        };

        KArray<DataContext> * unknownScopeData_;
    };
}
