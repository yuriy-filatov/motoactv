#include <stdio.h>
#include "EGL/egl.h"
#include "GLES/gl.h"
#include <iostream>

#define WINDOW_DEFAULT_WIDTH    640
#define WINDOW_DEFAULT_HEIGHT   480

#define WINDOW_BPP              16

static int showGraphics();

static int sWindowWidth = WINDOW_DEFAULT_WIDTH;
static int sWindowHeight = WINDOW_DEFAULT_HEIGHT;
static EGLDisplay sEglDisplay = EGL_NO_DISPLAY;
static EGLContext sEglContext = EGL_NO_CONTEXT;
static EGLSurface sEglSurface = EGL_NO_SURFACE;

const char *egl_strerror(unsigned err)
{
    switch(err){
    case EGL_SUCCESS: return "SUCCESS";
    case EGL_NOT_INITIALIZED: return "NOT INITIALIZED";
    case EGL_BAD_ACCESS: return "BAD ACCESS";
    case EGL_BAD_ALLOC: return "BAD ALLOC";
    case EGL_BAD_ATTRIBUTE: return "BAD_ATTRIBUTE";
    case EGL_BAD_CONFIG: return "BAD CONFIG";
    case EGL_BAD_CONTEXT: return "BAD CONTEXT";
    case EGL_BAD_CURRENT_SURFACE: return "BAD CURRENT SURFACE";
    case EGL_BAD_DISPLAY: return "BAD DISPLAY";
    case EGL_BAD_MATCH: return "BAD MATCH";
    case EGL_BAD_NATIVE_PIXMAP: return "BAD NATIVE PIXMAP";
    case EGL_BAD_NATIVE_WINDOW: return "BAD NATIVE WINDOW";
    case EGL_BAD_PARAMETER: return "BAD PARAMETER";
    case EGL_BAD_SURFACE: return "BAD_SURFACE";
//    case EGL_CONTEXT_LOST: return "CONTEXT LOST";
    default: return "UNKNOWN";
    }
}

void egl_error(const char *name)
{
    unsigned err = eglGetError();
    if(err != EGL_SUCCESS) {
        fprintf(stderr,"%s(): egl error 0x%x (%s)\n",
                name, err, egl_strerror(err));
    }
}

static void checkGLErrors()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        fprintf(stderr, "GL Error: 0x%04x\n", (int)error);
}


static void checkEGLErrors()
{
    EGLint error = eglGetError();
    // GLESonGL seems to be returning 0 when there is no errors?
    if (error && error != EGL_SUCCESS)
        fprintf(stderr, "EGL Error: 0x%04x\n", (int)error);
}
static int initGraphics()
{
    EGLint s_configAttribs[] = {
         EGL_RED_SIZE,       5,
         EGL_GREEN_SIZE,     6,
         EGL_BLUE_SIZE,      5,
 #if 1
         EGL_DEPTH_SIZE,     16,
         EGL_STENCIL_SIZE,   0,
 #else
         EGL_ALPHA_SIZE,     EGL_DONT_CARE,
         EGL_DEPTH_SIZE,     EGL_DONT_CARE,
         EGL_STENCIL_SIZE,   EGL_DONT_CARE,
         EGL_SURFACE_TYPE,   EGL_DONT_CARE,
 #endif
         EGL_NONE
     };

     EGLint numConfigs = -1;
     EGLint majorVersion;
     EGLint minorVersion;
     EGLConfig config;
     EGLContext context;
     EGLSurface surface;

     EGLDisplay dpy;

     dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
     egl_error("eglGetDisplay");
     fprintf(stderr,"dpy = 0x%08x\n", (unsigned) dpy);

     eglInitialize(dpy, &majorVersion, &minorVersion);
     egl_error("eglInitialize");

     eglGetConfigs(dpy, NULL, 0, &numConfigs);
     egl_error("eglGetConfigs");
     fprintf(stderr,"num configs %d\n", numConfigs);

     eglChooseConfig(dpy, s_configAttribs, &config, 1, &numConfigs);
     egl_error("eglChooseConfig");

     surface = eglCreateWindowSurface(dpy, config,
             android_createDisplaySurface(), NULL);
     egl_error("eglMapWindowSurface");

     fprintf(stderr,"surface = %p\n", surface);

     context = eglCreateContext(dpy, config, NULL, NULL);
     egl_error("eglCreateContext");
     fprintf(stderr,"context = %p\n", context);

     eglMakeCurrent(dpy, surface, surface, context);
     egl_error("eglMakeCurrent");

     eglQuerySurface(dpy, surface, EGL_WIDTH, &sWindowWidth);
     eglQuerySurface(dpy, surface, EGL_HEIGHT, &sWindowHeight);

    sEglDisplay = dpy;
    sEglSurface = surface;
    sEglContext = context;

    return EGL_TRUE;
}

static void deinitGraphics()
{
    eglMakeCurrent(sEglDisplay, NULL, NULL, NULL);
    eglDestroyContext(sEglDisplay, sEglContext);
    eglDestroySurface(sEglDisplay, sEglSurface);
    eglTerminate(sEglDisplay);
}

int main(int argc, char *argv[])
{
    // not referenced:
    argc = argc;
    argv = argv;

    //std::cout << "Enter EGL test \n";
    if (!initGraphics())
    {
        fprintf(stderr, "Graphics initialization failed.\n");
        return -1;
    }

    showGraphics();
    deinitGraphics();

    //std::cout << "Exiting EGL test \n";
    return 0;
}

int showGraphics()
{
        const static GLfloat v[] = {
                0.25, 0.25, 0.0,
                0.75, 0.25, 0.0,
                0.25, 0.75, 0.0,
                0.75, 0.75, 0.0
        };

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrthof(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
        glVertexPointer(3, GL_FLOAT, 0, v);
        glEnableClientState(GL_VERTEX_ARRAY);

        /** display graphic **/

        glClearColor(0.0, 1.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glColor4f (1.0, 0.0, 0.0, 1.0);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glFlush();
        eglSwapBuffers(sEglDisplay, sEglSurface);  /* actually write to the RGB 16-bit file */

        return 0;

} 
