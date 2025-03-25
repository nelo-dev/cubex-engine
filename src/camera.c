#include "camera.h"
#include "chunk.h"

#include <stdlib.h>
#include <math.h>

void updateViewMat(Camera cam) {
    dvec3 tempCamPos = dvec3_copy(cam->camPos);  // Use . instead of -> for struct

    tempCamPos.x = fmod(tempCamPos.x, (float) SC_LEN);
    tempCamPos.z = fmod(tempCamPos.z, (float) SC_LEN);

    dvec3 center = dvec3_add(tempCamPos, cam->camFront);

    // Prepare vec3 for glm_lookat
    vec3 camPosVec, centerVec, camUpVec;

    dvec3_to_vec3(tempCamPos, camPosVec);
    dvec3_to_vec3(center, centerVec);
    dvec3_to_vec3(cam->camUp, camUpVec);  // Assuming cam.camUp is dvec3 as well

    glm_lookat(camPosVec, centerVec, camUpVec, cam->view);
}

Camera createCamera(dvec3 position) {
    Camera_T *cam = calloc(1, sizeof(Camera_T));

    // Initialize camUp and camFront using dvec3
    dvec3 camUp = {0.01, 1.0, 0.0};  // Up vector
    dvec3 camFront = {1.0, 0.0, 0.0}; // Front vector

    // Copy dvec3 values to the camera structure
    cam->camPos = position;
    cam->camFront = camFront;
    cam->camUp = camUp;
    cam->fov = glm_rad(65.0f);
    cam->zoom = 0.0f;
    
    // Set additional camera properties
    cam->fov = glm_rad(65.0f);
    cam->speed = 0.5f;

    // Update the view matrix using the dvec3 values
    updateViewMat(cam);
    
    return cam;
}

void destroyCamera(Camera cam)
{
    free(cam);
}

void moveCamera(Camera cam, uint32_t direction)
{
    dvec3 temp_front = {1.0, 1.0, 1.0};  // Initialize dvec3

    if (direction == CAM_DIRECTION_FRONT)
        temp_front = dvec3_scale(cam->camFront, cam->speed);

    if (direction == CAM_DIRECTION_BACK)
        temp_front = dvec3_scale(cam->camFront, -cam->speed);

    if (direction == CAM_DIRECTION_LEFT) {
        temp_front = dvec3_cross(cam->camFront, cam->camUp);  // Cross product
        temp_front = dvec3_normalize(temp_front);             // Normalize
        temp_front = dvec3_scale(temp_front, -cam->speed);    // Scale
    }

    if (direction == CAM_DIRECTION_RIGHT) {
        temp_front = dvec3_cross(cam->camFront, cam->camUp);  // Cross product
        temp_front = dvec3_normalize(temp_front);             // Normalize
        temp_front = dvec3_scale(temp_front, cam->speed);     // Scale
    }

    if (direction == CAM_DIRECTION_UP) {
        temp_front = (dvec3){0.0, cam->speed, 0.0};           // Move up
    }

    if (direction == CAM_DIRECTION_DOWN) {
        temp_front = (dvec3){0.0, -cam->speed, 0.0};          // Move down
    }

    cam->camPos = dvec3_add(cam->camPos, temp_front);          // Add temp_front to camPos
    updateViewMat(cam);
}

void swingCamera(Camera cam, double x, double y, float sensitivity)
{
    cam->yaw += x * sensitivity;
    cam->pitch += -y * sensitivity;

    // Clamp pitch
    if (cam->pitch >= 89.0)
        cam->pitch = 89.0;
    if (cam->pitch <= -89.0)
        cam->pitch = -89.0;

    // Convert degrees to radians for yaw and pitch calculations
    double rad_yaw = cam->yaw * M_PI / 180.0;
    double rad_pitch = cam->pitch * M_PI / 180.0;
    
    // Calculate the direction vector (cam_dir)
    dvec3 cam_dir;
    cam_dir.x = cos(rad_yaw) * cos(rad_pitch);
    cam_dir.y = sin(rad_pitch);
    cam_dir.z = sin(rad_yaw) * cos(rad_pitch);

    // Normalize cam_dir
    cam_dir = dvec3_normalize(cam_dir);

    // Copy cam_dir to camFront
    cam->camFront = cam_dir;

    // Update the camera view matrix
    updateViewMat(cam);
}


void updateCameraProjMat(Camera cam, float aspectRatio)
{
    glm_perspective(cam->fov - glm_rad(cam->zoom), aspectRatio, 0.1f, 10000.0f, cam->proj);
    cam->proj[1][1] *= -1;
}

void ExtractFrustumPlanes(Plane frustum[6], mat4 viewProj) {
    float *m = (float*)viewProj;  // Flatten the 4x4 matrix.
    
    // Left plane: row 4 + row 1
    frustum[0].a = m[3]  + m[0];
    frustum[0].b = m[7]  + m[4];
    frustum[0].c = m[11] + m[8];
    frustum[0].d = m[15] + m[12];

    // Right plane: row 4 - row 1
    frustum[1].a = m[3]  - m[0];
    frustum[1].b = m[7]  - m[4];
    frustum[1].c = m[11] - m[8];
    frustum[1].d = m[15] - m[12];

    // Bottom plane: row 4 + row 2
    frustum[2].a = m[3]  + m[1];
    frustum[2].b = m[7]  + m[5];
    frustum[2].c = m[11] + m[9];
    frustum[2].d = m[15] + m[13];

    // Top plane: row 4 - row 2
    frustum[3].a = m[3]  - m[1];
    frustum[3].b = m[7]  - m[5];
    frustum[3].c = m[11] - m[9];
    frustum[3].d = m[15] - m[13];

    // Near plane: row 4 + row 3
    frustum[4].a = m[3]  + m[2];
    frustum[4].b = m[7]  + m[6];
    frustum[4].c = m[11] + m[10];
    frustum[4].d = m[15] + m[14];

    // Far plane: row 4 - row 3
    frustum[5].a = m[3]  - m[2];
    frustum[5].b = m[7]  - m[6];
    frustum[5].c = m[11] - m[10];
    frustum[5].d = m[15] - m[14];

    // Normalize each plane.
    for (int i = 0; i < 6; i++) {
        float t = sqrtf(frustum[i].a * frustum[i].a +
                        frustum[i].b * frustum[i].b +
                        frustum[i].c * frustum[i].c);
        frustum[i].a /= t;
        frustum[i].b /= t;
        frustum[i].c /= t;
        frustum[i].d /= t;
    }
}

// Check if an AABB (defined by min and max vec3 coordinates) is inside the frustum.
bool AABBInFrustum(const Plane frustum[6], const vec3 min, const vec3 max) {
    for (int i = 0; i < 6; i++) {
        // Determine the "positive vertex" based on the plane normal.
        float x = (frustum[i].a >= 0) ? max[0] : min[0];
        float y = (frustum[i].b >= 0) ? max[1] : min[1];
        float z = (frustum[i].c >= 0) ? max[2] : min[2];

        // If this vertex is outside the plane, the entire AABB is culled.
        if (frustum[i].a * x + frustum[i].b * y + frustum[i].c * z + frustum[i].d < 0)
            return false;
    }
    return true;
}
