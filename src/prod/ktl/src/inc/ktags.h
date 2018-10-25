/*++

Copyright (c) 2010  Microsoft Corporation

Module Name:

    ktags.h

Abstract:

    Header file which contains the KTL pool tags.

Author:

    RVD Group 15-Oct-2010

Environment:

    Kernel mode and User mode

Notes:

--*/

//
// Memory allocation tags for KTL objects.
//

const ULONG KTL_TAG_ARRAY               = 'yarA';   // 'Aray' Default TAG for KArray allocations
const ULONG KTL_TAG_QUEUE               = 'euQK';   // 'KQue' Default TAG for KQueue allocations
const ULONG KTL_TAG_STACK               = 'ktSK';   // 'KStk' Default TAG for KStack allocations
const ULONG KTL_TAG_BASE                = 'saBK';   // 'KBas' Default TAG for all allocations.
const ULONG KTL_TAG_KOBJECT             = 'jbOK';   // 'KObj' Default TAG for KObject<> allocations.
const ULONG KTL_TAG_KSHARED             = 'ahSK';   // 'KSha' Default TAG for KShared<> allocations.
const ULONG KTL_TAG_BUFFER              = 'fuBK';   // 'KBuf' Default TAG for KBuffer() allocations.
const ULONG KTL_TAG_IO_BUFFER           = 'bOIK';   // 'KIOb' Default TAG for KIoBuffer() allocations.
const ULONG KTL_TAG_THREAD              = 'rhTK';   // 'KThr' Default TAG for KThread() allocations.
const ULONG KTL_TAG_THREAD_POOL         = 'opTK';   // 'KTpo' Default TAG for KThreadPool() allocations.
const ULONG KTL_TAG_WSTRING             = 'tsWK';   // 'KWst' Default TAG for KWString() allocations.
const ULONG KTL_TAG_FILE                = 'liFK';   // 'KFil' Default TAG for KFile() allocations.
const ULONG KTL_TAG_AVL_TREE            = 'lvAK';   // 'KAvl' Default TAG for KAvlTree<> allocations.
const ULONG KTL_TAG_TIMER               = 'rmTK';   // 'KTmr' Default TAG for KTimer allocations.
const ULONG KTL_TAG_NAMESPACE           = 'maNK';   // 'KNam' Default TAG for KNamespace() allocations.
const ULONG KTL_TAG_BITMAP              = 'tiBK';   // 'KBit' Default TAG for KBitmap() allocations.
const ULONG KTL_TAG_LOGGER              = 'goLK';   // 'KLog' Default TAG for RvdLogger allocations.
const ULONG KTL_TAG_WEAKREF             = 'erWK';   // 'KWre' Default TAG for KWeakRef<> allocations.
const ULONG KTL_TAG_TEST                = 'tseT';   // 'Test' Default TAG for Test related allocations.
const ULONG KTL_TAG_SHIFT_ARRAY         = 'AfSK';   // 'KSfA' Default TAG for KShiftArray<> allocations.
const ULONG KTL_TAG_RANGE_LOCK          = 'olRK';   // 'KRlo' Default TAG for KRangeLock<> allocations.
const ULONG KTL_TAG_NET                 = 'teNK';   // 'KNet' Default TAG for KNet* family of objects.
const ULONG KTL_TAG_REGISTRY            = 'geRK';   // 'KReg' Default TAG for KRegistry objects
const ULONG KTL_TAG_KHASHTABLE          = 'hsHK';   // 'KHsh' Default TAG for KHashTable allocations
const ULONG KTL_TAG_NET_BLOCK           = 'BteN';   // 'NetB' Default TAG for KMemoryChannel block allocations.
const ULONG KTL_TAG_SERIAL              = 'reSK';   // 'KSer' Default TAG for KSerial block allocations.
const ULONG KTL_TAG_VARIANT             = 'raVK';   // 'KVar' Default TAG for KVariant block allocations.
const ULONG KTL_TAG_DGRAMMETA           = 'mgDK';   // 'KDgm' Default TAG for KDatagramMetadata
const ULONG KTL_TAG_CMDLINE             = 'lmCK';   // 'KCml' Default TAG for KCmdLineParser block allocations.
const ULONG KTL_TAG_UTILITY             = 'litU';   // 'Util' Default TAG for Utility related allocations.
const ULONG KTL_TAG_READ_CACHE          = 'acRK';   // 'KRca' Default TAG for KReadCache<> allocations.
const ULONG KTL_TAG_CACHED_BLOCK_FILE   = 'fbCK';   // 'KCbf' Default TAG for KCachedBlockFile() allocations.
const ULONG KTL_TAG_NAMESVC             = 'vsNK';   // 'KNsv' Default TAG for KNameSvc (Winfab)
const ULONG KTL_TAG_WINFAB              = 'baFW';   // 'WFab' Default TAG for Winfab service/client wrappers
const ULONG KTL_TAG_NET_CHANNEL         = 'CteN';   // 'NetC' Default TAG for KNetChannel related allocations.
const ULONG KTL_TAG_RESOURCE_LOCK       = 'klrK';   // 'Krlk' Default TAG for KResourceLock() allocations.
const ULONG KTL_TAG_PERFCTR             = 'frPK';   // 'KPrf' Default TAG for KPerfCounter* objects.
const ULONG KTL_TAG_XMLPARSER           = 'PmXK';   // 'KXmP' Default TAG for KXmlParser objects.
const ULONG KTL_TAG_DOM                 = 'moDK';   // 'KDom' Default TAG for KDom objects.
const ULONG KTL_TAG_MESSAGE_PORT        = 'pmtK';   // 'Ktmp' Default TAG for KMessagePort() objects.
const ULONG KTL_TAG_BUFFERED_STRINGVIEW = 'tSBK';   // 'KBSt' Default TAG for KBufferedStringView objects.
const ULONG KTL_TAG_URI                 = 'irUK';   // 'KUri' Default TAG for KHttp* objects.
const ULONG KTL_TAG_HTTP                = 'ttHK';   // 'KHtt' Default TAG for KHttp* objects.
const ULONG KTL_TAG_WEB_SOCKET          = 'sbWK';   // 'KWbs' Default TAG for KWebSocket objects.
const ULONG KTL_TAG_NETWORK_ENDPOINT    = 'peNK';   // 'KNep' Default TAG for KNetworkEndpoint objects
const ULONG KTL_TAG_TASK                = 'saTK';   // 'KTas' Default TAG for KTask objects.
const ULONG KTL_TAG_ALPC                = 'cpLK';   // 'KLpc' Default TAG for KLpc related objects.
const ULONG KTL_TAG_DEFERRED_CALL       = 'acDK';   // 'KDca' Default TAG for KDeferredCall objects.
const ULONG KTL_TAG_ASYNC_SERVICE       = 'svAK';   // 'KAsv' Default TAG for KAsyncServicebase related objects.
const ULONG KTL_TAG_MGMT_SERVER         = 'sgMK';   // 'KMgs' Default TAG for KMgmtServer
const ULONG KTL_TAG_SSPI                = 'psSK';   // 'KSsp' Default TAG for KSspi
const ULONG KTL_TAG_COM_DLL             = 'lDCK';   // 'KCDl' Default TAG for KtlComDll
const ULONG KTL_TAG_Awaitable			= 'twAK';   // 'KAwt' Default TAG for awaitable related allocations

