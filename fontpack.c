// Make binary files for fonts, from 8 bit grey scale source

#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

int
main (int argc, const char *argv[])
{
   int width = 0,
      height = 0,
      bpp = 0,
      pack = 0,
      debug = 0;
   const char *head = NULL;
   poptContext optCon;          // context for parsing command-line options
   {                            // POPT
      const struct poptOption optionsTable[] = {
         {"width", 'w', POPT_ARG_INT, &width, 0, "Width", "N"},
         {"height", 'h', POPT_ARG_INT, &height, 0, "Height", "N"},
         {"bpp", 'b', POPT_ARG_INT, &bpp, 0, "Target BPP", "N (<=8)"},
         {"head", 'H', POPT_ARG_STRING, &head, 0, "Header", "name"},
         {"pack", 'p', POPT_ARG_NONE, &pack, 0, "Pack"},
         {"debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug"},
         POPT_AUTOHELP {}
      };

      optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp (optCon, "8 bit grey files");

      int c;
      if ((c = poptGetNextOpt (optCon)) < -1)
         errx (1, "%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

      if (!poptPeekArg (optCon) || !width || !height || !bpp || bpp > 8)
      {
         poptPrintUsage (optCon, stderr, 0);
         return -1;
      }
      int line = (width * bpp + 7) / 8;
      int isize = width * height;
      int osize = line * height;
      if (debug)
         warnx ("Width %d (%d bytes, %d bpp), Height %d, File size %d", width, line, bpp, height, osize);
      unsigned char *buf = malloc (isize + 1);
      if (!buf)
         errx (1, "malloc");
      if (head)
         printf ("const unsigned char %s%s[]={", pack ? "* const " : "", head);
      printf ("// %dx%d (%d bpp), %d bytes per character\n", width, height, bpp, osize);
      if (pack)
         printf ("// packing bytes: 3 bits lx + 5 bits nx-1, 8 bits ly, 1 bit lx + 7 bits ny-1\n");
      const char *fn;
      while ((fn = poptGetArg (optCon)))
      {
         int f = open (fn, O_RDONLY);
         if (f < 1)
            err (1, "Cannot open %s", fn);
         size_t l = read (f, buf, isize + 1);
         close (f);
         if (l != isize)
         {
            warnx ("Bad file size %ld/%ld %s", l, isize, fn);
            continue;
         }
         if (bpp < 8)
         {                      // Pack
            unsigned char *i = buf,
               *o = buf;
            for (int y = 0; y < height; y++)
            {
               unsigned char v,
                 s = 0;
               for (int x = 0; x < width; x++)
               {
                  if (!s)
                  {
                     v = 0;
                     s = 8;
                  }
                  s -= bpp;
                  v |= ((*i++ >> (8 - bpp)) << s);
                  if (!s || x + 1 == width)
                     *o++ = v;
               }
            }
         }
         int lx = 0,
            hx = line - 1,
            ly = 0,
            hy = height - 1;
         if (pack)
         {                      // Pack
            lx = line;
            hx = 0;
            ly = height;
            hy = 0;
            for (int y = 0; y < height; y++)
               for (int x = 0; x < line; x++)
                  if (buf[y * line + x])
                  {
                     if (x < lx)
                        lx = x;
                     if (x > hx)
                        hx = x;
                     if (y < ly)
                        ly = y;
                     if (y > hy)
                        hy = y;
                  }
            if (lx > hx)
               printf ("NULL,			");
            else
            {
               if (lx > 15)
               {
                  warnx ("Lx %d>15 %s", lx, fn);
                  lx = 15;
               }
               if (hx - lx > 31)
                  errx (1, "Hx %d (hx-lx) %d>31 %s", hx, hx - lx, fn);
               if (hy - ly > 127)
                  errx (1, "Hy %d (hy-ly) %d>127 %s", hy, hy - ly, fn);
               printf ("(const unsigned char[]){0x%02X,0x%02X,0x%02X,", ((lx & 7) << 5) + (hx - lx), ly,
                       ((lx & 8) << 4) + (hy - ly));
            }
         }
         {                      // Name
            const char *s = strrchr (fn, '/');
            if (s)
               s++;
            else
               s = fn;
            const char *e = strrchr (s, '.');
            if (!e)
               e = s + strlen (s);
            char c = ' ';
            if (e - s == 5 && *s == 'u' && s[1] == '0' && s[2] == '0')
               c = (((isupper (s[3]) ? 9 : 0) + (s[3] & 0xF)) << 4) + (isupper (s[4]) ? 9 : 0) + (s[4] & 0xF);
            if (c < ' ' || c > 0x7F || c == '\\')
               c = ' ';
            printf ("	// %.*s %c\n", (int) (e - s), s, c);
         }
         // Output
         if (lx > hx)
            continue;
         for (int y = ly; y <= hy; y++)
         {
            for (int x = lx; x <= hx; x++)
            {
               printf ("0x%02X", buf[y * line + x]);
               if (x + 1 <= hx || y + 1 <= hy)
                  printf (",");
            }
            if (!pack || y + 1 <= hy)
               printf ("\n");
         }
         if (pack)
            printf ("},\n");
      }
      if (head)
         printf ("};\n");
      free (buf);
   }
   poptFreeContext (optCon);
   return 0;
}
