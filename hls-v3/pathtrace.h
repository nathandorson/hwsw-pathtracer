#include <stdint.h>
#include "ap_fixed.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"
#include "cstring"

#ifndef PATHTRACE_H_
#define PATHTRACE_H_

#define MAX_SCENE_OBJECTS 16

#define NUM_PARALLEL 16

#define WIDTH  480
#define HEIGHT 1

#define SHAPETYPE_PLANE    0
#define SHAPETYPE_SPHERE   1
#define SHAPETYPE_TRI      2
#define SHAPETYPE_NOTHING  3

//typedef ap_fixed<32, 16> fp_t;
typedef float fp_t;

typedef struct {
  fp_t x;
  fp_t y;
  fp_t z;
} vec_t;

typedef struct __attribute__((packed)) {
  vec_t origin;
  vec_t direction;
  fp_t _pad[2];
} ray_t;

typedef hls::axis<ray_t, 1, 1, 1> ray_data;
typedef hls::stream<ray_data> ray_stream;

typedef struct {
  vec_t loc;
  int scene_index;
} rayhit_t;

typedef hls::axis<rayhit_t, 1, 1, 1> rayhit_data;
typedef hls::stream<rayhit_data> rayhit_stream;

//typedef ap_uint<2> shapetype;
typedef uint8_t shapetype;

typedef struct {
  vec_t coords[3];
  shapetype type;
} shape_t;

typedef struct {
  vec_t pt;
  int scene_index;
  fp_t dist;
} intersect_t;

extern void raycast(
  ray_stream &rays_in,
  shape_t scene[MAX_SCENE_OBJECTS],
  rayhit_stream &rayhits_out
);

#endif
