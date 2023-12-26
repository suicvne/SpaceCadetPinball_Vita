#include <psp2/types.h>
extern "C" {
#define SCE_LIVEAREA_FORMAT_VER_CURRENT    "01.00"
#define SCE_LIVEAREA_FLAG_NONE             (0)
SceInt32 sceLiveAreaUpdateFrameSync(const char* formatVer,
                                    const char* frameXmlStr,
                                    SceInt32 frameXmlLen,
                                    const char* dirpathTop,
                                    SceUInt32 flag);
}