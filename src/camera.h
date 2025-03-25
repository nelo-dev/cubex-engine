#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>
#include <cglm/cglm.h>
#include "../lib/dvec3/dvec3.h"

#define CAM_DIRECTION_FRONT 0
#define CAM_DIRECTION_BACK  1
#define CAM_DIRECTION_LEFT  2
#define CAM_DIRECTION_RIGHT 3
#define CAM_DIRECTION_UP    4
#define CAM_DIRECTION_DOWN  5

typedef struct {
    float a, b, c, d;
} Plane;

typedef struct Camera_T 
{
    mat4 proj;
    mat4 view;

    dvec3 camPos;
    dvec3 camFront;
    dvec3 camUp;

    float yaw;
    float pitch;
    float fov;
    float speed;

    float zoom;
} Camera_T;

typedef Camera_T * Camera;

Camera createCamera(dvec3 position);
void destroyCamera(Camera cam);
void swingCamera(Camera cam, double x, double y, float sensitivity);
void updateCameraProjMat(Camera cam, float aspectRatio);
void moveCamera(Camera cam, uint32_t direction);

void ExtractFrustumPlanes(Plane frustum[6], mat4 viewProj);
bool AABBInFrustum(const Plane frustum[6], const vec3 min, const vec3 max);

#endif
