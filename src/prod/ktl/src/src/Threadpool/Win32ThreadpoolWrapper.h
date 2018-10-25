#include "MinPal.h"
#include "Threadpool.h"

using namespace KtlThreadpool;

// TP_POOL Related
typedef struct _TP_POOL
{
    ThreadpoolMgr *pThreadpoolMgr;
} TP_POOL, *PTP_POOL;


// TP_WORK Related
typedef struct _TP_CALLBACK_ENVIRON_V3  *PTP_CALLBACK_ENVIRON;
typedef struct _TP_CALLBACK_INSTANCE *PTP_CALLBACK_INSTANCE;
typedef struct _TP_WORK *PTP_WORK;

typedef VOID (*PTP_WORK_CALLBACK)(
        PTP_CALLBACK_INSTANCE Instance,
        PVOID                 Context,
        PTP_WORK              Work
);

typedef struct _TP_WORK
{
    PVOID CallbackParameter;
    PTP_WORK_CALLBACK WorkCallback;
    PTP_CALLBACK_ENVIRON CallbackEnvironment;
} TP_WORK, *PTP_WORK;

// TP_CALLBACK_ENVIRON Related
typedef DWORD TP_VERSION, *PTP_VERSION;

typedef struct _TP_CALLBACK_INSTANCE
{
    PTP_WORK Work;
} TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;

typedef VOID (*PTP_SIMPLE_CALLBACK)(
        PTP_CALLBACK_INSTANCE Instance,
        PVOID                 Context
);

typedef enum _TP_CALLBACK_PRIORITY {
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
    TP_CALLBACK_PRIORITY_INVALID,
    TP_CALLBACK_PRIORITY_COUNT = TP_CALLBACK_PRIORITY_INVALID
} TP_CALLBACK_PRIORITY;

typedef struct _TP_CLEANUP_GROUP {
    // do not care
} TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP;

typedef struct _ACTIVATION_CONTEXT {
    // do not care
} ACTIVATION_CONTEXT;

typedef VOID (*PTP_CLEANUP_GROUP_CANCEL_CALLBACK)(
        PVOID ObjectContext,
        PVOID CleanupContext
);

typedef struct _TP_CALLBACK_ENVIRON_V3 {
    TP_VERSION                         Version;
    PTP_POOL                           Pool;
    PTP_CLEANUP_GROUP                  CleanupGroup;
    PTP_CLEANUP_GROUP_CANCEL_CALLBACK  CleanupGroupCancelCallback;
    PVOID                              RaceDll;
    struct _ACTIVATION_CONTEXT        *ActivationContext;
    PTP_SIMPLE_CALLBACK                FinalizationCallback;
    union {
        DWORD                          Flags;
        struct {
            DWORD                      LongFunction :  1;
            DWORD                      Persistent   :  1;
            DWORD                      Private      : 30;
        } s;
    } u;
    TP_CALLBACK_PRIORITY               CallbackPriority;
    DWORD                              Size;
} TP_CALLBACK_ENVIRON_V3;

typedef TP_CALLBACK_ENVIRON_V3 TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;

VOID KtlInitializeThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);

VOID KtlDestroyThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);

VOID KtlSetThreadpoolCallbackPool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp);

PTP_POOL KtlCreateThreadpool(PVOID reserved);

VOID KtlCloseThreadpool(PTP_POOL ptpp);

BOOL KtlSetThreadpoolThreadMinimum(PTP_POOL ptpp, DWORD cthrdMic);

VOID KtlSetThreadpoolThreadMaximum(PTP_POOL ptpp, DWORD cthrdMost);

PTP_WORK KtlCreateThreadpoolWork(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe);

VOID KtlCloseThreadpoolWork(PTP_WORK pwk);

VOID KtlSubmitThreadpoolWork(PTP_WORK pwk);

VOID KtlWaitForThreadpoolWorkCallbacks(PTP_WORK pwk, BOOL fCancelPendingCallbacks);
