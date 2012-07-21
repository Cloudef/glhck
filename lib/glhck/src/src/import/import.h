#ifndef __import_h__
#define __import_h__

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
int _glhckImportModel(_glhckObject *object, const char *file, int animated);

#if !GLHCK_IMPORT_DYNAMIC
/* OpenCTM http://openctm.sourceforge.net/ */
//int _glhckImportOpenCTM(_glhckObject *object, const char *file, int animated);
//int _glhckFormatOpenCTM(const char *file);

/* MikuMikuDance PMD */
int _glhckImportPMD(_glhckObject *object, const char *file, int animated);
int _glhckFormatPMD(const char *file);

/* Assimp wrapper */
//int _glhckImportAssimp(_glhckObject *object, const char *file, int animated);
//int _glhckFormatAssimp(const char *file);
#endif

/* Wraps around the other imports by figuring out the header
 * 1. pointer to texture object
 * 2. filename
 */
int _glhckImportImage(_glhckTexture *texture, const char *file, unsigned int flags);

#if !GLHCK_IMPORT_DYNAMIC
/* TGA */
int _glhckImportTGA(_glhckTexture *texture, const char *file, unsigned int flags);
int _glhckFormatTGA(const char *file);
#endif

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
unsigned int* _glhckTriStrip(unsigned int *indices, size_t num_indices, size_t *out_num_indices);

#endif /* __import_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
