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
float rotate_num = 0;
float x_delta = 0.15f;
float z_delta = 0.15f;
float r_delta = 2.0f;
bool firstmouse = true;
float last_x;
float last_y;
float translate_x = 0.0f;
float translate_z = 0.0f;
bool key_w = false;
bool key_s = false;
bool key_a = false;
bool key_d = false;
glm::vec3 new_translate_vector = glm::vec3(0.0f);
const float COLLI_THRESHOLD_CHICKEN = 1.0f;
bool swap_alien_vehicle_texture[4] = {false};
bool chicken_collision[4] = {false};
bool swap_spacecraft_texture = false;

unsigned int amount_rock = 1500;
unsigned int amount_pie = 15;
unsigned int amount_tomato = 15;


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


Object* objects = (Object*)malloc(sizeof(Object) * 22);
//skybox: 19, planet: 18, rocks: 17, spacecraft: 16, alien_people: 0-3, alien_vehicle: 4-7, chicken: 8-11, tomato: 20, pie: 21

Shader Shader0; //object shader
Shader Shader1; //skybox shader
Shader Shader2; //rock shader
Shader Shader3; //planet

Texture textureSpacecraft1;
Texture textureSpacecraft2;
Texture textureAlienPeople;
Texture textureAlienVehicle;
Texture texturePlanet;
Texture textureChicken;
Texture textureRock;
Texture texturePlanetNM;

Texture textureTomato;
Texture texturePie;


GLuint cubemapTexture;

int spaceCraftForward = 0;
int spaceCraftHorizontal = 0;

//newly added
Model spaceCraft;
Model alienPeople;
Model alienVehicle;
Model planet;
Model chicken;
Model rock;
Model tomato;
Model pie;

Camera cam;

void generateInstancedArray(unsigned int amount, int id, float radius, int scale_m)
{
    glm::mat4* modelMatrices;
    modelMatrices = new glm::mat4[amount];
    srand(glfwGetTime()); // initialize random seed
//    float radius = 150.0;
    float offset = 25.0f;
    
    for (unsigned int i = 0; i < amount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(1.0f,0.0f,0.0f));
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x/10, y/10, z/10));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = (rand() % scale_m) / 100.0f + 0.05;
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = (rand() % 360);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        
        // 4. now add to list of matrices
        modelMatrices[i] = model;
//        modelMatrices[i] = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f,0.5f,0.5f));
    }
    
//    bind the buffer
    
        unsigned int rockBuffer;
        glGenBuffers(1, &rockBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, rockBuffer);
        glBufferData(GL_ARRAY_BUFFER, amount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

        unsigned int VAO = objects[id].vaoID;
        glBindVertexArray(VAO);
        // set attribute pointers for matrix (4 times vec4)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4)));
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(2 * sizeof(glm::vec4)));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(3 * sizeof(glm::vec4)));

        glVertexAttribDivisor(3, 1);
        glVertexAttribDivisor(4, 1);
        glVertexAttribDivisor(5, 1);
        glVertexAttribDivisor(6, 1);

        glBindVertexArray(0);
}


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

bool collision_detection(glm::mat4 matA, glm::mat4 matB, const float COLLI_THRESHOLD){
    glm::vec4 vecA = matA * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 vecB = matB * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if(glm::distance(vecA, vecB) <= COLLI_THRESHOLD){
        return true;
    }
    else return false;
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
    
    //skybox: 19, planet: 18, rocks: 17, spacecraft: 16, alien_people: 0-3, alien_vehicle: 4-7, chicken: 8-11, tomato: 20, pie: 21
    
    //alienpeople
    alienPeople = loadOBJ("./resources/alienpeople/alienpeople.obj");
    for(int k = 0; k <= 3; k++){
        load_Data(alienPeople, k);
    }
    
    //alien vehicle
    alienVehicle = loadOBJ("./resources/alienvehicle/alienvehicle.obj");
    for(int k = 4; k <= 7; k++){
        load_Data(alienVehicle, k);
    }
    
    //chicken
    chicken = loadOBJ("./resources/chicken/chicken.obj");
    for(int k = 8; k <= 11; k++){
        load_Data(chicken, k);
    }
    
    //spacecraft
    spaceCraft = loadOBJ("./resources/spacecraft/spacecraft.obj");
    load_Data(spaceCraft, 16);
    
    //rock
    rock = loadOBJ("./resources/rock/rock.obj");
    load_Data(rock, 17);
    
    //planet
    planet = loadOBJ("./resources/planet/planet.obj");
    load_Data(planet, 18);
    
    //tomato
    tomato = loadOBJ("./resources/tomato/tomato.obj");
    load_Data(tomato, 20);
    
    //pie
    pie = loadOBJ("./resources/pie/pie.obj");
    load_Data(pie, 21);
    
    
    
    //skybox
    GLfloat skyboxVertices[] = {
        -50.0f, 50.0f, -50.0f, //0
        -50.0f, -50.0f, -50.0f, //1
        50.0f, -50.0f, -50.0f, //2
        50.0f, 50.0f, -50.0f, //3
        -50.0f, 50.0f, 50.0f, //4
        -50.0f, -50.0f, 50.0f, //5
        50.0f, -50.0f, 50.0f, //6
        50.0f, 50.0f, 50.0f //7
    };
    GLuint skyboxIndices[] = {
        2, 6, 7, 7, 3, 2,
        1, 0, 4, 4, 5, 1,
        0, 3, 7, 7, 4, 0,
        1, 5, 6, 6, 2, 1,
        1, 2, 3, 3, 0, 1,
        5, 4, 7, 7, 6, 5
    };
    glGenVertexArrays(1, &objects[19].vaoID);
    glBindVertexArray(objects[19].vaoID);
    glGenBuffers(1, &objects[19].vboID);
    glBindBuffer(GL_ARRAY_BUFFER, objects[19].vboID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glGenBuffers(1, &objects[19].eboID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[19].eboID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    
    
    
    //Load Textures
    textureSpacecraft1.setupTexture("./resources/spacecraft/spacecraftTexture.bmp");
    textureSpacecraft2.setupTexture("./resources/spacecraft/leisure_spacecraftTexture.bmp");
    textureAlienPeople.setupTexture("./resources/alienpeople/alienTexture.bmp");
    textureAlienVehicle.setupTexture("./resources/alienvehicle/colorful_alien_vehicleTexture.bmp");
    texturePlanet.setupTexture("./resources/planet/planetTexture.bmp");
    textureChicken.setupTexture("./resources/chicken/chickenTexture.bmp");
    textureRock.setupTexture("./resources/rock/rockTexture.bmp");
    texturePlanetNM.setupTexture("./resources/planet/planetNormal.bmp");
    
    texturePie.setupTexture("./resources/pie/pie_tex.jpg");
    textureTomato.setupTexture("./resources/tomato/TomatoBeef.jpg");
    
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
    
    generateInstancedArray(amount_rock, 17, 150, 20);
    generateInstancedArray(amount_pie, 21, 250, 3);
    generateInstancedArray(amount_tomato, 20, 350, 3);
    
    

    
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
    Shader2.setupShader("RockVertexShaderCode.glsl", "RockFragmentShaderCode.glsl");
    Shader3.setupShader("NMVertexShaderCode.glsl", "NMFragmentShaderCode.glsl");
    cam = Camera(glm::vec3(0.0f, 1.5f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -30.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void paintGL(void)  //always run
{
    // for debug purpose
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // skybox: 19, planet: 18, rocks: 17, spacecraft: 16, alien_people: 0-3, alien_vehicle: 4-7, chicken: 8-11

    glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
    GLint modelTransformMatrixUniformLocation = glGetUniformLocation(Shader0.ID, "modelTransformMatrix");
    
    //spacecraft
    glm::mat4 spacecraftPreScaleMatrix = glm::mat4(1.0f);
    spacecraftPreScaleMatrix = glm::scale(spacecraftPreScaleMatrix, glm::vec3(0.002f,0.002f,0.002f));
    glm::mat4 spacecraftPreRotateMatrix = glm::mat4(1.0f);
    spacecraftPreRotateMatrix = glm::rotate(spacecraftPreRotateMatrix,glm::radians(180.0f), glm::vec3(1.0f,0.0f,0.0f));
    glm::mat4 spacecraftRotateMatrix = glm::mat4(1.0f);
    spacecraftRotateMatrix = glm::rotate(spacecraftRotateMatrix, glm::radians(rotate_num), glm::vec3(0.0f,1.0f,0.0f));
    glm::vec3 localFrontVector = glm::vec3(0.0f,0.0f,-1.0f);
    glm::vec3 localRightVector = glm::vec3(1.0f,0.0f,0.0f);
    glm::vec3 worldFrontVector = glm::normalize(spacecraftRotateMatrix * spacecraftPreRotateMatrix * glm::vec4(localFrontVector, 1.0f));
    glm::vec3 worldRightVector = glm::normalize(spacecraftRotateMatrix * spacecraftPreRotateMatrix * glm::vec4(localRightVector, 1.0f));
    glm::mat4 spacecraftTranslateMatrix = glm::mat4(1.0f);
    if(key_w){
        new_translate_vector = glm::vec3(translate_x,0.0f,translate_z) - z_delta * worldFrontVector;
        translate_x = new_translate_vector.x;
        translate_z = new_translate_vector.z;
    }
    if(key_s){
        new_translate_vector = glm::vec3(translate_x,0.0f,translate_z) + z_delta * worldFrontVector;
        translate_x = new_translate_vector.x;
        translate_z = new_translate_vector.z;
    }
    if(key_a){
        new_translate_vector = glm::vec3(translate_x,0.0f,translate_z) - x_delta * worldRightVector;
        translate_x = new_translate_vector.x;
        translate_z = new_translate_vector.z;
    }
    if(key_d){
        new_translate_vector = glm::vec3(translate_x,0.0f,translate_z) + x_delta * worldRightVector;
        translate_x = new_translate_vector.x;
        translate_z = new_translate_vector.z;
    }
    spacecraftTranslateMatrix = glm::translate(spacecraftTranslateMatrix, new_translate_vector);
    
    //projection
    glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), ((float)SCR_WIDTH) / SCR_HEIGHT, 0.1f, 100.0f);
    cam = Camera(spacecraftTranslateMatrix * spacecraftRotateMatrix * glm::vec4(0.0f, 1.5f, 3.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f - rotate_num, -15.0f);
    glm::mat4 viewMatrix = cam.GetViewMatrix();
    projectionMatrix =  projectionMatrix * viewMatrix;
    glm::vec3 lightPosition(0.5f, 2.0f, 2.0f);
    
    //alienpeople
    glm::mat4 alienPeoplePrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(0.25f,0.25f,0.25f));
    
    //alienvehicle
    glm::mat4 alienVehiclePrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(0.15f,0.15f,0.15f));
    
    //alientranslate matrix
    glm::mat4 alienOriginalTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f,0.0f,-4.0f));
    glm::mat4 alienTranslateMatrix = glm::mat4(1.0f);
    
    //chicken parameter
    glm::mat4 chickenPrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(0.002f,0.002f,0.002f));
    glm::mat4 chickenOriginalTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f,0.0f,-4.0f));
    glm::mat4 chickenTranslateMatrix = glm::mat4(1.0f);
    glm::mat4 chickenPreRotateMatrix = glm::rotate(glm::mat4(1.0f),glm::radians(90.0f), glm::vec3(0.0f,0.0f,1.0f));
    
    //planet setup
    glm::mat4 planetPrescaleMatrix = glm::scale(glm::mat4(1.0f),glm::vec3(1.5f,1.5f,1.5f));
    glm::mat4 planetTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,-40.0f));
    glm::mat4 rockTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,1.2f,-40.0f));
    glm::mat4 tomatoTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,1.2f,-40.0f));
    glm::mat4 pieTranslateMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,1.2f,-40.0f));
    
    //rotation
    glm::mat4 selfRotate = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime()/6, glm::vec3(0.0f,1.0f,0.0f));
    glm::mat4 selfRotateTomato = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime()/100, glm::vec3(0.0f,1.0f,0.0f));
    glm::mat4 selfRotatePie = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime()/100, glm::vec3(0.0f,1.0f,0.0f));
    
    glm::mat4 tomatoPrescale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f,0.1f,0.1f));
    glm::mat4 piePrescale = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f,0.1f,0.1f));
    
    //rendering start here
    glViewport(0, 0, SCR_WIDTH*2, SCR_HEIGHT*2);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //rendering the skybox
    glDepthMask(GL_FALSE);
    Shader1.use();
    glBindVertexArray(objects[19].vaoID);
    GLint SBProjectionMatrixUniformLocation = glGetUniformLocation(Shader1.ID, "projectionMatrix");
    glUniformMatrix4fv(SBProjectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[19].eboID);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glDepthMask(GL_TRUE);
    //end
    
    Shader0.use();
    
    GLint projectionMatrixUniformLocation = glGetUniformLocation(Shader0.ID, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    GLint dirLightParameterUniformLocation = glGetUniformLocation(Shader0.ID, "dir_light_parameter");
    glUniform1i(dirLightParameterUniformLocation, dir_light_parameter);
    
    GLint lightPositionUniformLocation = glGetUniformLocation(Shader0.ID, "lightPositionWorld");
    glUniform3fv(lightPositionUniformLocation, 1, &lightPosition[0]);
    
    GLint eyePositionUniformLocation = glGetUniformLocation(Shader0.ID, "eyePositionWorld");
    glm::vec3 eyePosition = cam.Position;
    glUniform3fv(eyePositionUniformLocation, 1, &eyePosition[0]);
    

    GLuint TextureID = glGetUniformLocation(Shader0.ID, "myTextureSampler0");
    
    //space craft
    glBindVertexArray(objects[16].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[16].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
    modelTransformMatrix = spacecraftTranslateMatrix * spacecraftRotateMatrix * spacecraftPreRotateMatrix * spacecraftPreScaleMatrix * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    if(!swap_spacecraft_texture){
        glBindTexture(GL_TEXTURE_2D, textureSpacecraft1.ID);
    }
    else{
        glBindTexture(GL_TEXTURE_2D, textureSpacecraft2.ID);
    }
    glUniform1i(TextureID, 0);
    glDrawElements(GL_TRIANGLES, (int)spaceCraft.indices.size(), GL_UNSIGNED_INT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //alien bundle
    for (int k = 0; k <= 3; k++)
    {
        glm::mat4 selfRotate = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime()/6, glm::vec3(0.0f,1.0f,0.0f));
        if (k >= 0)
        {
            if (k==0)
            {
                alienTranslateMatrix = glm::translate(alienOriginalTranslateMatrix, glm::vec3(0.0f,0.0f,0.0f));
                chickenTranslateMatrix = glm::translate(chickenOriginalTranslateMatrix, glm::vec3(-4.0f,0.0f,0.0f));
            }
            if (k==1)
            {
                alienTranslateMatrix = glm::translate(alienOriginalTranslateMatrix, glm::vec3(4.0f,0.0f,-8.0f));
                chickenTranslateMatrix = glm::translate(chickenOriginalTranslateMatrix, glm::vec3(6.0f,0.0f,-8.0f));
            }
            if (k==2)
            {
                alienTranslateMatrix = glm::translate(alienOriginalTranslateMatrix, glm::vec3(-4.0f,0.0f,-16.0f));
                chickenTranslateMatrix = glm::translate(chickenOriginalTranslateMatrix, glm::vec3(-8.0f,0.0f,-15.0f));
            }
            if (k==3)
            {
                alienTranslateMatrix = glm::translate(alienOriginalTranslateMatrix, glm::vec3(0.0f,0.0f,-24.0f));
                chickenTranslateMatrix = glm::translate(chickenOriginalTranslateMatrix, glm::vec3(2.0f,0.0f,-25.0f));
            }
        }
        
        //alienpeople
        glBindVertexArray(objects[k].vaoID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[k].eboID);
        modelTransformMatrix = glm::mat4(1.0f);
        modelTransformMatrix = alienTranslateMatrix * alienPeoplePrescaleMatrix * selfRotate * modelTransformMatrix;
        glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureAlienPeople.ID);
        glUniform1i(TextureID, 0);
        glDrawElements(GL_TRIANGLES, (int)alienPeople.indices.size(), GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        //alienvehicle
        if(chicken_collision[k]){
            swap_alien_vehicle_texture[k] = true;
        }
        glBindVertexArray(objects[k + 4].vaoID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[k + 4].eboID);
        modelTransformMatrix = glm::mat4(1.0f);
        modelTransformMatrix = alienTranslateMatrix * alienVehiclePrescaleMatrix * selfRotate * modelTransformMatrix;
        glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
        glActiveTexture(GL_TEXTURE0);
        if(!swap_alien_vehicle_texture[k]){
            glBindTexture(GL_TEXTURE_2D, textureAlienPeople.ID);
        }
        else{
            glBindTexture(GL_TEXTURE_2D, textureAlienVehicle.ID);
        }
        glUniform1i(TextureID, 0);
        glDrawElements(GL_TRIANGLES, (int)alienVehicle.indices.size(), GL_UNSIGNED_INT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        if(collision_detection(spacecraftTranslateMatrix, chickenTranslateMatrix, COLLI_THRESHOLD_CHICKEN)){
            chicken_collision[k] = true;
        }
        if(chicken_collision[0] && chicken_collision[1] && chicken_collision[2] && chicken_collision[3]){
            swap_spacecraft_texture = true;
        }
        //chicken
        if(!chicken_collision[k]){
            glBindVertexArray(objects[k + 8].vaoID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[k + 8].eboID);
            modelTransformMatrix = glm::mat4(1.0f);
            modelTransformMatrix = chickenTranslateMatrix * chickenPreRotateMatrix * chickenPrescaleMatrix * modelTransformMatrix;
            glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureChicken.ID);
            glUniform1i(TextureID, 0);
            glDrawElements(GL_TRIANGLES, (int)chicken.indices.size(), GL_UNSIGNED_INT, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    
    GLuint PlanetTextureID = glGetUniformLocation(Shader3.ID, "myTextureSampler0");
    GLuint PlanetNormalMapID = glGetUniformLocation(Shader3.ID, "myTextureSampler1");
    
    Shader3.use();
    
    projectionMatrixUniformLocation = glGetUniformLocation(Shader3.ID, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    dirLightParameterUniformLocation = glGetUniformLocation(Shader3.ID, "dir_light_parameter");
    glUniform1i(dirLightParameterUniformLocation, dir_light_parameter);
    
    lightPositionUniformLocation = glGetUniformLocation(Shader3.ID, "lightPositionWorld");
    glUniform3fv(lightPositionUniformLocation, 1, &lightPosition[0]);
    
    eyePositionUniformLocation = glGetUniformLocation(Shader3.ID, "eyePositionWorld");
    eyePosition = cam.Position;
    glUniform3fv(eyePositionUniformLocation, 1, &eyePosition[0]);
    
   
    
    //planet
    
    glBindVertexArray(objects[18].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[18].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
    modelTransformMatrix = planetTranslateMatrix * selfRotate * planetPrescaleMatrix * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    //1
    texturePlanet.bind(0);
    Shader3.setInt("myTextureSampler0", 0);
    texturePlanetNM.bind(1);
    Shader3.setInt("myTextureSampler1", 1);
//    texturePlanetNM.bind(2);
//    Shader3.setInt("myTextureSampler_normal", 2);

    glActiveTexture(GL_TEXTURE0);
    glActiveTexture(GL_TEXTURE1);
//    glActiveTexture(GL_TEXTURE2);
    
    //2
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, 0);
//    glUniform1i(PlanetTextureID, 0);
//
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, 1);
//    glUniform1i(PlanetNormalMapID, 1);
    
    //3
//    glUniform1i(PlanetTextureID, 0);
//    glUniform1i(PlanetNormalMapID, 1);
//
//    glActiveTexture(GL_TEXTURE0 + 0);
//    glBindTexture(GL_TEXTURE_2D, texturePlanet.ID);
//
//    glActiveTexture(GL_TEXTURE0 + 1);
//    glBindTexture(GL_TEXTURE_2D, texturePlanetNM.ID);
    
    glDrawElements(GL_TRIANGLES, (int)planet.indices.size(), GL_UNSIGNED_INT, 0);
//    glBindTexture(GL_TEXTURE_2D, 0);
    
    Shader2.use();
    
    projectionMatrixUniformLocation = glGetUniformLocation(Shader2.ID, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    dirLightParameterUniformLocation = glGetUniformLocation(Shader2.ID, "dir_light_parameter");
    glUniform1i(dirLightParameterUniformLocation, dir_light_parameter);
    
    lightPositionUniformLocation = glGetUniformLocation(Shader2.ID, "lightPositionWorld");
    glUniform3fv(lightPositionUniformLocation, 1, &lightPosition[0]);
    
    eyePositionUniformLocation = glGetUniformLocation(Shader2.ID, "eyePositionWorld");
    eyePosition = cam.Position;
    glUniform3fv(eyePositionUniformLocation, 1, &eyePosition[0]);
    

    TextureID = glGetUniformLocation(Shader2.ID, "myTextureSampler0");
    
    //rocks
    glBindVertexArray(objects[17].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[17].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
//    planetPrescaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f,10.0f,10.0f));
    modelTransformMatrix = rockTranslateMatrix * selfRotate * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureRock.ID);
    glUniform1i(TextureID, 0);
    glDrawElementsInstanced(GL_TRIANGLES, (int)rock.indices.size(), GL_UNSIGNED_INT, 0, amount_rock);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //tomato
    glBindVertexArray(objects[20].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[20].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
//    planetPrescaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f,10.0f,10.0f));
    modelTransformMatrix = tomatoTranslateMatrix * selfRotateTomato * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureTomato.ID);
    glUniform1i(TextureID, 0);
    glDrawElementsInstanced(GL_TRIANGLES, (int)tomato.indices.size(), GL_UNSIGNED_INT, 0, amount_tomato);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    //pie
    glBindVertexArray(objects[21].vaoID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, objects[21].eboID);
    modelTransformMatrix = glm::mat4(1.0f);
//    planetPrescaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f,10.0f,10.0f));
    modelTransformMatrix = pieTranslateMatrix * selfRotatePie * modelTransformMatrix;
    glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturePie.ID);
    glUniform1i(TextureID, 0);
    glDrawElementsInstanced(GL_TRIANGLES, (int)pie.indices.size(), GL_UNSIGNED_INT, 0, amount_pie);
    glBindTexture(GL_TEXTURE_2D, 0);
    
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // Sets the mouse-button callback for the current window.
    
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
    // Sets the cursor position callback for the current window
    if (firstmouse){
        last_x = x;
        last_y = y;
        firstmouse = false;
    }
    if(x < last_x){
        rotate_num += r_delta;
    }
    if(x > last_x){
        rotate_num -= r_delta;
    }
    last_x = x;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Sets the scoll callback for the current window.
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // Sets the Keyboard callback for the current window.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_W){
        key_w = true;
    }
    if (key == GLFW_KEY_W && action == GLFW_RELEASE){
        key_w = false;
    }
    if(key == GLFW_KEY_S){
        key_s = true;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE){
        key_s = false;
    }
    if(key == GLFW_KEY_A){
        key_a = true;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE){
        key_a = false;
    }
    if(key == GLFW_KEY_D){
        key_d = true;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE){
        key_d = false;
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
