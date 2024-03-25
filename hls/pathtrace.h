#include <stdint.h>
#include "ap_fixed.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "cstring"

#ifndef PATHTRACE_H_
#define PATHTRACE_H_

#define MAX_SCENE_OBJECTS 16

#define WIDTH  480
#define HEIGHT 360

#define SHAPETYPE_PLANE    0
#define SHAPETYPE_SPHERE   1
#define SHAPETYPE_TRI      2
#define SHAPETYPE_ENDSCENE 3

//typedef ap_fixed<32, 16> fp_t;
typedef float fp_t;

typedef struct {
  fp_t x;
  fp_t y;
  fp_t z;
} vec_t;

typedef vec_t color_t;

typedef struct {
  vec_t origin;
  vec_t direction;
} ray_t;

//typedef ap_uint<2> shapetype;
typedef uint8_t shapetype;

typedef struct {
  vec_t coords[3];
  color_t color;
  uint8_t emittance;
  shapetype type;
} shape_t;

typedef struct {
  vec_t pt;
  shape_t shape;
  fp_t dist;
  bool hit;
} intersect_t;

extern void pathtrace (
  ray_t rays[WIDTH*HEIGHT],
  shape_t scene[16],
  color_t pixels[WIDTH*HEIGHT]
);

extern void trace_ray_stream (
  hls::stream<ap_axis<sizeof(ray_t),1,1,1>>& ray_stream,
  shape_t scene[MAX_SCENE_OBJECTS],
  hls::stream<ap_axis<sizeof(color_t),1,1,1>>& pixel_stream
);

#endif
