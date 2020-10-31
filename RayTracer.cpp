/*==================================================================================
* COSC 363  Computer Graphics (2020)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab07.pdf, Lab08.pdf for details.
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "Plane.h"
#include "Cylinder.h"
#include "Cone.h"
#include "SceneObject.h"
#include "Ray.h"
#include "TextureBMP.h"
#include <GL/freeglut.h>

#define PI 3.14159

using namespace std;

const float WIDTH = 20.0;  
const float HEIGHT = 20.0;
const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -WIDTH * 0.5;
const float XMAX =  WIDTH * 0.5;
const float YMIN = -HEIGHT * 0.5;
const float YMAX =  HEIGHT * 0.5;


vector<SceneObject*> sceneObjects;
TextureBMP texture;

//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
    glm::vec3 backgroundCol(1);                     //Background colour = (0,0,0)
    glm::vec3 lightPos(5.0, 2.0, -50.0);                 //Light's position
    glm::vec3 color(0);
    SceneObject* obj;
    SceneObject* obstacle;

    ray.closestPt(sceneObjects);                    //Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;       //no intersection
    obj = sceneObjects[ray.index];                  //object on which the closest point of intersection is found


    if (ray.index == 2) {               // Textureing on sphere3
        float theta = acos((ray.hit.x + 5.0)/3.0);
        float phi = acos(-(ray.hit.y - 5.0)/3.0);
        float texcoords = theta/PI;
        float texcoordt = phi/PI;
        if (texcoords > 0 && texcoords < 1 && texcoordt > 0 && texcoordt < 1) {
            color = texture.getColorAt(texcoords, texcoordt);
            obj->setColor(color);
        }

    }

    if (ray.index == 3) {               // Chequered pattern
        int stripeWidth = 5;
        int iz = (ray.hit.z) / stripeWidth;
        int ix = (ray.hit.x - 20) / stripeWidth;
        int k = iz % 2;
        int l = ix % 2;
        if ((k == 0 && l == 0) || (k != 0 && l != 0)) color = glm::vec3(0.4, 0.7, 0.8);
        else color = glm::vec3(0.8, 0.8, 0);
        obj->setColor(color);

    }

    if (ray.index == 4 || ray.index == 5) {         // Tilted stripe pattern
        int yzsum = ((ray.hit.y)*2.0/sqrt(3.0) + (ray.hit.z)) / 20.0;
        int k = yzsum % 2;
        if (k == 0) color = glm::vec3(0.4, 0.0, 0.2);
        else color = glm::vec3(0.4, 0.7, 0.8);
        obj->setColor(color);

    }

    color = obj->lighting(lightPos, -ray.dir, ray.hit);

    float t = (ray.hit.z + 40.0) / (-200.0 + 40.0);     // Interpolation parameter for fog
    color = (1-t)*color + t*glm::vec3(1.0, 1.0, 1.0);

    glm::vec3 lightVec = lightPos - ray.hit;
    Ray shadowRay(ray.hit, lightVec);
    shadowRay.closestPt(sceneObjects);
    if ((shadowRay.index > -1) && (shadowRay.dist < glm::length(lightVec))) {
        obstacle = sceneObjects[shadowRay.index];
        if (obstacle->isRefractive() && obstacle->isReflective()) {
            float obstacleRefl = obstacle->getReflectionCoeff();
            float obstacleRefr = obstacle->getRefractionCoeff();
            color = (1.0f - 0.4f * obstacleRefl - 0.4f * obstacleRefr) * color;
        }
        else if (obstacle->isReflective()) {
            float obstacleRefl = obstacle->getReflectionCoeff();
            color = (1.0f - 0.8f * obstacleRefl) * color;
        }
        else if (obstacle->isRefractive()) {
            float obstacleRefr = obstacle->getRefractionCoeff();
            color = (0.8f * obstacleRefr + 0.2f) * color;
        }
        else if (obstacle->isTransparent()) {
            float obstacleTrans = obstacle->getTransparencyCoeff();
            color = (0.8f * obstacleTrans + 0.2f) * color;
        }
        else {
            color = 0.2f * obj->getColor();     // 0.2 = ambient scale factor
        }
    }



    if (obj->isRefractive() && step < MAX_STEPS) {
        float eta = 1.0f / obj->getRefractiveIndex();
        float rho = obj->getRefractionCoeff();
        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 refrVec = glm::refract(ray.dir, normalVec, eta);
        Ray refrRay(ray.hit, refrVec);
        refrRay.closestPt(sceneObjects);
        glm::vec3 internalNormalVec = obj->normal(refrRay.hit);
        glm::vec3 secondRefrVec = glm::refract(refrVec, -internalNormalVec, 1.0f/eta);
        Ray secondRefrRay(refrRay.hit, secondRefrVec);
        glm::vec3 refractedColor = trace(secondRefrRay, step + 1);
        color = color + (rho * refractedColor);
    }

    if (obj->isReflective() && step < MAX_STEPS) {
        float rho = obj->getReflectionCoeff();
        glm::vec3 normalVec = obj->normal(ray.hit);
        glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
        Ray reflectedRay(ray.hit, reflectedDir);
        glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
        color = color + (rho * reflectedColor);
    }

    if (obj->isTransparent() && step < MAX_STEPS) {
        float eta = obj->getTransparencyCoeff();
        Ray transmittedRay(ray.hit, ray.dir);
        glm::vec3 transmittedColor = trace(transmittedRay, step + 1);
        color = color + (eta * transmittedColor);
    }

    return color;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
    float xp, yp;  //grid point
    float cellX = (XMAX-XMIN)/NUMDIV;  //cell width
    float cellY = (YMAX-YMIN)/NUMDIV;  //cell height
    glm::vec3 eye(0., 0., 0.);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();


    for(int i = 0; i < NUMDIV; i++) //Scan every cell of the image plane
    {
        xp = XMIN + i*cellX;
        for(int j = 0; j < NUMDIV; j++)
        {
            yp = YMIN + j*cellY;

            glm::vec3 dir1(xp+0.25*cellX, yp+0.25*cellY, -EDIST);  //direction of the primary ray
            glm::vec3 dir2(xp+0.75*cellX, yp+0.25*cellY, -EDIST);
            glm::vec3 dir3(xp+0.25*cellX, yp+0.75*cellY, -EDIST);
            glm::vec3 dir4(xp+0.75*cellX, yp+0.75*cellY, -EDIST);

            Ray ray1 = Ray(eye, dir1);
            Ray ray2 = Ray(eye, dir2);
            Ray ray3 = Ray(eye, dir3);
            Ray ray4 = Ray(eye, dir4);

            glm::vec3 col1 = trace(ray1, 2); //Trace the primary ray and get the colour value
            glm::vec3 col2 = trace(ray2, 2);
            glm::vec3 col3 = trace(ray3, 2);
            glm::vec3 col4 = trace(ray4, 2);

            float diff12 = fabs(col1.r-col2.r)+fabs(col1.g-col2.g)+fabs(col1.b-col2.b);
            float diff13 = fabs(col1.r-col3.r)+fabs(col1.g-col3.g)+fabs(col1.b-col3.b);
            float diff14 = fabs(col1.r-col4.r)+fabs(col1.g-col4.g)+fabs(col1.b-col4.b);


            if ((diff12 > 0.2) || (diff13 > 0.2) || (diff14 > 0.2)) {
                glBegin(GL_QUADS);
                glColor3f(col1.r, col1.g, col1.b);
                glVertex2f(xp, yp);
                glVertex2f(xp+0.5*cellX, yp);
                glVertex2f(xp+0.5*cellX, yp+0.5*cellY);
                glVertex2f(xp, yp+0.5*cellY);
                glEnd();

                glBegin(GL_QUADS);
                glColor3f(col2.r, col2.g, col2.b);
                glVertex2f(xp+0.5*cellX, yp);
                glVertex2f(xp+cellX, yp);
                glVertex2f(xp+cellX, yp+0.5*cellY);
                glVertex2f(xp+0.5*cellX, yp+0.5*cellY);
                glEnd();

                glBegin(GL_QUADS);
                glColor3f(col3.r, col3.g, col3.b);
                glVertex2f(xp, yp+0.5*cellY);
                glVertex2f(xp+0.5*cellX, yp+0.5*cellY);
                glVertex2f(xp+0.5*cellX, yp+cellY);
                glVertex2f(xp, yp+cellY);
                glEnd();

                glBegin(GL_QUADS);
                glColor3f(col4.r, col4.g, col4.b);
                glVertex2f(xp+0.5*cellX, yp+0.5*cellY);
                glVertex2f(xp+cellX, yp+0.5*cellY);
                glVertex2f(xp+cellX, yp+cellY);
                glVertex2f(xp+0.5*cellX, yp+cellY);
                glEnd();
            }
            else {

                glm::vec3 dir(xp+0.5*cellX, yp+0.5*cellY, -EDIST);
                Ray ray = Ray(eye, dir);
                glm::vec3 col = trace (ray, 1);

                glBegin(GL_QUADS);
                glColor3f(col.r, col.g, col.b);
                glVertex2f(xp, yp);             //Draw each cell with its color value
                glVertex2f(xp+cellX, yp);
                glVertex2f(xp+cellX, yp+cellY);
                glVertex2f(xp, yp+cellY);
                glEnd();
            }
        }
    }

    glFlush();
}



//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);
    texture = TextureBMP("VaseTexture.bmp");

    glClearColor(0, 0, 0, 1);

    Sphere *sphere1 = new Sphere(glm::vec3(5.0, 5.0, -70.0), 2.5);
    sphere1->setColor(glm::vec3(0.7, 0.1, 0.7));
    sphere1->setReflectivity(true, 0.3);
    sceneObjects.push_back(sphere1);         //Add sphere to scene objects


    Sphere *sphere2 = new Sphere(glm::vec3(-5.0, -5.0, -60.0), 4.5);
    sphere2->setColor(glm::vec3(0.1, 0.1, 0.1));
    sphere2->setRefractivity(true, 0.9, 1.01);
    sphere2->setReflectivity(true, 0.4);
    sceneObjects.push_back(sphere2);

    Sphere *sphere3 = new Sphere(glm::vec3(-5.0, 5.0, -70.0), 2.5);
    sphere3-> setColor(glm::vec3(1, 0, 0));
    sceneObjects.push_back(sphere3);


    // Three planes
    Plane *plane1 = new Plane(glm::vec3(-20.0, -15.0, -40.0),
                             glm::vec3(20.0, -15.0, -40.0),
                             glm::vec3(20.0, -15.0, -200.0),
                             glm::vec3(-20.0, -15.0, -200.0));
    plane1->setSpecularity(false);
    sceneObjects.push_back(plane1);

    float cornerHeight = 20.0*sqrt(3.0)-15.0;
    Plane *plane2 = new Plane(glm::vec3(0.0, cornerHeight, -200.0),
                              glm::vec3(0.0, cornerHeight, -40.0),
                              glm::vec3(-20.0, -15.0, -40.0),
                              glm::vec3(-20.0, -15.0, -200.0));
    plane2->setSpecularity(false);
    sceneObjects.push_back(plane2);

    Plane *plane3 = new Plane(glm::vec3(20.0, -15.0, -200.0),
                              glm::vec3(20.0, -15.0, -40.0),
                              glm::vec3(0.0, cornerHeight, -40.0),
                              glm::vec3(0.0, cornerHeight, -200.0));
    plane3->setSpecularity(false);
    sceneObjects.push_back(plane3);



    // Tetrahydron
        Plane *triangle1 = new Plane(glm::vec3(-6, 0.0, -2.0*sqrt(3.0)-90.0),
                                     glm::vec3(0.0, 0.0, 4.0*sqrt(3.0)-90.0),
                                     glm::vec3(6, 0.0, -2.0*sqrt(3.0)-90.0));
        triangle1->setColor(glm::vec3(0.0, 0.7, 0.0));
        Plane *triangle2 = new Plane(glm::vec3(-6, 0.0, -2.0*sqrt(3.0)-90.0),
                                     glm::vec3(0.0, -9.0, -90.0),
                                     glm::vec3(0.0, 0.0, 4.0*sqrt(3.0)-90.0));
        triangle2->setColor(glm::vec3(0.0, 0.7, 0.0));
        Plane *triangle3 = new Plane(glm::vec3(0.0, 0.0, 4.0*sqrt(3.0)-90.0),
                                     glm::vec3(0.0, -9.0, -90.0),
                                     glm::vec3(6, 0.0, -2.0*sqrt(3.0)-90.0));
        triangle3->setColor(glm::vec3(0.0, 0.7, 0.0));
        Plane *triangle4 = new Plane(glm::vec3(6, 0.0, -2.0*sqrt(3.0)-90.0),
                                     glm::vec3(0.0, -9.0, -90.0),
                                     glm::vec3(-6, 0.0, -2.0*sqrt(3.0)-90.0));
        triangle4->setColor(glm::vec3(0.0, 0.7, 0.0));
        sceneObjects.push_back(triangle1);
        sceneObjects.push_back(triangle2);
        sceneObjects.push_back(triangle3);
        sceneObjects.push_back(triangle4);

    Cylinder *cylinder = new Cylinder(glm::vec3(5.0, -15.0, -70.0), 2.5, 7.0);
    cylinder->setColor(glm::vec3(0.5, 0.0, 1.0));
    sceneObjects.push_back(cylinder);

    Cone *cone = new Cone(glm::vec3(0.0, -15.0, -70), 2.5, 7.0);
    cone->setColor(glm::vec3(1.0, 0.5, 0.0));
    sceneObjects.push_back(cone);
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Ray Tracing Assignment");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
