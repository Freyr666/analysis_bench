#include "error.h"
#include <malloc.h>
#include <string.h>

const char*
param_to_string (PARAMETER p)
{
        switch (p) {
        case BLACK: {
                return "black";
                break;
        }
        case LUMA: {
                return "luma";
                break;
        }
        case FREEZE: {
                return "freeze";
                break;
        }
        case DIFF: {
                return "diff";
                break;
        }
        case BLOCKY: {
                return "blocky";
                break;
        }
        default:
                return "unknown";
                break;
        }
}

void
err_flags_cmp (ErrFlags* flags, BOUNDARY* bounds, gint64 time, gboolean upper, float* dur, float dur_d, float val)
{
        ErrFlags flag = { FALSE, FALSE, time };
        if (bounds->peak_en) {
                flag.peak = upper ? (val > bounds->peak) : (val < bounds->peak);
        }
        if (bounds->cont_en) {
                gboolean b = upper ? (val > bounds->cont) : (val < bounds->cont);
                if (b) {
                        *dur += dur_d;
                } else {
                        *dur = 0.0;
                }
                flag.cont = *dur > bounds->duration;
        }
        *flags = flag;
}

Errors*
errors_new(guint fr)
{
        Errors* rval;
        
        if (fr == 0) return NULL;

        rval = (Errors*)malloc(sizeof(Errors));

        if ( ! rval ) return NULL;

        rval->frames_total = fr;
        rval->current = 0;
        rval->err_flags = (ErrFlags*) malloc (sizeof(ErrFlags) * PARAM_NUMBER * fr);
        return rval;
}

void
errors_reset(Errors* e)
{
        e->current = 0;
}

void
errors_delete(Errors* e)
{
        free(e->err_flags);
        free(e);
        e = NULL;
}

gint
errors_append(Errors* e, ErrFlags flags [PARAM_NUMBER])
{
        if (e->current >= e->frames_total) return -1;
        for (int p = 0; p < PARAM_NUMBER; p++) {
                int pos = p * e->frames_total + e->current;             
                e->err_flags[pos] = flags[p];
        }
        e->current++;
        return 0;
}

gboolean
errors_is_full(Errors* e)
{
        return e->current == e->frames_total;
}

gpointer
errors_dump(Errors* e, gsize* sz)
{
        if (sz == NULL) return NULL;

        *sz = e->frames_total;
        int size      = sizeof(ErrFlags) * e->frames_total * PARAM_NUMBER;
        ErrFlags* buf = (ErrFlags*) malloc (size);
        memcpy(buf, e->err_flags, size);
        return buf;
}
