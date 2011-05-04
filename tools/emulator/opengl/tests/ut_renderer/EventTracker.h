#ifndef _EVENT_TRACKER_H
#define _EVENT_TRACKER_H

#include <X11/Xlib.h>

namespace EventTracker
{

bool start(int portNum);
void addWindow(Display *dpy, Window w);

}  // of namespac EventTracker

#endif
