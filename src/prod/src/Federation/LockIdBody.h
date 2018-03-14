// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{

class LockIdBody : public Serialization::FabricSerializable
{
public:
    LockIdBody() 
        : id_(0)
    {
    }

    LockIdBody(int64 id) 
        : id_(id)
    {
    }

    int64 get_Id() const { return this->id_; }

    FABRIC_FIELDS_01(id_);

private:

    int64 id_;
};

}
