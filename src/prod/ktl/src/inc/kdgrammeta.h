/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    kdgrammeta.h

    Description:
      Kernel Tempate Library (KTL): Networking & Transport Support

      Defines classes for networking and datagram transmission

    History:
      raymcc          28-Feb-2011         Initial draft

--*/

#pragma once


// KDatagramMetadata
//
// Contains a set of metadata items which can be attached to a datagram.  This object can be reused
// as required.
//
class KDatagramMetadata : public KObject<KDatagramMetadata>, public KShared<KDatagramMetadata>
{
    struct MetadataItem
    {
        ULONG       _Id;
        KVariant    _Data;
        MetadataItem()
        {
            _Id = 0;
        }
    };

    KDatagramMetadata(
        __in KAllocator& Alloc
        )
        : _Items(Alloc), _Allocator(Alloc)
        {
        }

public:

    typedef KSharedPtr<KDatagramMetadata> SPtr;
    typedef KUniquePtr<KDatagramMetadata> UPtr;

    static NTSTATUS Create(__in KAllocator& Alloc,
                           __out KDatagramMetadata::SPtr& Ptr)
    {
        NTSTATUS status;

        Ptr = _new(KTL_TAG_DGRAMMETA, Alloc) KDatagramMetadata(Alloc);

        status = Ptr->Status();
        if (! NT_SUCCESS(status))
        {
            Ptr = nullptr;
        }

        return(status);
    }

   ~KDatagramMetadata()
    {
        Clear();
    }

    // Count
    //
    // Returns the number of items in the metadata container.
    //
    ULONG Count()
    {
        return _Items.Count();
    }

    // Clear
    //
    // Removes all items.
    //
    void Clear()
    {
        for (ULONG ix = 0; ix < _Items.Count(); ix++)
        {
            if (_Items[ix])
            {
                _delete(_Items[ix]);
            }
        }

        _Items.Clear();
    }

    // RemoveById
    //
    // Removes the metadata item by its user-assigned Id value.
    //
    // Parameters:
    //      Id          The user-assigned id of the item to remove.
    //
    // Return values:
    //      TRUE if the item was found.
    //      FALSE if the item was not found.
    //
    BOOLEAN RemoveById(
        __in ULONG Id
        )
    {
        for (ULONG ix = 0; ix < _Items.Count(); ix++)
        {
            if (_Items[ix]->_Id == Id)
            {
                _delete(_Items[ix]);
                _Items.Remove(ix);
                return TRUE;
            }
        }

        return FALSE;
    }

    // Set
    //
    // Sets the specified item, either creating it or overwriting any previous item
    // with the matching id.
    //
    // Parameters:
    //      Id      The user-assigned id of the metadata item.
    //      Data    The data item.
    //
    // Return value:
    //  STATUS_SUCCESS
    //  STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS Set(
        __in ULONG Id,
        __in const KVariant& Data
        )
    {
        // See if the item already exists.
        //
        for (ULONG ix = 0; ix < _Items.Count(); ix++)
        {
            if (_Items[ix]->_Id == Id)
            {
                _Items[ix]->_Data = Data;
                return _Items.Status();
            }
        }

        // If here, we'll just append the new item.
        //
        MetadataItem* NewItem = _new (KTL_TAG_DGRAMMETA, _Allocator) MetadataItem();
        if (!NewItem)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NewItem->_Id = Id;
        NewItem->_Data = Data;
        _Items.Append(NewItem);
        return _Items.Status();
    }

    // Get
    //
    // Retrieves the specified item by its specified id.
    //
    // Parameters:
    //
    //      Id      The user-assigned id of the metadata item.
    //      Data    Receives the item.
    // Return value:
    //      STATUS_SUCCESS                If the item was found and returned.
    //      STATUS_INSUFFICIENT_RESOURCES If not enough memory to return the item.
    //      STATUS_OBJECT_NAME_NOT_FOUND  If the specified Id was not found.
    //
    NTSTATUS Get(
        __in ULONG Id,
        __in KVariant& Data
        )
    {
        // See if the item exists.

        for (ULONG ix = 0; ix < _Items.Count(); ix++)
        {
            if (_Items[ix]->_Id == Id)
            {
                Data = _Items[ix]->_Data;
                return STATUS_SUCCESS;
            }
        }
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }


    // GetAt()
    //
    // Used for enumerating all metadata items, primarily
    // for debugging, diagnostics, etc.
    //
    // Parameters:
    //      Index       The index of the item to be retrieved.
    //      Id          Receives the id value at that location.
    //      Item        Receives the value.
    //
    // Return value:
    //      TRUE if the item was retrieved.
    //      FALSE if the item was not retrieved (range error).
    //
	_Success_(return == TRUE)
    BOOLEAN GetAt(
        __in  ULONG Index,
        __out ULONG& Id,
        __out KVariant& Item
        )
    {
        if (Index >= _Items.Count())
        {
            return FALSE;
        }
        Id = _Items[Index]->_Id;
        Item = _Items[Index]->_Data;
        return TRUE;
    }

private:

    K_DENY_COPY(KDatagramMetadata);

    KArray<MetadataItem*> _Items;
    KAllocator& _Allocator;
};


