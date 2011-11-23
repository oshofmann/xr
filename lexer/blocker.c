#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <xr_common.h>

enum {
   TYPE_LINES,
   TYPE_OBJS,
   TYPE_VALS,
   TYPE_NO_JSON,
};

static void usage(char *argv0)
{
   fprintf(stderr, "Usage: %s <output_dir> <block_ents> <prefix[2]> <obj_id> "
         "<lines|objs|vals|no_json>\n", argv0);
   exit(1);
}

static int do_mkstemp(char template[], char tempfile[], int len)
{
   memcpy(tempfile, template, len);
   return mkstemp(tempfile);
}

static void output_master(int lo_lno, char *lo_obj, char obj_id[], int type)
{
   switch (type) {
      case TYPE_LINES:
         printf("{\"lo\":%d,", lo_lno);
         break;
      case TYPE_OBJS:
         printf("{%s", lo_obj);
         free(lo_obj);
         break;
      case TYPE_VALS:
         printf("{\"lo\":%s", lo_obj);
         free(lo_obj);
         break;
   }

   printf("\"obj\":\"%s\"},\n", obj_id);
}

int main(int argc, char **argv)
{
   size_t len;
   char *line = NULL;
   FILE *in;
   char template[] = "block.XXXXXX";
   char tempfile[sizeof(template)];
   char newpath[OBJ_NAME_LEN];
   char *prefix;
   char *obj_id;
   int cur_fd;
   int type;
   int lno;
   int lo_lno;
   char *lo_obj;
   long block_lines;
   ssize_t line_len;
   struct hash_window hw;
   SHA_CTX file_sha;
   md_t file_md;

   if (argc != 6 || strlen(argv[3]) != 2)
      usage(argv[0]);
   else
      prefix = argv[3];

   if ((block_lines = strtol(argv[2], NULL, 10)) == LONG_MAX)
      usage(argv[0]);

   obj_id = argv[4];

   if (!strcmp(argv[5], "lines"))
      type = TYPE_LINES;
   else if (!strcmp(argv[5], "objs"))
      type = TYPE_OBJS;
   else if (!strcmp(argv[5], "vals"))
      type = TYPE_VALS;
   else if (!strcmp(argv[5], "no_json"))
      type = TYPE_NO_JSON;
   else
      usage(argv[0]);

   in = stdin;
   SHA1_Init(&file_sha);
   init_hash(&hw);

   if (argc > 2 && chdir(argv[1]) == -1)
      return 1;

   cur_fd = do_mkstemp(template, tempfile, sizeof(template));
   if (type != TYPE_NO_JSON) {
      dprintf(cur_fd, "%s [\n", json_pre);
      printf("%s [\n", json_pre);
   }

   lo_lno = lno = 1;
   while ((line_len = getline(&line, &len, in)) != -1) {
      char *first_val;
      char *cur_val;
      md_t *line_md;

      if (type != TYPE_NO_JSON) {
         first_val = strchr(line, '\t');
         if (!first_val) {
            fprintf(stderr, "Bad format line %d\n", lno);
            exit(1);
         }

         cur_val = first_val;
         while (cur_val) {
            *cur_val = ' ';
            cur_val = strchr(cur_val, '\t');
         }

         if (lo_lno == lno) {
            if (type == TYPE_OBJS) {
               *first_val = 0;
               lo_obj = strdup(line);
               *first_val = ' ';
            } else if (type == TYPE_VALS) {
               if (line[line_len - 1] == '\n')
                  line[line_len - 1] = 0;
               lo_obj = strdup(line);
               if (line[line_len - 1] == 0)
                  line[line_len - 1] = '\n';
            }
         }
      }

      line_md = next_hash(line, &hw);
      SHA1_Update(&file_sha, line, line_len);

      if (type == TYPE_OBJS) {
         line[line_len-1] = 0;
         dprintf(cur_fd, "{%s}\n,", line);
      } else {
         dprintf(cur_fd, "%s", line);
      }

      if (gen_break(line_md, block_lines)) {
         SHA1_Final(file_md.d, &file_sha);

         rename_obj(cur_fd, tempfile, prefix, newpath, &file_md);

         if (type != TYPE_NO_JSON) {
            dprintf(cur_fd, "]%s", json_post);
            write_frag_last(cur_fd, newpath);
            output_master(lo_lno, lo_obj, newpath, type);
         }

         close(cur_fd);

         lo_lno = lno + 1;

         SHA1_Init(&file_sha);
         cur_fd = do_mkstemp(template, tempfile, sizeof(template));

         if (type != TYPE_NO_JSON)
            dprintf(cur_fd, "%s [\n", json_pre);
      }

      lno++;
   }

   SHA1_Final(file_md.d, &file_sha);
   rename_obj(cur_fd, tempfile, prefix, newpath, &file_md);

   if (type != TYPE_NO_JSON) {
      dprintf(cur_fd, "]%s", json_post);
      write_frag_last(cur_fd, newpath);
      output_master(lo_lno, lo_obj, newpath, type);
      printf("]%s", json_post);
      fwrite_obj_last(stdout, obj_id, lno);
   }

   close(cur_fd);


   return 0;
}
