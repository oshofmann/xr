#ifndef XR_COMMON_H
#define XR_COMMON_H

#include <openssl/sha.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <xr_config.h>

typedef struct {
   unsigned char d[SHA_DIGEST_LENGTH];
} md_t;

struct hash_window {
   int pos;
   md_t hashes[HASH_WINDOW - 1];
};

static void init_hash(struct hash_window *w)
{
   memset(w, 0, sizeof(*w));
}

static md_t *next_hash(const char *str, struct hash_window *w)
{
   SHA_CTX sc;
   unsigned char md[SHA_DIGEST_LENGTH];
   static md_t wnd_hash;

   SHA1_Init(&sc);
   SHA1_Update(&sc, str, strlen(str));

   /* pos contains the oldest hash */
   int pos = w->pos;
   SHA1_Update(&sc, &w->hashes[pos], (HASH_WINDOW - 1 - pos)*sizeof(md_t));
   if (pos)
      SHA1_Update(&sc, &w->hashes[0], pos*sizeof(md_t));

   /* Put the new hash in place and add to the window */
   SHA1(str, strlen(str), w->hashes[pos].d);
   SHA1_Update(&sc, &w->hashes[pos], sizeof(md_t));

   SHA1_Final(wnd_hash.d, &sc);

   w->pos++;
   if (w->pos == sizeof(w->hashes)/sizeof(w->hashes[0]))
      w->pos = 0;

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

static int rename_obj(int fd, char oldpath[], const char prefix[],
      char newpath[], md_t *md)
{
   int i;
   int pos = sprintf(newpath, "%s", prefix);
   sprintf(&newpath[pos], "%02hhx", md->d[0]);
   pos += 2;
   newpath[pos++] = '/';
   for (i = 1; i < SHA_DIGEST_LENGTH; i++, pos += 2)
      sprintf(&newpath[pos], "%02hhx", md->d[i]);
   sprintf(&newpath[pos], block_ext);

   for (i = 0; i < pos; i++) {
      if (newpath[i] == '/') {
         newpath[i] = 0;
         if (mkdir(newpath, 0755) == -1 && errno != EEXIST)
            errdie("mkdir");
         newpath[i] = '/';
      }
   }

   if (rename(oldpath, newpath) == -1)
      return -1;

   if (fchmod(fd, 0644) == -1)
      return -1;
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

#endif
