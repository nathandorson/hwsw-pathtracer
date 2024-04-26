#include "pathtrace.h"
#include "pt_math.hpp"



vec_t to_vec(float_vec_t v)
{
  return { (fp_t)v.x, (fp_t)v.y, (fp_t)v.z };
}

float_vec_t to_float_vec(vec_t v)
{
  return { (float)v.x, (float)v.y, (float)v.z };
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
      dir_dot_norm = dot(ray->direction, to_vec(shape->coords[1]));
      if (dir_dot_norm < 0.0001 && dir_dot_norm > -0.0001) {
        return;
      }
      diff = sub(to_vec(shape->coords[0]), ray->origin);
      t = dot(diff, to_vec(shape->coords[1])) / dir_dot_norm;
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
      edge1 = sub(to_vec(shape->coords[1]), to_vec(shape->coords[0]));
      edge2 = sub(to_vec(shape->coords[2]), to_vec(shape->coords[0]));
      ray_cross_edge2 = cross(ray->direction, edge2);
      det = dot(edge1, ray_cross_edge2);
      if (det < 0.0001 && det > -0.0001) {
        return;
      }
      inv_det = (fp_t) 1.0 / det;
      s = sub(ray->origin, to_vec(shape->coords[0]));
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
  ray_t rays[BATCH_SIZE],
  intersect_t intersects[BATCH_SIZE],
  shape_t obj,
  int scene_index
) {

  // Cast each ray against the given shape
  // If hit, update the intersection if closer
Loop_Over_Rays:
  for (int i = 0; i < BATCH_SIZE; i++) {
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

  ray_t rays[BATCH_SIZE];
  intersect_t intersects[BATCH_SIZE];
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=rays
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=intersects

  int i = 0;
Main_Loop:
  while (1) {
    // Read in a ray to process, put it in a partitioned array
    ray_data tmp_ray;
    rays_in.read(tmp_ray);
    rays[i] = { to_vec(tmp_ray.data.origin), to_vec(tmp_ray.data.direction) };

    i = i + 1;

    // Process a batch, write to output stream
    if (i == BATCH_SIZE) {
      i = 0;

      // Reset intersections.
Intersect_Reset_Loop:
      for (int j = 0; j < BATCH_SIZE; j++) {
        intersects[j].dist = 255;
        intersects[j].scene_index = 0;
      }

Loop_Over_Scene:
      for (int j = 0; j < MAX_SCENE_OBJECTS; j++) {
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE II=202
        test_rays_against_obj(rays, intersects, scene[j], j + 1); // Scene index results will be 1 indexed so that non-hits are 0.
      }

      for (int j = 0; j < BATCH_SIZE; j++) {
        rayhit_data tmp_rayhit;
        tmp_rayhit.data = { to_float_vec(intersects[j].pt), intersects[j].scene_index };
        tmp_rayhit.dest = tmp_ray.dest;
        tmp_rayhit.id = tmp_ray.id;
        tmp_rayhit.keep = tmp_ray.keep;
        tmp_rayhit.last = (j == (BATCH_SIZE - 1)) ? tmp_ray.last : (ap_uint<1>)0;

        rayhits_out.write(tmp_rayhit);
      }
    }

    if (tmp_ray.last) {
      break;
    }
  }

}
