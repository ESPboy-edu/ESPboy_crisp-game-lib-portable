#include "machineDependent.h"
#include "vector.h"
#include <math.h>

#include "fastmath.h"
#define sinf fast_sin
#define cosf fast_cos


Vector* vectorSet(Vector* vec, float x, float y) {
  vec->x = x;
  vec->y = y;
  return vec;
}

Vector* vectorAdd(Vector* vec, float x, float y) {
  vec->x += x;
  vec->y += y;
  return vec;
}

Vector* vectorMul(Vector* vec, float v) {
  vec->x *= v;
  vec->y *= v;
  return vec;
}

Vector* rotate(Vector* vec, float angle) {
  float tx = vec->x;
  vec->x = tx * cosf(angle) - vec->y * sinf(angle);
  vec->y = tx * sinf(angle) + vec->y * cosf(angle);
  return vec;
}

Vector* addWithAngle(Vector* vec, float angle, float length) {
  vec->x += cosf(angle) * length;
  vec->y += sinf(angle) * length;
  return vec;
}

float angleTo(Vector* vec, float x, float y) {
  return atan2f(y - vec->y, x - vec->x);
}

float distanceTo(Vector* vec, float x, float y) {
  float ox = x - vec->x;
  float oy = y - vec->y;
  return sqrtf(ox * ox + oy * oy);
}

float vectorAngle(Vector* vec) { return atan2f(vec->y, vec->x); }

float vectorLength(Vector* vec) {
  return sqrtf(vec->x * vec->x + vec->y * vec->y);
}
