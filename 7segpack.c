// Generate packed 7seg data

#include <stdio.h>
#include <string.h>
#include <popt.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <err.h>

int debug = 0;

int
main (int argc, const char *argv[])
{
   int width = 700;
   int height = 900;
   poptContext optCon;          // context for parsing command-line options
   {                            // POPT
      const struct poptOption optionsTable[] = {
         {"width", 'w', POPT_ARG_INT, &width, 0, "Width", "pixels"},
         {"height", 'h', POPT_ARG_INT, &height, 0, "Height", "pixels"},
         {"debug", 'v', POPT_ARG_NONE, &debug, 0, "Debug"},
         POPT_AUTOHELP {}
      };

      optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
      //poptSetOtherOptionHelp (optCon, "");

      int c;
      if ((c = poptGetNextOpt (optCon)) < -1)
         errx (1, "%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

      int size = width * height;
      uint8_t *img = malloc (size);
      if (!img)
         errx (1, "malloc");
      memset (img, 0, size);
      uint8_t *buf = malloc (size);
      if (!buf)
         errx (1, "malloc");

      // Loading
      int l = 0;
      const char *fn;
      while ((fn = poptGetArg (optCon)))
      {
         l++;
         if (l == 17)
            errx (1, "Too many files");
         FILE *f = fopen (fn, "r");
         if (!f)
            err (1, "Not found %s", fn);
         if (fread (buf, 1, size, f) != size)
            errx (1, "Failed to load %s", fn);
         fclose (f);
         for (int i = 0; i < size; i++)
            if (buf[i] & 0x80)
            {
               if (img[i])
                  warnx ("overlap %d/%d at %d/%d", img[i], l, i % width, i / width);
               img[i] = l;
            }
      }
      // Packing / output
      printf ("// Packed 7seg image %dx%d\n", width, height);
      printf ("// Packing is lines, each has:-\n"       //
              "// - 4 bits number of runs in this line (can be 0 if all background)\n"  //
              "// - 4 bits number of additional lines the same as this\n"       //
              "// - number of runs, each run is:-\n"    //
              "// - 4 bits segment id\n"        //
              "// - 10 bits background run\n"   //
              "// - 10 bits segment run\n");
      printf ("const uint32_t width_7seg=%d;\n", width);
      printf ("const uint32_t height_7seg=%d;\n", height);
      printf ("uint8_t pack_7seg[]={\n");
      free (buf);
      uint8_t *i = img;
      uint8_t prev[15 * 3],
        prevp = 0;
      int y = 0,
         prevn = 0;
      int bytes = 0;
      void eol (void)
      {
         bytes += 1 + prevp;
         printf (" 0x%02X,", ((prevp / 3) << 4) | (prevn - 1));
         for (int i = 0; i < prevp; i++)
            printf ("0x%02X,", prev[i]);
         for (int i = prevp; i < 3 * 5; i++)
            printf ("     ");
         if (prevn == 1)
            printf (" // Row  %3d\n", y - 1);
         else
            printf (" // Rows %3d-%3d\n", y - prevn, y - 1);
      }
      for (y = 0; y < height; y++)
      {
         uint8_t line[15 * 3],
           linep = 0;
         int x = 0;
         while (x < width)
         {
            // Spaces
            int s = x;
            while (x < width && !*i)
            {
               i++;
               x++;
            }
            if (x < width)
            {
               // Segment
               int c = x;
               if (c - s >= 1024)
                  errx (1, "white space run %d row %d", c - s, y);
               l = *i;
               while (x < width && *i == l)
               {
                  i++;
                  x++;
               }
               if (x - c >= 1024)
                  errx (1, "segment run %d row %d", x - c, y);
               if (linep >= sizeof (line))
                  errx (1, "Too many runs row %d", y);
               line[linep++] = ((l - 1) << 4) + ((c - s) >> 6);
               line[linep++] = ((c - s) << 2) | ((x - c) >> 8);
               line[linep++] = x - c;
            }
         }
         // Check duplicate
         if (prevn < 16 && linep == prevp && !memcmp (line, prev, linep))
            prevn++;
         else
         {
            if (y)
               eol ();
            prevn = 1;
            memcpy (prev, line, prevp = linep);
         }
      }
      eol ();
      printf ("}; // %d bytes\n", bytes);
      free (img);
   }

   poptFreeContext (optCon);
   return 0;
}
