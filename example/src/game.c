#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if GLHCK_USE_GLES1
#  define GLFW_INCLUDE_ES1
#elif GLHCK_USE_GLES2
#  define GLFW_INCLUDE_ES2
#endif
#include "GLFW/glfw3.h"
#include "glhck/glhck.h"

typedef enum GameFont {
   FONT_DEFAULT,
   FONT_LAST
} GameFont;

static float FONT_SIZE[FONT_LAST] = { 12 };
static const char *WINDOW_TITLE = "THE GAME";

typedef struct GameBullet {
   kmAABB aabb;
   kmVec3 position;
   struct GameBullet *next;
   float angle, liveTime;
} GameBullet;

typedef struct GameActor {
   kmAABB aabb;
   glhckObject *object, *muzzle;
   glhckBone *muzzleBone;
   glhckAnimator *animator;
   kmVec3 position, rotation;
   struct GameActor *next;
   float shootTimer;
   unsigned char flags;
} GameActor;

typedef struct GameCamera {
   glhckCamera *handle;
   glhckObject *object;
   kmVec3 position, rotation;
} GameCamera;

typedef struct GameText {
   glhckText *handle;
   unsigned int fonts[FONT_LAST];
} GameText;

typedef struct GameTime {
   float now, last, delta, duration;
   unsigned int frame, fps, update;
} GameTime;

typedef struct GameWindow {
   char *title;
   glhckContext *context;
   GLFWwindow *handle;
   glhckCollisionWorld *collisionWorld;
   glhckObject *world, *bulletObject;
   GameBullet *bullet;
   GameActor *actor, *player;
   GameCamera *camera;
   GameText *text;
   GameTime time;
   int width, height, running;
} GameWindow;

static void glfwErrorCallback(int error, const char *msg)
{
   (void)error;
   printf("-!- GLFW: %s\n", msg);
}

static void glfwCloseCallback(GLFWwindow *handle)
{
   GameWindow *window = glfwGetWindowUserPointer(handle);
   window->running = 0;
}

static void glfwSizeCallback(GLFWwindow *handle, int width, int height)
{
   GameWindow *window = glfwGetWindowUserPointer(handle);
   window->width = width; window->height = height;
   glfwMakeContextCurrent(window->handle);
   glhckContextSet(window->context);
   glhckDisplayResize(width, height);
}

static void glfwKeyCallback(GLFWwindow *handle, int key, int scancode, int action, int mods)
{
   (void)scancode; (void)action; (void)mods;
   GameWindow *window = glfwGetWindowUserPointer(handle);

   switch (key) {
      case GLFW_KEY_ESCAPE:
         window->running = 0;
         break;
      default:break;
   }
}

static float fIntrp(float f, float o, float d)
{
   const float inv = 1.0f-d;
   return f*inv + o*d;
}

static float fModIntrp(float f, float o, float d, float m)
{
   if (fabsf(o-f) > m*0.5f) {
      if (o > f) f += m;
      else o += m;
   }
   return fmodf(fIntrp(f, o, d), m);
}

static inline kmVec3* kmVec3Intrp(kmVec3* pOut, const kmVec3* pIn, const kmVec3* other, float d)
{
   const float inv = 1.0f - d;
   pOut->x = pIn->x*inv + other->x*d;
   pOut->y = pIn->y*inv + other->y*d;
   pOut->z = pIn->z*inv + other->z*d;
   return pOut;
}

static inline kmVec3* kmVec3ModIntrp(kmVec3* pOut, const kmVec3* pIn, const kmVec3* other, float d, float m)
{
   pOut->x = fModIntrp(pIn->x, other->x, d, m);
   pOut->y = fModIntrp(pIn->y, other->y, d, m);
   pOut->z = fModIntrp(pIn->z, other->z, d, m);
   return pOut;
}

static void gameCameraFree(GameCamera *camera)
{
   if (camera->handle) glhckCameraFree(camera->handle);
   free(camera);
}

static void gameBulletFree(GameWindow *window, GameBullet *bullet)
{
   GameBullet *b;

   for (b = window->bullet; b && b->next != bullet; b = b->next);
   if (b) b->next = NULL; else if (window->bullet == bullet) window->bullet = bullet->next;
   free(bullet);
}

static GameBullet* gameBulletNew(GameWindow *window)
{
   GameBullet *bullet, *b;

   if (!(bullet = calloc(1, sizeof(GameBullet))))
      goto fail;

   bullet->liveTime = 1.0f;
   for (b = window->bullet; b && b->next; b = b->next);
   if (!b) window->bullet = bullet; else b->next = bullet;
   return bullet;

fail:
   printf("-!- bullet creation failed\n");
   if (bullet) gameBulletFree(window, bullet);
   return NULL;
}

static void gameActorFree(GameWindow *window, GameActor *actor)
{
   GameActor *a;

   for (a = window->actor; a && a->next != actor; a = a->next);
   if (a) a->next = NULL; else if (window->actor == actor) window->actor = actor->next;
   if (actor->object) glhckObjectFree(actor->object);
   if (actor->muzzle) glhckObjectFree(actor->muzzle);
   if (actor->animator) glhckAnimatorFree(actor->animator);
   free(actor);
}

static GameActor* gameActorNew(GameWindow *window)
{
   GameActor *actor, *a;
   glhckBone **bones;
   glhckAnimation **animations;
   unsigned int numBones, numAnimations;

   glhckImportModelParameters animatedParams;
   memcpy(&animatedParams, glhckImportDefaultModelParameters(), sizeof(glhckImportModelParameters));
   animatedParams.animated = 1;

   if (!(actor = calloc(1, sizeof(GameActor))))
      goto fail;

   if (!(actor->object = glhckModelNew("example/media/area/test.glhckm", 5.0f, &animatedParams)))
      goto fail;

   if (!(actor->muzzleBone = glhckObjectGetBone(actor->object, "Armature_Bone.002_R.006")))
      goto fail;

   if (!(actor->muzzle = glhckModelNew("example/media/area/muzzle.glhckm", 1.0f, NULL)))
      goto fail;

   if (window->player) {
      char r = (size_t)actor % 255;
      char g = (size_t)actor->object % 255;
      char b = (size_t)actor->muzzle % 255;
      glhckMaterialDiffuseb(glhckObjectGetMaterial(glhckObjectChildren(actor->object, NULL)[0]), r, g, b, 255);
   }

   glhckObjectCull(actor->muzzle, 0);
   glhckObjectScalef(actor->muzzle, 0.0f, 0.0f, 0.0f);
   glhckObjectAddChild(actor->object, actor->muzzle);

   glhckCollisionWorldAddAABBRef(window->collisionWorld, &actor->aabb, actor->object);

   actor->position.x = rand() % 30 - 15;
   actor->position.z = rand() % 30 - 15;
   actor->position.y = 100;

   unsigned int i, numChilds;
   glhckObject **childs = glhckObjectChildren(actor->muzzle, &numChilds);
   for (i = 0; i < numChilds; ++i)
      glhckMaterialBlendFunc(glhckObjectGetMaterial(childs[i]), GLHCK_ONE, GLHCK_ONE);

   if ((bones = glhckObjectBones(actor->object, &numBones)) &&
       (animations = glhckObjectAnimations(actor->object, &numAnimations))) {

      if (!(actor->animator = glhckAnimatorNew()))
         goto fail;

      glhckAnimatorAnimation(actor->animator, animations[0]);
      glhckAnimatorInsertBones(actor->animator, bones, numBones);
   }

   for (a = window->actor; a && a->next; a = a->next);
   if (!a) window->actor = actor; else a->next = actor;
   return actor;

fail:
   printf("-!- actor creation failed\n");
   if (actor) gameActorFree(window, actor);
   return NULL;
}

static GameCamera* gameCameraNew(void)
{
   GameCamera *camera;

   if (!(camera = calloc(1, sizeof(GameCamera))))
      goto fail;

   if (!(camera->handle = glhckCameraNew()))
      goto fail;

   camera->object = glhckCameraGetObject(camera->handle);
   glhckCameraRange(camera->handle, 5.0f, 300.0f);
   glhckCameraFov(camera->handle, 42.0f);
   return camera;

fail:
   printf("-!- camera creation failed\n");
   if (camera) gameCameraFree(camera);
   return NULL;
}

static void gameTextFree(GameText *text)
{
   if (text->handle) glhckTextFree(text->handle);
   free(text);
}

static GameText* gameTextNew(void)
{
   GameText *text;

   if (!(text = calloc(1, sizeof(GameText))))
      goto fail;

   if (!(text->handle = glhckTextNew(256, 256)))
      goto fail;

   text->fonts[FONT_DEFAULT] = glhckTextFontNewKakwafont(text->handle, NULL);
   return text;

fail:
   printf("-!- text creation failed\n");
   if (text) gameTextFree(text);
   return NULL;
}

static void gameWindowFree(GameWindow *window)
{
   while (window->bullet) gameBulletFree(window, window->bullet);
   while (window->actor) gameActorFree(window, window->actor);
   if (window->collisionWorld) glhckCollisionWorldFree(window->collisionWorld);
   if (window->bulletObject) glhckObjectFree(window->bulletObject);
   if (window->world) glhckObjectFree(window->world);
   if (window->camera) gameCameraFree(window->camera);
   if (window->text) gameTextFree(window->text);

   if (window->context) {
      glhckContextSet(window->context);
      glhckContextTerminate();
   }

   if (window->handle) {
      glfwHideWindow(window->handle);
      glfwDestroyWindow(window->handle);
      window->handle = NULL;
   }

   if (window->title) free(window->title);
   free(window);
}

static GameWindow* gameWindowNew(int argc, char **argv)
{
   GameWindow *window;

   if (!(window = calloc(1, sizeof(GameWindow))))
      goto fail;

   window->width = 800; window->height = 480;
   if (!(window->handle = glfwCreateWindow(window->width, window->height, WINDOW_TITLE, NULL, NULL)))
      goto fail;

   if (!(window->context = glhckContextCreate(argc, argv)))
      goto fail;

   glfwMakeContextCurrent(window->handle);
   glhckContextSet(window->context);

   if (!glhckDisplayCreate(window->width, window->height, GLHCK_RENDER_AUTO))
      goto fail;

   if (!(window->camera = gameCameraNew()))
      goto fail;

   if (!(window->text = gameTextNew()))
      goto fail;

   if (!(window->collisionWorld = glhckCollisionWorldNew(NULL)))
      goto fail;

   if (!(window->bulletObject = glhckCubeNew(0.2f)))
      goto fail;

   if (!(window->world = glhckModelNew("example/media/area/area.glhckm", 8.0f, NULL)))
      goto fail;

   if (!(window->player = gameActorNew(window)))
      goto fail;

   glhckObject **childs;
   unsigned int i, numChilds;
   childs = glhckObjectChildren(window->world, &numChilds);
   for (i = 0; i < numChilds; ++i)
      glhckCollisionWorldAddAABB(window->collisionWorld, glhckObjectGetAABB(childs[i]), childs[i]);

   glhckObject *obj = glhckModelNew("example/media/area/area.glhckm", 8.0f, NULL);
   glhckObjectAddChild(window->world, obj);
   glhckObjectMovef(obj, 76.8f, 0.0f, 0.0f);
   glhckObjectRotationf(obj, 0.0f, 90.0f, 0.0f);
   childs = glhckObjectChildren(obj, &numChilds);
   for (i = 0; i < numChilds; ++i)
      glhckCollisionWorldAddAABB(window->collisionWorld, glhckObjectGetAABB(childs[i]), childs[i]);

   obj = glhckModelNew("example/media/area/area.glhckm", 8.0f, NULL);
   glhckObjectAddChild(window->world, obj);
   glhckObjectMovef(obj, 0.0f, 0.0f, -76.8f);
   glhckObjectRotationf(obj, 0.0f, 270.0f, 0.0f);
   childs = glhckObjectChildren(obj, &numChilds);
   for (i = 0; i < numChilds; ++i)
      glhckCollisionWorldAddAABB(window->collisionWorld, glhckObjectGetAABB(childs[i]), childs[i]);

   obj = glhckModelNew("example/media/area/area.glhckm", 8.0f, NULL);
   glhckObjectAddChild(window->world, obj);
   glhckObjectMovef(obj, 76.8f, 0.0f, -76.8f);
   glhckObjectRotationf(obj, 0.0f, 180.0f, 0.0f);
   childs = glhckObjectChildren(obj, &numChilds);
   for (i = 0; i < numChilds; ++i)
      glhckCollisionWorldAddAABB(window->collisionWorld, glhckObjectGetAABB(childs[i]), childs[i]);

   for (i = 0; i < 0; ++i) gameActorNew(window);

   window->running = 1;
   glfwSetWindowUserPointer(window->handle, window);
   glfwSetWindowCloseCallback(window->handle, glfwCloseCallback);
   glfwSetWindowSizeCallback(window->handle, glfwSizeCallback);
   glfwSetKeyCallback(window->handle, glfwKeyCallback);
   return window;

fail:
   printf("-!- window creation failed\n");
   if (window) gameWindowFree(window);
   return NULL;
}

static void gameWindowUpdateTitle(GameWindow *window)
{
   char *title;
   float msec = (glfwGetTime()-window->time.now)*1000.0f;
   int perc = (msec*100)/(1000.0f/60);

   if (!(title = malloc(snprintf(NULL, 0, "%s [%dFPS] [%.2fms] [%d%%]", WINDOW_TITLE, window->time.fps, msec, perc)+1)))
      return;

   sprintf(title, "%s [%dFPS] [%.2fms] [%d%%]", WINDOW_TITLE, window->time.fps, msec, perc);
   glfwSetWindowTitle(window->handle, title);

   if (window->title) free(window->title);
   window->title = title;
}

static int gameTimeBegin(GameTime *time)
{
   time->last = time->now;
   time->now = glfwGetTime();
   time->delta = time->now - time->last;
   glhckRenderTime(time->now);
   return (time->delta < 1.0);
}

static int gameTimeEnd(GameTime *time)
{
   int changed = 0;

   time->duration += time->delta;
   time->frame++;

   if (time->update < time->now) {
      time->fps = (float)time->frame / time->duration;
      time->update = time->now + 1.0f;
      changed = 1;
   }

   return changed;
}

static void gameTextStash(GameText *text, GameFont font, float x, float y, const char *str)
{
   if (!str) return;
   glhckTextStash(text->handle, text->fonts[font], FONT_SIZE[font], x, y, str, NULL);
}

static GameBullet* gameBulletLogic(GameWindow *window, GameBullet *bullet)
{
   static const float spd = 80.0f;
   GameBullet *next = bullet->next;

   if (bullet->liveTime <= 0.0f) {
      gameBulletFree(window, bullet);
      return next;
   }

   kmVec3 lastPosition, velocity;
   kmVec3Assign(&lastPosition, &bullet->position);

   bullet->liveTime -= 1.0f * window->time.delta;
   bullet->position.x -= cos((bullet->angle + 90) * kmPIOver180) * spd * window->time.delta;
   bullet->position.z += sin((bullet->angle + 90) * kmPIOver180) * spd * window->time.delta;

   glhckCollisionInData colData;
   memset(&colData, 0, sizeof(colData));
   kmVec3Subtract(&velocity, &bullet->position, &lastPosition);
   colData.velocity = &velocity;
   colData.userData = bullet;

   bullet->aabb.min.x = bullet->position.x - 0.1;
   bullet->aabb.min.y = bullet->position.y - 0.1;
   bullet->aabb.min.z = bullet->position.z - 0.1;
   bullet->aabb.max.x = bullet->position.x + 0.1;
   bullet->aabb.max.y = bullet->position.y + 0.1;
   bullet->aabb.max.z = bullet->position.z + 0.1;
   if (glhckCollisionWorldCollideAABB(window->collisionWorld, &bullet->aabb, &colData))
      bullet->liveTime = 0.0f;

   return next;
}

static int gameActorTest(const glhckCollisionInData *data, const glhckCollisionPrimitive *collider)
{
   GameActor *actor = data->userData;
   return (actor->object != glhckCollisionPrimitiveGetUserData(collider));
}

static void gameActorResponse(const glhckCollisionOutData *collision)
{
   GameActor *actor = collision->userData;
   kmVec3Add(&actor->position, &actor->position, collision->pushVector);
}

static void gameActorLogic(GameWindow *window, GameActor *actor, GameActor *player, float *outSpd)
{
   float spd = 30.0f;
   char flags = 0;
   float rotation;
   kmVec3 lastPosition, velocity;

   enum {
      UP = 1 << 0,
      DN = 1 << 1,
      LT = 1 << 2,
      RT = 1 << 3,
      ST = 1 << 4
   };

   kmVec3Assign(&lastPosition, &actor->position);

   if (actor == player) {
      if (glfwGetKey(window->handle, GLFW_KEY_UP)    == GLFW_PRESS) flags |= UP;
      if (glfwGetKey(window->handle, GLFW_KEY_DOWN)  == GLFW_PRESS) flags |= DN;
      if (glfwGetKey(window->handle, GLFW_KEY_LEFT)  == GLFW_PRESS) flags |= LT;
      if (glfwGetKey(window->handle, GLFW_KEY_RIGHT) == GLFW_PRESS) flags |= RT;
      if (glfwGetKey(window->handle, GLFW_KEY_Q)     == GLFW_PRESS) flags |= ST;
   } else {
      int rand = (int)window->time.now + ((size_t)actor >> 3) - ((size_t)actor->next >> 4);
      if (actor->position.z < player->position.z-10) flags |= UP;
      if (actor->position.z > player->position.z+10) flags |= DN;
      if (actor->position.x < player->position.x-10) flags |= LT;
      if (actor->position.x > player->position.x+10) flags |= RT;
      if (rand % 8 == 0) flags |= ST;
      if (rand % 2 == 0) flags |= (1 << rand%4);
      if (!flags) flags = actor->flags;
   }

   rotation = actor->rotation.y;

   if (flags & ST && actor->shootTimer <= 0.0f) {
      GameBullet *bullet = gameBulletNew(window);
      glhckBoneGetPositionAbsoluteOnObject(actor->muzzleBone, actor->object, &bullet->position);
      bullet->angle = rotation;
      bullet->position.y += 1.5f;
      bullet->position.x -= cos((bullet->angle + 90) * kmPIOver180) * 3.0f;
      bullet->position.z += sin((bullet->angle + 90) * kmPIOver180) * 3.0f;
      actor->shootTimer = 1.0f;
   }
   if (actor->shootTimer > 0.0f) spd *= 0.5f;

   if (flags & UP) {
      actor->position.z += spd * window->time.delta;
      rotation = (flags&RT?-45:0) + (flags&LT?45:0);
   }
   if (flags & DN) {
      actor->position.z -= spd * window->time.delta;
      rotation = 180.0f + (flags&RT?45:0) + (flags&LT?-45:0);
   }
   if (flags & LT) {
      actor->position.x += spd * window->time.delta;
      rotation = 90.0f + (flags&UP?-45:0) + (flags&DN?45:0);
   }
   if (flags & RT) {
      actor->position.x -= spd * window->time.delta;
      rotation = 270.0f + (flags&UP?45:0) + (flags&DN?-45:0);
   }

   if (actor->shootTimer > 0.0f) {
      actor->shootTimer -= 10.0f * window->time.delta;
   }

   actor->flags = flags;
   actor->rotation.y = fModIntrp(actor->rotation.y, rotation, spd * window->time.delta, 360);

   if (actor == player && glfwGetKey(window->handle, GLFW_KEY_SPACE) == GLFW_PRESS)
      actor->position.y += 70.0f * window->time.delta;
   else
      actor->position.y -= 70.0f * window->time.delta;

   glhckCollisionInData colData;
   memset(&colData, 0, sizeof(colData));
   kmVec3Subtract(&velocity, &actor->position, &lastPosition);
   colData.velocity = &velocity;
   colData.test = gameActorTest;
   colData.response = gameActorResponse;
   colData.userData = actor;

#if 1
   actor->aabb.min.x = actor->position.x - 3;
   actor->aabb.min.y = actor->position.y - 6;
   actor->aabb.min.z = actor->position.z - 3;
   actor->aabb.max.x = actor->position.x + 3;
   actor->aabb.max.y = actor->position.y + 5;
   actor->aabb.max.z = actor->position.z + 3;
   glhckCollisionWorldCollideAABB(window->collisionWorld, &actor->aabb, &colData);
#endif

#if 0
   typedef struct kmSphere {
      kmVec3 point;
      kmScalar radius;
   } kmSphere;

   kmSphere sphere;
   sphere.point = actor->position;
   sphere.radius = 6;
   glhckCollisionWorldCollideSphere(window->collisionWorld, &sphere, &colData);
#endif

   if (outSpd) *outSpd = spd;
}

static void gameWindowLogic(GameWindow *window)
{
   kmVec3 intrp, target;
   GameActor *a;
   GameBullet *b;
   float spd;

   for (a = window->actor; a; a = a->next) {
      gameActorLogic(window, a, window->player, &spd);
      kmVec3Intrp(&intrp, glhckObjectGetPosition(a->object), &a->position, spd * window->time.delta);
      glhckObjectPosition(a->object, &intrp);
      kmVec3ModIntrp(&intrp, glhckObjectGetRotation(a->object), &a->rotation, spd * window->time.delta, 360);
      glhckObjectRotation(a->object, &intrp);
   }

   for (b = window->bullet; b; b = gameBulletLogic(window, b));

   kmVec3Assign(&target, glhckObjectGetPosition(window->player->object));
   kmVec3Assign(&window->camera->position, &target);
   window->camera->position.y += 40;
   window->camera->position.z -= 40;
   target.y += 10;

   glhckObjectPosition(window->camera->object, &window->camera->position);
   glhckObjectTarget(window->camera->object, &target);
   glhckCameraUpdate(window->camera->handle);
}

static void gameWindowRender(GameWindow *window)
{
   GameActor *a;
   GameBullet *b;

   glhckObjectRenderAll(window->world);

   for (a = window->actor; a; a = a->next) {
      glhckObjectScalef(a->muzzle, 0.0f, 0.0f, 0.0f);
      if (a->animator && a->flags && a->flags != 1<<4) {
         glhckAnimatorUpdate(a->animator, window->time.now);
         glhckAnimatorTransform(a->animator, a->object);
      }
      glhckObjectRenderAll(a->object);
   }

   for (b = window->bullet; b; b = b->next) {
      glhckObjectPosition(window->bulletObject, &b->position);
      glhckObjectRotationf(window->bulletObject, 0.0f, b->angle, 0.0f);
      glhckObjectRender(window->bulletObject);
   }

   for (a = window->actor; a; a = a->next) {
      if (a->shootTimer <= 0.0f) continue;
      kmVec3 bpos;
      glhckBoneGetPositionRelativeOnObject(a->muzzleBone, a->object, &bpos);
      bpos.y += 1.5f;
      bpos.z += 2.0f;
      glhckObjectPosition(a->muzzle, &bpos);
      glhckObjectScalef(a->muzzle, a->shootTimer/1.0f, 1.0f, a->shootTimer/1.0f);
      glhckObjectRenderAll(a->muzzle);
   }

   gameTextStash(window->text, FONT_DEFAULT, 0, FONT_SIZE[FONT_DEFAULT], window->title);
   glhckTextRender(window->text->handle);
   glhckTextClear(window->text->handle);

   glfwSwapBuffers(window->handle);
   glhckRenderClear(GLHCK_COLOR_BUFFER_BIT | GLHCK_DEPTH_BUFFER_BIT);
}

static int gameWindowRun(GameWindow *window)
{
   glhckContextSet(window->context);
   glfwMakeContextCurrent(window->handle);
   glfwSwapInterval(0);
   glEnable(GL_MULTISAMPLE);
   if (gameTimeBegin(&window->time)) {
      gameWindowLogic(window);
      gameWindowRender(window);
   }
   if (gameTimeEnd(&window->time)) gameWindowUpdateTitle(window);

   if (!window->running) {
      gameWindowFree(window);
      return 0;
   }

   return 1;
}

int main(int argc, char **argv)
{
   GameWindow **window = NULL;
   glhckCompileFeatures features;
   unsigned int i, windows = 1, running;

   glfwSetErrorCallback(glfwErrorCallback);
   if (!glfwInit()) goto fail;

   glhckGetCompileFeatures(&features);
   glfwDefaultWindowHints();
   glfwWindowHint(GLFW_SAMPLES, 4);
   if (features.render.glesv1 || features.render.glesv2) {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      glfwWindowHint(GLFW_DEPTH_BITS, 16);
   }
   if (features.render.glesv2) {
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
   }
   if (features.render.opengl) {
      glfwWindowHint(GLFW_DEPTH_BITS, 24);
   }

   if (argc > 1) {
      windows = strtol(argv[1], NULL, 10);
      if (windows < 1) windows = 1;
   }

   if (!(window = calloc(windows+1, sizeof(GameWindow*))))
      goto fail;

   for (i = 0; i < windows; ++i)
      if (!(window[i] = gameWindowNew(argc, argv)))
         goto fail;

   do {
      glfwPollEvents();
      for (i = 0, running = 0; i < windows; running += window[i]?1:0, ++i)
         if (window[i] && !gameWindowRun(window[i])) window[i] = NULL;
   } while (running);

   free(window);
   glfwTerminate();
   return EXIT_SUCCESS;

fail:
   if (window) {
      for (i = 0; i < windows; ++i)
         if (window[i]) gameWindowFree(window[i]);
      free(window);
   }
   glfwTerminate();
   return EXIT_FAILURE;
}

/* vim: set ts=8 sw=3 tw=0 :*/
