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
int scale_parameter = 0;
int dir_light_parameter = 2;
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

Shader Shader0;
Shader Shader1;

Texture textureSpacecraft_i;
Texture textureSpacecraft_f;
Texture textureAlienPeople;
Texture textureAlienVehicle;
Texture texturePlanet;
Texture textureChicken;
GLuint cubemapTexture;

int spaceCraftForward = 0;
int spaceCraftHorizontal = 0;

//newly added
Model spaceCraft;
Model alienPeople;
Model alienVehicle;
Model planet;
Model chicken;

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

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
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
    
    //spacecraft
    spaceCraft = loadOBJ("./resources/spacecraft/spacecraft.obj");
    load_Data(spaceCraft, 3);
    //    spaceCraft = loadOBJ("./resources/alienpeople/alienpeople.obj");
    //    glGenVertexArrays(1, &objects[3].vaoID);
    //    glBindVertexArray(objects[3].vaoID);
    //    glGenBuffers(1, &objects[3].vboID);
    //    glBindBuffer(GL_ARRAY_BUFFER, objects[3].vboID);
    //    glBufferData(GL_ARRAY_BUFFER, spaceCraft.vertices.size() * sizeof(Vertex), &spaceCraft.vertices[0], GL_STATIC_DRAW);
    //    glGenBuffers(1, &objects[3].eboID);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[3].eboID);
    //    glBufferData(GL_ELEMENT_ARRAY_BUFFER, spaceCraft.indices.size() * sizeof(unsigned int), &spaceCraft.indices[0], GL_STATIC_DRAW);
    //    glEnableVertexAttribArray(0);
    //    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    //    glEnableVertexAttribArray(1);
    //    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    //    glEnableVertexAttribArray(2);
    //    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    

    //alienpeople
    alienPeople = loadOBJ("./resources/alienpeople/alienpeople.obj");
    load_Data(alienPeople, 4);
    //    glGenVertexArrays(1, &objects[4].vaoID);
    //    glBindVertexArray(objects[4].vaoID);
    //    glGenBuffers(1, &objects[4].vboID);
    //    glBindBuffer(GL_ARRAY_BUFFER, objects[4].vboID);
    //    glBufferData(GL_ARRAY_BUFFER, alienPeople.vertices.size() * sizeof(Vertex), &alienPeople.vertices[0], GL_STATIC_DRAW);
    //    glGenBuffers(1, &objects[4].eboID);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[4].eboID);
    //    glBufferData(GL_ELEMENT_ARRAY_BUFFER, alienPeople.indices.size() * sizeof(unsigned int), &alienPeople.indices[0], GL_STATIC_DRAW);
    //    glEnableVertexAttribArray(0);
    //    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    //    glEnableVertexAttribArray(1);
    //    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    //    glEnableVertexAttribArray(2);
    //    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    
    //alien vehicle
    alienVehicle = loadOBJ("./resources/alienvehicle/alienvehicle.obj");
    load_Data(alienVehicle, 5);
    
    //planet
    planet = loadOBJ("./resources/planet/planet.obj");
    load_Data(planet, 6);
    
    //chicken
    chicken = loadOBJ("./resources/chicken/chicken.obj");
    load_Data(chicken, 7);
    
    //skybox: 8
    GLfloat skyboxVertices[] = {
        -10.0f, 10.0f, -10.0f, //0
        -10.0f, -10.0f, -10.0f, //1
        10.0f, -10.0f, -10.0f, //2
        10.0f, 10.0f, -10.0f, //3
        -10.0f, 10.0f, 10.0f, //4
        -10.0f, -10.0f, 10.0f, //5
        10.0f, -10.0f, 10.0f, //6
        10.0f, 10.0f, 10.0f //7
    };
    GLuint skyboxIndices[] = {
        2, 6, 7, 7, 3, 2,
        1, 0, 4, 4, 5, 1,
        0, 3, 7, 7, 4, 0,
        1, 5, 6, 6, 2, 1,
        1, 2, 3, 3, 0, 1,
        5, 4, 7, 7, 6, 5
    };
    glGenVertexArrays(1, &objects[8].vaoID);
    glBindVertexArray(objects[8].vaoID);
    glGenBuffers(1, &objects[8].vboID);
    glBindBuffer(GL_ARRAY_BUFFER, objects[8].vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glGenBuffers(1, &objects[8].eboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[8].eboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    
    
    //Load Textures
    textureSpacecraft_i.setupTexture("./resources/spacecraft/spacecraftTexture.bmp");
    textureAlienPeople.setupTexture("./resources/alienpeople/alienTexture.bmp");
    textureAlienVehicle.setupTexture("./resources/alienvehicle/colorful_alien_vehicleTexture.bmp");
    texturePlanet.setupTexture("./resources/planet/planetTexture.bmp");
    textureChicken.setupTexture("./resources/chicken/chickenTexture.bmp");
    
    std::vector<std::string> faces
    {
        "./resources/universe_skybox/right.bmp",
        "./resources/universe_skybox/left.bmp",
        "./resources/universe_skybox/top.bmp",
        "./resources/universe_skybox/bottom.bmp",
        "./resources/universe_skybox/front.bmp",
        "./resources/universe_skybox/back.bmp"
    };
    cubemapTexture = loadCubemap(faces);
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
    Shader1.setupShader("SBVertexShaderCode.glsl", "SBFragmentShaderCode.glsl");
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
    
    glm::mat4 modelRotateMatrix = glm::mat4(1.0f);
    modelRotateMatrix = glm::rotate(modelRotateMatrix, glm::radians(90.0f + r_delta * rotate_num), glm::vec3(0.0f, 1.0f, 0.0f));
    modelRotateMatrix = glm::rotate(modelRotateMatrix, glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
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
    
    //chicken parameter
    glm::mat4 chickenPrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(0.002f,0.002f,0.002f));
    glm::mat4 chickenTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,0.0f,-4.0f));
    glm::mat4 chickenPreRotateMatrix = glm::rotate(glm::mat4(1.0f),glm::radians(90.0f), glm::vec3(0.0f,0.0f,1.0f));
    
    //planet setup
    glm::mat4 planetPrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(1.5f,1.5f,1.5f));
    glm::mat4 planetTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,-40.0f));
    
    //rendering start here
    glViewport(0, 0, SCR_WIDTH*2, SCR_HEIGHT*2);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //rendering the skybox
    glDepthMask(GL_FALSE);
    Shader1.use();
    glBindVertexArray(objects[8].vaoID);
    GLint SBProjectionMatrixUniformLocation = glGetUniformLocation(Shader1.ID, "projectionMatrix");
    glUniformMatrix4fv(SBProjectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[8].eboID);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);
    
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
    

    GLuint TextureID = glGetUniformLocation(Shader0.ID, "myTextureSampler0");
    
    //space craft
    glBindVertexArray(objects[3].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[3].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
    modelTransformMatrix = spacecraftPreRotateMatrix * spacecraftPreScaleMatrix * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureSpacecraft_i.ID);
    glUniform1i(TextureID, 0);
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
            chickenTranslateMatrix = glm::translate(chickenTranslateMatrix, glm::vec3(0.0f,0.0f,-8.0f));
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
        glDrawElements(GL_TRIANGLES, (int)alienVehicle.indices.size(), GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        //chicken
        glBindVertexArray(objects[7].vaoID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[7].eboID);
        modelTransformMatrix = glm::mat4(1.0f);
        modelTransformMatrix = chickenTranslateMatrix * chickenPreRotateMatrix * chickenPrescaleMatrix * modelTransformMatrix;
        glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureChicken.ID);
        glUniform1i(TextureID, 0);
        glDrawElements(GL_TRIANGLES, (int)chicken.indices.size(), GL_UNSIGNED_INT, 0);
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
