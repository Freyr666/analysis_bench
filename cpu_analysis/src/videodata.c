/* videodata.c
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

#include "videodata.h"
#include <malloc.h>
#include <string.h>

float
param_of_video_params (VideoParams* vp, PARAMETER p)
{
        switch (p) {
        case BLACK:  return vp->black_pix;
        case LUMA:   return vp->avg_bright;
        case FREEZE: return vp->frozen_pix;
        case DIFF:   return vp->avg_diff;
        case BLOCKY: return vp->blocks;
        default:     return 0.0;
        }
}

VideoData*
video_data_new(guint fr)
{
  if (fr == 0) return NULL;
  
  VideoData* rval;
  rval = (VideoData*)malloc(sizeof(VideoData));
  rval->frames = fr;
  rval->current = 0;
  rval->data = (VideoParams*)malloc(sizeof(VideoParams) * fr);
  return rval;
}
  
void
video_data_delete(VideoData* dt)
{
  free(dt->data);
  dt->data = NULL;
  free(dt);
  dt = NULL;
}

gint
video_data_append(VideoData* dt,
		  VideoParams* par)
{
  if(dt->current == dt->frames) return -1;
  guint i = dt->current;
  dt->data[i] = *par;
  dt->current++;
  return 0;
}

gboolean
video_data_is_full(VideoData* dt)
{
  if (dt->current == dt->frames) return TRUE;
  return FALSE;
}

#include <glib/gprintf.h>

gpointer
video_data_dump(VideoData* dt, gsize* sz) {
        if (sz == NULL) return NULL;
        
        *sz         = dt->current;
        VideoParams* buf = (VideoParams*) malloc(sizeof(VideoParams) * (*sz));
        memcpy(buf, dt->data, (sizeof(VideoParams) * (*sz)));
        return buf;
}

gchar*
video_data_to_string(VideoData* dt,
		     const guint stream,
		     const guint prog,
		     const guint pid)
{
  guint i;
  gchar* string;

  string = g_strdup_printf("v%d:%d:%d", stream, prog, pid);
  for (i = 0; i < dt->frames; i++) {
    gchar* pr_str = string;
    gchar* tmp = g_strdup_printf(":*:%f:%f:%f:%f:%f",
				 dt->data[i].frozen_pix,
				 dt->data[i].black_pix,
				 dt->data[i].blocks,
				 dt->data[i].avg_bright,
				 dt->data[i].avg_diff);
    string = g_strconcat(pr_str, tmp, NULL);
    g_free(tmp);
    g_free(pr_str);
  }
  return string;
}
