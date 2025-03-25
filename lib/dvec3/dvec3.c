#include "dvec3.h"
#include <math.h>

// Copies a dvec3 (since we pass by value, this just returns the same vector)
dvec3 dvec3_copy(dvec3 src) {
    return src;
}

// Adds two dvec3 vectors
dvec3 dvec3_add(dvec3 a, dvec3 b) {
    dvec3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

// Multiplies two dvec3 vectors element-wise
dvec3 dvec3_mul(dvec3 a, dvec3 b) {
    dvec3 result = {a.x * b.x, a.y * b.y, a.z * b.z};
    return result;
}

// Scales a dvec3 by a scalar value
dvec3 dvec3_scale(dvec3 v, double scalar) {
    dvec3 result = {v.x * scalar, v.y * scalar, v.z * scalar};
    return result;
}

// Normalizes a dvec3
dvec3 dvec3_normalize(dvec3 v) {
    double length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0) {
        double invLength = 1.0 / length;
        dvec3 result = {v.x * invLength, v.y * invLength, v.z * invLength};
        return result;
    } else {
        return (dvec3){0.0, 0.0, 0.0};
    }
}

// Computes the cross product of two dvec3 vectors
dvec3 dvec3_cross(dvec3 a, dvec3 b) {
    dvec3 result = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return result;
}

void dvec3_to_vec3(dvec3 v, vec3 out) {
    out[0] = (float)v.x;
    out[1] = (float)v.y;
    out[2] = (float)v.z;
}

void mat4_multiply_high_precision(const mat4 a, const mat4 b, mat4 result) {
    double temp[4][4] = {0.0}; // Temporary storage for high precision calculations

    // Multiply the matrices with high precision
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[i][j] = 0.0;
            for (int k = 0; k < 4; k++) {
                temp[i][j] += (double)a[i][k] * (double)b[k][j];
            }
        }
    }

    // Convert the result back to float
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result[i][j] = (float)temp[i][j];
        }
    }
}