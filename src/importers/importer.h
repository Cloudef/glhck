#ifndef __glhck_importer_h__
#define __glhck_importer_h__

#include <glhck/import.h>
#include "buffer/buffer.h"

#define IMAGE_DIMENSIONS_OK(w, h) \
   (((w) > 0) && ((h) > 0) && ((unsigned long long)(w) * (unsigned long long)(h) <= (1ULL << 29) - 1))

struct glhckImporterInfo {
   int (*check)(chckBuffer*);
   int (*imageImport)(chckBuffer*, glhckImportImageStruct*);
   int (*modelImport)(chckBuffer*, glhckImportModelStruct*);
};

int _glhckImporterInit(void);
void _glhckImporterTerminate(void);
int _glhckImporterRegister(const char *file);
const struct glhckImporterInfo* _glhckImporterGetInfo(size_t importer, const char **outName);

#endif /* __glhck_importer_h__ */
