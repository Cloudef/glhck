#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int glswInit();
int glswShutdown();
int glswSetPath(const char* pathPrefix, const char* pathSuffix);
const char* glswGetShader(const char* effectKey);
const char* glswGetError();
int glswAddDirectiveToken(const char* token, const char* directive);

#ifdef __cplusplus
}
#endif
