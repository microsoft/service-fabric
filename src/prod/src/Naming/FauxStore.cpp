// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FauxStore.h"

namespace Naming
{
    using namespace Common;
    using namespace std;
    using namespace Transport;
    using namespace Federation;
    using namespace ClientServerTransport;

    LONGLONG FauxStore::OperationSequenceNumber(0);
    // Async Operations

    class FauxStoreAsyncOperation : public AsyncOperation
    {
    public:
        FauxStoreAsyncOperation(
            FauxStore & store,
            MessageUPtr && message,
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent) :
            AsyncOperation(callback, parent),
            store_(store),
            message_(move(message))
        {
        }

        static ErrorCode End(AsyncOperationSPtr const & asyncOperation, MessageUPtr & reply) 
        {            
            reply = move(AsyncOperation::Get<FauxStoreAsyncOperation>(asyncOperation)->reply_);
            return AsyncOperation::End<FauxStoreAsyncOperation>(asyncOperation)->Error;
        }

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr)
        {            
            ErrorCode error;
            if (message_->Action == NamingTcpMessage::CreateNameAction)
            {
                NamingUri name;
                if (message_->GetBody(name))
                {
                    bool worked = store_.AddName(name);
                    if (!worked)
                    {
                        error = ErrorCodeValue::OperationFailed;
                    }
                    else
                    {
                        reply_ = NamingMessage::GetPeerNameOperationReply();
                    }
                }
                else
                {
                    error = ErrorCodeValue::InvalidMessage;
                }
            }
            else if (message_->Action == NamingTcpMessage::DeleteNameAction)
            {
                NamingUri name;
                if (message_->GetBody(name))
                {
                    bool worked = store_.DeleteName(name);
                    if (!worked)
                    {
                        error = ErrorCodeValue::OperationFailed;
                    }
                    else
                    {
                        reply_ = NamingMessage::GetPeerNameOperationReply();
                    }
                }
                else
                {
                    error = ErrorCodeValue::InvalidMessage;
                }
            }
            else if (message_->Action == NamingTcpMessage::NameExistsAction)
            {
                NamingUri name;
                if (message_->GetBody(name))
                {
                    bool success = store_.NameExists(name);
                    reply_ = NamingMessage::GetPeerNameExistsReply(NameExistsReplyMessageBody(success, UserServiceState::None));
                }
                else
                {
                    error = ErrorCodeValue::InvalidMessage;
                }
            }
            else if (message_->Action == NamingTcpMessage::PropertyBatchAction)
            {
                NamePropertyOperationBatch body;
                if (!message_->GetBody(body))
                {
                    error = ErrorCodeValue::InvalidMessage;
                }

                NamePropertyOperationBatchResult result;
                NamingUri name = body.NamingUri;
                if (error.IsSuccess() && store_.NameExists(name))
                {
                    vector<size_t> putIndexes;
                    for (size_t i = 0; i < body.Operations.size(); ++i)
                    {
                        if (!result.Error.IsSuccess())
                        {
                            // stop processing more ops
                            break;
                        }

                        switch(body.Operations[i].OperationType)
                        {
                        case NamePropertyOperationType::CheckExistence:
                            if (store_.PropertyExists(name, body.Operations[i].PropertyName) != body.Operations[i].BoolParam)
                            {
                                result.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, i);
                            }
                            break;
                        case NamePropertyOperationType::PutProperty:
                        case NamePropertyOperationType::PutCustomProperty:
                            // Remember put indexes to revert in case of failure
                            putIndexes.push_back(i);
                            store_.AddProperty(
                                name,
                                body.Operations[i].PropertyName, 
                                body.Operations[i].PropertyTypeId,
                                move(body.Operations[i].TakeBytesParam()),
                                body.Operations[i].CustomTypeId);
                            break;
                        case NamePropertyOperationType::DeleteProperty:
                            // NOTE: this deletes properties without regard of whether the batch is successful.
                            // The tests need to take this into consideration.
                            store_.RemoveProperty(name, body.Operations[i].PropertyName);
                            break;
                        case NamePropertyOperationType::GetProperty:
                            {
                            FauxStore::PropertyData data;
                            if (store_.TryGetProperty(name, body.Operations[i].PropertyName, data))
                            {
                                NamePropertyMetadata metadata(data.Name, data.TypeId, static_cast<ULONG>(data.PropertyValue.size()), data.CustomTypeId);
                                if (body.Operations[i].BoolParam)
                                {
                                    // IncludeValues
                                    vector<byte> valueCopy = data.PropertyValue;
                                    result.AddProperty(NamePropertyResult(
                                        NamePropertyMetadataResult(name, move(metadata), DateTime::Now(), data.SequenceNumber, i),
                                        move(valueCopy)));
                                }
                                else
                                {
                                    result.AddMetadata(
                                        NamePropertyMetadataResult(name, move(metadata), DateTime::Now(), data.SequenceNumber, i));
                                }
                            }                        
                            }
                            break;
                        case NamePropertyOperationType::CheckValue:
                            {
                            FauxStore::PropertyData data;
                            if (!store_.TryGetProperty(name, body.Operations[i].PropertyName, data) || 
                                data.TypeId != body.Operations[i].PropertyTypeId ||
                                data.PropertyValue.size() != body.Operations[i].BytesParam.size())
                            {
                                result.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, i);
                            }
                            else
                            {
                                for (size_t ix = 0; ix != data.PropertyValue.size(); ++ix)
                                {
                                    if (data.PropertyValue[ix] != body.Operations[i].BytesParam[ix])
                                    {
                                        result.SetFailedOperationIndex(ErrorCodeValue::PropertyCheckFailed, i);
                                        break;
                                    }
                                }
                            }
                            }
                            break;
                        default:
                            Assert::CodingError(
                                "FauxStore does not support property operation {0}",
                                body.Operations[i].OperationType);
                        }
                    }

                    if (!result.Error.IsSuccess())
                    {
                        for (size_t ix = 0; ix < putIndexes.size(); ++ix)
                        {
                            store_.RemoveProperty(name, body.Operations[ix].PropertyName);
                        }
                    }
                }
                else
                {
                    error = ErrorCodeValue::OperationFailed;
                }

                reply_ = NamingMessage::GetPeerPropertyBatchReply(result);
            }
            else if (message_->Action == NamingTcpMessage::CreateServiceAction)
            {
                CreateServiceMessageBody body;
                if (message_->GetBody(body))
                {
                    bool worked = store_.AddService(body.NamingUri, body.PartitionedServiceDescriptor);
                    if (!worked)
                    {
                        error = ErrorCodeValue::OperationFailed;
                    }
                    else
                    {
                        reply_ = NamingMessage::GetPeerNameOperationReply();
                    }
                }
                else
                {
                    error = ErrorCodeValue::InvalidMessage;
                }
            }
            else if (message_->Action == NamingTcpMessage::DeleteServiceAction)
            {
                DeleteServiceMessageBody body;
                if (message_->GetBody(body))
                {
                    NamingUri serviceName = body.NamingUri;
                    bool worked = store_.RemoveService(serviceName);
                    if (!worked)
                    {
                        error = ErrorCodeValue::OperationFailed;
                    }
                    else
                    {
                        reply_ = NamingMessage::GetPeerNameOperationReply();
                    }
                }
                else
                {
                    error = ErrorCodeValue::InvalidMessage;
                }
            }
            else if (message_->Action == NamingMessage::ServiceExistsAction)
            {
                ServiceCheckRequestBody body;
                if (message_->GetBody(body))
                {
                    if (body.IsPrefixMatch)
                    {
                        error = ErrorCodeValue::NotImplemented;
                    }
                    else
                    {
                        if (store_.ServiceExists(body.Name))
                        {
                            PartitionedServiceDescriptor psd(store_.GetService(body.Name));
                            psd.Version = store_.GetStoreVersion();
                            reply_ = NamingMessage::GetServiceCheckReply(ServiceCheckReplyBody(move(psd)));
                        }
                        else
                        {
                            reply_ = NamingMessage::GetServiceCheckReply(ServiceCheckReplyBody(store_.GetStoreVersion()));
                        }
                    }
                }
                else
                {
                    error = ErrorCodeValue::InvalidMessage;
                }
            }
            else
            {
                error = ErrorCodeValue::OperationFailed;
            }

            TryComplete(thisSPtr, error);
        }

    private:
        FauxStore & store_;
        MessageUPtr message_;
        MessageUPtr reply_;
    };

    // FauxStore Implementation

    NodeId FauxStore::get_Id() const
    {
        return id_;
    }

    bool FauxStore::AddName(NamingUri const & name)
    {
        AcquireWriteLock lock(lock_);
        if (NameExistsCallerHoldsLock(name))
        {
            return false;
        }
        else
        {
            stuff_.insert(pair<NamingUri, vector<PropertyData>>(name, vector<PropertyData>()));
            return true;
        }
    }

    bool FauxStore::DeleteName(NamingUri const & name)
    {
        AcquireWriteLock lock(lock_);
        if (NameExistsCallerHoldsLock(name))
        {
            stuff_.erase(name);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool FauxStore::NameExists(NamingUri const & name)
    {
        AcquireReadLock lock(lock_);
        return NameExistsCallerHoldsLock(name);
    }

    bool FauxStore::NameExistsCallerHoldsLock(NamingUri const & name)
    {
        return stuff_.find(name) != stuff_.end();
    }

    bool FauxStore::AddProperty(NamingUri const & name, wstring const & prop, ::FABRIC_PROPERTY_TYPE_ID typeId, vector<byte> && value, std::wstring const & customTypeId)
    {
        AcquireWriteLock lock(lock_);
        if (PropertyExistsCallerHoldsLock(name, prop))
        {
            return false;
        }
        else
        {
            stuff_[name].push_back(PropertyData(prop, customTypeId, OperationSequenceNumber++, typeId, move(value)));
            return true;
        }
    }

    bool FauxStore::RemoveProperty(NamingUri const & name, wstring const & prop)
    {
        AcquireWriteLock lock(lock_);
        if (PropertyExistsCallerHoldsLock(name, prop))
        {
            stuff_[name].erase(
                find_if(stuff_[name].begin(), stuff_[name].end(), [&prop] (PropertyData const & propData) -> bool { return propData.Name == prop; }));
            return true;
        }
        else
        {
            return false;
        }
    }

    bool FauxStore::PropertyExists(NamingUri const & name, wstring const & prop)
    {
        AcquireReadLock lock(lock_);
        return PropertyExistsCallerHoldsLock(name, prop);
    }

    bool FauxStore::PropertyExistsCallerHoldsLock(NamingUri const & name, wstring const & prop)
    {
        return (NameExistsCallerHoldsLock(name) && 
            find_if(stuff_[name].begin(), stuff_[name].end(), [&prop] (PropertyData const & propData) -> bool { return propData.Name == prop; }) != stuff_[name].end());
    }

    bool FauxStore::TryGetProperty(
        NamingUri const & name, 
        std::wstring const & propName, 
        __out PropertyData & propData)
    {
        AcquireReadLock lock(lock_);
        if (NameExistsCallerHoldsLock(name))
        {
            auto it = find_if(stuff_[name].begin(), stuff_[name].end(), [&propName] (PropertyData const & propData) -> bool { return propData.Name == propName; });
            if (it != stuff_[name].end())
            {
                propData.Name = it->Name;
                propData.SequenceNumber = it->SequenceNumber;
                propData.CustomTypeId = it->CustomTypeId;
                propData.TypeId = it->TypeId;
                propData.PropertyValue = it->PropertyValue;
                return true;
            }
        }

        return false;
    }

    bool FauxStore::AddService(NamingUri const & name, PartitionedServiceDescriptor const & psd)
    {
        AcquireWriteLock lock(lock_);
        if (NameExistsCallerHoldsLock(name))
        {
            if (ServiceExistsCallerHoldsLock(name))
            {
                return false;
            }
            else
            {
                services_[name] = psd;
                return true;
            }
        }
        else
        {
            return false;
        }
    }

    bool FauxStore::RemoveService(NamingUri const & name)
    {
        AcquireWriteLock lock(lock_);
        if (NameExistsCallerHoldsLock(name))
        {
            if (ServiceExistsCallerHoldsLock(name))
            {
                services_.erase(name);
                return true;
            }
        }

        return false;
    }

    bool FauxStore::ServiceExists(NamingUri const & name)
    {
        AcquireReadLock lock(lock_);
        return ServiceExistsCallerHoldsLock(name);
    }

    bool FauxStore::ServiceExistsCallerHoldsLock(NamingUri const & name)
    {
        return services_.find(name) != services_.end();
    }

    PartitionedServiceDescriptor FauxStore::GetService(NamingUri const & name)
    {
        AcquireReadLock lock(lock_);
        return services_[name];
    }

    __int64 FauxStore::GetStoreVersion()
    {
        AcquireReadLock lock(lock_);
        return storeVersion_;
    }

    void FauxStore::IncrementStoreVersion()
    {
        AcquireWriteLock lock(lock_);
        ++storeVersion_;
    }

    AsyncOperationSPtr FauxStore::BeginProcessRequest(
        MessageUPtr && message, 
        TimeSpan const,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
    {
        Trace.WriteNoise(Constants::TestSource, "FauxStore received a client operation message {0}", message);
        return AsyncOperation::CreateAndStart<FauxStoreAsyncOperation>(*this, move(message), callback, parent);
    }

    ErrorCode FauxStore::EndProcessRequest(AsyncOperationSPtr const & operation, MessageUPtr & reply)
    {
        return FauxStoreAsyncOperation::End(operation, reply);
    }
}
