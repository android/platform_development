#include "EventTracker.h"
#include "TcpStream.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

namespace EventTracker
{

#define UT_RENDER_EVENT "UT_RENDER_EVENT"

static pthread_t s_evThread = NULL;
static Window s_evWin = (Window)0;
static sighandler_t s_prevSigIntHandler = NULL;

static void sigint_handler(int s)
{
    //cleanly exit
    if (s_evThread) {
        // cancel the event thread and wait for it to exit
        pthread_cancel(s_evThread);
        void *ret;
        pthread_join(s_evThread, &ret);
    }

    //
    // call previous signal handler or cleanly exit
    //
    if (s_prevSigIntHandler != SIG_IGN) {
        if (s_prevSigIntHandler != SIG_DFL) {
            (*s_prevSigIntHandler)(s);
        }
        else {
            exit(0);
        }
    }
}

void getReply(TcpStream *stream, bool echo = false)
{
    char reply[128];
    size_t len = 128;
    if( stream->read(reply, &len) && len > 0) {
        if (echo) write(1, reply, len);
    }
}

static void cleanup(void *arg)
{
   TcpStream *stream = (TcpStream *)arg;
   printf("EXITING event tracker\n");
   char *msg = (char *)stream->allocBuffer(6);
   memcpy(msg, "exit\r\n", 6);
   stream->commitBuffer(6);
   s_evThread = NULL;
}

static void *run(void *data)
{
   int portNum = (int)data;

   Display *dpy = XOpenDisplay(NULL);
   if (!dpy) {
       fprintf(stderr,"Could not open display\n");
       return NULL;
   }

   TcpStream stream;
   if (stream.connect("localhost", portNum) < 0) {
       fprintf(stderr,"Could not open port %d\n", portNum);
       XCloseDisplay(dpy);
       return NULL;
   }
   printf("Opened connection to localhost:%d for event injection\n", portNum);
   getReply(&stream);

   s_evWin = XCreateWindow(dpy, DefaultRootWindow(dpy),
                           0, 0, 1, 1, 0,
                           CopyFromParent,
                           InputOnly,
                           CopyFromParent,
                           0, NULL);
   XSync(dpy, False);

   Atom ut_render_atom = XInternAtom(dpy, UT_RENDER_EVENT, False);

   pthread_cleanup_push(cleanup, &stream);

   XEvent ev;
   while( 1 ) {
       XNextEvent(dpy, &ev);
 
       if (ev.xany.type == ClientMessage &&
           ev.xclient.message_type == ut_render_atom) {

           if (0 == ev.xclient.data.l[0]) {
               // We have asked to quit
               break;
           }

           printf("Adding window 0x%lx\n", ev.xclient.data.l[0]);
           XSelectInput(dpy, ev.xclient.data.l[0],
                        StructureNotifyMask |
                        KeyPressMask |
                        KeyReleaseMask |
                        ButtonPressMask | 
                        ButtonReleaseMask |
                        Button1MotionMask );
       }
       else if (ev.xany.type == ButtonPress ||
                ev.xany.type == ButtonRelease)
       {
           char *msg = (char *)stream.allocBuffer(256);
           sprintf(msg, "event send EV_ABS:ABS_X:%d\r\n"
                        "event send EV_ABS:ABS_Y:%d\r\n"
                        "event send EV_ABS:ABS_Z:0\r\n"
                        "event send EV_KEY:BTN_TOUCH:%d\r\n"
                        "event send EV_SYN:0:0\r\n",
                        ev.xbutton.x,
                        ev.xbutton.y,
                        ev.xbutton.type == ButtonPress ? 1 : 0);
           stream.commitBuffer(strlen(msg));
           getReply(&stream);
       }
       else if (ev.xany.type == MotionNotify) {
           char *msg = (char *)stream.allocBuffer(256);
           sprintf(msg, "event send EV_ABS:ABS_X:%d\r\n"
                        "event send EV_ABS:ABS_Y:%d\r\n"
                        "event send EV_ABS:ABS_Z:0\r\n"
                        "event send EV_SYN:0:0\r\n",
                        ev.xmotion.x,
                        ev.xmotion.y);
           stream.commitBuffer(strlen(msg));
           getReply(&stream);
       }
   }

   pthread_cleanup_pop(1);

   return NULL;
}

bool start(int portNum)
{
    if (s_evThread) {
        return false; // already started
    }

    if (pthread_create(&s_evThread, NULL, run, (void *)portNum)) {
        perror("creating event thread");
        return false;
    }

    //
    // install sigint handler to cleanly exit
    //
    s_prevSigIntHandler = signal(SIGINT, sigint_handler);

    return true;
}

void addWindow(Display *dpy, Window w)
{
    if (s_evWin) {
        XEvent ev;
        ev.xclient.type = ClientMessage;
        ev.xclient.display = dpy;
        ev.xclient.window = s_evWin;
        ev.xclient.message_type = XInternAtom(dpy, UT_RENDER_EVENT, False);
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = w;
        printf("sending ClientMessage w=%x root=%x\n", (int)w, (int)DefaultRootWindow(dpy));
        XSendEvent(dpy, s_evWin, False, 0, &ev);
        XFlush(dpy);
    }
}

}  // of namespac EventTracker
