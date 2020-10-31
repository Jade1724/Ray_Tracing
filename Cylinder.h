#ifndef CYLINDER_H
#define CYLINDER_H

#include <glm/glm.hpp>
#include "SceneObject.h"


class Cylinder : public SceneObject
{

private:
    glm::vec3 center = glm::vec3(0);
    float radius = 1;
    float height = 1;

public:
        Cylinder() {};  //Default constructor creates a unit sphere

        Cylinder(glm::vec3 c, float r, float h) : center(c), radius(r), height(h){}

        float intersect(glm::vec3 p0, glm::vec3 dir);

        glm::vec3 normal(glm::vec3 p);

};


#endif //!CYLINDER_H
