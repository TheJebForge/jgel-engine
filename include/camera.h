#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>

class Camera{
    public:
    Camera(const glm::vec3& pos, float fov, float aspect, float zNear, float zFar){
        perspective = glm::perspective(fov,aspect,zNear,zFar);
        this->fov = fov;
        position = pos;
        zn = zNear;
        zf = zFar;
        forw = glm::vec3(0,0,1);
        up = glm::vec3(0,1,0);
        left = glm::vec3(1,0,0);
    }

    Camera(){}

    void DoCamera(const glm::vec3& pos, float fov, float aspect, float zNear, float zFar){
        perspective = glm::perspective(fov,aspect,zNear,zFar);
        this->fov = fov;
        position = pos;
        zn = zNear;
        zf = zFar;
        forw = glm::vec3(0,0,1);
        up = glm::vec3(0,1,0);
        left = glm::vec3(1,0,0);
    }

    inline glm::mat4 getViewProjection() const {

        return perspective * glm::lookAt(position,position + forw, up);
    }
    glm::vec3 forw;
    glm::vec3 up;
    glm::vec3 left;
    glm::vec3 position;
    float fov = 90;
    float zn;
    float zf;

    glm::quat rotation;

    inline glm::vec3* getPosition() { return &position; }
    inline glm::vec3* getForward() { return &forw; }
    inline glm::vec3* getUp() { return &up; }
    inline glm::vec3* getLeft() { return &left; }

protected:
private:
    glm::mat4 perspective;



};

#endif // CAMERA_H_INCLUDED
