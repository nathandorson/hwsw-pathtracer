#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "pathtrace.h"

int main () {
  FILE *fp;
  fp=fopen("out.dat","w");

  ray_t *rays = (ray_t*) malloc(sizeof(ray_t) * NUM_PARALLEL);
  shape_t scene[MAX_SCENE_OBJECTS];
  rayhit_t rayhits[NUM_PARALLEL];

  rays[0] = {{0,0,0}, {0,1,0}}; // Towards positive y
  rays[1] = {{1,0,0}, {0,-1,0}}; // Towards negative y
  rays[2] = {{16,0,0}, {0,-1,0}}; // Towards negative y, but should miss the triangle

  for (int i = 0; i < MAX_SCENE_OBJECTS; i++)
	  scene[i].type = SHAPETYPE_NOTHING;

  scene[0].coords[0] = {0, 1, 0};
  scene[0].coords[1] = {0, 1, 0};
  scene[0].coords[2] = {0, 1, 0};
  scene[0].type = SHAPETYPE_PLANE;

  scene[1].coords[0] = {4, -1, 4}; // behind camera
  scene[1].coords[1] = {-4, -1, 4};
  scene[1].coords[2] = {0, -1, -4};
  scene[1].type = SHAPETYPE_TRI;

  ray_stream rays_in;

  for (int i = 0; i < 16; i++) {
    ray_data tmp = {0};
    tmp.data = rays[i];
    tmp.last = i == 15;
    rays_in.write(tmp);
  }

  rayhit_stream rayhits_out;

  printf("Tracing rays...\n");
  raycast(rays_in, scene, rayhits_out);

  for (int i = 0; i < 16; i++) {
    rayhit_data tmp = {0};
    rayhits_out.read(tmp);
    rayhits[i] = tmp.data;
  }

  fprintf(fp,"Ray 1 hit scene object: %d\n",(int)rayhits[0].scene_index);
  fprintf(fp,"Ray 2 hit scene object: %d\n",(int)rayhits[1].scene_index);
  fprintf(fp,"Ray 3 hit scene object: %d\n",(int)rayhits[2].scene_index);

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

