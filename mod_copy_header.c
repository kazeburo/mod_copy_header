#include "apr.h"
#include "apr_strings.h"
#include "apr_lib.h"

#define APR_WANT_STRFUNC
#include "apr_want.h"

#include "ap_config.h"
#include "httpd.h"
#include "http_config.h"
#include "http_log.h"
#include "http_request.h"
#include "http_protocol.h"

typedef struct {
    int active;
    apr_table_t *headers;
} copyheader_dir_config;

#define DIR_CMD_PERMS OR_INDEXES

#define ACTIVE_ON       1
#define ACTIVE_OFF      0
#define ACTIVE_DONTCARE 2

module AP_MODULE_DECLARE_DATA copyheader_module;

static void *create_dir_copyheader_config(apr_pool_t *p, char *dummy)
{
    copyheader_dir_config *new =
    (copyheader_dir_config *) apr_pcalloc(p, sizeof(copyheader_dir_config));
    new->active = ACTIVE_DONTCARE;
    new->headers = apr_table_make(p, 4);
    return (void *) new;
};

static const char *set_copyheaderactive(cmd_parms *cmd, void *in_dir_config, int arg)
{
    copyheader_dir_config *dir_config = in_dir_config;

    dir_config->active = ACTIVE_ON;
    if (arg == 0) {
        dir_config->active = ACTIVE_OFF;
    }
    return NULL;
}

static const char *set_copyheaders(cmd_parms *cmd, void *in_dir_config,
                                     const char *header)
{
    copyheader_dir_config *dir_config = in_dir_config;
    apr_table_setn(dir_config->headers, header, "on" );
    return NULL;
}

static const command_rec copyheader_cmds[] =
{
    AP_INIT_FLAG("CopyHeaderActive", set_copyheaderactive, NULL, DIR_CMD_PERMS,
                 "Limited to 'on' or 'off'"),
    AP_INIT_TAKE1("CopyHeader", set_copyheaders, NULL, DIR_CMD_PERMS,
                  "a header name to copy to note"),
    {NULL}
};

static void *merge_copyheader_dir_configs(apr_pool_t *p, void *basev, void *addv)
{
    copyheader_dir_config *new = (copyheader_dir_config *) apr_pcalloc(p, sizeof(copyheader_dir_config));
    copyheader_dir_config *base = (copyheader_dir_config *) basev;
    copyheader_dir_config *add = (copyheader_dir_config *) addv;

    if (add->active == ACTIVE_DONTCARE) {
        new->active = base->active;
    }
    else {
        new->active = add->active;
    }

    new->headers = apr_table_overlay(p, add->headers,
                                        base->headers);
    return new;
};

static apr_status_t copyheader_filter(ap_filter_t *f,
                                   apr_bucket_brigade *b)
{
    request_rec *r;
    copyheader_dir_config *conf;
    const char *val;
    int i;


    r = f->r;
    conf = (copyheader_dir_config *) ap_get_module_config(r->per_dir_config,
                                                       &copyheader_module);

    const apr_array_header_t *ch = apr_table_elts(conf->headers);
    apr_table_entry_t *elts = (apr_table_entry_t *)ch->elts;

    for (i = 0; i < ch->nelts; i++) {
        val = apr_table_get(r->err_headers_out, elts[i].key);
        if ( val == NULL ) {
            val = apr_table_get(r->headers_out, elts[i].key);
        }
        if ( val != NULL ) {
            apr_table_setn(r->notes, elts[i].key, val);
        }
    }
    ap_remove_output_filter(f);
    return ap_pass_brigade(f->next, b);
};


static void copyheader_insert_filter(request_rec *r)
{
    copyheader_dir_config *conf;

    /* Say no to subrequests */
    if (r->main != NULL) {
        return;
    }

    conf = (copyheader_dir_config *) ap_get_module_config(r->per_dir_config,
                                                       &copyheader_module);

    /* Check to see if the filter is enabled and if there are any applicable
     * config directives for this directory scope
     */
    if ( conf->active != ACTIVE_ON || apr_is_empty_table(conf->headers) ) {
        return;
    }
    ap_add_output_filter("MOD_COPYHEADER", NULL, r, r->connection);
    return;
}


static void register_hooks(apr_pool_t *p)
{
    ap_register_output_filter("MOD_COPYHEADER", copyheader_filter, NULL,
                              AP_FTYPE_CONTENT_SET-2);
    ap_hook_insert_error_filter(copyheader_insert_filter, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_insert_filter(copyheader_insert_filter, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA copyheader_module =
{
    STANDARD20_MODULE_STUFF,
    create_dir_copyheader_config,  /* dir config creater */
    merge_copyheader_dir_configs,  /* dir merger --- default is to override */
    NULL,                       /* server config */
    NULL,                       /* merge server configs */
    copyheader_cmds,               /* command apr_table_t */
    register_hooks              /* register hooks */
};
