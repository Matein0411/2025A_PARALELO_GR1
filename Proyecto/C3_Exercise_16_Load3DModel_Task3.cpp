#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <vector>
#include <iostream>

#include <AL/al.h>
#include <AL/alc.h>
#include <fstream>
#include <vector>
#include <array>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>

// Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// Settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Camera
Camera camera(glm::vec3(0.0f, 10.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool spotlightOn = false;
bool keyPressed = false;

// pasos
const int NUM_STEPS = 4;
ALuint stepBuffers[NUM_STEPS] = { 0 };
int currentStep = 0;
float lastStepTime = 0.0f;
float stepInterval = 0.4f;
std::vector<ALuint> stepSources;

// Carga un archivo WAV PCM simple
bool LoadWavFile(const char* filename, std::vector<char>& buffer, ALenum& format, ALsizei& freq)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;

    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) return false;

    file.seekg(22, std::ios::beg); // Saltar a canales
    short channels;
    file.read(reinterpret_cast<char*>(&channels), 2);

    file.read(reinterpret_cast<char*>(&freq), 4);

    file.seekg(34, std::ios::beg); // Saltar a bits por muestra
    short bitsPerSample;
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    // Buscar "data"
    char dataHeader[4];
    int dataSize = 0;
    while (file.read(dataHeader, 4)) {
        file.read(reinterpret_cast<char*>(&dataSize), 4);
        if (std::strncmp(dataHeader, "data", 4) == 0) break;
        file.seekg(dataSize, std::ios::cur);
    }
    if (dataSize == 0) return false;

    buffer.resize(dataSize);
    file.read(buffer.data(), dataSize);

    // Formato OpenAL
    if (channels == 1) {
        format = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
    }
    else {
        format = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
    }
    return true;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

int main()
{
    // Init GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 16 Task 2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        return -1;
    }

    // OpenGL configuration
    glEnable(GL_DEPTH_TEST);

    // Build and compile shader
    Shader lightCubeShader("shaders/lightcube.vs", "shaders/lightcube.fs");
    Shader ourShader("shaders/shader_exercise16_mloading.vs", "shaders/shader_exercise16_mloading.fs");

    // Load models
    Model sceneModel("models/FNAF2/FNAF2.obj");
    Model foxyModel("models/foxy_the_pirate_fox/foxy.obj");
    Model jackModel("models/jack-o-bonnie_rig/osoRoto.obj");
    Model frankeyModel("models/frankey/zomb.obj");
    Model bunnyModel("models/bunny/bunny.obj");

    //********************************************************************
    // Cargar frames de animaci贸n para Freddy
    std::vector<Model> freddyFrames;
    int totalFrames = 18; // N煤mero de frames de animaci贸n
    float fps = 13.0f;    // Velocidad de animaci贸n

    for (int i = 1; i <= totalFrames; ++i) {
        std::stringstream ss;
        ss << "models/bearFreddy/pos" << i << ".obj";
        freddyFrames.push_back(Model(ss.str()));
    }
    //*******************************************************************

    // 1. Mapa de emisi贸n de Foxy
    unsigned int emissionMap;
    glGenTextures(1, &emissionMap);
    glBindTexture(GL_TEXTURE_2D, emissionMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("models/foxy_the_pirate_fox/mapaOJOS.png", &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else { std::cout << "Error al cargar mapaOJOS.png\n"; }
    stbi_image_free(data);

    // 2. Mapa de emisi贸n de Jack-O-Bonnie
    unsigned int jackEmissionMap;
    glGenTextures(1, &jackEmissionMap);
    glBindTexture(GL_TEXTURE_2D, jackEmissionMap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int jackWidth, jackHeight, jackChannels;
    unsigned char* jackData = stbi_load("models/jack-o-bonnie_rig/piel.png", &jackWidth, &jackHeight, &jackChannels, 0);
    if (jackData) {
        GLenum format = (jackChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, jackWidth, jackHeight, 0, format, GL_UNSIGNED_BYTE, jackData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else { std::cout << "Error al cargar emissionJack.png\n"; }
    stbi_image_free(jackData);

    // 3. Mapa de emisi贸n de Frankey
    unsigned int frankeyEmissionMap;
    glGenTextures(1, &frankeyEmissionMap);
    glBindTexture(GL_TEXTURE_2D, frankeyEmissionMap);
    // Configurar par谩metros de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int frankeyWidth, frankeyHeight, frankeyChannels;
    unsigned char* frankeyData = stbi_load("models/frankey/green.png", &frankeyWidth, &frankeyHeight, &frankeyChannels, 0);
    if (frankeyData) {
        GLenum format = (frankeyChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, frankeyWidth, frankeyHeight, 0, format, GL_UNSIGNED_BYTE, frankeyData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else { std::cout << "Error al cargar green.png\n"; }
    stbi_image_free(frankeyData);

    camera.MovementSpeed = 6.0f;

    // --- OpenAL ---
    ALCdevice* device = alcOpenDevice(NULL);
    ALCcontext* context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    // --- M鷖ica de ambiente ---
    std::vector<char> audioData;
    ALenum format;
    ALsizei freq;
    ALuint bufferId = 0, sourceId = 0;
    if (!LoadWavFile("sounds/spooky_escene.wav", audioData, format, freq)) {
        std::cout << "No se pudo cargar el archivo de m鷖ica ambiente." << std::endl;
    }
    else {
        alGenBuffers(1, &bufferId);
        alBufferData(bufferId, format, audioData.data(), (ALsizei)audioData.size(), freq);

        alGenSources(1, &sourceId);
        alSourcei(sourceId, AL_BUFFER, bufferId);
        alSourcei(sourceId, AL_LOOPING, AL_TRUE);
        alSourcef(sourceId, AL_GAIN, 0.2f);
        alSourcePlay(sourceId);
    }

    // --- Sonidos de pasos ---
    for (int i = 0; i < NUM_STEPS; ++i) {
        std::vector<char> stepData;
        ALenum stepFormat;
        ALsizei stepFreq;
        std::string filename = "sounds/step" + std::to_string(i + 1) + ".wav";
        //std::string filename = "sounds/step.wav";
        if (!LoadWavFile(filename.c_str(), stepData, stepFormat, stepFreq)) {
            std::cout << "No se pudo cargar " << filename << std::endl;
        }
        else {
            alGenBuffers(1, &stepBuffers[i]);
            alBufferData(stepBuffers[i], stepFormat, stepData.data(), (ALsizei)stepData.size(), stepFreq);
        }
    }

    // Forma Pointlight
    float cubeVertices[] = {
        // positions
        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,

        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f, -0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,

         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,

        -0.1f, -0.1f, -0.1f,
         0.1f, -0.1f, -0.1f,
         0.1f, -0.1f,  0.1f,
         0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f,  0.1f,
        -0.1f, -0.1f, -0.1f,

        -0.1f,  0.1f, -0.1f,
         0.1f,  0.1f, -0.1f,
         0.1f,  0.1f,  0.1f,
         0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f,  0.1f,
        -0.1f,  0.1f, -0.1f
    };

    unsigned int lightCubeVAO, VBO;
    glGenVertexArrays(1, &lightCubeVAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Elimina fuentes de pasos que ya terminaron
        for (auto it = stepSources.begin(); it != stepSources.end(); ) {
            ALint state;
            alGetSourcei(*it, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING) {
                alDeleteSources(1, &(*it));
                it = stepSources.erase(it);
            }
            else {
                ++it;
            }
        }

        // Time logic
        float currentTime = glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        // Input
        processInput(window);

        // Render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Activate shader
        ourShader.use();

        // Spotlight
        glm::vec3 pointPositions[5] = {
            glm::vec3(85.5457f, 18.8488f, -135.15f), // verde
            glm::vec3(-42.1298f, 14.4956f, -39.5177f),  // rojo
            glm::vec3(-65.6087f, 21.8362f, -100.089f),// azul
            glm::vec3(-19.6117f, 18.763f, 14.7068f), // violeta
            glm::vec3(0.405586f, 14.9602f, -0.436662f)   // amarilla (l醡para)
        };

        glm::vec3 pointColors[5] = {
            glm::vec3(0.7f, 1.4f, 0.7f) * 2.5f,  // verde m醩 brillante
            glm::vec3(0.8f, 0.4f, 0.4f) * 2.5f,  // rojo
            glm::vec3(0.4f, 0.6f, 1.0f) * 1.6f,  // azul tenue pero claro
            glm::vec3(0.6f, 0.4f, 0.7f) * 1.3f,  // violeta
            glm::vec3(0.8f, 0.65f, 0.3f) * 1.5f  // l醡para
        };

        // Efectos e intesidad luces
        for (int i = 0; i < 5; ++i) {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            ourShader.setVec3(base + ".position", pointPositions[i]);

            if (i == 4) {
                // Luz 5 l醡para cuadrada con parpadeo lento 
                float time = glfwGetTime();
                float damagedFlicker = (sin(time * 2.0f) + sin(time * 3.1f + 1.5f)) * 0.25f + 1.0f;
                damagedFlicker = std::max(0.3f, std::min(damagedFlicker, 1.5f)); // nunca se apaga totalmente
                ourShader.setVec3(base + ".color", pointColors[i] * damagedFlicker);
            }
            else if (i == 3) {
                // Luz 4 parpadeo tipo foco da馻do
                float flicker = sin(glfwGetTime() * 10.0f) * 0.5f + 1.0f;
                ourShader.setVec3(base + ".color", pointColors[i] * flicker);
            }
            else {
                // Otras luces normales
                ourShader.setVec3(base + ".color", pointColors[i]);
            }


            ourShader.setFloat(base + ".constant", 1.0f);
            ourShader.setFloat(base + ".linear", 0.07f);     
            ourShader.setFloat(base + ".quadratic", 0.017f);
        }

        // Set camera/view/projection matrices
        //glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
          //  (float)SCR_WIDTH / (float)SCR_HEIGHT,
          //  0.1f, 100.0f);

        // Posici贸n del espectador
        // ourShader.setVec3("viewPos", camera.Position);

        // Spotlight activado solo si se mantiene presionada la tecla 'K'
        if (spotlightOn) {
            ourShader.setVec3("lightPos", camera.Position);
            ourShader.setVec3("lightDir", camera.Front);
            ourShader.setFloat("cutOff", glm::cos(glm::radians(12.5f)));
            ourShader.setFloat("outerCutOff", glm::cos(glm::radians(17.5f)));
        }
        else {
            // spotlight apagado: valores neutros
            ourShader.setVec3("lightPos", glm::vec3(0.0f));
            ourShader.setVec3("lightDir", glm::vec3(0.0f, -1.0f, 0.0f));
            ourShader.setFloat("cutOff", glm::cos(glm::radians(0.0f)));
            ourShader.setFloat("outerCutOff", glm::cos(glm::radians(0.0f)));
        }

        // Definir la matriz de proyecci贸n una sola vez
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        // Transformaciones de vista/proyecci贸n
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // Model transformation for scene
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0); // Textura vac铆a
        ourShader.setInt("emissionMap", 3);
        ourShader.setFloat("emissionIntensity", 0.0f); // Intensidad cero

        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(4.0f));
        ourShader.setMat4("model", modelMatrix);
        sceneModel.Draw(ourShader);

        // ==========================================================================
        // Renderizar Freddy (modelo animado)
        // ==========================================================================
            // Desactivamos temporalmente la textura de emisi贸n
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0); // Textura vac铆a
        ourShader.setInt("emissionMap", 3);
        ourShader.setFloat("emissionIntensity", 0.0f); // Intensidad cero

        glm::mat4 freddyMatrix = glm::mat4(1.0f);

        freddyMatrix = glm::translate(freddyMatrix, glm::vec3(0.0f, 7.0f, -156.0f)); // Posici贸n 
        freddyMatrix = glm::rotate(freddyMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        freddyMatrix = glm::scale(freddyMatrix, glm::vec3(0.30f)); // Escala 
        ourShader.setMat4("model", freddyMatrix);

        // Seleccionar frame actual basado en el tiempo
        int currentFrame = (int)(currentTime * fps) % totalFrames;
        freddyFrames[currentFrame].Draw(ourShader);

        // ==========================================================================
        //  Instancias de Foxy
        // ==========================================================================
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, emissionMap);
        ourShader.setInt("emissionMap", 3);
        ourShader.setFloat("emissionIntensity", 2.0f);

        vector<glm::vec3> foxyPositions = {
            //glm::vec3(8.0f, -3.5f, -56.0f),
            glm::vec3(19.0f, 1.0f, 27.0f),
            // glm::vec3(19.0f, 9.0f, 27.0f)
        };

        for (const auto& pos : foxyPositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(-180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotaci贸n para alinear el modelo
            model = glm::scale(model, glm::vec3(6.5f));
            ourShader.setMat4("model", model);
            foxyModel.Draw(ourShader);
        }

        // ==========================================================================
        //  Instancias de Jack-O-Bonnie
        // ==========================================================================
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, jackEmissionMap);
        ourShader.setInt("emissionMap", 4);
        ourShader.setFloat("emissionIntensity", 1.2f);

        std::vector<glm::vec3> jackPositions = {
            //glm::vec3(8.0f, -3.5f, -53.0f),
            glm::vec3(59.113, 1.0, -119.036),
            //glm::vec3(-55.7778, 1.0, -142.388)
        };

        for (const auto& pos : jackPositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(4.5f));
            ourShader.setMat4("model", model);
            jackModel.Draw(ourShader);
        }

        // ==========================================================================
        //  Instancias de Frankey
        // ==========================================================================
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, frankeyEmissionMap);
        ourShader.setInt("emissionMap", 5);
        ourShader.setFloat("emissionIntensity", 4.0f);

        std::vector<glm::vec3> frankeyPositions = {
            //glm::vec3(-6.0f, -2.5f, -57.0f),
            glm::vec3(-55.7778, 16.0, -110.388),
            // glm::vec3(-2.0f, -2.5f, -55.0f)
        };

        for (const auto& pos : frankeyPositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::scale(model, glm::vec3(33.0f));
            ourShader.setMat4("model", model);
            frankeyModel.Draw(ourShader);
        }

        // ==========================================================================
        //  Instancias de Bunny
        // ==========================================================================
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, 0); // Sin emisi贸n para Bunny
        ourShader.setInt("emissionMap", 6);
        ourShader.setFloat("emissionIntensity", 0.0f);

        std::vector<glm::vec3> bunnyPositions = {
            //glm::vec3(10.0f, -3.5f, -60.0f),
            glm::vec3(-58.0f, 5.0f, -100.0f),
            // glm::vec3(13.0f, -3.5f, -61.0f)
        };

        for (const auto& pos : bunnyPositions) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(6.5f));
            ourShader.setMat4("model", model);
            bunnyModel.Draw(ourShader);
        }

        // ======== Dibujar cubo visual de la l醡para cuadrada ========
        lightCubeShader.use();
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        lightCubeShader.setVec3("lightColor", glm::vec3(0.8f, 0.65f, 0.3f) * 1.5f); // mismo color que la 5ta point light

        glm::mat4 lampModel = glm::mat4(1.0f);
        lampModel = glm::translate(lampModel, glm::vec3(0.405586f, 14.9602f, -0.436662f)); // posici髇 de la l醡para
        lightCubeShader.setMat4("model", lampModel);

        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (sourceId) {
        alSourceStop(sourceId);
        alDeleteSources(1, &sourceId);
    }
    if (bufferId) {
        alDeleteBuffers(1, &bufferId);
    }
    for (auto src : stepSources) {
        alDeleteSources(1, &src);
    }
    for (int i = 0; i < NUM_STEPS; ++i) {
        if (stepBuffers[i]) {
            alDeleteBuffers(1, &stepBuffers[i]);
        }
    }

    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);

    glfwTerminate();
    return 0;
}

// Input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    bool moving = false;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.ProcessKeyboard(FORWARD, deltaTime);
        moving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.ProcessKeyboard(BACKWARD, deltaTime);
        moving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.ProcessKeyboard(LEFT, deltaTime);
        moving = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.ProcessKeyboard(RIGHT, deltaTime);

        moving = true;
    }

    float currentTime = glfwGetTime();
    if (moving && currentTime - lastStepTime > stepInterval) {
        ALuint stepSource;
        alGenSources(1, &stepSource);
        alSourcei(stepSource, AL_BUFFER, stepBuffers[currentStep]);
        alSourcePlay(stepSource);
        stepSources.push_back(stepSource);
        currentStep = (currentStep + 1) % NUM_STEPS;
        lastStepTime = currentTime;
    }

    // glfw: whenever the window size changed (by OS or user resize) this callback function executes
    static bool oPressedLastFrame = false;
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if (!oPressedLastFrame) {
            std::cout << "Camara: ("
                << camera.Position.x << ", "
                << camera.Position.y << ", "
                << camera.Position.z << ")\n";
            oPressedLastFrame = true;
        }
    }
    else {
        oPressedLastFrame = false;
    }

    // Toggle con tecla K
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && !keyPressed)
    {
        spotlightOn = !spotlightOn;
        keyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_RELEASE)
    {
        keyPressed = false;
    }
}

// Resize viewport
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Mouse movement
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Scroll
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}