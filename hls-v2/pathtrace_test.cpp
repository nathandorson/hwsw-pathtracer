#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pathtrace.h"

int main () {
  FILE *fp;
  fp=fopen("out.dat","w");

  ray_t *rays = (ray_t*) malloc(sizeof(ray_t) * WIDTH*HEIGHT);
  shape_t scene[MAX_SCENE_OBJECTS];
  color_t *pixels = (color_t*) malloc(sizeof(color_t) * WIDTH*HEIGHT);

  rays[0] = {{0,0,0}, {0,1,0}}; // Positive y

  for (int i = 0; i < MAX_SCENE_OBJECTS; i++)
	  scene[i].type = SHAPETYPE_ENDSCENE;

  scene[0].coords[0] = {0, 1, 0};
  scene[0].coords[1] = {0, 1, 0};
  scene[0].coords[2] = {0, 1, 0};
  scene[0].color = {255, 100, 255};
  scene[0].emittance = 0;
  scene[0].type = SHAPETYPE_PLANE;

  scene[1].coords[0] = {0, -1, 0}; // behind camera
  scene[1].coords[1] = {0, 1, 0};
  scene[1].coords[2] = {0, 1, 0};
  scene[1].color = {255, 255, 0};
  scene[1].emittance = 1;
  scene[1].type = SHAPETYPE_PLANE;

  printf("tracing scene...\n");
  pathtrace(rays, scene, pixels, 4, 1);

  fprintf(fp,"First pixel is: [%d %d %d]\n",(int)pixels[0].x, (int)pixels[0].y, (int)pixels[0].z);

  fclose(fp);

  
  









  printf ("Comparing against output data \n");
  if (system("diff -w out.dat out.gold.dat")) {
    fprintf(stdout, "*******************************************\n");
    fprintf(stdout, "FAIL: Output DOES NOT match the golden output\n");
    fprintf(stdout, "*******************************************\n");
    return 0;
  } else {
    fprintf(stdout, "*******************************************\n");
    fprintf(stdout, "PASS: The output matches the golden output!\n");
    fprintf(stdout, "*******************************************\n");
    return 0;
  }
}

