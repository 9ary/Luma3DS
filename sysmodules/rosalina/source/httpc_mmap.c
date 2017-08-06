// File derived from libctru
// https://github.com/smealum/ctrulib/blob/master/libctru/source/services/httpc.c

#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <3ds/types.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/synchronization.h>
#include <3ds/services/sslc.h>
#include <3ds/services/httpc.h>
#include <3ds/ipc.h>
#include "httpc_mmap.h"

Handle __httpc_servhandle;
static int __httpc_refcount;

u32 *__httpc_sharedmem_addr;
static u32 __httpc_sharedmem_size;
static Handle __httpc_sharedmem_handle;

static Result HTTPC_Initialize(Handle handle, u32 sharedmem_size, Handle sharedmem_handle);
static Result HTTPC_Finalize(Handle handle);

Result httpcInitMmap(u32 sharedmem_size)
{
	Result ret=0;
	u32 tmp;

	if (AtomicPostIncrement(&__httpc_refcount)) return 0;

	ret = srvGetServiceHandle(&__httpc_servhandle, "http:C");
	if (R_SUCCEEDED(ret))
	{
		__httpc_sharedmem_size = sharedmem_size;
		__httpc_sharedmem_handle = 0;

		if(__httpc_sharedmem_size)
		{
			__httpc_sharedmem_addr = (u32 *)0x14000000 - __httpc_sharedmem_size;
			ret = svcControlMemory((u32 *)&__httpc_sharedmem_addr, (u32)__httpc_sharedmem_addr, (u32)__httpc_sharedmem_addr, __httpc_sharedmem_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);

			if (R_SUCCEEDED(ret))
			{
				memset(__httpc_sharedmem_addr, 0, __httpc_sharedmem_size);
				ret = svcCreateMemoryBlock(&__httpc_sharedmem_handle, (u32)__httpc_sharedmem_addr, __httpc_sharedmem_size, 0, 3);
			}
		}

		if (R_SUCCEEDED(ret))ret = HTTPC_Initialize(__httpc_servhandle, __httpc_sharedmem_size, __httpc_sharedmem_handle);
		if (R_FAILED(ret)) svcCloseHandle(__httpc_servhandle);
	}
	if (R_FAILED(ret)) AtomicDecrement(&__httpc_refcount);

	if (R_FAILED(ret) && __httpc_sharedmem_handle)
	{
		svcCloseHandle(__httpc_sharedmem_handle);
		__httpc_sharedmem_handle = 0;
		__httpc_sharedmem_size = 0;

		svcControlMemory(&tmp, (u32)__httpc_sharedmem_addr, (u32)__httpc_sharedmem_addr, __httpc_sharedmem_size, MEMOP_FREE, MEMPERM_DONTCARE);
		__httpc_sharedmem_addr = NULL;
	}

	return ret;
}

void httpcExitMmap(void)
{
	u32 tmp;

	if (AtomicDecrement(&__httpc_refcount)) return;

	HTTPC_Finalize(__httpc_servhandle);

	svcCloseHandle(__httpc_servhandle);

	if(__httpc_sharedmem_handle)
	{
		svcCloseHandle(__httpc_sharedmem_handle);
		__httpc_sharedmem_handle = 0;
		__httpc_sharedmem_size = 0;

		svcControlMemory(&tmp, (u32)__httpc_sharedmem_addr, (u32)__httpc_sharedmem_addr, __httpc_sharedmem_size, MEMOP_FREE, MEMPERM_DONTCARE);
		__httpc_sharedmem_addr = NULL;
	}
}

static Result HTTPC_Initialize(Handle handle, u32 sharedmem_size, Handle sharedmem_handle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=IPC_MakeHeader(0x1,1,4); // 0x10044
	cmdbuf[1]=sharedmem_size; // POST buffer size (page aligned)
	cmdbuf[2]=IPC_Desc_CurProcessHandle();
	cmdbuf[4]=IPC_Desc_SharedHandles(1);
	cmdbuf[5]=sharedmem_handle;// POST buffer memory block handle

	Result ret=0;
	if(R_FAILED(ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

static Result HTTPC_Finalize(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=IPC_MakeHeader(0x39,0,0); // 0x390000

	Result ret=0;
	if(R_FAILED(ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}
