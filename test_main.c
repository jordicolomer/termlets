#include <assert.h>

int test_windows1()
{
  int grid[2][2] = {
      {1, 1},
      {1, 2}};
  log_init("app.log");
  Window *w1 = malloc(sizeof *w1);
  Window_init(w1, 0, -1, 0, -1, 2, 2);

  Window *w2 = malloc(sizeof *w2);
  Window_init(w2, 1, -1, 1, -1, 1, 1);
  Window_append(w1, w2);

  for (int i = 0; i <= 3; i++)
  {
    for (int j = 0; j <= 3; j++)
    {
      Geometry rect = {0, 0, w1->width, w1->height};
      Window *found = Window_find_widget(w1, rect, i, j);
      if (found)
      {
        int idx = 0;
        if (found == w1)
          idx = 1;
        if (found == w2)
          idx = 2;
        assert(idx == grid[j][i]);
        printf("found: %d %d %d %d\n", i, j, idx, grid[j][i]);
      }
    }
  }
}

int test_windows2()
{
  int grid[5][5] = {
      {2, 2, 2, 1, 1},
      {2, 3, 2, 1, 1},
      {2, 2, 2, 1, 1},
      {1, 1, 4, 4, 1},
      {1, 1, 4, 4, 1},
  };
  log_init("app.log");
  Window *w1 = malloc(sizeof *w1);
  Window_init(w1, 0, -1, 0, -1, 5, 5);

  Window *w2 = malloc(sizeof *w2);
  Window_init(w2, 0, -1, 0, -1, 3, 3);
  Window_append(w1, w2);

  Window *w3 = malloc(sizeof *w3);
  Window_init(w3, 1, -1, 1, -1, 1, 1);
  Window_append(w2, w3);

  Window *w4 = malloc(sizeof *w4);
  Window_init(w4, 2, -1, 3, -1, 2, 2);
  Window_append(w1, w4);

  for (int i = 0; i < 5; i++)
  {
    for (int j = 0; j < 5; j++)
    {
      Geometry rect = {0, 0, w1->width, w1->height};
      Window *found = Window_find_widget(w1, rect, i, j);
      if (found)
      {
        int idx = 0;
        if (found == w1)
          idx = 1;
        if (found == w2)
          idx = 2;
        if (found == w3)
          idx = 3;
        if (found == w4)
          idx = 4;
        assert(idx == grid[j][i]);
        printf("found: %d %d %d %d\n", i, j, idx, grid[j][i]);
        // printf("found: %d %d %d\n", i, j, idx);
      }
    }
  }
}