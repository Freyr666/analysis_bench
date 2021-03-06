/*
 * TODO copyright
 */

#ifndef LGPL_LIC
#define LIC "Proprietary"
#define URL "http://www.niitv.ru/"
#else
#define LIC "LGPL"
#define URL "https://github.com/Freyr666/ats-analyzer/"
#endif

/**
 * SECTION:element-gpu_analysis
 * @title: gpu_analysis
 *
 * QoE analysis based on shaders.
 *
 * ## Examples
 * |[
 * gst-launch-1.0 videotestsrc ! glupload ! gstgpu_analysis ! glimagesink
 * ]|
 * FBO (Frame Buffer Object) and GLSL (OpenGL Shading Language) are required.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gl/gstglfuncs.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLES3/gl31.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "gstgpuanalysis.h"

#include "analysis.h"

#define MODULUS(n,m)                            \
  ({                                            \
    __typeof__(n) _n = (n);                     \
    __typeof__(m) _m = (m);                     \
    __typeof__(n) res = _n % _m;                \
    res < 0 ? _m + res : res;                   \
  })

#define GST_CAT_DEFAULT gst_gpu_analysis_debug_category
GST_DEBUG_CATEGORY_STATIC (gst_gpu_analysis_debug_category);

#define gst_gpu_analysis_parent_class parent_class

static void gst_gpu_analysis_set_property (GObject * object,
                                           guint property_id,
                                           const GValue * value,
                                           GParamSpec * pspec);

static void gst_gpu_analysis_get_property (GObject * object,
                                           guint property_id,
                                           GValue * value,
                                           GParamSpec * pspec);

//static void gst_gpu_analysis_finalize(GObject *object);

static void gst_gpu_analysis_dispose (GObject *object);

static GstStateChangeReturn gst_gpu_analysis_change_state (GstElement * element,
                                                           GstStateChange transition);

static gboolean gst_gpu_analysis_set_caps (GstBaseTransform * trans,
                                           GstCaps * incaps,
                                           GstCaps * outcaps);

static GstFlowReturn gst_gpu_analysis_transform_ip (GstBaseTransform * filter,
                                                    GstBuffer * inbuf);

static gboolean gpu_analysis_apply (GstGPUAnalysis * va, GstGLMemory * mem);

//static void gst_gpu_analysis_timeout_loop (GstGPUAnalysis * va);

//static gboolean gst_gl_base_filter_find_gl_context (GstGLBaseFilter * filter);

/* signals */
enum
  {
    DATA_SIGNAL,
    STREAM_LOST_SIGNAL,
    STREAM_FOUND_SIGNAL,
    LAST_SIGNAL
  };

/* args */
enum
  {
    PROP_0,
    PROP_TIMEOUT,
    PROP_LATENCY,
    PROP_PERIOD,
    PROP_BLACK_PIXEL_LB,
    PROP_PIXEL_DIFF_LB,
    PROP_BLACK_CONT,
    PROP_BLACK_CONT_EN,
    PROP_BLACK_PEAK,
    PROP_BLACK_PEAK_EN,
    PROP_BLACK_DURATION,
    PROP_LUMA_CONT,
    PROP_LUMA_CONT_EN,
    PROP_LUMA_PEAK,
    PROP_LUMA_PEAK_EN,
    PROP_LUMA_DURATION,
    PROP_FREEZE_CONT,
    PROP_FREEZE_CONT_EN,
    PROP_FREEZE_PEAK,
    PROP_FREEZE_PEAK_EN,
    PROP_FREEZE_DURATION,
    PROP_DIFF_CONT,
    PROP_DIFF_CONT_EN,
    PROP_DIFF_PEAK,
    PROP_DIFF_PEAK_EN,
    PROP_DIFF_DURATION,
    PROP_BLOCKY_CONT,
    PROP_BLOCKY_CONT_EN,
    PROP_BLOCKY_PEAK,
    PROP_BLOCKY_PEAK_EN,
    PROP_BLOCKY_DURATION,
    LAST_PROP
  };

static guint      signals[LAST_SIGNAL]   = { 0 };
static GParamSpec *properties[LAST_PROP] = { NULL, };

/* pad templates */
static const gchar caps_string[] =
  "video/x-raw(memory:GLMemory),format=(string){I420,NV12,NV21,YV12}";

/* class initialization */
G_DEFINE_TYPE_WITH_CODE (GstGPUAnalysis,
                         gst_gpu_analysis,
                         GST_TYPE_GL_BASE_FILTER,
                         GST_DEBUG_CATEGORY_INIT (gst_gpu_analysis_debug_category,
                                                  "gpu_analysis", 0,
                                                  "debug category for gpu_analysis element"));

static void
gst_gpu_analysis_class_init (GstGPUAnalysisClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS (klass);
  GstGLBaseFilterClass *base_filter = GST_GL_BASE_FILTER_CLASS(klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_pad_template (element_class,
                                      gst_pad_template_new ("sink",
                                                            GST_PAD_SINK,
                                                            GST_PAD_ALWAYS,
                                                            gst_caps_from_string (caps_string)));
  gst_element_class_add_pad_template (element_class,
                                      gst_pad_template_new ("src",
                                                            GST_PAD_SRC,
                                                            GST_PAD_ALWAYS,
                                                            gst_caps_from_string (caps_string)));

  gst_element_class_set_static_metadata (element_class,
                                         "Gstreamer element for video analysis",
                                         "Video data analysis",
                                         "filter for video analysis",
                                         "freyr <sky_rider_93@mail.ru>");

  gobject_class->set_property = gst_gpu_analysis_set_property;
  gobject_class->get_property = gst_gpu_analysis_get_property;
  //gobject_class->finalize     = gst_gpu_analysis_finalize;
  gobject_class->dispose      = gst_gpu_analysis_dispose;
  element_class->change_state = gst_gpu_analysis_change_state;
  
  base_transform_class->passthrough_on_same_caps = FALSE;
  //base_transform_class->transform_ip_on_passthrough = TRUE;
  base_transform_class->transform_ip = gst_gpu_analysis_transform_ip;
  base_transform_class->set_caps = gst_gpu_analysis_set_caps;
  base_filter->supported_gl_api = GST_GL_API_OPENGL3;

  signals[DATA_SIGNAL] =
    g_signal_new("data", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(GstGPUAnalysisClass, data_signal), NULL, NULL,
                 g_cclosure_marshal_generic, G_TYPE_NONE,
                 1, GST_TYPE_BUFFER);
  signals[STREAM_LOST_SIGNAL] =
    g_signal_new("stream-lost", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(GstGPUAnalysisClass, stream_lost_signal), NULL, NULL,
                 g_cclosure_marshal_generic, G_TYPE_NONE, 0);
  signals[STREAM_FOUND_SIGNAL] =
    g_signal_new("stream-found", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
                 G_STRUCT_OFFSET(GstGPUAnalysisClass, stream_found_signal), NULL, NULL,
                 g_cclosure_marshal_generic, G_TYPE_NONE, 0);

  properties [PROP_TIMEOUT] =
    g_param_spec_uint("timeout", "Stream-lost event timeout",
                      "Seconds needed for stream-lost being emitted.",
                      1, G_MAXUINT, 10, G_PARAM_READWRITE);
  properties [PROP_LATENCY] =
    g_param_spec_uint("latency", "Latency",
                      "Measurment latency (frame), bigger latency may reduce GPU stalling. 1 - instant measurment, 2 - 1-frame latency, etc.",
                      1, MAX_LATENCY, 3, G_PARAM_READWRITE);
  properties [PROP_PERIOD] =
    g_param_spec_uint("period", "Period",
                      "Measuring period",
                      1, 60, 1, G_PARAM_READWRITE);
  properties [PROP_BLACK_PIXEL_LB] =
    g_param_spec_uint("black_pixel_lb", "Black pixel lb",
                      "Black pixel value lower boundary",
                      0, 256, 16, G_PARAM_READWRITE);
  properties [PROP_PIXEL_DIFF_LB] =
    g_param_spec_uint("pixel_diff_lb", "Freeze pixel lb",
                      "Freeze pixel value lower boundary",
                      0, 256, 0, G_PARAM_READWRITE);
  properties [PROP_BLACK_CONT] =
    g_param_spec_float("black_cont", "Black cont err boundary",
                       "Black cont err meas",
                       0., 100.0, 90.0, G_PARAM_READWRITE);
  properties [PROP_BLACK_CONT_EN] =
    g_param_spec_boolean("black_cont_en", "Black cont err enabled",
                         "Enable black cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_BLACK_PEAK] =
    g_param_spec_float("black_peak", "Black peak err boundary",
                       "Black peak err meas",
                       0., 100.0, 100.0, G_PARAM_READWRITE);
  properties [PROP_BLACK_PEAK_EN] =
    g_param_spec_boolean("black_peak_en", "Black peak err enabled",
                         "Enable black peak err meas", FALSE, G_PARAM_READWRITE);
  properties [PROP_BLACK_DURATION] =
    g_param_spec_float("black_duration", "Black duration boundary",
                       "Black err duration",
                       0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties [PROP_LUMA_CONT] =
    g_param_spec_float("luma_cont", "Luma cont err boundary",
                       "Luma cont err meas",
                       0., 256.0, 20.0, G_PARAM_READWRITE);
  properties [PROP_LUMA_CONT_EN] =
    g_param_spec_boolean("luma_cont_en", "Luma cont err enabled",
                         "Enable luma cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_LUMA_PEAK] =
    g_param_spec_float("luma_peak", "Luma peak err boundary",
                       "Luma peak err meas",
                       0., 256.0, 17.0, G_PARAM_READWRITE);
  properties [PROP_LUMA_PEAK_EN] =
    g_param_spec_boolean("luma_peak_en", "Luma peak err enabled",
                         "Enable luma peak err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_LUMA_DURATION] =
    g_param_spec_float("luma_duration", "Luma duration boundary",
                       "Luma err duration",
                       0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties [PROP_FREEZE_CONT] =
    g_param_spec_float("freeze_cont", "Freeze cont err boundary",
                       "Freeze cont err meas",
                       0., 100.0, 90., G_PARAM_READWRITE);
  properties [PROP_FREEZE_CONT_EN] =
    g_param_spec_boolean("freeze_cont_en", "Freeze cont err enabled",
                         "Enable freeze cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_FREEZE_PEAK] =
    g_param_spec_float("freeze_peak", "Freeze peak err boundary",
                       "Freeze peak err meas",
                       0., 100.0, 100.0, G_PARAM_READWRITE);
  properties [PROP_FREEZE_PEAK_EN] =
    g_param_spec_boolean("freeze_peak_en", "Freeze peak err enabled",
                         "Enable freeze peak err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_FREEZE_DURATION] =
    g_param_spec_float("freeze_duration", "Freeze duration boundary",
                       "Freeze err duration",
                       0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties [PROP_DIFF_CONT] =
    g_param_spec_float("diff_cont", "Diff cont err boundary",
                       "Diff cont err meas",
                       0., 256.0, 0.05, G_PARAM_READWRITE);
  properties [PROP_DIFF_CONT_EN] =
    g_param_spec_boolean("diff_cont_en", "Diff cont err enabled",
                         "Enable diff cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_DIFF_PEAK] =
    g_param_spec_float("diff_peak", "Diff peak err boundary",
                       "Diff peak err meas",
                       0., 265.0, 0.01, G_PARAM_READWRITE);
  properties [PROP_DIFF_PEAK_EN] =
    g_param_spec_boolean("diff_peak_en", "Diff peak err enabled",
                         "Enable diff peak err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_DIFF_DURATION] =
    g_param_spec_float("diff_duration", "Diff duration boundary",
                       "Diff err duration",
                       0., G_MAXFLOAT, 2., G_PARAM_READWRITE);
  properties [PROP_BLOCKY_CONT] =
    g_param_spec_float("blocky_cont", "Blocky cont err boundary",
                       "Blocky cont err meas",
                       0., 100.0, 10., G_PARAM_READWRITE);
  properties [PROP_BLOCKY_CONT_EN] =
    g_param_spec_boolean("blocky_cont_en", "Blocky cont err enabled",
                         "Enable blocky cont err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_BLOCKY_PEAK] =
    g_param_spec_float("blocky_peak", "Blocky peak err boundary",
                       "Blocky peak err meas",
                       0., 100.0, 15., G_PARAM_READWRITE);
  properties [PROP_BLOCKY_PEAK_EN] =
    g_param_spec_boolean("blocky_peak_en", "Blocky peak err enabled",
                         "Enable blocky peak err meas", TRUE, G_PARAM_READWRITE);
  properties [PROP_BLOCKY_DURATION] =
    g_param_spec_float("blocky_duration", "Blocky duration boundary",
                       "Blocky err duration",
                       0., G_MAXFLOAT, 3., G_PARAM_READWRITE);

  g_object_class_install_properties(gobject_class, LAST_PROP, properties);
}

static void
gst_gpu_analysis_init (GstGPUAnalysis *gpu_analysis)
{
  gpu_analysis->timeout = 10;
  gpu_analysis->period  = 1;
  gpu_analysis->black_pixel_lb = 16;
  gpu_analysis->pixel_diff_lb = 0;

  /* TODO cleanup this mess */
  for (guint i = 0; i < PARAM_NUMBER; i++) {
    gpu_analysis->params_boundary[i].cont_en = TRUE; /* TODO FALSE */
    gpu_analysis->params_boundary[i].peak_en = TRUE; /* TODO FALSE */
  }

  gpu_analysis->params_boundary[BLACK].cont = 90.00;
  gpu_analysis->params_boundary[BLACK].peak = 100.00;
  gpu_analysis->params_boundary[BLACK].duration = 2.00;

  gpu_analysis->params_boundary[LUMA].cont = 20.00;
  gpu_analysis->params_boundary[LUMA].peak = 17.00;
  gpu_analysis->params_boundary[LUMA].duration = 2.00;

  gpu_analysis->params_boundary[FREEZE].cont = 90.00;
  gpu_analysis->params_boundary[FREEZE].peak = 100.00;
  gpu_analysis->params_boundary[FREEZE].duration = 2.00;

  gpu_analysis->params_boundary[DIFF].cont_en = FALSE; /* TODO FALSE */
  gpu_analysis->params_boundary[DIFF].peak_en = FALSE; /* TODO FALSE */
        
  gpu_analysis->params_boundary[DIFF].cont = 0.05;
  gpu_analysis->params_boundary[DIFF].peak = 0.02;
  gpu_analysis->params_boundary[DIFF].duration = 2.00;

  gpu_analysis->params_boundary[BLOCKY].cont = 10.00;
  gpu_analysis->params_boundary[BLOCKY].peak = 15.00;
  gpu_analysis->params_boundary[BLOCKY].duration = 3.00;

  /* private */
  gpu_analysis->latency = 3;
  gpu_analysis->buffer_ptr = 0;
  gpu_analysis->acc_buffer = NULL;
  gpu_analysis->shader = NULL;
  gpu_analysis->shader_block = NULL;
  gpu_analysis->tex = NULL;
  gpu_analysis->prev_buffer = NULL;
  gpu_analysis->prev_tex = NULL;
  gpu_analysis->gl_settings_unchecked = TRUE;

  for (int i = 0; i < MAX_LATENCY; i++) {
    gpu_analysis->buffer[i] = 0;
  }

  data_ctx_init (&gpu_analysis->errors);
  /*
    g_rec_mutex_init (&gpu_analysis->task_lock);
    gpu_analysis->timeout_task =
      gst_task_new ((GstTaskFunction)gst_gpu_analysis_timeout_loop,
                    gpu_analysis,
                    NULL);
    gst_task_set_lock (gpu_analysis->timeout_task, &gpu_analysis->task_lock);
  */  
}

static void
gst_gpu_analysis_dispose (GObject *object)  
//gst_gpu_analysis_finalize(GObject *object)
{
  GstGLContext *context = GST_GL_BASE_FILTER (object)->context;
  GstGPUAnalysis *gpu_analysis = GST_GPUANALYSIS (object);
  //printf ("GPU dispose 1\n");
  if (context)
    gst_object_unref(context);
  //printf ("GPU dispose 2\n");
  data_ctx_delete (&gpu_analysis->errors);
  //printf ("GPU dispose 3\n");
  //gst_object_unref (gpu_analysis->timeout_task);
  gst_object_unref (gpu_analysis->shader);
  gst_object_unref (gpu_analysis->shader_block);
  //printf ("GPU dispose 4\n");
  //G_OBJECT_CLASS (parent_class)->finalize(object);
  G_OBJECT_CLASS (parent_class)->dispose(object);
  //printf ("GPU dispose 5\n");
}

static void
gst_gpu_analysis_set_property (GObject * object,
                               guint property_id,
                               const GValue * value,
                               GParamSpec * pspec)
{
  GstGPUAnalysis *gpu_analysis = GST_GPUANALYSIS (object);

  GST_DEBUG_OBJECT (gpu_analysis, "set_property");

  switch (property_id) {
  case PROP_TIMEOUT:
    gpu_analysis->timeout = g_value_get_uint(value);
    break;
  case PROP_LATENCY:
    gpu_analysis->latency = g_value_get_uint(value);
    break;
  case PROP_PERIOD:
    gpu_analysis->period = g_value_get_uint(value);
    break;
  case PROP_BLACK_PIXEL_LB:
    gpu_analysis->black_pixel_lb = g_value_get_uint(value);
    break;
  case PROP_PIXEL_DIFF_LB:
    gpu_analysis->pixel_diff_lb = g_value_get_uint(value);
    break;
  case PROP_BLACK_CONT:
    gpu_analysis->params_boundary[BLACK].cont = g_value_get_float(value);
    break;
  case PROP_BLACK_CONT_EN:
    gpu_analysis->params_boundary[BLACK].cont_en = g_value_get_boolean(value);
    break;
  case PROP_BLACK_PEAK:
    gpu_analysis->params_boundary[BLACK].peak = g_value_get_float(value);
    break;
  case PROP_BLACK_PEAK_EN:
    gpu_analysis->params_boundary[BLACK].peak_en = g_value_get_boolean(value);
    break;
  case PROP_BLACK_DURATION:
    gpu_analysis->params_boundary[BLACK].duration = g_value_get_float(value);
    break;
  case PROP_LUMA_CONT:
    gpu_analysis->params_boundary[LUMA].cont = g_value_get_float(value);
    break;
  case PROP_LUMA_CONT_EN:
    gpu_analysis->params_boundary[LUMA].cont_en = g_value_get_boolean(value);
    break;
  case PROP_LUMA_PEAK:
    gpu_analysis->params_boundary[LUMA].peak = g_value_get_float(value);
    break;
  case PROP_LUMA_PEAK_EN:
    gpu_analysis->params_boundary[LUMA].peak_en = g_value_get_boolean(value);
    break;
  case PROP_LUMA_DURATION:
    gpu_analysis->params_boundary[LUMA].duration = g_value_get_float(value);
    break;
  case PROP_FREEZE_CONT:
    gpu_analysis->params_boundary[FREEZE].cont = g_value_get_float(value);
    break;
  case PROP_FREEZE_CONT_EN:
    gpu_analysis->params_boundary[FREEZE].cont_en = g_value_get_boolean(value);
    break;
  case PROP_FREEZE_PEAK:
    gpu_analysis->params_boundary[FREEZE].peak = g_value_get_float(value);
    break;
  case PROP_FREEZE_PEAK_EN:
    gpu_analysis->params_boundary[FREEZE].peak_en = g_value_get_boolean(value);
    break;
  case PROP_FREEZE_DURATION:
    gpu_analysis->params_boundary[FREEZE].duration = g_value_get_float(value);
    break;
  case PROP_DIFF_CONT:
    gpu_analysis->params_boundary[DIFF].cont = g_value_get_float(value);
    break;
  case PROP_DIFF_CONT_EN:
    gpu_analysis->params_boundary[DIFF].cont_en = g_value_get_boolean(value);
    break;
  case PROP_DIFF_PEAK:
    gpu_analysis->params_boundary[DIFF].peak = g_value_get_float(value);
    break;
  case PROP_DIFF_PEAK_EN:
    gpu_analysis->params_boundary[DIFF].peak_en = g_value_get_boolean(value);
    break;
  case PROP_DIFF_DURATION:
    gpu_analysis->params_boundary[DIFF].duration = g_value_get_float(value);
    break;
  case PROP_BLOCKY_CONT:
    gpu_analysis->params_boundary[BLOCKY].cont = g_value_get_float(value);
    break;
  case PROP_BLOCKY_CONT_EN:
    gpu_analysis->params_boundary[BLOCKY].cont_en = g_value_get_boolean(value);
    break;
  case PROP_BLOCKY_PEAK:
    gpu_analysis->params_boundary[BLOCKY].peak = g_value_get_float(value);
    break;
  case PROP_BLOCKY_PEAK_EN:
    gpu_analysis->params_boundary[BLOCKY].peak_en = g_value_get_boolean(value);
    break;
  case PROP_BLOCKY_DURATION:
    gpu_analysis->params_boundary[BLOCKY].duration = g_value_get_float(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
gst_gpu_analysis_get_property (GObject * object,
                               guint property_id,
                               GValue * value,
                               GParamSpec * pspec)
{
  GstGPUAnalysis *gpu_analysis = GST_GPUANALYSIS (object);

  GST_DEBUG_OBJECT (gpu_analysis, "get_property");

  switch (property_id) {
  case PROP_TIMEOUT: 
    g_value_set_uint(value, gpu_analysis->timeout);
    break;
  case PROP_LATENCY: 
    g_value_set_uint(value, gpu_analysis->latency);
    break;
  case PROP_PERIOD: 
    g_value_set_uint(value, gpu_analysis->period);
    break;
  case PROP_BLACK_PIXEL_LB:
    g_value_set_uint(value, gpu_analysis->black_pixel_lb);
    break;
  case PROP_PIXEL_DIFF_LB:
    g_value_set_uint(value, gpu_analysis->pixel_diff_lb);
    break;
  case PROP_BLACK_CONT:
    g_value_set_float(value, gpu_analysis->params_boundary[BLACK].cont);
    break;
  case PROP_BLACK_CONT_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[BLACK].cont_en);
    break;
  case PROP_BLACK_PEAK:
    g_value_set_float(value, gpu_analysis->params_boundary[BLACK].peak);
    break;
  case PROP_BLACK_PEAK_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[BLACK].peak_en);
    break;
  case PROP_BLACK_DURATION:
    g_value_set_float(value, gpu_analysis->params_boundary[BLACK].duration);
    break;
  case PROP_LUMA_CONT:
    g_value_set_float(value, gpu_analysis->params_boundary[LUMA].cont);
    break;
  case PROP_LUMA_CONT_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[LUMA].cont_en);
    break;
  case PROP_LUMA_PEAK:
    g_value_set_float(value, gpu_analysis->params_boundary[LUMA].peak);
    break;
  case PROP_LUMA_PEAK_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[LUMA].peak_en);
    break;
  case PROP_LUMA_DURATION:
    g_value_set_float(value, gpu_analysis->params_boundary[LUMA].duration);
    break;
  case PROP_FREEZE_CONT:
    g_value_set_float(value, gpu_analysis->params_boundary[FREEZE].cont);
    break;
  case PROP_FREEZE_CONT_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[FREEZE].cont_en);
    break;
  case PROP_FREEZE_PEAK:
    g_value_set_float(value, gpu_analysis->params_boundary[FREEZE].peak);
    break;
  case PROP_FREEZE_PEAK_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[FREEZE].peak_en);
    break;
  case PROP_FREEZE_DURATION:
    g_value_set_float(value, gpu_analysis->params_boundary[FREEZE].duration);
    break;
  case PROP_DIFF_CONT:
    g_value_set_float(value, gpu_analysis->params_boundary[DIFF].cont);
    break;
  case PROP_DIFF_CONT_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[DIFF].cont_en);
    break;
  case PROP_DIFF_PEAK:
    g_value_set_float(value, gpu_analysis->params_boundary[DIFF].peak);
    break;
  case PROP_DIFF_PEAK_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[DIFF].peak_en);
    break;
  case PROP_DIFF_DURATION:
    g_value_set_float(value, gpu_analysis->params_boundary[DIFF].duration);
    break;
  case PROP_BLOCKY_CONT:
    g_value_set_float(value, gpu_analysis->params_boundary[BLOCKY].cont);
    break;
  case PROP_BLOCKY_CONT_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[BLOCKY].cont_en);
    break;
  case PROP_BLOCKY_PEAK:
    g_value_set_float(value, gpu_analysis->params_boundary[BLOCKY].peak);
    break;
  case PROP_BLOCKY_PEAK_EN:
    g_value_set_boolean(value, gpu_analysis->params_boundary[BLOCKY].peak_en);
    break;
  case PROP_BLOCKY_DURATION:
    g_value_set_float(value, gpu_analysis->params_boundary[BLOCKY].duration);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static GstStateChangeReturn
gst_gpu_analysis_change_state (GstElement * element,
                               GstStateChange transition)
{
  GstGPUAnalysis *gpu_analysis = GST_GPUANALYSIS (element);

  switch (transition) {
    /* Initialize task and clocks */
  case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    {
      gpu_analysis->time_now_us = g_get_real_time ();
      update_all_timestamps (&gpu_analysis->error_state, gpu_analysis->time_now_us);
      for (int i = 0; i < PARAM_NUMBER; i++)
        gpu_analysis->error_state.cont_err_duration[i] = 0.;
      
      gpu_analysis->next_data_message_ts = 0;
      /*
      atomic_store(&gpu_analysis->got_frame, FALSE);
      atomic_store(&gpu_analysis->task_should_run, TRUE);

      gst_task_start (gpu_analysis->timeout_task);
      */
    }
    break;
  case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    {
      gpu_analysis->tex = NULL;
      gst_buffer_replace(&gpu_analysis->prev_buffer, NULL);
      gpu_analysis->prev_tex = NULL;
      /*
      atomic_store(&gpu_analysis->task_should_run, FALSE);

      gst_task_join (gpu_analysis->timeout_task);
      */
    }
    break;
  default:
    break;
  }

  return
    GST_ELEMENT_CLASS (gst_gpu_analysis_parent_class)->change_state (element,
                                                                     transition);
}

static gboolean
_find_local_gl_context (GstGLBaseFilter * filter)
{
  if (gst_gl_query_local_gl_context (GST_ELEMENT (filter), GST_PAD_SRC,
                                     &filter->context))
    return TRUE;
  if (gst_gl_query_local_gl_context (GST_ELEMENT (filter), GST_PAD_SINK,
                                     &filter->context))
    return TRUE;
  return FALSE;
}

static void
shader_create (GstGLContext * context, GstGPUAnalysis * va)
{
  GError * error;
  if (!(va->shader =
        gst_gl_shader_new_link_with_stages(context,
                                           &error,
                                           gst_glsl_stage_new_with_string (context,
                                                                           GL_COMPUTE_SHADER,
                                                                           GST_GLSL_VERSION_450,
                                                                           GST_GLSL_PROFILE_CORE,
                                                                           shader_source),
                                           NULL)))
    {
      GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
                         ("Failed to initialize shader"), (NULL));
    }
  if (!(va->shader_block =
        gst_gl_shader_new_link_with_stages(context,
                                           &error,
                                           gst_glsl_stage_new_with_string (context,
                                                                           GL_COMPUTE_SHADER,
                                                                           GST_GLSL_VERSION_450,
                                                                           GST_GLSL_PROFILE_CORE,
                                                                           shader_source_block),
                                           NULL)))
    {
      GST_ELEMENT_ERROR (va, RESOURCE, NOT_FOUND,
                         ("Failed to initialize shader block"), (NULL));
    }

  for (int i = 0; i < va->latency; i++)
    {
      if (va->buffer[i])
        {
          glDeleteBuffers(1, &va->buffer[i]);
          va->buffer[i] = 0;
        }
      glGenBuffers(1, &va->buffer[i]);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, va->buffer[i]);
      glBufferData(GL_SHADER_STORAGE_BUFFER,
                   (va->in_info.width / 8)
                   * (va->in_info.height / 8)
                   * sizeof(struct accumulator),
                   NULL,
                   GL_DYNAMIC_COPY);
    }
}

static gboolean
gst_gpu_analysis_set_caps (GstBaseTransform * trans,
                           GstCaps * incaps,
                           GstCaps * outcaps)
{
  GstGPUAnalysis *gpu_analysis = GST_GPUANALYSIS (trans);
  
  if (!gst_video_info_from_caps (&gpu_analysis->in_info, incaps))
    goto wrong_caps;
  if (!gst_video_info_from_caps (&gpu_analysis->out_info, outcaps))
    goto wrong_caps;

  gpu_analysis->frame_duration_double =
    (double) gpu_analysis->in_info.fps_d / (double) gpu_analysis->in_info.fps_n;

  gpu_analysis->frame_duration_us =
    gst_util_uint64_scale (GST_TIME_AS_USECONDS(GST_SECOND),
                           gpu_analysis->in_info.fps_d,
                           gpu_analysis->in_info.fps_n);

  /* Preallocate number of frames per second + 10% */
  gpu_analysis->frames_prealloc_per_s =
    gst_util_uint64_scale (1,
                           gpu_analysis->in_info.fps_n * 110,
                           gpu_analysis->in_info.fps_d * 100);
  
  data_ctx_reset (&gpu_analysis->errors,
                  (gpu_analysis->period
                   * gpu_analysis->frames_prealloc_per_s));
  
  if (gpu_analysis->acc_buffer)
    free(gpu_analysis->acc_buffer);

  gpu_analysis->acc_buffer =
    (struct accumulator *) malloc((gpu_analysis->in_info.width / 8)
                                  * (gpu_analysis->in_info.height / 8)
                                  * sizeof(struct accumulator));

  if ( (! GST_GL_BASE_FILTER (trans)->context)
       && (! _find_local_gl_context(GST_GL_BASE_FILTER(trans))))
    {
      GST_WARNING ("Could not find a context");
      return FALSE;
    }

  gst_gl_context_thread_add(GST_GL_BASE_FILTER (trans)->context,
                            (GstGLContextThreadFunc) shader_create,
                            gpu_analysis);
        
  return GST_BASE_TRANSFORM_CLASS(parent_class)->set_caps(trans,incaps,outcaps);

  /* ERRORS */
 wrong_caps:
  GST_WARNING ("Wrong caps - could not understand input or output caps");
  return FALSE;
}

static void
_update_flags_and_timestamps (struct data_ctx * ctx,
                              struct state * state,
                              gint64 new_ts)
{
  struct flag * current_flag;
  
  for (int p = 0; p < PARAM_NUMBER; p++)
    {
      /* Cont flag */
      current_flag = &ctx->errs[p]->cont_flag;

      if (current_flag->value)
        {
          current_flag->timestamp = get_timestamp (state, p);
          /* TODO check if this is actually true */
          current_flag->span = new_ts - current_flag->timestamp;
        }
      else
        update_timestamp (state, p, new_ts);

      /* Peak flag */
      current_flag = &ctx->errs[p]->peak_flag;

      if (current_flag->value)
        {
          current_flag->timestamp = new_ts;
          /* TODO check if this is actually true */
          current_flag->span = GST_SECOND;
        }
    }
}

static void
_set_flags (struct data_ctx * ctx,
            struct boundary bounds [PARAM_NUMBER],
            struct state * state,
            double frame_duration,
            double values [PARAM_NUMBER])
{
  for (int p = 0; p < PARAM_NUMBER; p++)
    {
      data_ctx_flags_cmp (ctx,
                          p,
                          &bounds[p],
                          param_boundary_is_upper (p),
                          &(state->cont_err_duration[p]),
                          frame_duration,
                          values[p]);
    }
}

#include <time.h>

static GstFlowReturn
gst_gpu_analysis_transform_ip (GstBaseTransform * trans,
                               GstBuffer * buf)
{
  GstMemory        *tex;
  GstVideoFrame    gl_frame;
  GstGPUAnalysis *gpu_analysis = GST_GPUANALYSIS (trans);
  int              height = gpu_analysis->in_info.height;
  int              width  = gpu_analysis->in_info.width;
  double           values [PARAM_NUMBER] = { 0 };
  clock_t          start, end;
  double           cpu_time_used;

  if (G_UNLIKELY(!gst_pad_is_linked (GST_BASE_TRANSFORM_SRC_PAD(trans))))
    return GST_FLOW_OK;

  /* Initialize clocks if needed */
  if (G_UNLIKELY (gpu_analysis->next_data_message_ts == 0))
    gpu_analysis->next_data_message_ts =
      GST_BUFFER_TIMESTAMP (buf) + (GST_SECOND * gpu_analysis->period);

  if (!gst_video_frame_map (&gl_frame,
                            &gpu_analysis->in_info,
                            buf,
                            GST_MAP_READ | GST_MAP_GL))
    goto inbuf_error;

  /* map[0] corresponds to the Y component of Yuv */
  tex = gl_frame.map[0].memory;

  if (!gst_is_gl_memory (tex))
    {
      GST_ERROR_OBJECT (gpu_analysis, "Input memory must be GstGLMemory");
      goto unmap_error;
    }

  start = clock ();
  
  /* Frame is fine, so inform the timeout_loop task */
  atomic_store(&gpu_analysis->got_frame, TRUE);

  /* Evaluate the intermidiate parameters via shader */
  gpu_analysis_apply (gpu_analysis, GST_GL_MEMORY_CAST (tex));

  /* Merge intermediate values */
  for (int i = 0; i < (width * height / 64); i++)
    {
      values[FREEZE] += gpu_analysis->acc_buffer[i].frozen;
      values[BLACK] += gpu_analysis->acc_buffer[i].black;
      values[DIFF] += gpu_analysis->acc_buffer[i].diff;
      values[LUMA] += gpu_analysis->acc_buffer[i].bright;
      values[BLOCKY] += (float)gpu_analysis->acc_buffer[i].visible;
    }
  
  values[FREEZE] = 100.0 * values[FREEZE] / (width * height);
  values[LUMA] = 255.0 * values[LUMA] / (width * height);
  values[DIFF] = values[DIFF] / (width * height);
  values[BLACK] = 100.0 * values[BLACK] / (width * height);
  values[BLOCKY] = 100.0 * values[BLOCKY] / (width * height / 64);

  end = clock ();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

  GstStructure * s = gst_structure_new_empty ("perf");
  gst_structure_set (s,
                     "time", G_TYPE_DOUBLE, cpu_time_used,
                     NULL);
  /* Post element message containing the performance data */
  gst_element_post_message (GST_ELEMENT (trans),
                            gst_message_new_application (GST_OBJECT (trans), s));

  //g_print ("Shader Results: [block: %f; luma: %f; black: %f; diff: %f; freeze: %f]\n",
  //          values[BLOCKY], values[LUMA], values[BLACK], values[DIFF], values[FREEZE]);
  //g_print ("Frame: %d Limit: %d\n", gpu_analysis->frame, gpu_analysis->frame_limit);

  gpu_analysis->time_now_us += gpu_analysis->frame_duration_us;

  /* errors */
  _set_flags (&gpu_analysis->errors,
              gpu_analysis->params_boundary,
              &gpu_analysis->error_state,
              gpu_analysis->frame_duration_double,
              values);
  
  for (int p = 0; p < PARAM_NUMBER; p++)
    data_ctx_add_point (&gpu_analysis->errors,
                        p,
                        values[p],
                        gpu_analysis->time_now_us);

  /* Send data message if needed */
  if (GST_BUFFER_TIMESTAMP (buf)
      >= gpu_analysis->next_data_message_ts)
    {
      size_t data_size;
      
      gpu_analysis->time_now_us = g_get_real_time ();
      
      /* Update clock */
      gpu_analysis->next_data_message_ts =
        GST_BUFFER_TIMESTAMP (buf) + (GST_SECOND * gpu_analysis->period);

      _update_flags_and_timestamps (&gpu_analysis->errors,
                                    &gpu_analysis->error_state,
                                    gpu_analysis->time_now_us);

      gpointer d = data_ctx_pull_out_data (&gpu_analysis->errors, &data_size);
      
      data_ctx_reset (&gpu_analysis->errors,
                      (gpu_analysis->period
                       * gpu_analysis->frames_prealloc_per_s));

      GstBuffer* data = gst_buffer_new_wrapped (d, data_size);
      g_signal_emit(gpu_analysis, signals[DATA_SIGNAL], 0, data);
      
      gst_buffer_unref (data);
    }

  gst_video_frame_unmap (&gl_frame);
        
  /* Cleanup */
  gst_buffer_replace (&gpu_analysis->prev_buffer, buf);

  return GST_FLOW_OK;
                
 unmap_error:
  gst_video_frame_unmap (&gl_frame);
 inbuf_error:
  return GST_FLOW_ERROR;
}

static void
analyse (GstGLContext *context, GstGPUAnalysis * va)
{
  const GstGLFuncs * gl = context->gl_vtable;
  int width = va->in_info.width;
  int height = va->in_info.height;
  int stride = va->in_info.stride[0];
  struct accumulator * data = NULL;
        
  glGetError();

  if (G_LIKELY(va->prev_tex))
    {
      gl->ActiveTexture (GL_TEXTURE1);
      gl->BindTexture (GL_TEXTURE_2D, gst_gl_memory_get_texture_id (va->prev_tex));
      glBindImageTexture(1, gst_gl_memory_get_texture_id (va->prev_tex),
                         0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
    }

  gl->ActiveTexture (GL_TEXTURE0);      
  gl->BindTexture (GL_TEXTURE_2D, gst_gl_memory_get_texture_id (va->tex));
  glBindImageTexture(0, gst_gl_memory_get_texture_id (va->tex),
                     0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
        
  glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 10, va->buffer[va->buffer_ptr]);
        
  gst_gl_shader_use (va->shader);

  GLuint prev_ind = va->prev_tex != 0 ? 1 : 0;
  gst_gl_shader_set_uniform_1i(va->shader, "tex", 0);
  gst_gl_shader_set_uniform_1i(va->shader, "tex_prev", prev_ind);
  gst_gl_shader_set_uniform_1i(va->shader, "width", width);
  gst_gl_shader_set_uniform_1i(va->shader, "height", height);
  gst_gl_shader_set_uniform_1i(va->shader, "stride", stride);
  gst_gl_shader_set_uniform_1i(va->shader, "black_bound", va->black_pixel_lb);
  gst_gl_shader_set_uniform_1i(va->shader, "freez_bound", va->pixel_diff_lb);
        
  glDispatchCompute(width / 8, height / 8, 1);

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  gst_gl_shader_use (va->shader_block);

  gst_gl_shader_set_uniform_1i(va->shader_block, "tex", 0);
  gst_gl_shader_set_uniform_1i(va->shader_block, "width", width);
  gst_gl_shader_set_uniform_1i(va->shader_block, "height", height);

  glDispatchCompute(width / 8, height / 8, 1);
        
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        
  /* Get prev results */

  guint prev = MODULUS(((int)va->buffer_ptr - (int)va->latency + 1), (int)va->latency);
        
  glBindBufferBase (GL_SHADER_STORAGE_BUFFER, 0, va->buffer[prev]);
  //glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, va->buffer[prev],
  //                  0, (width / 8) * (height / 8) * sizeof(struct accumulator));
  data = glMapBufferRange(GL_SHADER_STORAGE_BUFFER,
                          0, (width / 8) * (height / 8) * sizeof(struct accumulator),
                          GL_MAP_READ_BIT);

  memcpy(va->acc_buffer, data, (width / 8) * (height / 8) * sizeof(struct accumulator));

  /* Cleanup */
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  va->buffer_ptr = MODULUS((va->buffer_ptr+1), va->latency);
        
  va->prev_tex = va->tex;
}

static void
_check_defaults_ (GstGLContext *context, GstGPUAnalysis * va) {
  int size, type;
        
  glGetInternalformativ(GL_TEXTURE_2D, GL_RED, GL_INTERNALFORMAT_RED_SIZE, 1, &size);
  glGetInternalformativ(GL_TEXTURE_2D, GL_RED, GL_INTERNALFORMAT_RED_TYPE, 1, &type);

  if (size == 8 && type == GL_UNSIGNED_NORMALIZED)
    {
      va->gl_settings_unchecked = FALSE;
    }
  else
    {
      GST_ERROR("Platform default red representation is not GL_R8");
      exit(-1);
    }
}

static gboolean
gpu_analysis_apply (GstGPUAnalysis * va, GstGLMemory * tex)
{
  GstGLContext *context = GST_GL_BASE_FILTER (va)->context;

  if (G_UNLIKELY(va->shader == NULL))
    {
      GST_WARNING("GL shader is not ready for analysis");
      return TRUE;
    }

  /* Ensure that GL platform defaults meet the expectations */
  if (G_UNLIKELY(va->gl_settings_unchecked))
    {
      gst_gl_context_thread_add(context, (GstGLContextThreadFunc) _check_defaults_, va);
    }
  /* Check texture format */
  if (G_UNLIKELY(gst_gl_memory_get_texture_format(tex) != GST_GL_RED))
    {
      GST_ERROR("GL texture format should be GL_RED");
      exit(-1);
    }

  va->tex = tex;

  /* Compile shader */
  //if (G_UNLIKELY (!va->shader) )
  //        gst_gl_context_thread_add(context, (GstGLContextThreadFunc) shader_create, va);

  /* Analyze */
  gst_gl_context_thread_add(context, (GstGLContextThreadFunc) analyse, va);

  return TRUE;
}

/*
static void
gst_gpu_analysis_timeout_loop (GstGPUAnalysis * gpu_analysis)
{
  gint countdown = gpu_analysis->timeout;
  static gboolean stream_is_lost = FALSE; / TODO maybe this should be true /
  
  while (countdown--)
    {
      / In case we need kill the task /
      if (G_UNLIKELY (! atomic_load (&gpu_analysis->task_should_run)))
        return;

      if (G_LIKELY (atomic_load (&gpu_analysis->got_frame)))
        {
          atomic_store (&gpu_analysis->got_frame, FALSE);
          countdown = gpu_analysis->timeout;
          
          if (stream_is_lost)
            {
              stream_is_lost = FALSE;
              g_signal_emit(gpu_analysis, signals[STREAM_FOUND_SIGNAL], 0);
            }
        }

      sleep (1);
    }

  / No frame appeared before countdown exp /
  if ( !stream_is_lost )
    {
      stream_is_lost = TRUE;
      g_signal_emit(gpu_analysis, signals[STREAM_LOST_SIGNAL], 0);
    }
    }
*/    
   
static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin,
                               "gpuanalysis",
                               GST_RANK_NONE,
                               GST_TYPE_GPUANALYSIS);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.1.9"
#endif
#ifndef PACKAGE
#define PACKAGE "gpuanalysis"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "gpuanalysis_package"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN URL
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   gpuanalysis,
                   "Package for video data analysis",
                   plugin_init, VERSION, LIC, PACKAGE_NAME, GST_PACKAGE_ORIGIN)
