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
            /*
                Represents a view over a vector
            */
            template<typename T, typename Predicate>
            class FilteredVector
            {
                DENY_COPY(FilteredVector);
            public:
                typedef typename std::vector<T>::iterator VectorIteratorType;
                typedef typename std::vector<T>::const_iterator VectorConstIteratorType;
                typedef boost::iterators::filter_iterator<Predicate, VectorIteratorType> IteratorType;
                typedef boost::iterators::filter_iterator<Predicate, VectorConstIteratorType> ConstIteratorType;

                FilteredVector(std::vector<T> & vector, Predicate predicate) : vector_(vector), predicate_(predicate)
                {
                }

                __declspec(property(get = get_IsEmpty)) bool IsEmpty;
                bool get_IsEmpty() const { return begin() == end(); }

                IteratorType begin()
                {
                    return IteratorType(predicate_, vector_.begin(), vector_.end());
                }

                IteratorType end()
                {
                    return IteratorType(predicate_, vector_.end(), vector_.end());
                }

                ConstIteratorType begin() const
                {
                    return ConstIteratorType(predicate_, vector_.begin(), vector_.end());
                }

                ConstIteratorType end() const
                {
                    return ConstIteratorType(predicate_, vector_.end(), vector_.end());
                }

                void Clear()
                {
                    vector_.erase(
                        std::remove_if(vector_.begin(), vector_.end(), predicate_),
                        vector_.end());
                }

            private:
                std::vector<T> & vector_;
                Predicate predicate_;
            };
        }
    }
}



