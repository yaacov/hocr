/***************************************************************************
 *            main.c
 *
 *  Fri Aug 12 20:13:33 2005
 *  Copyright  2005-2007  Yaacov Zamir
 *  <kzamir@walla.co.il>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
 */

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "hocr.h"
#include "ho_gtk.h"

gchar *image_in_filename = NULL;
gchar *text_out_filename = NULL;
gchar *image_out_path = NULL;
gboolean version = FALSE;
gboolean debug = FALSE;
gboolean no_gtk = FALSE;

gint threshold = 0;
gint adaptive_threshold = 0;
gint adaptive_threshold_type = 0;
gint scale_by = 0;
gboolean remove_dots = FALSE;
gboolean remove_images = FALSE;

gint paragraph_setup = 0;
gboolean show_grid = FALSE;
gboolean save_bw = FALSE;
gboolean save_layout = FALSE;

gboolean only_image_proccesing = FALSE;
gboolean only_layout_analysis = FALSE;

gint spacing_code = 0;
gint lang_code = 0;
gint font_code = 0;

GError *error = NULL;

/* black and white text image */
ho_bitmap *m_page_text = NULL;

/* text layout */
ho_layout *l_page = NULL;

/* text for output */
gchar *text_out = NULL;

static gchar *copyright_message = "hocr - Hebrew OCR utility\n\
%s\n\
http://hocr.berlios.de\n\
Copyright (C) 2005-2007 Yaacov Zamir <kzamir@walla.co.il>\n\
\n\
This program is free software; you can redistribute it and/or modify\n\
it under the terms of the GNU General Public License as published by\n\
the Free Software Foundation; either version 2 of the License, or\n\
(at your option) any later version.\n\
\n\
This program is distributed in the hope that it will be useful,\n\
but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
GNU General Public License for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.\n";

static GOptionEntry entries[] = {
  {"image-in", 'i', 0, G_OPTION_ARG_FILENAME, &image_in_filename,
   "use FILE as input image file name", "FILE"},
  {"text-out", 'o', 0, G_OPTION_ARG_FILENAME, &text_out_filename,
   "use FILE as output text file name", "FILE"},
  {"images-out-path", 'O', 0, G_OPTION_ARG_FILENAME, &image_out_path,
   "use PATH for output images", "PATH"},
  {"thresholding-type", 'a', 0, G_OPTION_ARG_INT,
   &adaptive_threshold_type,
   "thresholding type, 0 normal, 1 none, 2 fine",
   "NUM"},
  {"threshold", 't', 0, G_OPTION_ARG_INT, &threshold,
   "use NUM as threshold value, 1..100",
   "NUM"},
  {"adaptive-threshold", 'T', 0, G_OPTION_ARG_INT, &adaptive_threshold,
   "use NUM as adaptive threshold value, 1..100",
   "NUM"},
  {"scale", 's', 0, G_OPTION_ARG_INT, &scale_by,
   "scale input image by SCALE 1..9, 0 auto",
   "SCALE"},
  {"remove-halfton", 'r', 0, G_OPTION_ARG_NONE, &remove_dots,
   "remove halfton dots from input image",
   NULL},
  {"remove-images", 'R', 0, G_OPTION_ARG_NONE, &remove_images,
   "remove images from input image",
   NULL},
  {"colums setup", 'c', 0, G_OPTION_ARG_INT, &paragraph_setup,
   "colums setup: 1 free, 2.. #colums, 0 auto",
   "NUM"},
  {"draw-grid", 'g', 0, G_OPTION_ARG_NONE, &show_grid,
   "draw grid on output images", NULL},
  {"save-bw", 'b', 0, G_OPTION_ARG_NONE, &save_bw,
   "save proccesd bw image", NULL},
  {"save-bw-exit", 'B', 0, G_OPTION_ARG_NONE, &only_image_proccesing,
   "save proccesd bw image and exit", NULL},
  {"save-layout", 'l', 0, G_OPTION_ARG_NONE, &save_layout,
   "save layout image", NULL},
  {"save-layout-exit", 'L', 0, G_OPTION_ARG_NONE, &only_layout_analysis,
   "save layout image and exit", NULL},
  {"no-gtk", 'n', 0, G_OPTION_ARG_NONE, &no_gtk,
   "do not use gtk for file input and output", NULL},
  {"debug", 'd', 0, G_OPTION_ARG_NONE, &debug,
   "print debuging information while running", NULL},
  {"version", 'v', 0, G_OPTION_ARG_NONE, &version,
   "print version information and exit", NULL},
  {NULL}
};

int
hocr_printerr (const char *msg)
{
  g_printerr ("%s: %s\n\
Try `--help' for more information.\n", g_get_prgname (), msg);
  return TRUE;
}

int
hocr_cmd_parser (int *argc, char **argv[])
{
  GOptionContext *context;

  /* get user args */
  context = g_option_context_new ("- Hebrew OCR utility.");

  g_option_context_add_main_entries (context, entries, PACKAGE_NAME);
  g_option_context_parse (context, argc, argv, &error);

  /* free allocated memory */
  g_option_context_free (context);

  if (error)
    {
      hocr_printerr ("unrecognized option");
      exit (1);
    }

  if (!no_gtk && !image_in_filename)
    {
      hocr_printerr ("no input image file name (use -n for pipeing)");
      exit (1);
    }

  if (version)
    {
      g_printf (copyright_message, BUILD);
      exit (0);
    }

  if (debug)
    g_print ("%s - Hebrew OCR utility\n", PACKAGE_STRING);

  /* sanity check */
  if (adaptive_threshold_type > 2 || adaptive_threshold_type < 0)
    {
      hocr_printerr ("unknown thresholding type using normal settings");
      adaptive_threshold_type = 0;
    }

  if (scale_by > 9 || scale_by < 0)
    {
      hocr_printerr ("unknown scaling factor using auto settings");
      scale_by = 0;
    }

  if (paragraph_setup > 4 || paragraph_setup < 0)
    {
      hocr_printerr ("unknown paragraph setup using auto settings");
      paragraph_setup = 0;
    }

  if (threshold > 100 || threshold < 0)
    {
      hocr_printerr ("unknown thresholding value using auto settings");
      threshold = 0;
    }

  if (adaptive_threshold > 100 || adaptive_threshold < 0)
    {
      hocr_printerr ("unknown thresholding value using auto settings");
      adaptive_threshold = 0;
    }

  return FALSE;
}

/* FIXME: this function use globals */
ho_bitmap *
hocr_load_input_bitmap ()
{
  ho_pixbuf *pix = NULL;
  ho_bitmap *m_bw = NULL;

  ho_bitmap *m_bw_temp = NULL;
  ho_bitmap *m_bw_temp2 = NULL;

  /* read image from file */
  if (no_gtk)
    pix = ho_pixbuf_pnm_load (image_in_filename);
  else
    pix = ho_gtk_pixbuf_load (image_in_filename);

  if (!pix)
    {
      hocr_printerr ("can't read input image\n");
      exit (1);
    }

  if (debug)
    g_print (" input image is %d by %d pixels, with %d color channels\n",
	     pix->width, pix->height, pix->n_channels);

  m_bw =
    ho_pixbuf_to_bitmap_wrapper (pix, scale_by, adaptive_threshold_type,
				 threshold, adaptive_threshold);

  if (!m_bw)
    {
      hocr_printerr ("can't convert to black and white\n");
      exit (1);
    }

  /* prompt user scale */
  if (debug && scale_by)
    if (scale_by == 1)
      g_print (" user no scaling.\n", scale_by);
    else
      g_print (" user scale by %d.\n", scale_by);

  /* do we need to auto scale ? */
  if (!scale_by)
    {
      /* get fonts size for autoscale */
      if (ho_dimentions_font_width_height_nikud (m_bw, 6, 200, 6, 200))
	{
	  hocr_printerr ("can't create object map\n");
	  exit (1);
	}

      /* if fonts are too small and user wants auto scale, re-scale image */
      if (m_bw->font_height < 10)
	scale_by = 4;
      else if (m_bw->font_height < 25)
	scale_by = 2;
      else
	scale_by = 1;

      if (debug)
	if (scale_by > 1)
	  g_print (" auto scale by %d.\n", scale_by);
	else
	  g_print (" no auto scale neaded.\n");

      if (scale_by > 1)
	{
	  /* re-create bitmap */
	  ho_bitmap_free (m_bw);
	  m_bw =
	    ho_pixbuf_to_bitmap_wrapper (pix, scale_by,
					 adaptive_threshold_type, threshold,
					 adaptive_threshold);

	  if (!m_bw)
	    {
	      hocr_printerr
		("can't convert to black and white after auto scaleing \n");
	      exit (1);
	    }

	}
    }

  /* remove halfton dots  
     this also imply that input image is b/w */
  if (remove_dots)
    {
      if (debug)
	g_print (" remove halfton dots.\n");

      m_bw_temp = ho_bitmap_filter_remove_dots (m_bw, 4, 4);
      if (m_bw_temp)
	{
	  ho_bitmap_free (m_bw);
	  m_bw = m_bw_temp;
	  m_bw_temp = NULL;
	}
    }

  /* remove too big and too small objects */
  if (remove_images)
    {
      if (debug)
	g_print (" remove images.\n");

      /* adaptive threshold braks big objects use adaptive_threshold_type = 1 (none) */
      m_bw_temp = ho_pixbuf_to_bitmap_wrapper (pix, scale_by,
					       1, threshold,
					       adaptive_threshold);
      if (!m_bw_temp)
	{
	  hocr_printerr
	    ("can't convert to black and white for image removal \n");
	  exit (1);
	}

      /* get fonts size for autoscale */
      if (ho_dimentions_font_width_height_nikud (m_bw, 6, 200, 6, 200))
	{
	  hocr_printerr ("can't create object map\n");
	  exit (1);
	}

      /* remove big objects */
      m_bw_temp2 = ho_bitmap_filter_by_size (m_bw_temp,
					     m_bw->font_height * 4,
					     m_bw_temp->height,
					     m_bw->font_width * 9,
					     m_bw_temp->width);

      if (!m_bw_temp2)
	{
	  hocr_printerr
	    ("can't convert to black and white for image removal \n");
	  exit (1);
	}

      ho_bitmap_andnot (m_bw, m_bw_temp2);
      ho_bitmap_free (m_bw_temp2);

      /* remove small objects */
      m_bw_temp2 = ho_bitmap_filter_by_size (m_bw_temp,
					     1,
					     m_bw->font_height / 5,
					     1, m_bw->font_width / 8);
      ho_bitmap_free (m_bw_temp);

      if (!m_bw_temp2)
	{
	  hocr_printerr
	    ("can't convert to black and white for image removal \n");
	  exit (1);
	}

      ho_bitmap_andnot (m_bw, m_bw_temp2);
      ho_bitmap_free (m_bw_temp2);
    }

  /* free input pixbuf */
  ho_pixbuf_free (pix);

  return m_bw;
}

/* FIXME: this functions use globals */

int
hocr_exit ()
{
  int block_index;
  int line_index;

  /* allert user */
  if (debug)
    g_print ("free memory.\n");

  /* free image matrixes */
  ho_bitmap_free (m_page_text);

  /* free page layout  masks */
  ho_layout_free (l_page);

  /* free text */
  if (text_out)
    g_free (text_out);

  /* free file names */
  if (image_in_filename)
    g_free (image_in_filename);
  if (image_out_path)
    g_free (image_out_path);
  if (text_out_filename)
    g_free (text_out_filename);

  /* exit program */
  exit (0);

  return FALSE;
}

int
main (int argc, char *argv[])
{
  /* start of argument analyzing section 
   */

  hocr_cmd_parser (&argc, &argv);

  /* init gtk */
  if (!no_gtk)
    gtk_init (&argc, &argv);

  /* end of argument analyzing section */

  /* start of image proccesing section 
   */
  if (debug)
    g_print ("start image proccesing.\n");

  /* load the image from input filename */
  m_page_text = hocr_load_input_bitmap ();

  if (debug)
    if (scale_by > 1)
      g_print (" procced image is %d by %d pixels (origianl scaled by %d)\n",
	       m_page_text->width, m_page_text->height, scale_by);
    else
      g_print (" procced image is %d by %d pixels\n",
	       m_page_text->width, m_page_text->height);

  /* if only image proccesing or save b/w image */
  if (save_bw || only_image_proccesing)
    {
      gchar *filename;
      ho_pixbuf *pix_out;

      pix_out = ho_pixbuf_new_from_bitmap (m_page_text);

      if (show_grid)
	ho_pixbuf_draw_grid (pix_out, 120, 30, 0, 0, 0);

      /* create file name */
      if (no_gtk)
	filename = g_strdup_printf ("%s-I.pgm", image_out_path);
      else
	filename = g_strdup_printf ("%s-I.jpeg", image_out_path);

      /* save to file system */
      if (filename)
	{
	  if (no_gtk)
	    ho_pixbuf_pnm_save (pix_out, filename);
	  else
	    ho_gtk_pixbuf_save (pix_out, filename);

	  /* free locale memory */
	  ho_pixbuf_free (pix_out);
	  g_free (filename);
	}
    }

  if (debug)
    g_print ("end image proccesing.\n");

  /* if user want save the b/w picture image proccesing produced */
  if (only_image_proccesing)
    hocr_exit ();

  /* end of image proccesing section */
  /* remember: by now you have allocated and not freed: m_page_text */

  /* start of layout analyzing section
   */
  if (debug)
    g_print ("start image layout analysis.\n");

  l_page = ho_layout_new (m_page_text, paragraph_setup != 1);

  ho_layout_create_block_mask (l_page);

  if (debug)
    g_print ("  found %d blocks.\n", l_page->n_blocks);

  /* look for lines inside blocks */
  {
    int block_index;
    int line_index;

    for (block_index = 0; block_index < l_page->n_blocks; block_index++)
      {
	if (debug)
	  g_print ("  analyzing block %d.\n", block_index + 1);
	ho_layout_create_line_mask (l_page, block_index);

	if (debug)
	  g_print
	    ("    found %d lines. font height %d width %d, line spacing %d\n",
	     l_page->n_lines[block_index],
	     l_page->m_blocks_text[block_index]->font_height,
	     l_page->m_blocks_text[block_index]->font_width,
	     l_page->m_blocks_text[block_index]->line_spacing);

	/* look for words inside line */
	for (line_index = 0; line_index < l_page->n_lines[block_index];
	     line_index++)
	  {
	    if (debug)
	      g_print ("      analyzing line %d.\n", line_index + 1);
	    ho_layout_create_word_mask (l_page, block_index, line_index);

	    if (debug)
	      g_print ("        found %d words. font spacing %d\n",
		       l_page->n_words[block_index][line_index],
		       l_page->m_lines_text[block_index][line_index]->
		       font_spacing);
	  }
      }
  }

  if (debug)
    g_print ("end of image layout analysis.\n");

  /* if only layout analysis or save layout image */
  if (save_layout || only_layout_analysis)
    {
      gchar *filename;
      ho_pixbuf *pix_out;
      ho_bitmap *m_block_frame;
      int block_index;
      int line_index;

      /* allocate */
      pix_out = ho_pixbuf_new (3, m_page_text->width, m_page_text->height, 0);

      /* add text blocks */
      m_block_frame = ho_bitmap_edge (l_page->m_page_blocks_mask, 5);
      ho_pixbuf_draw_bitmap (pix_out, m_block_frame, 0, 0, 255, 150);
      ho_bitmap_free (m_block_frame);

      /* loop on all text blocks */
      for (block_index = 0; block_index < l_page->n_blocks; block_index++)
	{
	  for (line_index = 0; line_index < l_page->n_lines[block_index];
	       line_index++)
	    {
	      ho_pixbuf_draw_bitmap (pix_out,
				     l_page->
				     m_lines_words_mask[block_index]
				     [line_index], 255, 240, 0, 180);

	      m_block_frame =
		ho_bitmap_edge (l_page->
				m_lines_line_mask[block_index][line_index],
				5);
	      ho_pixbuf_draw_bitmap (pix_out, m_block_frame, 255, 0, 0, 255);
	      ho_bitmap_free (m_block_frame);
	    }
	}

      /* add grid */
      if (show_grid)
	ho_pixbuf_draw_grid (pix_out, 120, 30, 255, 0, 0);

      /* add text in black */
      ho_pixbuf_draw_bitmap (pix_out, m_page_text, 0, 0, 0, 255);

      /* create file name */
      if (no_gtk)
	filename = g_strdup_printf ("%s-L.pgm", image_out_path);
      else
	filename = g_strdup_printf ("%s-L.jpeg", image_out_path);

      /* save to file system */
      if (filename)
	{
	  if (no_gtk)
	    ho_pixbuf_pnm_save (pix_out, filename);
	  else
	    ho_gtk_pixbuf_save (pix_out, filename);

	  ho_pixbuf_free (pix_out);
	  g_free (filename);
	}
    }

  /* if user only want layout image exit now */
  if (only_layout_analysis)
    hocr_exit ();

  /* just testing */
  text_out = g_strdup_printf ("Hi, testing one, two three ...");

  /* start user output section 
   */
  /* save text */
  if (text_out_filename && text_out)
    {
      /* if filename is '-' print to stdout */
      if (text_out_filename[0] == '-' && text_out_filename[1] == '\0')
	g_printf (text_out);
      else
	g_file_set_contents (text_out_filename, text_out, -1, &error);

      if (error)
	{
	  hocr_printerr ("can't write text to file");
	}
    }

  /* end of user putput section */

  /* start of memory cleanup section 
   */

  hocr_exit ();

  return FALSE;
}
