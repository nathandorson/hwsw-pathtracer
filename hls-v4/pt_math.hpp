#include "hls_math.h"
#include "pathtrace.h"

#ifndef PTMATH_H_
#define PTMATH_H_

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

#endif
