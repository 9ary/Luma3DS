#include <3ds.h>
#include "scrup.h"

Result scrup_upload(u8 *jpeg_data, u32 jpeg_size)
{
#define TRY(expr) if(R_FAILED(res = (expr))) goto end;

    Result res = 0;
    httpcContext context;
    u32 statuscode = 0;
    u32 contentsize = 0, readsize = 0, size = 0;

    TRY(httpcOpenContext(&context, HTTPC_METHOD_POST, "https://wank.party/api/upload", 0));

    // This disables SSL cert verification, so https:// will be usable
    // TODO: add a real certificate fingerprint
    TRY(httpcSetSSLOpt(&context, SSLCOPT_DisableVerify));

    // Enable Keep-Alive connections
    TRY(httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED));

    // Set a User-Agent header so websites can identify your application
    TRY(httpcAddRequestHeaderField(&context, "User-Agent", "luma-scrup/1.0.0"));

    // TODO: read API key and UID from config file
    TRY(httpcAddPostDataAscii(&context, "key", "<redacted>"));
    TRY(httpcAddPostDataAscii(&context, "tg_uid", "<redacted>"));

    TRY(httpcAddPostDataBinary(&context, "file\"; filename=\"3dscrot.jpg", jpeg_data, jpeg_size));

    TRY(httpcBeginRequest(&context));
    TRY(httpcGetResponseStatusCodeTimeout(&context, &statuscode, 15ULL * 1000 * 1000 * 1000));

end:
    httpcCloseContext(&context);

    return res;

#undef TRY
}
