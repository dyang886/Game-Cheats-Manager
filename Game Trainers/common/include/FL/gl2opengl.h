/*      gl.h

        GL to OpenGL translator.
        If you include this, you might be able to port old GL programs.
        There are also much better emulators available on the net.

*/

#ifndef _FL_gl2opengl_h_
#define _FL_gl2opengl_h_

#include <FL/gl.h>
#include "gl_draw.H"

inline void clear() {glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);}
#define RGBcolor(r,g,b) glColor3ub(r,g,b)
#define bgnline() glBegin(GL_LINE_STRIP)
#define bgnpolygon() glBegin(GL_POLYGON)
#define bgnclosedline() glBegin(GL_LINE_LOOP)
#define endline() glEnd()
#define endpolygon() glEnd()
#define endclosedline() glEnd()
#define v2f(v) glVertex2fv(v)
#define v2s(v) glVertex2sv(v)
#define cmov(x,y,z) glRasterPos3f(x,y,z)
#define charstr(s) gl_draw(s)
#define fmprstr(s) gl_draw(s)
typedef float Matrix[4][4];
inline void pushmatrix() {glPushMatrix();}
inline void popmatrix() {glPopMatrix();}
inline void multmatrix(Matrix m) {glMultMatrixf((float *)m);}
inline void color(int n) {glIndexi(n);}
inline void rect(int x,int y,int r,int t) {gl_rect(x,y,r-x,t-y);}
inline void rectf(int x,int y,int r,int t) {glRectf(x,y,r+1,t+1);}
inline void recti(int x,int y,int r,int t) {gl_rect(x,y,r-x,t-y);}
inline void rectfi(int x,int y,int r,int t) {glRecti(x,y,r+1,t+1);}
inline void rects(int x,int y,int r,int t) {gl_rect(x,y,r-x,t-y);}
inline void rectfs(int x,int y,int r,int t) {glRects(x,y,r+1,t+1);}

#endif /* _FL_gl2opengl_h_ */
