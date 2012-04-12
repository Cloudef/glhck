#ifndef MMD_IMPORT_H
#define MMD_IMPORT_H

#include <stdint.h>

#define MMD_NAME_LEN      20
#define MMD_BONE_NAME_LEN 50
#define MMD_COMMENT_LEN   256
#define MMD_FILE_PATH_LEN 20

#ifdef __cplusplus
extern "C" {
#endif

/* header */
typedef struct mmd_header
{
   char  name[MMD_NAME_LEN];
   char  comment[MMD_COMMENT_LEN];
   float version;
} mmd_header;

/* texture */
typedef struct mmd_texture
{
   char file[MMD_FILE_PATH_LEN];
} mmd_texture;

/* bone */
typedef struct mmd_bone
{
   /* bone name */
   char name[MMD_NAME_LEN];

   /* type */
   uint8_t type;

   /* indices */
   unsigned short parent_bone_index;
   unsigned short tail_pos_bone_index;
   unsigned short ik_parend_bone_index;

   /* head position */
   float head_pos[3];
} mmd_bone;

/* IK */
typedef struct mmd_ik
{
   /* chain length */
   uint8_t  chain_length;

   /* indices */
   unsigned short bone_index;
   unsigned short target_bone_index;
   unsigned short iterations;

   /* cotrol weight */
   float cotrol_weight;

   /* child bone indices */
   unsigned short *child_bone_index;
} mmd_ik;

/* bone name */
typedef struct mmd_bone_name
{
   /* bone name */
   char name[MMD_BONE_NAME_LEN];
} mmd_bone_name;

/* Skin vertices */
typedef struct mmd_skin_vertices
{
   /* index */
   uint32_t index;

   /* translation */
   float translation[3];
} mmd_skin_vertices;

/* Skin */
typedef struct mmd_skin
{
   /* skin name */
   char name[MMD_NAME_LEN];

   /* vertices on this skin */
   uint32_t  num_vertices;

   /* skin type */
   uint8_t type;

   /* vertices */
   mmd_skin_vertices *vertices;
} mmd_skin;

/* all data */
typedef struct mmd_data
{
   /* count */
   uint32_t   num_vertices;
   uint32_t   num_indices;
   uint32_t   num_materials;
   unsigned short num_bones;
   unsigned short num_ik;
   unsigned short num_skins;
   uint8_t        num_skin_displays;
   uint8_t        num_bone_names;

   /* vertex */
   float          *vertices;
   float          *normals;
   float          *coords;

   /* index */
   unsigned short *indices;

   /* bone */
   uint32_t       *bone_indices;
   uint8_t        *bone_weight;
   uint8_t        *edge_flag;

   /* bone && ik arrays */
   mmd_bone       *bones;
   mmd_ik         *ik;
   mmd_bone_name  *bone_name;

   /* skin array */
   mmd_skin       *skin;
   uint32_t       *skin_display;

   /* material */
   float          *diffuse;
   float          *specular;
   float          *ambient;
   float          *alpha;
   float          *power;
   uint8_t        *toon;
   uint8_t        *edge;
   uint32_t       *face;
   mmd_texture    *texture;

} mmd_data;

/* allocate new mmd_data structure
 * which holds all vertices,
 * indices, materials and etc. */
mmd_data* mmd_new(void);

/* frees the MMD structure */
void mmd_free(mmd_data *mmd);

/* 1 - read header from MMD file */
int mmd_read_header(FILE *f, mmd_header *header);

/* 2 - read vertex data from MMD file */
int mmd_read_vertex_data(FILE *f, mmd_data *mmd);

/* 3 - read index data from MMD file */
int mmd_read_index_data(FILE *f, mmd_data *mmd);

/* 4 - read material data from MMD file */
int mmd_read_material_data(FILE *f, mmd_data *mmd);

/* 5 - read bone data from MMD file */
int mmd_read_bone_data(FILE *f, mmd_data *mmd);

/* 6 - read IK data from MMD file */
int mmd_read_ik_data(FILE *f, mmd_data *mmd);

/* 7 - read Skin data from MMD file */
int mmd_read_skin_data(FILE *f, mmd_data *mmd);

/* 8 - read Skin display data from MMD file */
int mmd_read_skin_display_data(FILE *f, mmd_data *mmd);

/* 9 - read bone name data from MMD file */
int mmd_read_bone_name_data(FILE *f, mmd_data *mmd);

/* EXAMPLE:
 *
 * FILE *f;
 * mmd_header header;
 * mmd_data   *mmd;
 *
 * uint32_t i;
 *
 * if (!(f = fopen("HatsuneMiku.pmd", "rb")))
 *    exit(EXIT_FAILURE);
 *
 * if (mmd_read_header(f, &header) != 0)
 *    exit(EXIT_FAILURE);
 *
 * // SJIS encoded, so probably garbage
 * // You might want to decode them to utf-8 using eg. iconv
 * puts(header.name);
 * puts(header.comment);
 *
 * if (!(mmd = mmd_new()))
 *    exit(EXIT_FAILURE);
 *
 * if (mmd_read_vertex_data(f, mmd) != 0)
 *    exit( EXIT_FAILURE );
 *
 * if (mmd_read_index_data(f, mmd) != 0)
 *    exit( EXIT_FAILURE );
 *
 * if (mmd_read_material_data(f, mmd) != 0)
 *    exit( EXIT_FAILURE );
 *
 * fclose(f);
 *
 * // there are many ways you could handle storing or rendering MMD object
 * // which has many materials and only one set of texture coordinates.
 * //
 * // 1. create one VBO for whole object and one IBO for each material
 * // 2. create atlas texture and then batch everything into one VBO and IBO
 * // 3. split into small VBO's and IBO's
 * // 4. render using shaders and do material reset
 * //
 * // I haven't yet tested rendering using shaders, but blender can do this.
 * // In my tests the fastest results can be archieved with option 1,
 * // however in real use scenario, I would say option 2 is going to be faster
 * // when you have lots of stuff in screen.
 *
 * // Atlas and texture packing is quite big chunk of code so
 * // I'm not gonna make example out of that.
 * // Option 1 approach below :
 *
 * // Pack single big VBO here
 * for(i = 0; i != mmd->num_vertices; ++i) {
 *    // handle vertices,
 *    // coords and normals here
 *
 *    // depending implentation
 *    // you can also handle indices
 *    // on num_indices loop
 *
 *    // mmd->vertices [i*3];
 *    // mmd->normals  [i*3];
 *    // mmd->coords   [i*2];
 * }
 *
 * // Split into small IBOs here
 * for(i = 0; i != mmd->num_materials; ++i) {
 *    // this is texture filename
 *    // mmd->texture[i].file;
 *
 *    // split each material
 *    // into child object
 *    // and give it its own indices
 *    // but share the same VBO
 *
 *    // you could probably
 *    // avoid splitting
 *    // with using some material reset
 *    // technique and shaders
 *
 *    // it's not that costly to share
 *    // one VBO and have multiple IBO's
 *    // tho
 *
 *    num_faces = mmd->face[i];
 *    for(i2 = start; i2 != start + num_faces; ++i2) {
 *       // loop for current material's
 *       // indices
 *       // mmd->indices[i2];
 *    }
 *    start += num_faces;
 * }
 *
 * mmd_free(mmd);
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* MMD_IMPORT_H */

/* vim: set ts=8 sw=3 tw=0 :*/
