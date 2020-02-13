/* gstcpu_analysis.h
 *
 * Copyright (C) 2016 freyr <sky_rider_93@mail.ru> 
 *
 * This file is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 3 of the 
 * License, or (at your option) any later version. 
 *
 * This file is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * Lesser General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef _GST_VIDEOANALYSIS_H_
#define _GST_VIDEOANALYSIS_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

#include "videodata.h"
#include "block.h"
#include "error.h"

G_BEGIN_DECLS

#define GST_TYPE_VIDEOANALYSIS                  \
        (gst_cpu_analysis_get_type())
#define GST_VIDEOANALYSIS(obj)                                          \
        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysis))
#define GST_VIDEOANALYSIS_CLASS(klass)                                  \
        (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIDEOANALYSIS,GstVideoAnalysisClass))
#define GST_IS_VIDEOANALYSIS(obj)                                       \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIDEOANALYSIS))
#define GST_IS_VIDEOANALYSIS_CLASS(obj)                                 \
        (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIDEOANALYSIS))

typedef struct _GstVideoAnalysis GstVideoAnalysis;
typedef struct _GstVideoAnalysisClass GstVideoAnalysisClass;

struct _GstVideoAnalysis
{
        GstVideoFilter base_cpu_analysis;
        /* public */
        gfloat period;
        gfloat loss;
        guint    black_pixel_lb;
        guint    pixel_diff_lb;
        BOUNDARY params_boundary [PARAM_NUMBER];
        guint    mark_blocks;
        /* private */
        float fps_period;
        gfloat cont_err_duration [PARAM_NUMBER];
        guint8 *past_buffer;
        VideoData *data;
        Errors    *errors;
        BLOCK *blocks;
};

struct _GstVideoAnalysisClass
{
        GstVideoFilterClass base_cpu_analysis_class;

        void (*data_signal) (GstVideoFilter *filter, guint64 ds, GstBuffer* d, guint64 es, GstBuffer* e);
};

GType gst_cpu_analysis_get_type (void);

G_END_DECLS

#endif
