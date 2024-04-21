#include "pathtrace.h"
#include "pt_math.hpp"

vec_t normal_shape_point(
  shape_t shape,
  vec_t point
) {
  switch (shape.type) {
    case SHAPETYPE_PLANE:
      return shape.coords[1];
    case SHAPETYPE_SPHERE:
      return normalize(sub(point, shape.coords[0]));
    default:
      return {0,0,0};
  }
}

void intersect_ray_shape(
  ray_t *ray,
  shape_t *shape,
  intersect_t *intersection,
  int scene_index
) {
  fp_t dir_dot_norm;
  vec_t traversed;
  vec_t diff, edge1, edge2, ray_cross_edge2, s, s_cross_edge1;
  fp_t t, dist, det, inv_det, u, v;
  switch (shape->type) {
    case SHAPETYPE_PLANE:
      dir_dot_norm = dot(ray->direction, shape->coords[1]);
      if (hls::abs(dir_dot_norm) < 0.0001) {
        return;
      }
      diff = sub(shape->coords[0], ray->origin);
      t = dot(diff, shape->coords[1]) / dir_dot_norm;
      if (t < 0) {
        return;
      }
      traversed = scale(ray->direction, t);
      dist = dot(traversed, traversed); // could be magnitude but this avoids a square root

      if (dist > intersection->dist || dist < 0.00001) // If too close, or not nearer than current best, break.
        return;

      intersection->scene_index = scene_index;
      intersection->pt = add(ray->origin, traversed);
      intersection->dist = dist;
      return;

    case SHAPETYPE_TRI:
      edge1 = sub(shape->coords[1], shape->coords[0]);
      edge2 = sub(shape->coords[2], shape->coords[0]);
      ray_cross_edge2 = cross(ray->direction, edge2);
      det = dot(edge1, ray_cross_edge2);
      if (hls::abs(det) < 0.0001) {
        return;
      }
      inv_det = (fp_t) 1.0 / det;
      s = sub(ray->origin, shape->coords[0]);
      u = inv_det * dot(s, ray_cross_edge2);
      if (u < 0 || u > 1)
        return;
      s_cross_edge1 = cross(s, edge1);
      v = inv_det * dot(ray->direction, s_cross_edge1);
      if (v < 0 || u + v > 1)
        return;
      t = inv_det * dot(edge2, s_cross_edge1);
      if (t < 0.00001)
        return;
      traversed = scale(ray->direction, t);
      dist = dot(traversed, traversed); // could be magnitude but this avoids a square root

      if (dist > intersection->dist || dist < 0.00001) // If too close, or not nearer than current best, break.
        return;

      intersection->scene_index = scene_index;
      intersection->pt = add(ray->origin, traversed);
      intersection->dist = dist;

//    case SHAPETYPE_SPHERE:
//      a = dot(ray->direction, ray->direction);
//      b = 2 * dot(ray->direction, sub(ray->origin, shape->coords[0]));
//      c = dot(sub(ray->origin, shape->coords[0]), sub(ray->origin, shape->coords[0])) - hls::exp2(shape->coords[1].x);
//      discrim = hls::exp2(b) - (4 * a * c);
//      if (discrim < 0)
//        return;
//      t1 = (-1 * b + hls::sqrt(discrim)) / (2 * a);
//      t2 = (-1 * b - hls::sqrt(discrim)) / (2 * a);
//      if (t1 < 0 && t2 < 0)
//        return;
//      t = hls::min(t1, t2);
//      traversed = scale(ray->direction, t);
//      dist = dot(traversed, traversed); // could be magnitude but this avoids a square root
//
//      if (dist > intersection->dist || dist < 0.00001) // If too close, or not nearer than current best, break.
//        return;
//
//      intersection->scene_index = scene_index;
//      intersection->pt = add(ray->origin, traversed);
//      intersection->dist = dist;
//      return;
    default:
      break;
  }
}

void test_rays_against_obj(
  ray_t rays[NUM_PARALLEL],
  intersect_t intersects[NUM_PARALLEL],
  shape_t obj,
  int scene_index
) {

  // Cast each ray against the given shape
  // If hit, update the intersection if closer
Loop_Over_Rays:
  for (int i = 0; i < NUM_PARALLEL; i++) {
#pragma HLS UNROLL factor=8
#pragma HLS LOOP_FLATTEN off
    intersect_ray_shape(rays + i, &obj, intersects + i, scene_index);
  }
}


void raycast(
  ray_stream &rays_in,
  shape_t scene[MAX_SCENE_OBJECTS],
  rayhit_stream &rayhits_out
) {
#pragma HLS INTERFACE axis port=rays_in
#pragma HLS INTERFACE mode=bram port=scene
#pragma HLS INTERFACE axis port=rayhits_out
#pragma HLS INTERFACE s_axilite port=return

  ray_t rays[NUM_PARALLEL];
  intersect_t intersects[NUM_PARALLEL];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=rays
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=intersects

  int i = 0;
Main_Loop:
  while (1) {
    // Read in a ray to process, put it in a partitioned array
    ray_data tmp_ray;
    rays_in.read(tmp_ray);
    rays[i] = tmp_ray.data;

    i = i + 1;

    // Process a batch, write to output stream
    if (i == NUM_PARALLEL) {
      i = 0;

      // Reset intersections.
Intersect_Reset_Loop:
      for (int j = 0; j < NUM_PARALLEL; j++) {
        intersects[j].dist = 999999;
      }

Loop_Over_Scene:
      for (int j = 0; j < MAX_SCENE_OBJECTS; j++) {
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE off
        test_rays_against_obj(rays, intersects, scene[j], j + 1); // Scene index results will be 1 indexed so that non-hits are 0.
      }

      for (int j = 0; j < NUM_PARALLEL; j++) {
        rayhit_data tmp_rayhit;
        tmp_rayhit.data = { intersects[j].pt, intersects[j].scene_index };
        tmp_rayhit.dest = tmp_ray.dest;
        tmp_rayhit.id = tmp_ray.id;
        tmp_rayhit.keep = tmp_ray.keep;
        tmp_rayhit.last = (j == (NUM_PARALLEL - 1)) ? tmp_ray.last : (ap_uint<1>)0;

        rayhits_out.write(tmp_rayhit);
      }
    }

    if (tmp_ray.last) {
      break;
    }
  }

}
