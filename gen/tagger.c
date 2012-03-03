#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <target.h>
#include <function.h>
#include <cgraph.h>
#include <c-pragma.h>
#include <cpplib.h>
#include <pointer-set.h>

#include <zlib.h>

#include "collapse_name.h"

#define tag_fdopen gzdopen
#define tag_printf gzprintf
#define tag_fclose gzclose
#define tag_FILE gzFile

int plugin_is_GPL_compatible;

static struct cpp_callbacks cpp_prev_cb;

static char *self;

static char *last_file;
static char *cur_file;

static const char *input_base;
static size_t input_base_len;

static const char *output_base;
static size_t output_base_len;

static char name_buf[PATH_MAX];

static char *tmp_tags;
static int tmp_fd;

static tag_FILE *output_file;

static int out_fd = -1;

#define COMPS(var, args...) const char *var[] = { args , NULL}

static void _xr_output(const char *comps[], char type, const char *file,
      int line, const char *scope_name, int scope_line)
{
   const char **comp = &comps[0];
   const char **next_comp = &comps[1];

	char *file_dup = strdupa(file);
	if (collapse_name(file_dup, file_dup))
		return;

   tag_printf(output_file, "\"id\":\"");
   while (*next_comp) {
      tag_printf(output_file, "%s.", *comp);
      comp = next_comp;
      next_comp++;
   }
   tag_printf(output_file, "%s\"\t", *comp);

   tag_printf(output_file, "\"type\":\"%c\"\t\"file\":\"%s\"\t\"line\":%d",
         type, file_dup, line);

   if (scope_name)
      tag_printf(output_file, "\t\"scope_name\":\"%s\"\t\"scope_line\":%d",
            scope_name, scope_line);

   tag_printf(output_file, "\n");
}

static void xr_output(const char *comps[], char type, const char *file,
      int line, const char *scope_name, int scope_line)
{
	if ((file = valid_file(file, input_base, input_base_len)))
      _xr_output(comps, type, file, line, scope_name, scope_line);
}

static void do_child(tree*, struct pointer_set_t*);

static tree xr_walk(tree *t, int *walk_subtrees, void *data)
{
   tree scope_decl = (tree)data;

   enum tree_code code = TREE_CODE(*t);
   // tree *tree_type;

   // printf("%s\n", tree_code_name[TREE_CODE(*t)]);

   switch (code) {
      case CALL_EXPR:
      {
         tree callee = CALL_EXPR_FN(*t);
         if (TREE_CODE(callee) == ADDR_EXPR) {
            tree fn = TREE_OPERAND(callee, 0);
            COMPS(comps, IDENTIFIER_POINTER(DECL_NAME(fn)));
            if (EXPR_HAS_LOCATION(*t))
               xr_output(comps, 'c', LOCATION_FILE(EXPR_LOCATION(*t)),
                     LOCATION_LINE(EXPR_LOCATION(*t)),
                     IDENTIFIER_POINTER(DECL_NAME(scope_decl)),
                     DECL_SOURCE_LINE(scope_decl));
         }
         /*
         else if (TREE_CODE(callee) == COMPONENT_REF) {
            tree fn_comp_var = TREE_OPERAND(callee, 0);
            tree fn_comp_type = TREE_TYPE(fn_comp_var);
            tree fn_field = TREE_OPERAND(callee, 1);
            printf("  %s.%s %s:%d\n",
                  IDENTIFIER_POINTER(TYPE_NAME(fn_comp_type)),
                  IDENTIFIER_POINTER(DECL_NAME(fn_field))
                  LOCATION_FILE(EXPR_LOCATION(*t)),
                  LOCATION_LINE(EXPR_LOCATION(*t)));
         }
         */
         break;
      }
      /*
      case FUNCTION_DECL:
      {
         COMPS(comps, IDENTIFIER_POINTER(DECL_NAME(*t)));
         xr_output(comps, 'f', DECL_SOURCE_FILE(*t), DECL_SOURCE_LINE(*t),
               NULL, 0);
         break;
      }
      case TYPE_DECL:
      {
         COMPS(comps, IDENTIFIER_POINTER(DECL_NAME(*t)));
         xr_output(comps, 't', DECL_SOURCE_FILE(*t), DECL_SOURCE_LINE(*t), NULL, 0);
         break;
      }
      case RECORD_TYPE:
         if (TREE_CODE(TYPE_NAME(*t)) == IDENTIFIER_NODE) {
            COMPS(comps, IDENTIFIER_POINTER(TYPE_NAME(*t)));
            xr_output(comps, 's', DECL_SOURCE_FILE(*t), DECL_SOURCE_LINE(*t), NULL, 0);
         } else {
            do_child(&TYPE_NAME(*t), pset);
         }
         break;
      case COMPONENT_REF:
      {
         tree parent_type = TREE_TYPE(TREE_OPERAND(*t, 0));
         tree child_field = TREE_OPERAND(*t, 1);
         //printf("  %s %p\n", IDENTIFIER_POINTER(TYPE_NAME(parent_type)),
               //DECL_SOURCE_FILE(parent_type));
         break;
      }
      */
      default:
         break;
   }

   *walk_subtrees = 1;

   /* Make sure to walk the type of each node */
   /* do_child(&TREE_TYPE(*t), pset); */

   return NULL;
}

static void do_child(tree *t, struct pointer_set_t *pset)
{
   if (*t != ERROR_MARK && pset && !pointer_set_insert(pset, *t)) {
      int x;
      xr_walk(t, &x, pset);
   }
}


static void xr_tree(void *gcc_data, void *user_data)
{
   tree t = DECL_SAVED_TREE(cfun->decl);
   COMPS(comps, IDENTIFIER_POINTER(DECL_NAME(cfun->decl)));
   xr_output(comps, 'f', DECL_SOURCE_FILE(cfun->decl),
         DECL_SOURCE_LINE(cfun->decl), NULL, 0);
   walk_tree_without_duplicates(&t, &xr_walk, cfun->decl);
}

static void xr_define(cpp_reader *r, unsigned int a, cpp_hashnode *node)
{
   if (cur_file) {
      COMPS(comps, NODE_NAME(node));
      _xr_output(comps, 'd', cur_file, LOCATION_LINE(a), NULL, 0);
   }
   if (cpp_prev_cb.define)
      cpp_prev_cb.define(r, a, node);
}

static void xr_used(cpp_reader *r, source_location l, cpp_hashnode *node)
{
   COMPS(comps, NODE_NAME(node));
   xr_output(comps, 'x', LOCATION_FILE(l), LOCATION_LINE(l), NULL, 0);
   if (cpp_prev_cb.used)
      cpp_prev_cb.used(r, l, node);
}

static void xr_file_change(cpp_reader *r, const struct line_map *lm)
{
   const char *lm_file = lm ? lm->to_file : NULL;
	last_file = cur_file;
	cur_file = NULL;

	if (lm_file) {
		const char *file;
		size_t name_len;
		struct stat st;

		if (!(file = valid_file(lm_file, input_base, input_base_len)))
			goto out;

		if (file[0] == '/') {
			if (collapse_name(file + 1, &name_buf[output_base_len]))
				goto out;
		} else {
			if (collapse_name(file, &name_buf[output_base_len]))
				goto out;
		}

      if (stat(name_buf, &st) == -1) {
         if (errno != ENOENT) {
            fprintf(stderr, "%s: Cannot stat %s: %s\n", self, name_buf,
                  strerror(errno));
            perror(self);
            exit(1);
         }

         char *part = name_buf + output_base_len;
         while(1) {
            part = strchr(part, '/');
            if (!part)
               break;

            *part = 0;

            if (mkdir(name_buf, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) == -1
                  && errno != EEXIST) {
               fprintf(stderr, "%s: Cannot create %s: %s\n", self, name_buf,
                     strerror(errno));
               exit(1);
            }

            *part = '/';
            part++;
         }
         
         if (open(name_buf, O_RDONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
               == -1) {
            fprintf(stderr, "%s: Cannot stat %s: %s\n", self, name_buf,
                  strerror(errno));
            exit(1);
         }
      }
		cur_file = &name_buf[output_base_len];
	}
out:

   if (cpp_prev_cb.file_change)
      cpp_prev_cb.file_change(r, lm);
}

static void xr_attributes(void *gcc_data, void *user_data)
{
   struct cpp_callbacks *cb = cpp_get_callbacks(parse_in);

   cpp_prev_cb.define = cb->define;
   cb->define = xr_define;

   cpp_prev_cb.used = cb->used;
   cb->used = xr_used;

   cpp_prev_cb.file_change = cb->file_change;
   cb->file_change = xr_file_change;
}

static void xr_finish(void *gcc_data, void *user_data)
{
   tag_fclose(output_file);
   close(tmp_fd);

   if (last_file != NULL) {
      size_t last_len = strlen(name_buf);
		strcpy(name_buf + last_len, ".gcc_tags");

      if (rename(tmp_tags, name_buf) == -1) {
         fprintf(stderr, "%s: Cannot rename %s -> %s: %s\n", self, tmp_tags,
               name_buf, strerror(errno));
         exit(1);
      }

      name_buf[last_len] = 0;
      dprintf(out_fd, "%s", name_buf);
   } else if (unlink(tmp_tags) == -1) {
      fprintf(stderr, "%s: Cannot unlink %s: %s\n", self, tmp_tags,
            strerror(errno));
      exit(1);
   }
}

int plugin_init(struct plugin_name_args *info,
      struct plugin_gcc_version *version)
{
   struct pointer_set_t *pset;
   const char tmplate[] = "tags.XXXXXX";
   int i;

   if (!plugin_default_version_check(version, &gcc_version))
      return 1;

   self = info->base_name;

   for (i = 0; i < info->argc; i++) {
      const char *key = info->argv[i].key;
      const char *val = info->argv[i].value;
      if (!strcmp("input-base", key)) {
         input_base = val;
         input_base_len = strlen(input_base);
      } else if (!strcmp("output-base", key)) {
         output_base = val;
         output_base_len = strlen(output_base);
			strcpy(name_buf, output_base);
      } else if (!strcmp("fd", key)) {
         out_fd = atoi(val);
      } else {
         fprintf(stderr, "%s: invalid argument %s\n", self, info->argv[i].key);
         return 1;
      }
   }

   if (out_fd == -1)
      return 0;

   tmp_tags = xmalloc(output_base_len + strlen(tmplate) + 1);
   strcpy(tmp_tags, output_base);
   strcpy(tmp_tags + output_base_len, tmplate);
   if ((tmp_fd = mkstemp(tmp_tags)) == -1) {
      fprintf(stderr, "%s: Cannot create %s: %s\n", self, tmp_tags, strerror(errno));
      return 1;
   }
   if ((output_file = tag_fdopen(tmp_fd, "w")) == NULL) {
      fprintf(stderr, "%s: Cannot open %s: %s\n", self, tmp_tags, strerror(errno));
      return 1;
   }

   /* pset = pointer_set_create(); */
   register_callback(info->base_name, PLUGIN_PRE_GENERICIZE, &xr_tree, NULL);
   register_callback(info->base_name, PLUGIN_ATTRIBUTES, &xr_attributes, NULL);
   register_callback(info->base_name, PLUGIN_FINISH, &xr_finish, NULL);

   return 0;
}
