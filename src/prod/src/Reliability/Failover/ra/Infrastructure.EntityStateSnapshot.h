// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template<typename T>
            class EntityStateSnapshot
            {
            public:
                typedef typename EntityTraits<T>::DataType DataType;
                typedef std::shared_ptr<DataType> DataSPtrType;

                EntityStateSnapshot()
                : isDeleted_(false)
                {
                }

                EntityStateSnapshot(DataSPtrType && data)
                : isDeleted_(false),
                  data_(std::move(data))
                {
                }

                __declspec(property(get = get_Data)) DataSPtrType const & Data;
                DataSPtrType const & get_Data() const { return data_; } 

                __declspec(property(get = get_IsDeleted)) bool IsDeleted;
                bool get_IsDeleted() const { return isDeleted_; }       

                void Delete()
                {
                    data_.reset();
                    isDeleted_ = true;
                }

                void SetData(DataSPtrType const & data)
                {
                    AssertIsNotDeleted();
                    data_ = data;
                }

                void SetData(DataSPtrType && data)
                {
                    AssertIsNotDeleted();
                    data_ = std::move(data);
                }

                EntityStateSnapshot Clone() const 
                {
                    if (data_ == nullptr)
                    {
                        // deleted or entity that has just been inserted
                        return *this;
                    }
                    
                    // perform deep copy
                    DataSPtrType copy = std::make_shared<DataType>(*data_);
                    return EntityStateSnapshot(std::move(copy));
                }

            private:
                void AssertIsNotDeleted() const { ASSERT_IF(isDeleted_, "cannot be deleted"); }
                DataSPtrType data_;
                bool isDeleted_;
            };
        }
    }
}

