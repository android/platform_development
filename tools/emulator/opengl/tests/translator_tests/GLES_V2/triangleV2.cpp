#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//#define GL_API
//#define GL_APIENTRY

#undef ANDROID
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#ifdef __APPLE__
extern "C" void * createGLView(void *nsWindowPtr, int x, int y, int width, int height);
#endif

#undef HAVE_MALLOC_H
#include <SDL.h>
#include <SDL_syswm.h>


#define WINDOW_WIDTH    500
#define WINDOW_HEIGHT   500

#define TEX_WIDTH 256
#define TEX_HEIGHT 256


#define F_to_X(d) ((d) > 32767.65535 ? 32767 * 65536 + 65535 :  \
               (d) < -32768.65535 ? -32768 * 65536 + 65535 : \
               ((GLfixed) ((d) * 65536)))
#define X_to_F(x)  ((float)(x))/65536.0f

//#define __FIXED__

static EGLint const attribute_list[] = {
    EGL_RED_SIZE, 1,
    EGL_GREEN_SIZE, 1,
    EGL_BLUE_SIZE, 1,
    EGL_NONE
};

unsigned char *genTexture(int width, int height, int comp)
{
    unsigned char *img = new unsigned char[width * height * comp];
    unsigned char *ptr = img;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char col = ((i / 8 + j / 8) % 2) * 255 ;
            for (int c = 0; c < comp; c++) {
                *ptr = col; ptr++;
            }
        }
    }
    return img;
}

unsigned char *genRedTexture(int width, int height, int comp)
{
    unsigned char *img = new unsigned char[width * height * comp];
        memset(img,0,width*height*comp);
    unsigned char *ptr = img;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char col = ((i / 8 + j / 8) % 2) * 255 ;
                        *ptr = col;
                         ptr+=comp;
        }
    }
    return img;
}


void usage(const char *progname)
{
    fprintf(stderr, "usage: %s [-n <nframes> -i -h]\n", progname);
    fprintf(stderr, "\t-h: this message\n");
    fprintf(stderr, "\t-i: immidate mode\n");
    fprintf(stderr, "\t-n nframes: generate nframes\n");
    fprintf(stderr, "\t-e: use index arrays\n");
    fprintf(stderr, "\t-t: use texture\n");
    fprintf(stderr, "\t-f: use fixed points\n");
    fprintf(stderr, "\t-p: use point size OES extention\n");
}



GLuint LoadShader(GLenum type,const char *shaderSrc)
{
   GLuint shader;
   GLint compiled;
   // Create the shader object
    shader = glCreateShader(type);
   if(shader == 0)
       return 0;
   // Load the shader source
   glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
   glCompileShader(shader);
    // Check the compile status
   glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
   if(!compiled)
   {
       GLint infoLen = 0;
       glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
       if(infoLen > 1)
       {
          char* infoLog = (char*)malloc(sizeof(char) * infoLen);
          glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
          printf("Error compiling shader:\n%s\n", infoLog);
          free(infoLog);
       }
       glDeleteShader(shader);
       return 0;
   }
   return shader;
}
///
// Initialize the shader and program object
//
int Init()
{
   char vShaderStr[] =
       "attribute vec4 vPosition;   \n"
       "void main()                 \n"
       "{                           \n"
       "   gl_Position = vPosition; \n"
       "}                           \n";
   char fShaderStr[] =
       "precision mediump float;                   \n"
       "void main()                                \n"
       "{                                          \n"
#ifndef __FIXED__
       " gl_FragColor = vec4(0.2, 0.5, 0.1, 1.0); \n"
#else
       " gl_FragColor = vec4(0.4, 0.3, 0.7, 1.0); \n"
#endif
       "}                                          \n";
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;
  // Load the vertex/fragment shaders
  vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
  fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);
  // Create the program object
  programObject = glCreateProgram();
  if(programObject == 0)
     return -1;
  glAttachShader(programObject, vertexShader);
  glAttachShader(programObject, fragmentShader);
  // Bind vPosition to attribute 0
  glBindAttribLocation(programObject, 0, "vPosition");
  // Link the program
  glLinkProgram(programObject);
  // Check the link status
  glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
  if(!linked)
  {
     GLint infoLen = 0;
     glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
     if(infoLen > 1)
     {
        char* infoLog = (char*)malloc(sizeof(char) * infoLen);
        glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
        printf("Error linking program:\n%s\n", infoLog);
        free(infoLog);
     }
     glDeleteProgram(programObject);
     return -1;
  }
  // Store the program object
#ifndef __FIXED__
  glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
#else
  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
#endif
  return programObject;
}


///
// Draw a triangle using the shader pair created in Init()
//
void Draw(EGLDisplay display,EGLSurface surface,int width,int height,GLuint program)
{
#ifndef __FIXED__
   GLfloat vVertices[] = {0.0f, 0.5f, 0.0f,
                           -0.5f, -0.5f, 0.0f,
                           0.5f, -0.5f, 0.0f};
#else

   GLfixed vVertices[] = {F_to_X(0.0f), F_to_X(0.5f),F_to_X(0.0f),
                           F_to_X(-0.5f),F_to_X(-0.5f), F_to_X(0.0f),
                           F_to_X(0.5f),F_to_X(-0.5f),F_to_X(0.0f)};
#endif    
   
    // Set the viewport
   glViewport(0, 0,width,height);
    // Clear the color buffer
   glClear(GL_COLOR_BUFFER_BIT);
    // Use the program object
   glUseProgram(program);
   // Load the vertex data
#ifndef __FIXED__
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
#else
   glVertexAttribPointer(0, 3, GL_FIXED, GL_FALSE, 0, vVertices);
#endif
   glEnableVertexAttribArray(0);
   glDrawArrays(GL_TRIANGLES, 0, 3);
   eglSwapBuffers(display,surface);
}

#ifdef _WIN32
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char **argv)
#endif
{
    GLuint  ui32Vbo = 0; // Vertex buffer object handle
    GLuint  ui32IndexVbo;
    GLuint  ui32Texture;

    int nframes = 100;
    bool immidateMode     = false;
    bool useIndices       = false;
    bool useTexture       = true;
    bool useCompTexture   = false;
    bool useFixed         = true;
    bool usePoints        = false;
    bool useCopy          = false;
    bool useSubCopy       = false;

    int c;
    extern char *optarg;

    #ifdef _WIN32
        HWND   windowId = NULL;
    #elif __linux__
        Window windowId = NULL;
    #elif __APPLE__
        void* windowId  = NULL;
    #endif

        //      // Inialize SDL window
        //
        if (SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_VIDEO)) {
            fprintf(stderr,"SDL init failed: %s\n", SDL_GetError());
            return -1;
        }

        SDL_Surface *surface = SDL_SetVideoMode(WINDOW_WIDTH,WINDOW_HEIGHT, 32, SDL_HWSURFACE);
        if (surface == NULL) {
            fprintf(stderr,"Failed to set video mode: %s\n", SDL_GetError());
            return -1;
        }

        SDL_SysWMinfo  wminfo;
        memset(&wminfo, 0, sizeof(wminfo));
        SDL_GetWMInfo(&wminfo);
    #ifdef _WIN32
        windowId = wminfo.window;
    #elif __linux__
        windowId = wminfo.info.x11.window;
    #elif __APPLE__
        windowId = createGLView(wminfo.nsWindowPtr,0,0,WINDOW_WIDTH,WINDOW_HEIGHT);
    #endif

        int major,minor,num_config;
        int attrib_list[] ={    
                                EGL_CONTEXT_CLIENT_VERSION, 2,
                                EGL_NONE
                           };
        EGLConfig configs[150];
        EGLSurface egl_surface;
        EGLContext ctx;
        EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(d,&major,&minor);
        printf("DISPLAY == %p major =%d minor = %d\n",d,major,minor);
        eglChooseConfig(d, attribute_list, configs, 150, &num_config);
        printf("config returned %d\n",num_config);
        egl_surface = eglCreateWindowSurface(d,configs[0],windowId,NULL);
        ctx = eglCreateContext(d,configs[0],EGL_NO_CONTEXT,attrib_list);
        printf("SURFACE == %p CONTEXT == %p\n",egl_surface,ctx);
        if(eglMakeCurrent(d,egl_surface,egl_surface,ctx)!= EGL_TRUE){
            printf("make current failed\n");
            return false;
        }
        printf("after make current\n");

        GLenum err = glGetError();
        if(err != GL_NO_ERROR) {
        printf("error before drawing ->>> %d  \n",err);
        } else {
        printf("no error before drawing\n");
        }

        int program = Init();
        if(program  < 0){
            printf("failed init shaders\n");
            return false;
        }

        Draw(d,egl_surface,WINDOW_WIDTH,WINDOW_HEIGHT,program);

                err = glGetError();
                if(err != GL_NO_ERROR)
            printf("error ->>> %d  \n",err);
        eglDestroySurface(d,egl_surface);
        eglDestroyContext(d,ctx);

// Just wait until the window is closed
        SDL_Event ev;
        while( SDL_WaitEvent(&ev) ) {
            if (ev.type == SDL_QUIT) {
                break;
            }
        }
    return 0;
}


