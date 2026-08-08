#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "window.h"
#include "frame.h"


Window *Chess_new(int window_x, int window_y)
{
   Window *frame = malloc(sizeof *frame);
   Window *w = Frame_init(frame, window_x, -1, window_y, -1, 8*3, 9, NULL, 0);

   int brown_cells = 94;
   int white_cells = 231;

   for (int i = 0 ; i < 8; i++){
      for (int j = 0 ; j < 8; j++){
         char * piece = " ";
         int bg = brown_cells;
         if ((i+j)%2==0) bg = white_cells;
         if (j == 0){
            if (i == 0 || i == 7) piece = " ♜";
            if (i == 1 || i == 6) piece = " ♞";
            if (i == 2 || i == 5) piece = " ♝";
            if (i == 3) piece = " ♛";
            if (i == 4) piece = " ♚";
         } 
         if (j == 1) piece = " ♟";
         if (j == 6) piece = " ♙";
         if (j == 7){
            if (i == 0 || i == 7) piece = " ♖";
            if (i == 1 || i == 6) piece = " ♘";
            if (i == 2 || i == 5) piece = " ♗";
            if (i == 3) piece = " ♔";
            if (i == 4) piece = " ♕";
         } 
         Window_add_widget(w, i*3, -1, j, -1, 3, 1, piece, 232, bg);
      }
   }

   return frame;
}
