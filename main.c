#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <cairo.h>
#include <cairo-xlib.h>

// Customizable global variables
int LINE_THICKNESS = 5;
int VISIBLE = 1;

int main(void) {
    Display *d;
    Window w;
    XEvent e;

    d = XOpenDisplay(NULL);
    if (d == NULL) {
        fprintf(stderr, "Unable to connect to X server\n");
        return 1;
    }

    Screen *s = DefaultScreenOfDisplay(d);
    int SCREEN_WIDTH = s->width;
    int SCREEN_HEIGHT = s->height;



    // Create an ARGB visual window
    XVisualInfo vinfo;
    XMatchVisualInfo(d, DefaultScreen(d), 32, TrueColor, &vinfo);
    XSetWindowAttributes attrs;
    attrs.colormap = XCreateColormap(d, DefaultRootWindow(d), vinfo.visual, AllocNone);
    attrs.border_pixel = 0;
    attrs.background_pixel = 0;  // Make the background fully transparent

    w = XCreateWindow(d, DefaultRootWindow(d), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, vinfo.depth, InputOutput, vinfo.visual, CWColormap | CWBorderPixel | CWBackPixel, &attrs);

    XClassHint *hint = XAllocClassHint();
    hint->res_name = "sketch";  // instance name
    hint->res_class = "sketch"; // class name
    XSetClassHint(d, w, hint);
    XFree(hint);

    // Bypass window manager
    attrs.override_redirect = 1;
    XChangeWindowAttributes(d, w, CWOverrideRedirect, &attrs);

    XSelectInput(d, w, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask);
    XMapWindow(d, w);

    cairo_surface_t *surf = cairo_xlib_surface_create(d, w, vinfo.visual, SCREEN_WIDTH, SCREEN_HEIGHT);
    cairo_t *cr = cairo_create(surf);
    
    // Create a similar transparent surface for storing the drawing
    cairo_surface_t *storedSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, SCREEN_WIDTH, SCREEN_HEIGHT);
    cairo_t *storedCr = cairo_create(storedSurface);

    cairo_set_line_width(storedCr, LINE_THICKNESS);
    cairo_set_line_cap(storedCr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgb(storedCr, 0, 0, 0);  // black

    int drawing = 0;
    double last_x = 0, last_y = 0;

    XGrabKey(d, XKeysymToKeycode(d, XK_space), ControlMask, DefaultRootWindow(d), True, GrabModeAsync, GrabModeAsync);

    while (1) {
        XNextEvent(d, &e);
        if (e.type == Expose) {
            // When the window is exposed, redraw its content
            cairo_set_source_surface(cr, storedSurface, 0, 0);
            cairo_paint(cr);
        } else if (e.type == KeyPress) {
            if (e.xkey.keycode == XKeysymToKeycode(d, XK_space) && (e.xkey.state & ControlMask)) {
                if (VISIBLE) {
                    XUnmapWindow(d, w);
                } else {
                    XMapWindow(d, w);
                }
                VISIBLE = !VISIBLE;
            }
        } else if (e.type == ButtonPress) {
            if (e.xbutton.button == Button1) {
                drawing = 1;
                last_x = e.xbutton.x;
                last_y = e.xbutton.y;
            }
        } else if (e.type == ButtonRelease) {
            if (e.xbutton.button == Button1) {
                drawing = 0;
            }
        } else if (e.type == MotionNotify && drawing) {
            cairo_move_to(storedCr, last_x, last_y);
            cairo_line_to(storedCr, e.xmotion.x, e.xmotion.y);
            cairo_stroke(storedCr);
            
            // Copy only a small region around the drawing to the main surface
            int regionSize = LINE_THICKNESS * 3; // Adjust as needed
            cairo_set_source_surface(cr, storedSurface, 0, 0);
            cairo_rectangle(cr, e.xmotion.x - regionSize, e.xmotion.y - regionSize, 2*regionSize, 2*regionSize);
            cairo_fill(cr);

            last_x = e.xmotion.x;
            last_y = e.xmotion.y;
        }
    }

    cairo_destroy(cr);
    cairo_destroy(storedCr);
    cairo_surface_destroy(surf);
    cairo_surface_destroy(storedSurface);
    XCloseDisplay(d);
    return 0;
}
