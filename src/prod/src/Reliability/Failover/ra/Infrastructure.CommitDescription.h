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
            template <typename T>
            struct CommitDescription
            {
                typedef typename EntityTraits<T>::DataType DataType;
                typedef std::shared_ptr<DataType> DataTypeSPtr;

                CommitTypeDescription Type;
                DataTypeSPtr Data;
            };
        }
    }
}


