#include <stdio.h>
#include "timelapse.h"

// global gphoto context
GPContext* glo_context;

//-----------------------------------------------------------------------------
static void ctx_error_fn(GPContext *context, const char *str, void *data)
{
        fprintf  (stderr, "\n*** Context error ***\n%s\n",str);
        fflush   (stderr);
}

//-----------------------------------------------------------------------------
static void ctx_status_fn(GPContext *context, const char *str, void *data)
{
        fprintf  (stderr, "%s\n", str);
        fflush   (stderr);
}

//-----------------------------------------------------------------------------
void create_context() 
{
	/* This is the mandatory part */
	glo_context = gp_context_new();

	/* All the parts below are optional! */
    gp_context_set_error_func (glo_context, ctx_error_fn, NULL);
    gp_context_set_status_func (glo_context, ctx_status_fn, NULL);
  
	/* also:
	gp_context_set_cancel_func    (p->context, ctx_cancel_func,  p);
        gp_context_set_message_func   (p->context, ctx_message_func, p);
        if (isatty (STDOUT_FILENO))
                gp_context_set_progress_funcs (p->context,
                        ctx_progress_start_func, ctx_progress_update_func,
                        ctx_progress_stop_func, p);
	*/
}
