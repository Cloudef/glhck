#ifndef __glhck_import_h__
#define __glhck_import_h__

/* Import struct for post processing */
typedef struct _glhckImagePostProcessStruct
{
   void *data;
   unsigned int flags;
   int width, height;
   glhckTextureFormat format;
   glhckDataType type;
} _glhckImagePostProcessStruct;

/* Load and unload functions when importers are configured
 * to be loaded dynamically */
#if GLHCK_IMPORT_DYNAMIC
int _glhckLoadImporters(void);
int _glhckUnloadImporters(void);
#endif

/* Wraps around the other imports by figuring out the header
 * 1. pointer to sceneobject
 * 2. filename
 * 3. 1 = import animation data, 0 = don't import
 */
int _glhckImportModel(glhckObject *object, const char *file, unsigned int flags,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype);

#if !GLHCK_IMPORT_DYNAMIC

#if GLHCK_IMPORT_OPENCTM
/* OpenCTM http://openctm.sourceforge.net/ */
int _glhckImportOpenCTM(glhckObject *object, const char *file, unsigned int flags,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
int _glhckFormatOpenCTM(const char *file);
#endif

#if GLHCK_IMPORT_MMD
/* MikuMikuDance PMD */
int _glhckImportPMD(glhckObject *object, const char *file, unsigned int flags,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
int _glhckFormatPMD(const char *file);
#endif

#if GLHCK_IMPORT_ASSIMP
/* Assimp Wrapper http://assimp.sourceforge.net/ */
int _glhckImportAssimp(glhckObject *object, const char *file, unsigned int flags,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
int _glhckFormatAssimp(const char *file);
#endif

#endif /* GLHCK_IMPORT_DYNAMIC */

/* Wraps around the other imports by figuring out the header
 * 1. pointer to texture object
 * 2. filename
 */
int _glhckImportImage(glhckTexture *texture, const char *file, unsigned int flags);

/* Post-process the image data */
int _glhckImagePostProcess(glhckTexture *texture, _glhckImagePostProcessStruct *data);

#if !GLHCK_IMPORT_DYNAMIC

#if GLHCK_IMPORT_PNG
/* PNG */
int _glhckImportPNG(glhckTexture *texture, const char *file, unsigned int flags);
int _glhckFormatPNG(const char *file);
#endif

#if GLHCK_IMPORT_JPEG
/* JPEG */
int _glhckImportJPEG(glhckTexture *texture, const char *file, unsigned int flags);
int _glhckFormatJPEG(const char *file);
#endif

#if GLHCK_IMPORT_TGA
/* TGA */
int _glhckImportTGA(glhckTexture *texture, const char *file, unsigned int flags);
int _glhckFormatTGA(const char *file);
#endif

#if GLHCK_IMPORT_BMP
/* BMP */
int _glhckImportBMP(glhckTexture *texture, const char *file, unsigned int flags);
int _glhckFormatBMP(const char *file);
#endif

#endif /* GLHCK_IMPORT_DYNAMIC */

/* Common helper functions.'
 * don't add here if it does not apply to other formats or importers */

/* Try to figure out real texture path
 * 1. Texture name/filename/path
 * 2. Model path
 *
 * free the returned string
 */
char* _glhckImportTexturePath(const char *texture_path, const char *asset_path);

/* check that image dimensions are OK */
int _glhckIsValidImageDimension(unsigned long long w,  unsigned long long h);

/* \brief returns tristripped indecies from triangle indecies */
glhckImportIndexData* _glhckTriStrip(const glhckImportIndexData *indices, unsigned int memb, unsigned int *outMemb);

#endif /* __glhck_import_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
