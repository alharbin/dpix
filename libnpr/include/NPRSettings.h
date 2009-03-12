#ifndef NPRSETTINGS_H_
#define NPRSETTINGS_H_

#include <QDir>
#include <QVector>
#include <QDomElement>
#include <QDomDocument>

typedef enum
{
    NPR_ENABLE_LINES,
    NPR_ENABLE_STYLIZED_LINES,
    NPR_ENABLE_POLYGONS,
    NPR_ENABLE_TRANSPARENT_POLYGONS,
    NPR_ENABLE_PAPER_TEXTURE,
    NPR_ENABLE_BACKGROUND_TEXTURE,
    NPR_ENABLE_CUTAWAY,
    NPR_ENABLE_CUTAWAY_LINES,
    NPR_ENABLE_LINE_ARTMAPS,
    NPR_ENABLE_COLOR_BLUR,
    NPR_ENABLE_VBOS,
    NPR_ENABLE_LIGHTING,

    NPR_CHECK_LINE_VISIBILITY,
    NPR_CHECK_LINE_VISIBILITY_AT_SPINE,
    NPR_CHECK_LINE_PRIORITY,
    NPR_FILTER_LINE_VISIBILITY,
    NPR_FILTER_LINE_PRIORITY,

    NPR_EXTRACT_CONTOURS,
    NPR_EXTRACT_SUGGESTIVE_CONTOURS,
    NPR_EXTRACT_RIDGES,
    NPR_EXTRACT_VALLEYS,
    NPR_EXTRACT_ISOPHOTES,
    NPR_EXTRACT_BOUNDARIES,
    NPR_EXTRACT_APPARENT_RIDGES,
    NPR_EXTRACT_PROFILES,
    NPR_CLASSIFY_BOUNDARIES,
    NPR_FREEZE_LINES,
    NPR_COLOR_LINES_BY_ID,

    NPR_VIEW_ITEM_BUFFER,
    NPR_VIEW_PRIORITY_BUFFER,
    NPR_VIEW_CLIP_BUFFER,
    NPR_VIEW_TRI_STRIPS,
    NPR_VIEW_CUTAWAY_BUFFER,

    NPR_COMPUTE_PVS,

    NPR_NUM_BOOL_SETTINGS
} NPRBoolSetting;

typedef enum
{
    NPR_EXTRACT_NUM_ISOPHOTES,
    NPR_ITEM_BUFFER_LAYERS,
    NPR_LINE_VISIBILITY_SUPERSAMPLE,
    NPR_LINE_VISIBILITY_METHOD,

    NPR_FOCUS_MODE,

    NPR_NUM_INT_SETTINGS

} NPRIntSetting;

typedef enum
{
    NPR_ITEM_BUFFER_LINE_WIDTH,
    NPR_SEGMENT_ATLAS_DEPTH_SCALE,
    NPR_SEGMENT_ATLAS_KERNEL_SCALE_X,
    NPR_SEGMENT_ATLAS_KERNEL_SCALE_Y,

    NPR_N_DOT_V_BIAS,

    NPR_NUM_FLOAT_SETTINGS

} NPRFloatSetting;

typedef enum
{
	NPR_FOCUS_NONE = 0,
	NPR_FOCUS_CAMERA,
	NPR_FOCUS_WORLD,
	NPR_FOCUS_SCREEN,
	NPR_FOCUS_OBJECT,
	NPR_FOCUS_CONFIDENCE,
	NPR_FOCUS_COLOR,

	NPR_NUM_FOCUS_MODES
} NPRFocusMode;

class NPRSettings
{
    public:
        NPRSettings() { loadDefaults(); }; 

        bool load( const QDomElement& element );
        void loadDefaults();
        bool save( QDomDocument& doc, QDomElement& element );
        void copyPersistent( const NPRSettings& settings );

        bool  get(NPRBoolSetting which) const { return _bools[which]; }
        int   get(NPRIntSetting which) const { return _ints[which]; }
        float get(NPRFloatSetting which) const { return _floats[which]; }

        void set(NPRBoolSetting which, bool value) { _bools[which] = value; }
        void set(NPRIntSetting which, int value) { _ints[which] = value; }
        void set(NPRFloatSetting which, float value) { _floats[which] = value; }

        void         setWorkingDir( const QDir& dir ) { _working_dir = dir; }
        const QDir&  workingDir() { return _working_dir; }

        static NPRSettings& instance() { return _instance; }

    protected:
        static NPRSettings _instance;

        QVector<bool>       _bools;
        QVector<int>        _ints;
        QVector<float>      _floats;

        QDir _working_dir;
};

#endif
