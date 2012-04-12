#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <GL/gl.h>

#include "GL/glfw3.h"
#include "GL/glhck.h"

#define LENGTH(X) (sizeof X / sizeof X[0])
#define BUFFER_OFFSET(i) ((char *)NULL+(i))

int RUNNING = 0;
int close_callback(GLFWwindow window)
{
   RUNNING = 0;
   return 1;
}

int main(int argc, char **argv)
{
   GLFWwindow window;
   glhckTexture *texture;
   unsigned int vbo, tex;

   if (!glfwInit())
      return EXIT_FAILURE;

   if (!(window = glfwOpenWindow(800, 480, GLFW_WINDOWED, "display test", NULL)))
      return EXIT_FAILURE;

   if (!glhckInit(argc, argv))
      return EXIT_FAILURE;

   if (!glhckCreateDisplay(800, 480, 0))
      return EXIT_FAILURE;

   RUNNING = 1;

   short coords[]   = { 1, 1,
                        0, 1,
                        1, 0,
                        0, 0 };

   short vertices[] = { 1, 1, 0,
                       -1, 1, 0,
                        1,-1, 0,
                       -1,-1, 0 };

   unsigned char image[256 * 256 * 3];

   int i = 0;
   srand(time(0));
   for (i = 0; i != 256 * 256 * 3; ++i)
      image[i] = i%rand()%255;

   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, LENGTH(vertices) * sizeof(short) + LENGTH(coords) * sizeof(short), NULL, GL_DYNAMIC_DRAW);
   glBufferSubData(GL_ARRAY_BUFFER, 0, LENGTH(vertices) * sizeof(short), &vertices[0]);
   glBufferSubData(GL_ARRAY_BUFFER, LENGTH(vertices) * sizeof(short), LENGTH(coords) * sizeof(short), &coords[0]);

   glEnable(GL_TEXTURE_2D);

#if 0
   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
#else
   if (!(texture = glhckTextureNew("test.png",
               GLHCK_TEXTURE_DEFAULTS)))
      return EXIT_FAILURE;
   glhckTextureBind(texture);
#endif

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glfwSetWindowCloseCallback(close_callback);
   while (RUNNING && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {
      glfwPollEvents();
      glVertexPointer(3, GL_SHORT, 0, &vertices[0]);
      glTexCoordPointer(2, GL_SHORT, 0, &coords[0]);
      // glVertexPointer(3, GL_SHORT, 0, BUFFER_OFFSET(0));
      // glTexCoordPointer(2, GL_SHORT, 0, BUFFER_OFFSET(LENGTH(vertices) * sizeof(short)));
      glDrawArrays(GL_TRIANGLE_STRIP, 0, LENGTH(vertices)/3);

      /* Actual swap and clear */
      glfwSwapBuffers();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glDeleteBuffers(1, &vbo);
   glDeleteTextures(1, &tex);

   glhckTerminate();
   glfwTerminate();
   return EXIT_SUCCESS;
}
