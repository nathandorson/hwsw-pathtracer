#include "pathtrace.h"
#include "pt_math.hpp"

void normal_shape_point(
  shape_t *shape,
  vec_t *point,
  vec_t *normal
) {
  switch (shape->type) {
    vec_t normal_at_pt;
	case SHAPETYPE_PLANE:
	  normal = &shape->coords[1];
	  break;
	case SHAPETYPE_SPHERE:
	  normal_at_pt = normalize(sub(*point, shape->coords[0]));
	  normal->x = normal_at_pt.x;
	  normal->y = normal_at_pt.y;
	  normal->z = normal_at_pt.z;
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

	  if (dist > dist_threshold || dist < 0.00001) // If too close, or not nearer than current best, break.
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

	  intersection->hit = true;
	  intersection->shape = *shape;
	  intersection->pt = add(ray->origin, traversed);
	  intersection->dist = dist;
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
  for (int i = 0; i < MAX_SCENE_OBJECTS; i++) {
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

Scene_Loop:
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
    normal_shape_point(&shape, &isect.pt, &normal);

    if (dot(normal, traced_ray.direction) > 0) {
      normal = scale(normal, -1);
    }

    // TODO random
    fp_t u = 0.2;
    fp_t v = 0.54;
    traced_ray.direction = vec_from_random(u, v); // Point in a random next direction

    if (dot(normal, traced_ray.direction) < 0) {
    	traced_ray.direction = scale(traced_ray.direction, -1);
    }

    color_t attenuated_color = scale(shape.color, dot(normal, ray->direction));
    *traced_color = color_mult(*traced_color, attenuated_color);

    traced_ray.origin = isect.pt;
  }
}


void trace_ray_stream (
  hls::stream<ap_axis<sizeof(ray_t),1,1,1>>& ray_stream,
  shape_t scene[MAX_SCENE_OBJECTS],
  hls::stream<ap_axis<sizeof(color_t),1,1,1>>& pixel_stream
) {
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis register both port=ray_stream
#pragma HLS INTERFACE axis register both port=pixel_stream
#pragma HLS INTERFACE m_axi depth=16 port=scene

  while (1) {
	ap_axis<sizeof(ray_t),1,1,1> ray_s;
	ray_stream.read(ray_s);

	ray_t ray;
	memcpy(&ray, &ray_s.data, sizeof(ray_t));
	color_t total_color;
	fp_t rpp = 1; // TODO

	for (int j = 0; j < rpp; j++) {
	  color_t color;
	  ray_color(&ray, scene, &color);
	  total_color = add(color, total_color);
	}
	total_color = div(total_color, (fp_t)rpp);

	ap_axis<sizeof(color_t),1,1,1> color_s;
	memcpy(&color_s.data, &total_color, sizeof(color_t));
	color_s.keep = ray_s.keep;
	color_s.strb = ray_s.strb;
	color_s.last = ray_s.last;
	color_s.dest = ray_s.dest;
	color_s.id = ray_s.id;
	color_s.user = ray_s.user;
	pixel_stream.write(color_s);

	if (ray_s.last)
	  break;
  }
}



void pathtrace (
  ray_t rays[WIDTH*HEIGHT],    // bram?
  shape_t scene[MAX_SCENE_OBJECTS], // bram? max 16 scene objects?
  color_t pixels[WIDTH*HEIGHT] // bram?
) {
#pragma HLS ARRAY_PARTITION type=complete variable=scene
#pragma HLS ARRAY_PARTITION type=complete variable=rays
#pragma HLS INTERFACE s_axilite port=return // lets us know when done?
#pragma HLS INTERFACE mode=bram port=rays
#pragma HLS INTERFACE mode=bram port=scene
#pragma HLS INTERFACE mode=bram port=pixels

Tracing_Loop:
  for (int i = 0; i < WIDTH*HEIGHT; i++) {
#pragma HLS UNROLL factor=4
	  pixels[i] = { 0, 0, 0 };
	  fp_t rpp = 1; // TODO

	  for (int j = 0; j < rpp; j++) {
		color_t color;
		ray_color(&rays[i], scene, &color);
		pixels[i] = add(color, pixels[i]);
	  }

	  pixels[i] = div(pixels[i], (fp_t)rpp);
  }
}
