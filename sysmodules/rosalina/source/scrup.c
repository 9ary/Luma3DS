#include <3ds.h>
#include <string.h>
#include "scrup.h"
#include "ifile.h"

Result scrup_upload(u8 *jpeg_data, u32 jpeg_size)
{
#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

    Result res = 0;
    httpcContext context;
    u32 statuscode = 0;
    IFile file;
    char config_buf[128];

    TRY(httpcOpenContext(&context, HTTPC_METHOD_POST, "https://wank.party/api/upload", 0));

    // This disables SSL cert verification, so https:// will be usable
    // TODO: figure out why Let's Encrypt root certs don't work
    TRY(httpcSetSSLOpt(&context, SSLCOPT_DisableVerify));
    TRY(httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED));
    TRY(httpcAddRequestHeaderField(&context, "User-Agent", "luma-scrup/1.0.0"));

    // Messy stuff, isolate it into a scope of its own
    {
        u64 total;
        FS_ArchiveID archiveId;
        s64 out;
        bool isSdMode;

        if(R_FAILED(svcGetSystemInfo(&out, 0x10000, 0x203))) svcBreak(USERBREAK_ASSERT);
        isSdMode = (bool)out;
        archiveId = isSdMode ? ARCHIVE_SDMC : ARCHIVE_NAND_RW;

        TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/scrup"), FS_OPEN_READ));
        TRY(IFile_GetSize(&file, &total));
        total = (total < (sizeof(config_buf) - 1)) ? total : (sizeof(config_buf) - 1);
        TRY(IFile_Read(&file, &total, config_buf, total));
        config_buf[total] = '\0';

        // This is probably going to crash if the config file is malformed, not my problem
        TRY(httpcAddPostDataAscii(&context, "key", strtok(config_buf, ";")));
        TRY(httpcAddPostDataAscii(&context, "tg_uid", strtok(NULL, ";")));
    }

    TRY(httpcAddPostDataBinary(&context, "file\"; filename=\"3dscrot.jpg", jpeg_data, jpeg_size));

    TRY(httpcBeginRequest(&context));
    TRY(httpcGetResponseStatusCodeTimeout(&context, &statuscode, 15ULL * 1000 * 1000 * 1000));

    if (statuscode != 200)
    {
        // BCD-encode the status code to make it human readable in the hex representation
        res = 0;
        while (statuscode)
        {
            res <<= 4;
            res |= statuscode % 10;
            statuscode /= 10;
        }
        res |= 0xFFFF0000;

        goto end;
    }

end:
    IFile_Close(&file);
    httpcCloseContext(&context);

    return res;

#undef TRY
}
