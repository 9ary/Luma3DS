#include <3ds.h>
#include "scrup.h"
#include "ifile.h"
#include "jsmn.h"
#include "memory.h"

static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
	return (int)strlen(s) == tok->end - tok->start &&
            strncmp(s, json + tok->start, tok->end - tok->start) == 0;
}

Result scrup_upload(u8 *jpeg_data, u32 jpeg_size)
{
#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

    Result res = 0;
    httpcContext context;
    u32 statuscode = 0;
    IFile file;
    char json_buf[1024];
    jsmn_parser jp;
    jsmntok_t jt[16];

    jsmn_init(&jp);

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
        int r;

        if(R_FAILED(svcGetSystemInfo(&out, 0x10000, 0x203))) svcBreak(USERBREAK_ASSERT);
        isSdMode = (bool)out;
        archiveId = isSdMode ? ARCHIVE_SDMC : ARCHIVE_NAND_RW;

        TRY(IFile_Open(&file, archiveId, fsMakePath(PATH_EMPTY, ""), fsMakePath(PATH_ASCII, "/luma/scrup"), FS_OPEN_READ));
        TRY(IFile_GetSize(&file, &total));
        total = (total < (sizeof(json_buf) - 1)) ? total : (sizeof(json_buf) - 1);
        TRY(IFile_Read(&file, &total, json_buf, total));

        r = jsmn_parse(&jp, json_buf, total, jt, sizeof(jt) / sizeof(jsmntok_t));
        if (r < 0)
        {
            res = -1;
            goto end;
        }

        for (int i = 0; i < r; i++)
        {
            if (jsoneq(json_buf, &jt[i], "api_key"))
            {
                i++;
                json_buf[jt[i].end] = '\0';
                TRY(httpcAddPostDataAscii(&context, "key", json_buf + jt[i].start));
                continue;
            }

            if (jsoneq(json_buf, &jt[i], "tg_uid"))
            {
                i++;
                json_buf[jt[i].end] = '\0';
                TRY(httpcAddPostDataAscii(&context, "tg_uid", json_buf + jt[i].start));
                continue;
            }
        }
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
