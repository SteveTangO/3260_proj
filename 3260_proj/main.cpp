/*
Group Information
Student ID:
 1155124421
 1155124275
Student Name:
 CHEN Tianhao
 TANG Zizhou
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "./Dependencies/stb_image/stb_image.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstdlib>


// screen setting
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
int scale_parameter = 0;
int dir_light_parameter = 2;
int dolphin_texture = 0;
int sea_texture = 0;
int rotate_num = 0;
int z_shift_num = 0;
int y_shift_num = 0;
float y_delta = 0.1f;
float z_delta = 0.1f;
float r_delta = 15.0f;
bool firstmouse = true;
float last_x;
float last_y;

typedef struct object_struct{
    GLuint vaoID;
    GLuint vboID;
    GLuint eboID;
} Object;

// struct for storing the obj file
struct Vertex {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct MouseController{
    bool LEFT_BUTTON = false;
    bool RIGHT_BUTTON = false;
    double MOUSE_Clickx = 0.0, MOUSE_Clicky = 0.0;
    double MOUSE_X = 0.0, MOUSE_Y = 0.0;
    int click_time = glfwGetTime();
};

Object* objects = (Object*)malloc(sizeof(Object) * 20);
GLuint shadowFrameBuffer = 0;

Shader Shader0;
Shader Shader1;
Texture Texture0;
Texture Texture1;
Texture Texture2;
Texture Texture3;
Texture Texture4;

Texture textureSpacecraft_i;
Texture textureSpacecraft_f;
Texture textureAlienPeople;
Texture textureAlienVehicle;
Texture texturePlanet;

unsigned int depthMapFBO;
unsigned int depthMap;

Model dolphin;
Model sea;
Model tower;

//newly added
Model spaceCraft;
Model alienPeople;
Model alienVehicle;
Model planet;

Camera cam;

MouseController mouseCrl;

Model loadOBJ(const char* objPath)
{
    // function to load the obj file
    // Note: this simple function cannot load all obj files.

    struct V {
        // struct for identify if a vertex has showed up
        unsigned int index_position, index_uv, index_normal;
        bool operator == (const V& v) const {
            return index_position == v.index_position && index_uv == v.index_uv && index_normal == v.index_normal;
        }
        bool operator < (const V& v) const {
            return (index_position < v.index_position) ||
                (index_position == v.index_position && index_uv < v.index_uv) ||
                (index_position == v.index_position && index_uv == v.index_uv && index_normal < v.index_normal);
        }
    };

    std::vector<glm::vec3> temp_positions;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    std::map<V, unsigned int> temp_vertices;

    Model model;
    unsigned int num_vertices = 0;

    std::cout << "\nLoading OBJ file " << objPath << "..." << std::endl;

    std::ifstream file;
    file.open(objPath);

    // Check for Error
    if (file.fail()) {
        std::cerr << "Impossible to open the file! Do you use the right path? See Tutorial 6 for details" << std::endl;
        exit(1);
    }

    while (!file.eof()) {
        // process the object file
        char lineHeader[128];
        file >> lineHeader;

        if (strcmp(lineHeader, "v") == 0) {
            // geometric vertices
            glm::vec3 position;
            file >> position.x >> position.y >> position.z;
            temp_positions.push_back(position);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            // texture coordinates
            glm::vec2 uv;
            file >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            // vertex normals
            glm::vec3 normal;
            file >> normal.x >> normal.y >> normal.z;
            temp_normals.push_back(normal);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            // Face elements
            V vertices[3];
            for (int i = 0; i < 3; i++) {
                char ch;
                file >> vertices[i].index_position >> ch >> vertices[i].index_uv >> ch >> vertices[i].index_normal;
            }

            // Check if there are more than three vertices in one face.
            std::string redundency;
            std::getline(file, redundency);
            if (redundency.length() >= 5) {
                std::cerr << "There may exist some errors while load the obj file. Error content: [" << redundency << " ]" << std::endl;
                std::cerr << "Please note that we only support the faces drawing with triangles. There are more than three vertices in one face." << std::endl;
                std::cerr << "Your obj file can't be read properly by our simple parser :-( Try exporting with other options." << std::endl;
                exit(1);
            }

            for (int i = 0; i < 3; i++) {
                if (temp_vertices.find(vertices[i]) == temp_vertices.end()) {
                    // the vertex never shows before
                    Vertex vertex;
                    vertex.position = temp_positions[vertices[i].index_position - 1];
                    vertex.uv = temp_uvs[vertices[i].index_uv - 1];
                    vertex.normal = temp_normals[vertices[i].index_normal - 1];

                    model.vertices.push_back(vertex);
                    model.indices.push_back(num_vertices);
                    temp_vertices[vertices[i]] = num_vertices;
                    num_vertices += 1;
                }
                else {
                    // reuse the existing vertex
                    unsigned int index = temp_vertices[vertices[i]];
                    model.indices.push_back(index);
                }
            } // for
        } // else if
        else {
            // it's not a vertex, texture coordinate, normal or face
            char stupidBuffer[1024];
            file.getline(stupidBuffer, 1024);
        }
    }
    file.close();

    std::cout << "There are " << num_vertices << " vertices in the obj file.\n" << std::endl;
    return model;
}

//avoid repeating
void load_Data(Model model, int id){
    glGenVertexArrays(1, &objects[id].vaoID);
    glBindVertexArray(objects[id].vaoID);
    glGenBuffers(1, &objects[id].vboID);
    glBindBuffer(GL_ARRAY_BUFFER, objects[id].vboID);
    glBufferData(GL_ARRAY_BUFFER, model.vertices.size() * sizeof(Vertex), &model.vertices[0], GL_STATIC_DRAW);
    glGenBuffers(1, &objects[id].eboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[id].eboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.indices.size() * sizeof(unsigned int), &model.indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
}

void get_OpenGL_info()
{
    // OpenGL information
    const GLubyte* name = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* glversion = glGetString(GL_VERSION);
    std::cout << "OpenGL company: " << name << std::endl;
    std::cout << "Renderer name: " << renderer << std::endl;
    std::cout << "OpenGL version: " << glversion << std::endl;
}


void sendDataToOpenGL()
{
    //TODO
    //Load objects and bind to VAO and VBO
    //Load textures
//    dolphin = loadOBJ("./resources/dolphin/dolphin.obj");
//    glGenVertexArrays(1, &objects[0].vaoID);
//    glBindVertexArray(objects[0].vaoID);
//    glGenBuffers(1, &objects[0].vboID);
//    glBindBuffer(GL_ARRAY_BUFFER, objects[0].vboID);
//    glBufferData(GL_ARRAY_BUFFER, dolphin.vertices.size() * sizeof(Vertex), &dolphin.vertices[0], GL_STATIC_DRAW);
//    glGenBuffers(1, &objects[0].eboID);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[0].eboID);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, dolphin.indices.size() * sizeof(unsigned int), &dolphin.indices[0], GL_STATIC_DRAW);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
//
//    sea = loadOBJ("./resources/sea/sea.obj");
//    glGenVertexArrays(1, &objects[1].vaoID);
//    glBindVertexArray(objects[1].vaoID);
//    glGenBuffers(1, &objects[1].vboID);
//    glBindBuffer(GL_ARRAY_BUFFER, objects[1].vboID);
//    glBufferData(GL_ARRAY_BUFFER, sea.vertices.size() * sizeof(Vertex), &sea.vertices[0], GL_STATIC_DRAW);
//    glGenBuffers(1, &objects[1].eboID);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[1].eboID);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sea.indices.size() * sizeof(unsigned int), &sea.indices[0], GL_STATIC_DRAW);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
//
//    tower = loadOBJ("./resources/tower/boat.obj");
//    glGenVertexArrays(1, &objects[2].vaoID);
//    glBindVertexArray(objects[2].vaoID);
//    glGenBuffers(1, &objects[2].vboID);
//    glBindBuffer(GL_ARRAY_BUFFER, objects[2].vboID);
//    glBufferData(GL_ARRAY_BUFFER, tower.vertices.size() * sizeof(Vertex), &tower.vertices[0], GL_STATIC_DRAW);
//    glGenBuffers(1, &objects[2].eboID);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[2].eboID);
//    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tower.indices.size() * sizeof(unsigned int), &tower.indices[0], GL_STATIC_DRAW);
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    
//spacecraft
    spaceCraft = loadOBJ("./resources/spacecraft/spacecraft.obj");
//    spaceCraft = loadOBJ("./resources/alienpeople/alienpeople.obj");
    glGenVertexArrays(1, &objects[3].vaoID);
    glBindVertexArray(objects[3].vaoID);
    glGenBuffers(1, &objects[3].vboID);
    glBindBuffer(GL_ARRAY_BUFFER, objects[3].vboID);
    glBufferData(GL_ARRAY_BUFFER, spaceCraft.vertices.size() * sizeof(Vertex), &spaceCraft.vertices[0], GL_STATIC_DRAW);
    glGenBuffers(1, &objects[3].eboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[3].eboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, spaceCraft.indices.size() * sizeof(unsigned int), &spaceCraft.indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

//alienpeople
    alienPeople = loadOBJ("./resources/alienpeople/alienpeople.obj");
    glGenVertexArrays(1, &objects[4].vaoID);
    glBindVertexArray(objects[4].vaoID);
    glGenBuffers(1, &objects[4].vboID);
    glBindBuffer(GL_ARRAY_BUFFER, objects[4].vboID);
    glBufferData(GL_ARRAY_BUFFER, alienPeople.vertices.size() * sizeof(Vertex), &alienPeople.vertices[0], GL_STATIC_DRAW);
    glGenBuffers(1, &objects[4].eboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[4].eboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, alienPeople.indices.size() * sizeof(unsigned int), &alienPeople.indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
//alien vehicle
    alienVehicle = loadOBJ("./resources/alienvehicle/alienvehicle.obj");
    load_Data(alienVehicle, 5);
    
//planet
    planet = loadOBJ("./resources/planet/planet.obj");
    load_Data(planet, 6);
    
    
    
//    Texture0.setupTexture("./resources/dolphin/dolphin_01.jpg");
//    Texture1.setupTexture("./resources/sea/sea_01.jpg");
//    Texture2.setupTexture("./resources/dolphin/dolphin_02.jpg");
//    Texture3.setupTexture("./resources/sea/sea_02.jpg");
//    Texture4.setupTexture("./resources/tower/wood2.jpg");
    
    textureSpacecraft_i.setupTexture("./resources/spacecraft/spacecraftTexture.bmp");
    textureAlienPeople.setupTexture("./resources/alienpeople/alienTexture.bmp");
    textureAlienVehicle.setupTexture("./resources/alienvehicle/colorful_alien_vehicleTexture.bmp");
    texturePlanet.setupTexture("./resources/planet/planetTexture.bmp");
    
    
    //Set up shadow texture
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initializedGL(void) //run only once
{
    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW not OK." << std::endl;
    }

    get_OpenGL_info();
    sendDataToOpenGL();

    //TODO: set up the camera parameters
    //TODO: set up the vertex shader and fragment shader
    Shader0.setupShader("VertexShaderCode.glsl", "FragmentShaderCode.glsl");
    Shader1.setupShader("ShadowVertexShaderCode.glsl", "ShadowFragmentShaderCode.glsl");
    cam = Camera(glm::vec3(0.0f, 2.0f, 2.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -30.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void paintGL(void)  //always run
{
    //for debug purpose
//    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
    GLint modelTransformMatrixUniformLocation = glGetUniformLocation(Shader0.ID, "modelTransformMatrix");
    
    glm::mat4 modelPreScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f, 0.01f, 0.01f));
    glm::mat4 modelScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(pow(1.05f, scale_parameter), pow(1.05f, scale_parameter), pow(1.05f, scale_parameter)));
    glm::mat4 modelRotateMatrix = glm::mat4(1.0f);
    modelRotateMatrix = glm::rotate(modelRotateMatrix, glm::radians(90.0f + r_delta * rotate_num), glm::vec3(0.0f, 1.0f, 0.0f));
    modelRotateMatrix = glm::rotate(modelRotateMatrix, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 modelPreTranslateMatrix = glm::mat4(1.0f);
    glm::mat4 modelTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, z_delta * z_shift_num));
    glm::mat4 boatModelPreTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, -0.9f, -1.5f));
    glm::mat4 boatModelPreScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), ((float)SCR_WIDTH) / SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 viewMatrix = cam.GetViewMatrix();
    projectionMatrix = projectionMatrix * viewMatrix;
    glm::vec3 lightPosition(0.5f, 2.0f, 2.0f);
    
    //spacecraft
    glm::mat4 spacecraftPreScaleMatrix = glm::mat4(1.0f);
    spacecraftPreScaleMatrix = glm::scale(spacecraftPreScaleMatrix, glm::vec3(0.002f,0.002f,0.002f));
    glm::mat4 spacecraftPreRotateMatrix = glm::mat4(1.0f);
    spacecraftPreRotateMatrix = glm::rotate(spacecraftPreRotateMatrix,glm::radians(180.0f), glm::vec3(1.0f,0.0f,0.0f));
    
    //alienpeople
    glm::mat4 alienPeoplePrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(0.25f,0.25f,0.25f));
    
    //alienvehicle
    glm::mat4 alienVehiclePrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(0.15f,0.15f,0.15f));
    
    //alientranslate matrix
    glm::mat4 alienTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f,0.0f,-4.0f));
    
    //planet setup
    glm::mat4 planetPrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(1.5f,1.5f,1.5f));
    glm::mat4 planetTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,-40.0f));
    
    //render the depth map
    glClearColor(0.53f, 0.81f, 0.98f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 7.5f;
    lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
    lightView = glm::lookAt(lightPosition, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;
    
    Shader1.use();
    GLuint depthMatrixLocation = glGetUniformLocation(Shader1.ID, "lightSpaceMatrix");
    GLuint depthModelMatrixLocation = glGetUniformLocation(Shader1.ID, "modelTransformMatrix");
    glUniformMatrix4fv(depthMatrixLocation, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glActiveTexture(GL_TEXTURE0);
    if(dolphin_texture == 0){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture0.ID);
    }
    else{
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture2.ID);
    }
    modelTransformMatrix = modelTranslateMatrix * modelPreTranslateMatrix * modelRotateMatrix * modelScaleMatrix * modelPreScaleMatrix;
    glUniformMatrix4fv(depthModelMatrixLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glBindVertexArray(objects[0].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[0].eboID);
    glDrawElements(GL_TRIANGLES, (int)dolphin.indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE0);
    if(sea_texture == 0){
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture1.ID);
    }
    else{
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture3.ID);
    }
    modelTransformMatrix = glm::mat4(1.0f);
    glUniformMatrix4fv(depthModelMatrixLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glBindVertexArray(objects[1].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[1].eboID);
    glDrawElements(GL_TRIANGLES, (int)sea.indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture4.ID);
    modelTransformMatrix = boatModelPreTranslateMatrix * boatModelPreScaleMatrix;
    glUniformMatrix4fv(depthModelMatrixLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glBindVertexArray(objects[2].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[2].eboID);
    glDrawElements(GL_TRIANGLES, (int)tower.indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glViewport(0, 0, SCR_WIDTH*2, SCR_HEIGHT*2);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glViewport(0, 0, SCR_WIDTH*2, SCR_HEIGHT*2);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Shader0.use();
    
    GLint projectionMatrixUniformLocation = glGetUniformLocation(Shader0.ID, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    GLint dirLightParameterUniformLocation = glGetUniformLocation(Shader0.ID, "dir_light_parameter");
    glUniform1i(dirLightParameterUniformLocation, dir_light_parameter);
    
    GLint lightPositionUniformLocation = glGetUniformLocation(Shader0.ID, "lightPositionWorld");
    glUniform3fv(lightPositionUniformLocation, 1, &lightPosition[0]);
    
    GLint eyePositionUniformLocation = glGetUniformLocation(Shader0.ID, "eyePositionWorld");
    glm::vec3 eyePosition(0.0f, 2.0f, 2.0f);
    glUniform3fv(eyePositionUniformLocation, 1, &eyePosition[0]);
    
    GLuint lightSpaceMatrixLocation = glGetUniformLocation(Shader0.ID, "lightSpaceMatrix");
    glUniformMatrix4fv(lightSpaceMatrixLocation, 1, GL_FALSE, &lightSpaceMatrix[0][0]);
    
    GLuint TextureID = glGetUniformLocation(Shader0.ID, "myTextureSampler0");
    GLuint shadowMapID = glGetUniformLocation(Shader0.ID, "shadowMap");
    
//    glBindVertexArray(objects[0].vaoID);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[0].eboID);
//    modelTransformMatrix =  modelTranslateMatrix * modelPreTranslateMatrix * modelRotateMatrix * modelScaleMatrix * modelPreScaleMatrix;
//    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
//    if(dolphin_texture == 0){
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, Texture0.ID);
//    }
//    else{
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, Texture2.ID);
//    }
//    glUniform1i(TextureID, 0);
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, depthMap);
//    glUniform1i(shadowMapID, 1);
//    glDrawElements(GL_TRIANGLES, (int)dolphin.indices.size(), GL_UNSIGNED_INT, 0);
//    glBindTexture(GL_TEXTURE_2D, 0);

    
//    glBindVertexArray(objects[2].vaoID);
//    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[2].eboID);
//    modelTransformMatrix = boatModelPreTranslateMatrix * boatModelPreScaleMatrix;
//    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, Texture4.ID);
//    glUniform1i(TextureID, 0);
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, depthMap);
//    glUniform1i(shadowMapID, 1);
//    glDrawElements(GL_TRIANGLES, (int)tower.indices.size(), GL_UNSIGNED_INT, 0);
//    glBindTexture(GL_TEXTURE_2D, 0);
    
    //space craft
    glBindVertexArray(objects[3].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[3].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
    modelTransformMatrix = spacecraftPreRotateMatrix * spacecraftPreScaleMatrix * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureSpacecraft_i.ID);
    glUniform1i(TextureID, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(shadowMapID, 1);
    glDrawElements(GL_TRIANGLES, (int)spaceCraft.indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //alien bundle
    int num;
    for (num=0;num<=3;num++)
    {
        glm::mat4 selfRotate = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime()/6, glm::vec3(0.0f,1.0f,0.0f));
        if (num>0)
        {
            alienTranslateMatrix = glm::translate(alienTranslateMatrix, glm::vec3(0.0f,0.0f,-8.0f));
        }
        //alienpeople
        glBindVertexArray(objects[4].vaoID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[4].eboID);
        modelTransformMatrix = glm::mat4(1.0f);
        modelTransformMatrix = alienTranslateMatrix * alienPeoplePrescaleMatrix * selfRotate * modelTransformMatrix;
        glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAlienPeople.ID);
        glUniform1i(TextureID, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(shadowMapID, 1);
        glDrawElements(GL_TRIANGLES, (int)alienPeople.indices.size(), GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        //alienvehicle
        glBindVertexArray(objects[5].vaoID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[5].eboID);
        modelTransformMatrix = glm::mat4(1.0f);
        modelTransformMatrix = alienTranslateMatrix * alienVehiclePrescaleMatrix * selfRotate * modelTransformMatrix;
        glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAlienPeople.ID);
        glUniform1i(TextureID, 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(shadowMapID, 1);
        glDrawElements(GL_TRIANGLES, (int)alienVehicle.indices.size(), GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    //planet
    glm::mat4 selfRotate = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime()/6, glm::vec3(0.0f,1.0f,0.0f));
    glBindVertexArray(objects[6].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[6].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
    modelTransformMatrix = planetTranslateMatrix * selfRotate * planetPrescaleMatrix * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturePlanet.ID);
    glUniform1i(TextureID, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glUniform1i(shadowMapID, 1);
    glDrawElements(GL_TRIANGLES, (int)planet.indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    
    
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Sets the mouse-button callback for the current window.
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
        mouseCrl.LEFT_BUTTON = true;
        firstmouse = true;
    }
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
        mouseCrl.LEFT_BUTTON = false;
        mouseCrl.MOUSE_Clickx = 0;
        mouseCrl.MOUSE_Clicky = 0;
    }
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    // Sets the cursor position callback for the current window
    if(mouseCrl.LEFT_BUTTON == true){
        if (firstmouse){
            last_x = x;
            last_y = y;
            firstmouse = false;
        }
        float xoffset = x - last_x;
        float yoffset = last_y - y;
        last_x = x;
        last_y = y;
        cam.ProcessMouseMovement(xoffset, yoffset);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Sets the scoll callback for the current window.
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Sets the Keyboard callback for the current window.
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        scale_parameter -= 1;
    }
    if(key == GLFW_KEY_X && action == GLFW_PRESS){
        scale_parameter += 1;
    }
    if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        dir_light_parameter += 1;
    }
    if(key == GLFW_KEY_S && action == GLFW_PRESS){
        dir_light_parameter -= 1;
    }
    if (key == GLFW_KEY_I && action == GLFW_PRESS) {
        y_shift_num += 1;
    }
    if(key == GLFW_KEY_K && action == GLFW_PRESS){
        y_shift_num -= 1;
    }
    if(key == GLFW_KEY_1 && action == GLFW_PRESS){
        dolphin_texture = 0;
    }
    if(key == GLFW_KEY_2 && action == GLFW_PRESS){
        dolphin_texture = 1;
    }
    if(key == GLFW_KEY_3 && action == GLFW_PRESS){
        sea_texture = 0;
    }
    if(key == GLFW_KEY_4 && action == GLFW_PRESS){
        sea_texture = 1;
    }
    if(key == GLFW_KEY_UP && action == GLFW_PRESS){
        z_shift_num -= 1;
    }
    if(key == GLFW_KEY_DOWN && action == GLFW_PRESS){
        z_shift_num += 1;
    }
    if(key == GLFW_KEY_LEFT && action == GLFW_PRESS){
        rotate_num += 1;
    }
    if(key == GLFW_KEY_RIGHT && action == GLFW_PRESS){
        rotate_num -= 1;
    }
}


int main(int argc, char* argv[])
{
    GLFWwindow* window;

    /* Initialize the glfw */
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    /* glfw: configure; necessary for MAC */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 2", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /*register callback functions*/
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    initializedGL();

    while (!glfwWindowShouldClose(window)) {
        /* Render here */
        paintGL();

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    glfwTerminate();
    
    free(objects);

    return 0;
}
