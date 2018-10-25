

// KSocketTable
//
// Forms the intermediate layer between KNetwork and the underlying raw KSocket/KNetServiceLayer
//
// All incoming sockets are added to this table first, and all requests to create outbound
// sockets come through here.
//
class KSocketTable : public KObject<KSocketTable>, public KShared<KSocketTable>
{
    K_FORCE_SHARED(KSocketTable);
public:

    NTSTATUS
    Create(
        __in KAllocator& Alloc,
        __out KSocketTable::SPtr& Table
        );




};
