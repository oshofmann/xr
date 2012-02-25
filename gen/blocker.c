#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <openssl/sha.h>

#define HASH_WINDOW      10
static const char block_ext[] = ".xr";
/* the prefix (tag_pre), a file separator after the first byte, 2 characters
 * for each digest byte, the extension, then 7 characters for mkstemp
 * .XXXXXX */
#define TAG_PRE_SZ 2
#define OBJ_NAME_LEN \
   (TAG_PRE_SZ + 1 + SHA_DIGEST_LENGTH*2 + sizeof(block_ext) + 6)

enum {
   TYPE_LINES,
   TYPE_OBJS,
   TYPE_NO_JSON,
};

typedef struct {
   unsigned char d[SHA_DIGEST_LENGTH];
} md_t;

struct hash_window {
   int pos;
   md_t hashes[HASH_WINDOW];
};

static void usage(char *argv0)
{
   fprintf(stderr, "Usage: %s <output_dir> <block_ents> <prefix[2]> <obj_id> "
         "<lines|objs|vals|no_json>\n", argv0);
   exit(1);
}

static const char lb[] = "{";
static const char rb[] = "},\n";

static char *buf = NULL;
ssize_t buf_size = 8192;
ssize_t buf_len = 0;
ssize_t buf_pos = 0;

static int in_fd;
static int block_lines;
static struct hash_window hw;

static int type;
static char *prefix;

int lo_lno;
int lno;
char *lo_obj;

static void init_hash(struct hash_window *w)
{
   memset(w, 0, sizeof(*w));
}

static md_t *next_hash(const char *str, ssize_t len, struct hash_window *w)
{
   SHA_CTX sc;
   static md_t wnd_hash;
   int pos;

   /* Add this line's hash to the window */
   SHA1(str, len, w->hashes[w->pos++].d);
   if (w->pos == HASH_WINDOW)
      w->pos = 0;
   pos = w->pos;

   /* Hash the window. pos contains the oldest hash */
   SHA1_Init(&sc);
   SHA1_Update(&sc, &w->hashes[pos], (HASH_WINDOW - pos)*sizeof(md_t));
   if (pos)
      SHA1_Update(&sc, &w->hashes[0], pos*sizeof(md_t));

   SHA1_Final(wnd_hash.d, &sc);

   return &wnd_hash;
}

static int gen_break(md_t *md, int lines)
{
   return *((int*)md->d) % lines == 0;
}

static void errdie(const char *s) {
   perror(s);
   exit(1);
}

static int name_obj(char prefix[], char newpath[], md_t *md)
{
   int ret;
   int i;

   int pos = sprintf(newpath, "%s", prefix);
   sprintf(&newpath[pos], "%02hhx", md->d[0]);
   pos += 2;
   newpath[pos++] = '/';
   for (i = 1; i < SHA_DIGEST_LENGTH; i++, pos += 2)
      sprintf(&newpath[pos], "%02hhx", md->d[i]);
   pos += sprintf(&newpath[pos], block_ext);
   // memcpy(&newpath[pos], ".XXXXXX", 8);

   ret = open(newpath, O_CREAT | O_EXCL | O_WRONLY, 0666);
   if (ret == -1 && errno != EEXIST) {
      /* If open fails, try to create the directory. It might be possible
       * to disambiguate this case through the open() return value, but I
       * don't care */
      for (i = 0; i < pos; i++) {
         if (newpath[i] == '/') {
            newpath[i] = 0;
            if (mkdir(newpath, 0755) == -1 && errno != EEXIST)
               errdie("mkdir");
            newpath[i] = '/';
         }
      }
      ret = open(newpath, O_CREAT | O_EXCL | O_WRONLY, 0666);
   }

   if (ret == -1 && errno != EEXIST)
      errdie("open");

   return ret;
}

static char json_pre[] = "var __xr_tmp =";
static char json_post[] = ";\n";

static void write_frag_last(int fd, char path[])
{
   dprintf(fd, "xr_frag_insert('%s', __xr_tmp);\n", path);
}

static void fwrite_obj_last(FILE *out, char path[], int lines)
{
   fprintf(out, "xr_obj_insert('%s', %d, __xr_tmp);\n", path, lines);
}

static void output_master(int lo_lno, char *lo_obj, char obj_id[])
{
   switch (type) {
      case TYPE_LINES:
         printf("{\"lo\":%d,", lo_lno);
         break;
      case TYPE_OBJS:
         printf("{%s,", lo_obj);
         break;
   }

   printf("\"obj\":\"%s\"},\n", obj_id);
}

static int read_block()
{
   /* Create the buffer if necessary */
   if (!buf) {
      buf = mmap(NULL, buf_size, PROT_READ|PROT_WRITE,
            MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      if (buf == MAP_FAILED)
         return -1;
   }

   ssize_t line_start = buf_pos;
   while(1) {
      ssize_t red;
      md_t *md;

      /* Allocate more buffer */
      if (buf_pos >= buf_size) {
         ssize_t old_size = buf_size;
         buf_size *= 2;
         buf = mremap(buf, old_size, buf_size, MREMAP_MAYMOVE);
         if (buf == MAP_FAILED) {
            *((char*)NULL) = 0;
            errdie("mremap");
         }
      }

      /* Read into the buffer */
      red = read(in_fd, buf + buf_len, buf_size - buf_len);
      if (red > 0)
         buf_len += red;

      /* Search for clues */
      for (; buf_pos < buf_len; buf_pos++) {
         switch(buf[buf_pos]) {
            case '\t':
               if (type != TYPE_NO_JSON)
                  buf[buf_pos] = ',';
               break;
            case '\n':
               lno++;
               md = next_hash(&buf[line_start], buf_pos - line_start, &hw);
               if (gen_break(md, block_lines)) {
                  buf_pos++;
                  return 0;
               }
               line_start = buf_pos + 1;
         }
      }

      /* If EOF or error, get out */
      if(red < 0)
         return -1;
      else if (red == 0)
         return 1;
   }
}

static void write_block()
{
   SHA_CTX sc;
   md_t md;
   char newpath[OBJ_NAME_LEN];
   int block_fd;

   /* Calculate the hash of this block */
   SHA1_Init(&sc);
   SHA1_Update(&sc, buf, buf_pos);
   SHA1_Final(md.d, &sc);

   if ((block_fd = name_obj(prefix, newpath, &md)) != -1) {
      ssize_t writ;
      ssize_t total = 0;

      if (type == TYPE_OBJS) {
         /* Format objects */
         char *line;
         char *end_line;
         char *end_block;

         dprintf(block_fd, "%s [\n", json_pre);

         line = buf;
         end_block = buf + buf_pos;
         while(line < end_block) {
            end_line = strchr(line, '\n');
            *end_line = 0;
            dprintf(block_fd, "{%s},\n", line);
            line = end_line + 1;
         }

         dprintf(block_fd, "]%s", json_post);
         write_frag_last(block_fd, newpath);
      } else {
         if (type == TYPE_LINES)
            dprintf(block_fd, "%s [\n", json_pre);

         while (writ = write(block_fd, buf, buf_pos - total)) {
            if (writ == -1)
               errdie("write");
            total += writ;
         }

         if (type == TYPE_LINES) {
            dprintf(block_fd, "]%s", json_post);
            write_frag_last(block_fd, newpath);
         }
      }

      close(block_fd);
   }
   
   if (type == TYPE_OBJS) {
      /* Output the master record */
      lo_obj = buf;
      *strchr(lo_obj, ',') = 0;
      output_master(-1, lo_obj, newpath);
   } else if (type == TYPE_LINES) {
      output_master(lo_lno, NULL, newpath);
      lo_lno = lno;
   }

   /* All the data up to buf_pos is processed */
   memmove(buf, buf + buf_pos, buf_len - buf_pos);
   buf_len -= buf_pos;
   buf_pos = 0;
}

int main(int argc, char **argv)
{
   char *obj_id;

   if (argc != 6)
      usage(argv[0]);

   if (chdir(argv[1]) == -1)
      return 1;

   if ((block_lines = strtol(argv[2], NULL, 10)) == LONG_MAX)
      usage(argv[0]);

   if (strlen(argv[3]) != 2)
      usage(argv[0]);

   prefix = argv[3];
   obj_id = argv[4];

   if (!strcmp(argv[5], "lines"))
      type = TYPE_LINES;
   else if (!strcmp(argv[5], "objs"))
      type = TYPE_OBJS;
   else if (!strcmp(argv[5], "no_json"))
      type = TYPE_NO_JSON;
   else
      usage(argv[0]);

   in_fd = 0;
   lno = lo_lno = 1;
   init_hash(&hw);

   if (type != TYPE_NO_JSON)
      printf("%s [\n", json_pre);

   while(1) {
      int ret = read_block();
      write_block();
      if (ret)
         break;
   }

   if (type != TYPE_NO_JSON) {
      printf("]%s", json_post);
      fwrite_obj_last(stdout, obj_id, lno);
   }

   return 0;

#if 0

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
#endif
}
