#include "hls_math.h"
#include "pathtrace.h"

#ifndef PTMATH_H_
#define PTMATH_H_

// U and V are randomly generated
vec_t vec_from_random(fp_t u, fp_t v)
{
  vec_t out;
  fp_t theta = u * (fp_t)(2.0 * 3.1415926535898);
  fp_t phi = hls::acos(2 * v - 1);
  fp_t sin_phi = hls::sin(phi);
  out.x = sin_phi * hls::cos(theta);
  out.y = sin_phi * hls::sin(theta);
  out.z = hls::cos(phi);
  return out;
}

fp_t dot(vec_t a, vec_t b)
{
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec_t cross(vec_t a, vec_t b)
{
  return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

vec_t sub(vec_t a, vec_t b)
{
  return { a.x - b.x, a.y - b. y, a.z - b.z };
}

vec_t scale(vec_t v, fp_t factor)
{
  return { factor * v.x, factor * v.y, factor * v.z };
}

vec_t div(vec_t v, fp_t divisor)
{
  return { v.x / divisor, v.y / divisor, v.z / divisor };
}

vec_t add(vec_t a, vec_t b)
{
  return { a.x + b.x, a.y + b. y, a.z + b.z };
}

fp_t magnitude(vec_t v)
{
  return hls::sqrt(dot(v, v));
}

vec_t normalize(vec_t v)
{
  return div(v, magnitude(v));
}

#endif
