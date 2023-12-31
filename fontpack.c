// Make binary files

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
         {"bpp", 'b', POPT_ARG_INT, &bpp, 0, "Target BPP", "N"},
         {"head", 'H', POPT_ARG_STRING, &head, 0, "Header", "name"},
         {"pack", 'p', POPT_ARG_NONE, &pack, 0, "Pack"},
         {"debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug"},
         POPT_AUTOHELP {}
      };

      optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
      poptSetOtherOptionHelp (optCon, "grey/mono files");

      int c;
      if ((c = poptGetNextOpt (optCon)) < -1)
         errx (1, "%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

      if (!poptPeekArg (optCon) || !width || !height || !bpp)
      {
         poptPrintUsage (optCon, stderr, 0);
         return -1;
      }
      if (bpp != 1 && bpp != 4 && bpp != 8)
         errx (1, "Unhandled BPP %d", bpp);
      int widthb = ((bpp == 1) ? (width + 7) / 8 : width);
      int size = widthb * height;
      int widtho = (width * bpp + 7) / 8;
      int osize = widtho * height;
      if (debug)
         warnx ("Width %d (%d bytes, %d bpp), Height %d, File size %d, Output size %d (%d)", width, widthb, bpp, height, size,
                osize, widtho);
      unsigned char *buf = malloc (size + 1);
      if (!buf)
         errx (1, "malloc");
      if (head)
         printf ("const unsigned char %s%s[]={", pack ? "* const " : "", head);
      printf ("// %dx%d (%d bpp), %d bytes per character\n", width, height, bpp, osize);
      const char *fn;
      while ((fn = poptGetArg (optCon)))
      {
         {                      // Name
            const char *s = strrchr (fn, '/');
            if (s)
               s++;
            else
               s = fn;
            const char *e = strrchr (s, '.');
            if (!e)
               e = s + strlen (s);
            printf ("// %.*s\n", (int) (e - s), s);
         }
         int f = open (fn, O_RDONLY);
         if (f < 1)
            err (1, "Cannot open %s", fn);
         size_t l = read (f, buf, size + 1);
         close (f);
         if (l != size)
         {
            warnx ("Bad file size %ld/%ld %s", l, size, fn);
            continue;
         }
         if (bpp == 4)
         {                      // Back to bytes
            unsigned char *q = buf;
            for (int y = 0; y < height; y++)
               for (int x = 0; x < widthb; x += 2)
                  *q++ = (buf[y * widthb + x] & 0xF0) + (x + 1 < widthb ? buf[y * widthb + x + 1] >> 4 : 0);
         }
         int lx = 0,
            hx = widtho,
            ly = 0,
            hy = height;
         if (pack)
         {                      // Pack
            lx = widtho;
            hx = 0;
            ly = height;
            hy = 0;
            for (int y = 0; y < height; y++)
               for (int x = 0; x < widtho; x++)
                  if (buf[y * widtho + x])
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
            hx++;
            hy++;
            if (lx > hx)
               lx = hx = ly = hy = 0;
            printf ("(const unsigned char[]){%d,%d,%d,%d,\n", lx, hx, ly, hy);
         }
         // Output
         for (int y = ly; y < hy; y++)
         {
            for (int x = lx; x < hx; x++)
               printf ("0x%02X,", buf[y * widtho + x]);
            if (!pack || y + 1 < hy)
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
