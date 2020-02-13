/* videodata.h
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
#ifndef VIDEODATA_H
#define VIDEODATA_H

#include <glib.h>
#include <gst/gst.h>
#include "error.h"

#define DATA_MARKER 0x8BA820F0

typedef struct __VideoParams VideoParams;
typedef struct __VideoData VideoData;

struct __VideoParams {
  float frozen_pix;
  float black_pix;
  float blocks;
  float avg_bright;
  float avg_diff;
  gint64 time; 
};

float param_of_video_params (VideoParams*, PARAMETER);

struct __VideoData {
  guint current;
  guint frames;
  VideoParams* data;
};

VideoData* video_data_new(guint fr);
#define video_data_reset(dt)(dt->current = 0)
void video_data_delete(VideoData* dt);
gint video_data_append(VideoData* dt,
		       VideoParams* par);
gboolean video_data_is_full(VideoData* dt);
gpointer video_data_dump(VideoData* dt, gsize* sz);
/* convert data into string 
 * format:
 * channel:*:frozen_pix:black_pix:blocks:avg_bright:avg_diff:*:frozen_pix:black_pix:blocks:avg_bright:avg_diff
 */
gchar* video_data_to_string(VideoData* dt,
			    const guint stream,
			    const guint prog,
			    const guint pid);

#endif /* VIDEOANALYSIS_API_H */
