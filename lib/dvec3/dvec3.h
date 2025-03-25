#ifndef DVEC3_H
#define DVEC3_H

#include <cglm/cglm.h>  // Make sure to include the cglm header for vec3

typedef struct {
    double x, y, z;
} dvec3;

// Function prototypes
dvec3 dvec3_copy(dvec3 src);
dvec3 dvec3_add(dvec3 a, dvec3 b);
dvec3 dvec3_mul(dvec3 a, dvec3 b);
dvec3 dvec3_scale(dvec3 v, double scalar);
dvec3 dvec3_normalize(dvec3 v);
dvec3 dvec3_cross(dvec3 a, dvec3 b);
void dvec3_to_vec3(dvec3 v, vec3 out);  // Function prototype for casting

void mat4_multiply_high_precision(const mat4 a, const mat4 b, mat4 result);

#endif // DVEC3_H
