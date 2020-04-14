#include <gst/gst.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstvideoanalysis.h"

#define PLUGIN_NAME     "gpuanalysis"
#define PLUGIN_DESC     "Package for picture and sound quality analysis"
#define PLUGIN_LICENSE  "Proprietary"
#define PACKAGE_VERSION "1.16.1"
#define PACKAGE_BUGREPORT "freyr ciceromarcus@yandex.ru"
#define PACKAGE "gpuanalysis"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = TRUE;

  ret &= gst_element_register (plugin,
      "gpuanalysis", GST_RANK_NONE, GST_TYPE_GPUANALYSIS);

  return ret;
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    gpuanalysis,
    PLUGIN_DESC,
    plugin_init, PACKAGE_VERSION, PLUGIN_LICENSE, PACKAGE, PACKAGE_BUGREPORT)
