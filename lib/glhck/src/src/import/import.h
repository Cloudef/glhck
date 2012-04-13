#ifndef __import_h__
#define __import_h__

/* Wraps around the other imports by figuring out the header
 * 1. pointer to sceneobject
 * 2. filename
 * 3. 1 = import animation data, 0 = don't import
 */
int _glhckImportModel(_glhckObject *object, const char *file, int animated);

#if GLHCK_IMPORT_OPENCTM
/* OpenCTM http://openctm.sourceforge.net/ */
int _glhckImportOpenCTM(_glhckObject *object, const char *file, int animated);
#endif /* WITH_OCTM */

#if GLHCK_IMPORT_PMD
/* MikuMikuDance PMD */
int _glhckImportPmd(_glhckObject *object, const char *file, int animated );
#endif /* WITH_PMD */

#if GLHCK_IMPORT_ASSIMP
/* Assimp wrapper */
int _glhckImportAssimp(_glhckObject *object, const char *file, int animated);
#endif /* WITH_ASSIMP */

/* Not using own importers anymore, SOIL ftw :)
 * 1. pointer to texture object
 * 2. filename
 */
int _glhckImportImage(_glhckTexture *object, const char *file);

/* Common helper functions.'
 * don't add here if it does not apply to other formats or importers */

/* Try to figure out real texture path
 * 1. Texture name/filename/path
 * 2. Model path
 *
 * free the returned string
 */
char* _glhckImportTexturePath(const char *texture_path, const char *asset_path);

#endif /* __import_h__ */
