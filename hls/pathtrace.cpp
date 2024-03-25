#include "pathtrace.h"
#include "pt_math.hpp"

void normal_shape_ray(
  shape_t *shape,
  ray_t *ray,
  vec_t *normal
) {
  switch (shape->type) {
	case SHAPETYPE_PLANE:
	  normal = &shape->coords[1]; // TODO make sure correct

	  break;
	case SHAPETYPE_SPHERE:

	  break;
	default:
	  break;
  }
}

void intersect_ray_shape(
  ray_t *ray,
  shape_t *shape,
  fp_t dist_threshold,
  intersect_t *intersection
) {
  fp_t dir_dot_norm;
  vec_t traversed;
  vec_t diff;
  fp_t t, dist;
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

	  if (dist > dist_threshold || dist < 0.00001) // If too close, or not nearer than current best, break.
		  break;

	  intersection->hit = true;
	  intersection->shape = *shape;
	  intersection->pt = add(ray->origin, traversed);
	  intersection->dist = dist;
	  break;
	case SHAPETYPE_SPHERE:

	  break;
	default:
	  break;
  }
}


void cast_ray(
  ray_t *ray,
  shape_t scene[],
  intersect_t *intersection
) {
  intersection->hit = false;

  fp_t closest_dist = 99999;

  // find the closest intersection
  // TODO number of obj in scene
  for (int i = 0; i < 16; i++) {
#pragma HLS PIPELINE off
	shape_t shape = scene[i];
	intersect_ray_shape(ray, &shape, closest_dist, intersection);
    closest_dist = intersection->dist;
  }
}


void ray_color(
  ray_t *ray,
  shape_t scene[],
  color_t *traced_color
) {
  *traced_color = {255, 255, 255};
  ray_t traced_ray;
  traced_ray.origin = ray->origin;
  traced_ray.direction = ray->direction;

  intersect_t isect;
  int depth = 4; // TODO TEMP

  // pipelined:
  for (int i = 0; i < depth; i++) {
    cast_ray(&traced_ray, scene, &isect);

    if (!isect.hit || i == depth) {
      traced_color->x = 0;
      traced_color->y = 0;
      traced_color->z = 0;
      return;
    }

    shape_t shape = isect.shape;

    if (shape.emittance > 0) {
      *traced_color = color_mult(*traced_color, shape.color);
      return;
    }

    vec_t normal;
    normal_shape_ray(&shape, &traced_ray, &normal);

    if (dot(normal, traced_ray.direction) > 0) {
      scale(normal, -1);
    }

    // TODO same hemisphere as normal
    // TODO random
    fp_t u = 0.2;
    fp_t v = 0.54;
    traced_ray.direction = vec_from_random(u, v); // Point in a random next direction
    color_t attenuated_color = scale(shape.color, dot(normal, ray->direction));
    *traced_color = color_mult(*traced_color, attenuated_color);

    traced_ray.origin = isect.pt;
  }
}


void pathtrace (
  ray_t rays[WIDTH*HEIGHT],    // bram?
  hls::stream<shape_t> &scene_stream, // stream in arbitrary nr. of scene objects?
  color_t pixels[WIDTH*HEIGHT] // bram?
) {
#pragma HLS ARRAY_PARTITION type=complete variable=rays
#pragma HLS INTERFACE mode=axis port=scene_stream
#pragma HLS INTERFACE mode=s_axilite port=return // lets us know when done?
#pragma HLS INTERFACE mode=bram port=rays

  shape_t scene[128]; // TODO temp limit on nr of scene objects
#pragma HLS ARRAY_PARTITION dim=1 type=complete variable=scene

  // read in all the scene objects
Scene_Load_Loop:
  for (int i = 0; i < 16; i++)
  {
	scene_stream.read(scene[i]);
	if (scene[i].type == SHAPETYPE_ENDSCENE) {
	  break;
	}
  }

Tracing_Loop:
  for (int r = 0; r < HEIGHT; r++) {
    for (int c = 0; c < WIDTH; c++) {
      pixels[r*WIDTH + c] = { 0, 0, 0 };
      fp_t rpp = 2; // TODO

      for (int i = 0; i < rpp; i++) {
    	color_t color;
    	ray_color(&rays[r*WIDTH + c], scene, &color);
    	pixels[r*WIDTH + c] = add(color, pixels[r*WIDTH + c]);
      }

      pixels[r*WIDTH + c] = div(pixels[r*WIDTH + c], (fp_t)rpp);
    }
  }
}
