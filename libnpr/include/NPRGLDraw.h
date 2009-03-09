#ifndef NPR_GL_DRAW_H_
#define NPR_GL_DRAW_H_

#include <QList>
#include "NPRGeometry.h"

class NPRStyle;
class NPRScene;
class GQTexture2D;
class GQFramebufferObject;
class GQShaderRef;
class NPRDrawable;
class CdaMaterial;

const int NPR_OPAQUE = 0x1;
const int NPR_TRANSLUCENT = 0x2;
const int NPR_DRAW_POLYGONS = 0x4;
const int NPR_DRAW_LINES = 0x8;
const int NPR_DRAW_PROFILES = 0x10;
const int NPR_DRAW_ALL_POLYGONS = NPR_DRAW_POLYGONS | NPR_OPAQUE | NPR_TRANSLUCENT;
const int NPR_DRAW_EVERYTHING = NPR_DRAW_POLYGONS | NPR_DRAW_LINES | NPR_OPAQUE | NPR_TRANSLUCENT;

const int NPR_SUCCESS = 1;
const int NPR_FAILURE = 0;

class NPRGLDraw
{
    public:
        static void drawMesh(const GQShaderRef* shader, const NPRScene& scene, int which, 
                             int draw_mode = NPR_DRAW_ALL_POLYGONS);
        static void drawMeshes(const GQShaderRef* shader, const NPRScene& scene, 
                               const QList<int>* list = 0, int draw_mode = NPR_DRAW_ALL_POLYGONS );
        static void drawMeshesDepth( const NPRScene& scene );

		static void drawFullScreenQuad( int texture_mode );
        static void drawFullScreenQuadFBO( const GQFramebufferObject& fbo );
        static void drawPaperQuad( const NPRStyle* style );
		static void drawBackgroundTex( const NPRStyle* style );

        static void drawPosAndNormalBuffer( const NPRScene& scene );
        static void drawPosAndNormalBufferFBO( const NPRScene& scene, 
                        GQFramebufferObject& fbo, float supersample_factor );
        static int  drawDepthBufferFBO( const NPRScene& scene, 
                        GQFramebufferObject& fbo, float supersample_factor );

        static void visualizeFBO(const GQFramebufferObject& fbo, int which);

        static void clearGLState();
        static void clearGLScreen(const vec& color, float depth); 
        static void clearGLDepth(float depth); 

        static void setUniformSSParams(const GQShaderRef& shader);
        static void setUniformViewParams(const GQShaderRef& shader);

        static void handleGLError(const char* file = 0, int line = 0);

    protected:
        static void drawPrimList(int mode, const NPRDrawable* drawable, 
                                 const NPRPrimPointerList& prims, int draw_mode );
        static void drawDrawablePolygons( const NPRDrawable* drawable, int type_mask );
        static void applyMaterialToGL( const CdaMaterial* material );

        static void setPerModelPolygonUniforms(const GQShaderRef* shader, 
                                               const NPRScene& scene, int which );

        static void init();
        static void initSupersampleTexture();

    protected:
        static bool _is_initialized;
        static GQTexture2D* _supersample_texture;

};

#endif /*NPR_GL_DRAW_H_*/
