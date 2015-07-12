/*  Copyright 2013 Theo Berkau

    This file is part of YabauseUT

    YabauseUT is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    YabauseUT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lapetus; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "tests.h"

extern vdp2_settings_struct vdp2_settings;

void vdp2_nbg0_test ();
void vdp2_nbg1_test ();
void vdp2_nbg2_test ();
void vdp2_nbg3_test ();
void vdp2_rbg0_test ();
void vdp2_rbg1_test ();
void vdp2_window_test ();
void vdp2_all_scroll_test();

//////////////////////////////////////////////////////////////////////////////

void hline(int x1, int y1, int x2, u8 color)
{
   int i;
   volatile u8 *buf=(volatile u8 *)(0x25E00000+(y1 * 512));

   for (i = x1; i < (x2+1); i++)
      buf[i] = color;
}

//////////////////////////////////////////////////////////////////////////////

void vline(int x1, int y1, int y2, u8 color)
{
   int i;
   volatile u8 *buf=(volatile u8 *)(0x25E00000+(y1 * 512) + x1);

   for (i = 0; i < (y2-y1+1); i++)
      buf[i * 512] = color;
}

//////////////////////////////////////////////////////////////////////////////

void draw_box(int x1, int y1, int x2, int y2, u8 color)
{
   hline(x1, y1, x2, color); 
   hline(x1, y2, x2, color); 
   vline(x1, y1, y2, color);
   vline(x2, y1, y2, color);
}

//////////////////////////////////////////////////////////////////////////////

void working_query(const char *question)
{
   // Ask the user if it's visible
   vdp_printf(&test_disp_font, 2 * 8, 20 * 8, 0xF, "%s", question);
   vdp_printf(&test_disp_font, 2 * 8, 21 * 8, 0xF, "C - Yes B - No");

   for(;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_B)
      {
         stage_status = STAGESTAT_BADGRAPHICS;
         break;
      }
      else if (per[0].but_push_once & PAD_C)
      {
         stage_status = STAGESTAT_DONE;
         break;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_test()
{
   // Put system in minimalized state
   interrupt_set_level_mask(0xF);

   init_iapetus(RES_320x224);
   vdp_rbg0_init(&test_disp_settings);
   vdp_set_default_palette();

   // Display On
   vdp_disp_on();

   unregister_all_tests();
//   register_test(&Vdp2InterruptTest, "Sound Request Interrupt");
//   register_test(&Vdp2RBG0Test, "RBG0 bitmap");
   register_test(&vdp2_window_test, "Window test");
   do_tests("VDP2 Screen tests", 0, 0);
}

//////////////////////////////////////////////////////////////////////////////

void load_font_8x8_to_vdp2_vram_1bpp_to_4bpp(u32 tile_start_address)
{
   int x, y;
   int chr;
   volatile u8 *dest = (volatile u8 *)(VDP2_RAM + tile_start_address);

   for (chr = 0; chr < 128; chr++)//128 ascii chars total
   {
      for (y = 0; y < 8; y++)
      {
         u8 scanline = font_8x8[(chr * 8) + y];//get one scanline

         for (x = 0; x < 8; x++)
         {
            //get the corresponding bit for the x pos
            u8 bit = (scanline >> (x ^ 7)) & 1;

            if ((x & 1) == 0) 
               bit *= 16;
            
            dest[(chr * 32) + (y * 4) + (x / 2)] |= bit;
         }
      }
   }
}

//////////////////////////////////////////////////////////////////////////////

void write_str_as_pattern_name_data(int x_pos, int y_pos, const char* str, int palette, u32 base, u32 tile_start_address)
{
   int x;

   int len = strlen(str);

   for (x = 0; x < len; x++)
   {
      int name = str[x];

      int offset = x + x_pos;

      volatile u32 *p = (volatile u32 *)(VDP2_RAM + base);
      p[(y_pos * 32 * 2) + offset] = (tile_start_address >> 5) | name | (palette << 16);
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_scroll_test_set_scroll(int pos)
{
   //scroll the first two layers a little slower
   //than 1px per frame in the x direction
   *(volatile u32 *)0x25F80070 = pos << 15;
   VDP2_REG_SCYIN0 = pos;

   *(volatile u32 *)0x25F80080 = -(pos << 15);
   VDP2_REG_SCYIN1 = pos;

   VDP2_REG_SCXN2 = pos;
   VDP2_REG_SCYN2 = pos;

   VDP2_REG_SCXN3 = -pos;
   VDP2_REG_SCYN3 = pos;
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_scroll_test_set_map(screen_settings_struct* settings, int which)
{
   int i;

   for (i = 0; i < 4; i++)
   {
      settings->map[i] = which;
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_all_scroll_test()
{
   int i;
   
   vdp_rbg0_deinit();

   VDP2_REG_CYCA0U = 0x0123; // NBG0, NBG1, NBG2, NBG3 Pattern name data read 
   VDP2_REG_CYCB0U = 0x4567; // NBG0, NBG1, NBG2, NBG3 Character pattern read

   const u32 tile_address = 0x40000;

   load_font_8x8_to_vdp2_vram_1bpp_to_4bpp(tile_address);

   screen_settings_struct settings;

   settings.color = 0;
   settings.is_bitmap = 0;
   settings.char_size = 0;
   settings.pattern_name_size = 0;
   settings.plane_size = 0;
   vdp2_scroll_test_set_map(&settings, 0);
   settings.transparent_bit = 0;
   settings.map_offset = 0;
   vdp_nbg0_init(&settings);

   vdp2_scroll_test_set_map(&settings, 1);
   vdp_nbg1_init(&settings);

   vdp2_scroll_test_set_map(&settings, 2);
   vdp_nbg2_init(&settings);

   vdp2_scroll_test_set_map(&settings, 3);
   vdp_nbg3_init(&settings);

   vdp_set_default_palette();

   //set pattern name data to the empty font tile
   for (i = 0; i < 256; i++)
   {                                     
      write_str_as_pattern_name_data(0, i, "                                                                ", 0, 0x000000, tile_address);
   }

   for (i = 0; i < 64; i += 4)
   {
      write_str_as_pattern_name_data(0, i,     "A button: Start scrolling. NBG0. Testing NBG0. This is NBG0.... ", 3, 0x000000, tile_address);
      write_str_as_pattern_name_data(0, i + 1, "B button: Stop scrolling.  NBG1. Testing NBG1. This is NBG1.... ", 4, 0x004000, tile_address);
      write_str_as_pattern_name_data(0, i + 2, "C button: Reset.           NBG2. Testing NBG2. This is NBG2.... ", 5, 0x008000, tile_address);
      write_str_as_pattern_name_data(0, i + 3, "Start:    Exit.            NBG3. Testing NBG3. This is NBG3.... ", 6, 0x00C000, tile_address);
   }

   vdp_disp_on();

   int do_scroll = 0;
   int scroll_pos = 0;

   for (;;)
   {
      vdp_vsync();

      if (do_scroll)
      {
         scroll_pos++;
         vdp2_scroll_test_set_scroll(scroll_pos);
      }

      if (per[0].but_push_once & PAD_A)
      {
         do_scroll = 1;
      }

      if (per[0].but_push_once & PAD_B)
      {
         do_scroll = 0;
      }

      if (per[0].but_push_once & PAD_C)
      {
         do_scroll = 0;
         scroll_pos = 0;
         vdp2_scroll_test_set_scroll(scroll_pos);
      }

      if (per[0].but_push_once & PAD_START)
         break;
   }

   //restore settings so the menus don't break

   vdp_nbg0_deinit();
   vdp_nbg1_deinit();
   vdp_nbg2_deinit();
   vdp_nbg3_deinit();

   vdp_rbg0_init(&test_disp_settings);

   //clear the vram we used
   for (i = 0; i < 0x11000; i++)
   {
      vdp2_ram[i] = 0;
   }
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg0_test ()
{
   screen_settings_struct settings;

   // Draw a box on our default screen
   draw_box(120, 180, 80, 40, 15);

   // Setup NBG0 for drawing
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_256COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
//   settings.map_offset = 0;
//   settings.parameteraddr = 0x25E60000;
   vdp_nbg0_init(&settings);

   // Draw some stuff on the screen

   working_query("Is the above graphics displayed?");

   // Disable NBG0
   vdp_nbg0_deinit();
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg1_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg2_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_nbg3_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_rbg0_test ()
{
   // Draw a box on our default screen
   draw_box(120, 180, 80, 40, 15);

   // Draw some graphics on the RBG0 layer

   working_query("Is the above graphics displayed?");
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_rbg1_test ()
{
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_resolution_test ()
{
/*
   vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);

   // Display Main Menu
   for(;;)
   {
      vdp_vsync();

      if (per[0].but_push_once & PAD_A)
      {
         if ((vidmode & 0x7) == 7)
            vidmode &= 0xF0;
         else
            vidmode++;
         vdp_init(vidmode);
         vdp_rbg0_init(&testdispsettings);
         vdp_set_default_palette();
         vdp_set_font(SCREEN_RBG0, &test_disp_font, 1);
         vdp_disp_on();
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode =   ", vidmode);
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);
      }
      else if (per[0].but_push_once & PAD_B)
      {
         if ((vidmode & 0x30) == 0x30)
            vidmode &= 0xCF;
         else
            vidmode += 0x10;
         vdp_init(vidmode);
         vdp_rbg0_init(&testdispsettings);
         vdp_set_default_palette();
         vdp_set_font(SCREEN_RBG0, &test_disp_font, 1);
         vdp_disp_on();
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode =   ", vidmode);
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);
      }
      else if (per[0].but_push_once & PAD_C)
      {
         if ((vidmode & 0xC0) == 0xC0)
            vidmode &= 0x3F;
         else
            vidmode += 0x40;
         vdp_init(vidmode);
         vdp_rbg0_init(&testdispsettings);
         vdp_set_default_palette();
         vdp_set_font(SCREEN_RBG0, &test_disp_font, 1);
         vdp_disp_on();
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode =   ", vidmode);
         vdp_printf(&test_disp_font, 0 * 8, 2 * 8, 0xF, "vidmode = %02X", vidmode);
      }
   }
*/
}

//////////////////////////////////////////////////////////////////////////////

void vdp2_window_test ()
{
   screen_settings_struct settings;
   int dir=-1;
   int counter=320-1, counter2=224-1;
   int i;
   int nbg0_wnd;
   int nbg1_wnd;

   // Draw a box on our default screen
//   DrawBox(120, 180, 80, 40, 15);

   // Setup NBG0 for drawing
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_256COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
   settings.map_offset = (0x20000 >> 17);
//   settings.parameteraddr = 0x25E60000;
   vdp_nbg0_init(&settings);

   // Setup NBG1 for drawing
   settings.is_bitmap = TRUE;
   settings.bitmap_size = BG_BITMAP512x256;
   settings.transparent_bit = 0;
   settings.color = BG_256COLOR;
   settings.special_priority = 0;
   settings.special_color_calc = 0;
   settings.extra_palette_num = 0;
   settings.map_offset = (0x40000 >> 17);
//   settings.parameteraddr = 0x25E60000;
   vdp_nbg1_init(&settings);

   // Draw some stuff on the screen

   vdp_set_font(SCREEN_NBG0, &test_disp_font, 1);
   test_disp_font.out = (u8 *)0x25E20000;
   for (i = 5; i < 24; i+=2)
      vdp_printf(&test_disp_font, 0 * 8, i * 8, 0xB, "NBG0 NBG0 NBG0 NBG0 NBG0 NBG0 NBG0");
   vdp_set_font(SCREEN_NBG1, &test_disp_font, 1);
   test_disp_font.out = (u8 *)0x25E40000;
   for (i = 6; i < 24; i+=2)
      vdp_printf(&test_disp_font, 0 * 8, i * 8, 0xC, "NBG1 NBG1 NBG1 NBG1 NBG1 NBG1 NBG1");
   vdp_set_font(SCREEN_RBG0, &test_disp_font, 0);
   test_disp_font.out = (u8 *)0x25E00000;

   vdp_set_priority(SCREEN_NBG0, 2);
   vdp_set_priority(SCREEN_NBG1, 3);

   VDP2_REG_WPSX0 = 0;
   VDP2_REG_WPSY0 = 0;
   VDP2_REG_WPEX0 = counter << 1;
   VDP2_REG_WPEY0 = counter2;
   VDP2_REG_WPSX1 = ((320 - 40) / 2) << 1;
   VDP2_REG_WPSY1 = (224 - 40) / 2;
   VDP2_REG_WPEX1 = ((320 + 40) / 2) << 1;
   VDP2_REG_WPEY1 = (224 + 40) / 2;
   nbg0_wnd = 0x83; // enable outside of window 0 for nbg0
   nbg1_wnd = 0x88; // enable inside of window 1 for nbg1
   VDP2_REG_WCTLA = (nbg1_wnd << 8) | nbg0_wnd;        
   vdp_disp_on();

//   WorkingQuerry("Is the above graphics displayed?");
   for(;;)
   {
      vdp_vsync();

      if(dir > 0)
      {
         if (counter2 >= (224-1))
         {
            dir = -1;
            nbg0_wnd ^= 1;
         }
         else
         {
            counter2++;
            counter=counter2 * (320-1) / (224-1);
         }
      }
      else
      {
         if (counter2 <= 0)
         {
            dir = 1;
            nbg0_wnd ^= 1;
         }
         else
         {
            counter2--;
            counter=counter2 * (320-1) / (224-1);
         }
      }

      VDP2_REG_WPEX0 = counter << 1;
      VDP2_REG_WPEY0 = counter2;
      VDP2_REG_WCTLA = (nbg1_wnd << 8) | nbg0_wnd;

      vdp_printf(&test_disp_font, 0 * 8, 26 * 8, 0xC, "%03d %03d", counter, counter2);

      if (per[0].but_push_once & PAD_START)
         break;
   }

   // Disable NBG0/NBG1
   vdp_nbg0_deinit();
   vdp_nbg1_deinit();
}

//////////////////////////////////////////////////////////////////////////////

