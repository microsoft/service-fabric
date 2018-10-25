// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include <Serialization.h>

using namespace Serialization;

FabricSerializable::FabricSerializable()
    : unknownScopeData_(nullptr)
{
}

FabricSerializable::~FabricSerializable()
{
    if (this->unknownScopeData_ != nullptr)
    {
        _delete (this->unknownScopeData_);
        this->unknownScopeData_ = nullptr;
    }
}

FabricSerializable::FabricSerializable(__in FabricSerializable const & serializable)
    : unknownScopeData_(nullptr)
{
    if (serializable.unknownScopeData_ != nullptr)
    {
        this->unknownScopeData_ = _new(WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) KArray<DataContext>(*serializable.unknownScopeData_);

        if (this->unknownScopeData_ == nullptr)
        {
            this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
}

FabricSerializable::FabricSerializable(__in FabricSerializable && serializable)
{
    this->unknownScopeData_ = serializable.unknownScopeData_;
    serializable.unknownScopeData_ = nullptr;
}

FabricSerializable & FabricSerializable::operator=(__in FabricSerializable const & serializable)
{
    if (this == &serializable)
    {
        return *this;
    }

    if (this->unknownScopeData_ != nullptr)
    {
        _delete (this->unknownScopeData_);
        this->unknownScopeData_ = nullptr;
    }

    if (serializable.unknownScopeData_ != nullptr)
    {
        this->unknownScopeData_ = _new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) KArray<DataContext>(*serializable.unknownScopeData_);

        if (this->unknownScopeData_ == nullptr)
        {
            this->SetStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return *this;
}

FabricSerializable & FabricSerializable::operator=(__in FabricSerializable && serializable)
{
    if (this == &serializable)
    {
        return *this;
    }

    this->unknownScopeData_ = serializable.unknownScopeData_;
    serializable.unknownScopeData_ = nullptr;

    return *this;
}

bool FabricSerializable::operator==(__in FabricSerializable const & serializable) const
{
    if (this->unknownScopeData_==nullptr)
    {
        return (serializable.unknownScopeData_==nullptr);        
    }
    else
    {
        if (serializable.unknownScopeData_==nullptr) 
        {
            return false;
        }
        else
        {
            if (this->unknownScopeData_->Count() !=  serializable.unknownScopeData_->Count())
            {
                return false;
            }

            for (ULONG idx=0; idx < this->unknownScopeData_->Count(); idx++)
            {
                if ( !((*unknownScopeData_)[idx] == (*serializable.unknownScopeData_)[idx]) )
                {
                    return false;
                }
            }
        }
    }

    return true;
}

NTSTATUS FabricSerializable::GetTypeInformation(__out FabricTypeInformation & typeInformation) const
{
    typeInformation.Clear();

    return STATUS_SUCCESS;
}

NTSTATUS FabricSerializable::Write(__in IFabricSerializableStream *)
{
    return STATUS_SUCCESS;
}

NTSTATUS FabricSerializable::Read(__in IFabricSerializableStream *)
{
    return STATUS_SUCCESS;
}

NTSTATUS FabricSerializable::GetUnknownData(__in ULONG scope, __out FabricIOBuffer & data)
{
    data.Clear();

    if (this->unknownScopeData_ != nullptr)
    {
        if (this->unknownScopeData_->Count() > scope)
        {
            DataContext & context = (*this->unknownScopeData_)[scope];

            data.length = context._length;
            data.buffer = &(*context._data);

            return STATUS_SUCCESS;
        }
    }

    return K_STATUS_NO_MORE_UNKNOWN_BUFFERS;
}

NTSTATUS FabricSerializable::SetUnknownData(__in ULONG scope, __in FabricIOBuffer buffer)
{
    ASSERT(buffer.length > 0);
    NTSTATUS status;

    if (this->unknownScopeData_ == nullptr)
    {
        this->unknownScopeData_ = _new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) KArray<DataContext>(FabricSerializationCommon::GetPagedKtlAllocator());

        if (this->unknownScopeData_ == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    // TODO: this is also weird
    while (this->unknownScopeData_->Count() < scope)
    {
        status = this->unknownScopeData_->Append(DataContext());

        if (NT_ERROR(status))
        {
            return status;
        }
    }

    #pragma prefast(suppress:6001, "saving raw pointer to a smart pointer");
    KUniquePtr<UCHAR, ArrayDeleter<UCHAR>> unknownDataBufferUPtr(_new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) UCHAR[buffer.length]);
    if (!unknownDataBufferUPtr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KMemCpySafe(unknownDataBufferUPtr, buffer.length, buffer.buffer, buffer.length);

    DataContext context(buffer.length, Ktl::Move(unknownDataBufferUPtr));

    status = this->unknownScopeData_->Append(Ktl::Move(context));

    return status;
}

FabricSerializable::DataContext::DataContext(DataContext && context)
{
    this->_length = context._length;
    context._length = 0;


    this->_data = Ktl::Move(context._data);
    context._data = nullptr;
}

FabricSerializable::DataContext& FabricSerializable::DataContext::operator=(DataContext const & context)
{
    if (this == &context)
    {
        return *this;
    }

    this->_length = context._length;

    #pragma prefast(suppress:6001, "saving raw pointer to a smart pointer");
    this->_data =  KUniquePtr<UCHAR, ArrayDeleter<UCHAR>>(_new (WF_SERIALIZATION_TAG, FabricSerializationCommon::GetPagedKtlAllocator()) UCHAR[this->_length]);
    if (!_data)
    {
        this->_length = 0;
        this->SetStatus(STATUS_INSUFFICIENT_RESOURCES);

        return *this;
    }

    KMemCpySafe(this->_data, this->_length, &(*(context._data)), this->_length);

    return *this;
}

FabricSerializable::DataContext& FabricSerializable::DataContext::operator=(DataContext && context)
{
    if (this == &context)
    {
        return *this;
    }

    this->_length = context._length;
    context._length = 0;

    this->_data = Ktl::Move(context._data);
    context._data = nullptr;

    return *this;
}

bool FabricSerializable::DataContext::operator==(DataContext & context)
{
    if (this->_length != context._length)
    {
        return false;
    }

    return (memcmp(_data, context._data, _length)==0);                
}
