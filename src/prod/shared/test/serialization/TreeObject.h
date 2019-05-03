// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

struct TreeObject : public Serialization::FabricSerializable
{
    typedef KUniquePtr<TreeObject> UPtr;

    KArray<KUniquePtr<TreeObject>> _children;
    KWString _name;
    ULONG _id;

    TreeObject()
        : _children(Common::GetSFDefaultNonPagedKAllocator())
        , _name(Common::GetSFDefaultNonPagedKAllocator())
    {
    }

    virtual ULONG TestId() const
    {
        return Id;
    }

    static const SHORT Id = 0;

    friend bool operator==(TreeObject const & lhs, TreeObject const & rhs);

    K_FIELDS_03(_name, _children, _id);
};

struct TreeObjectV2 : public TreeObject
{
    typedef KUniquePtr<TreeObjectV2> UPtr;

    KGuid _guid;

    TreeObjectV2() 
    {
        this->_guid.CreateNew();
    }

    virtual NTSTATUS GetTypeInformation(__out Serialization::FabricTypeInformation & typeInformation) const
    {
        typeInformation.buffer = reinterpret_cast<UCHAR const *>(&TreeObjectV2::Id);
        typeInformation.length = sizeof(TreeObjectV2::Id);

        return STATUS_SUCCESS;
    }

    virtual ULONG TestId() const
    {
        return Id;
    }

    static const SHORT Id = 1;

    friend bool operator==(TreeObjectV2 const & lhs, TreeObjectV2 const & rhs);

    K_FIELDS_01(_guid);
};

bool operator==(TreeObjectV2 const & lhs, TreeObjectV2 const & rhs)
{
    return (lhs._guid == rhs._guid) == TRUE;
}

bool operator==(TreeObject const & lhs, TreeObject const & rhs)
{
    bool equal = (lhs.TestId() == rhs.TestId());

    if (equal && lhs.TestId() == TreeObjectV2::Id)
    {
        // TODO: is this right?
        equal = (*static_cast<TreeObjectV2 const *>(&lhs) == *static_cast<TreeObjectV2 const *>(&rhs));
    }

    if (equal)
    {
        equal = (lhs._name.CompareTo(rhs._name) == 0) &&
                (lhs._id == rhs._id);
    }

    if (equal)
    {
        equal = IsArrayEqual(lhs._children, rhs._children);
    }

    return equal;
}

void FillTree(TreeObject::UPtr const & ptr, ULONG count, bool mixVersions, ULONG maxDepth = 5)
{
    if (maxDepth == 0)
    {
        return;
    }

    for (ULONG i = 0; i < count; ++i)
    {
        ULONG seed = 1;
        ULONG randomLong = RtlRandomEx(&seed);

        TreeObject::UPtr treeUPtr;

        if (randomLong % 2 == 0 || (!mixVersions) )
        {
            treeUPtr = TreeObject::UPtr(_new (WF_SERIALIZATION_TAG, Common::GetSFDefaultNonPagedKAllocator()) TreeObject());
        }
        else
        {
            treeUPtr = TreeObject::UPtr(_new (WF_SERIALIZATION_TAG, Common::GetSFDefaultNonPagedKAllocator()) TreeObjectV2());
        }

        treeUPtr->_name = ptr->_name;
        treeUPtr->_name += L"C";
        treeUPtr->_id = i;

        randomLong = RtlRandomEx(&seed);

        randomLong = (randomLong % 4);
        FillTree(treeUPtr, randomLong, mixVersions, maxDepth - 1);

        ptr->_children.Append(Ktl::Move(treeUPtr));
    }
}

TreeObject::UPtr CreateTree(bool mixVersions = false)
{
    TreeObject::UPtr treeUPtr(_new (WF_SERIALIZATION_TAG, Common::GetSFDefaultNonPagedKAllocator()) TreeObject());

    treeUPtr->_name = L"R";
    treeUPtr->_id = 0;

    FillTree(treeUPtr, 5, mixVersions, 5);

    return Ktl::Move(treeUPtr);
}

namespace Serialization
{
    template <>
    Serialization::IFabricSerializable * DefaultActivator<TreeObject>(Serialization::FabricTypeInformation typeInformation)
    {
        if (typeInformation.length == 0)
        {
            return _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultNonPagedKAllocator()) TreeObject();
        }
        else
        {
            return _new (WF_SERIALIZATION_TAG, Common::GetSFDefaultNonPagedKAllocator()) TreeObjectV2();
        }
    }
}
