// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This is needed for an array of serializable objects that are not pointers.  It will convert the array to a list of IFabricSerializable pointers
DEFINE_USER_ARRAY_UTILITY(BasicObjectUsingMacro);

struct BasicNestedObject : public FabricSerializable
{
    CHAR _char1;
    SHORT _short1;

    BasicObject _basicObject;

    KArray<BasicObjectUsingMacro> _basicObjectArray;

    BasicNestedObject() : _basicObjectArray(Common::GetSFDefaultPagedKAllocator())
    {
        this->SetConstructorStatus(this->_basicObjectArray.Status());
    }

    bool operator==(BasicNestedObject const & rhs)
    {
        bool equal = 
                   this->_short1 == rhs._short1
                && this->_char1 == rhs._char1
                && this->_basicObject == rhs._basicObject
                && IsArrayEqual(this->_basicObjectArray, rhs._basicObjectArray);

        return equal;
    }

    K_FIELDS_04(_char1, _short1, _basicObject, _basicObjectArray);
};
