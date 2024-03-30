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
  intersect_t *intersection
) {
  fp_t dir_dot_norm;
  vec_t traversed;
  vec_t diff;
  fp_t a, b, c, discrim, t1, t2, t, dist;
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
	  dist = magnitude(traversed); // TODO don't need the sqrt

	  if (dist > intersection->dist || dist < 0.00001) // If too close, or not nearer than current best, break.
		  break;

	  intersection->hit = true;
	  intersection->shape = *shape;
	  intersection->pt = add(ray->origin, traversed);
	  intersection->dist = dist;
	  break;
	case SHAPETYPE_SPHERE:
	  a = dot(ray->direction, ray->direction);
	  b = 2 * dot(ray->direction, sub(ray->origin, shape->coords[0]));
	  c = dot(sub(ray->origin, shape->coords[0]), sub(ray->origin, shape->coords[0])) - hls::exp2(shape->coords[1].x);
	  discrim = hls::exp2(b) - (4 * a * c);
	  if (discrim < 0)
		  break;
	  t1 = (-1 * b + hls::sqrt(discrim)) / (2 * a);
	  t2 = (-1 * b - hls::sqrt(discrim)) / (2 * a);
	  if (t1 < 0 && t2 < 0)
		  break;
	  t = hls::min(t1, t2);
	  traversed = scale(ray->direction, t);
	  dist = magnitude(traversed); // TODO don't need the sqrt

    if (dist > intersection->dist || dist < 0.00001) // If too close, or not nearer than current best, break.
      break;

	  intersection->hit = true;
	  intersection->shape = *shape;
	  intersection->pt = add(ray->origin, traversed);
	  intersection->dist = dist;
	  break;
	default:
	  break;
  }
}

void test_rays_against_obj(
  ray_t rays[WIDTH*HEIGHT],
  intersect_t intersections[WIDTH*HEIGHT],
  shape_t obj
) {

  // Cast each ray against the given shape
  // If hit, update the intersection if closer
Loop_Over_Rays:
  for (int i = 0; i < WIDTH*HEIGHT; i++) {
#pragma HLS UNROLL factor=4
    intersect_ray_shape(rays + i, &obj, intersections + i);
  }
}

void redirect_rays(
  ray_t rays[WIDTH*HEIGHT],
  intersect_t intersections[WIDTH*HEIGHT]
) {
Recast_Rays_Loop:
  for (int i = 0; i < WIDTH*HEIGHT; i++) {
#pragma HLS UNROLL factor=4

    if (intersections[i].done)
      continue;

    if (!intersections[i].hit) {
      intersections[i].acc = {0,0,0};
      intersections[i].done = true;
      continue;
    }

    shape_t shape = intersections[i].shape;

    if (shape.emittance > 0) {
      intersections[i].acc = color_mult(intersections[i].acc, shape.color);
      intersections[i].done = true;
      continue;
    }

    vec_t normal = normal_shape_point(shape, intersections[i].pt);

    // TODO random
    fp_t u = 0.2;
    fp_t v = 0.54;
    vec_t prev_dir = rays[i].direction;
    vec_t new_dir = vec_from_random(u, v); // Point in a random next direction

    // If new dir in same hemisphere as old dir, need to flip.
    if (dot(prev_dir, new_dir) > 0) {
      rays[i].direction = scale(new_dir, -1);
    } else {
      rays[i].direction = new_dir;
    }

    color_t attenuated_color = scale(shape.color, hls::abs(dot(normal, rays[i].direction)));
    intersections[i].acc = color_mult(intersections[i].acc, attenuated_color);

    rays[i].origin = intersections[i].pt;

    intersections[i].dist = 999999; // reset dist for next round
  }
}

void pathtrace (
  ray_t rays_bram[WIDTH*HEIGHT],    // bram?
  shape_t scene_bram[MAX_SCENE_OBJECTS], // bram? max 16 scene objects?
  color_t pixel_bram[WIDTH*HEIGHT], // bram?
  int depth,
  int rpp
) {
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=depth bundle=CTRL_BUS
#pragma HLS INTERFACE s_axilite port=rpp bundle=CTRL_BUS
#pragma HLS INTERFACE bram port=rays_bram
#pragma HLS INTERFACE bram port=scene_bram
#pragma HLS INTERFACE bram port=pixel_bram

  ray_t rays[WIDTH*HEIGHT];
  shape_t scene[MAX_SCENE_OBJECTS];
#pragma HLS ARRAY_PARTITION dim=1 factor=4 type=cyclic variable=scene
#pragma HLS ARRAY_PARTITION dim=1 factor=4 type=cyclic variable=rays

  // Load from bram into partitioned arrays:
  memcpy(rays, rays_bram, sizeof(rays));
  memcpy(scene, scene_bram, sizeof(scene));

  intersect_t intersections[WIDTH*HEIGHT];
#pragma HLS ARRAY_PARTITION dim=1 factor=4 type=cyclic variable=intersections
  // set all intersections to white, and far away, to start
  for (int i = 0; i < WIDTH*HEIGHT; i++) {
    intersections[i].acc = {255,255,255};
    intersections[i].dist = 999999;
  }

Loop_For_Depth:
  for (int i = 0; i < depth; i++) {
Loop_Over_Objects:
    for (int j = 0; j < MAX_SCENE_OBJECTS; j++) {
      test_rays_against_obj(rays, intersections, scene[j]);
    }
    // For the given bounce depth all rays + intersections are now updated with the correct intersection point
    redirect_rays(rays, intersections);
  }

  // copy all intersection colors where the intersection is finalized (done) to the pixel buffer
Color_Loop:
  for (int i = 0; i < WIDTH*HEIGHT; i++) {
    if (intersections[i].done) {
      pixel_bram[i] = intersections[i].acc;
    } else {
      pixel_bram[i] = {0,0,0};
    }
  }

}
