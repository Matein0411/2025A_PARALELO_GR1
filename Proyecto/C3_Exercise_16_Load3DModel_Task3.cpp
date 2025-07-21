#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>
#include <AL/al.h>
#include <AL/alc.h>
#include <fstream>
#include <vector>
#include <array>

#define STB_IMAGE_IMPLEMENTATION 
#include <learnopengl/stb_image.h>

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, -3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

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
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 16 Task 2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    Shader ourShader("shaders/shader_exercise16_mloading.vs", "shaders/shader_exercise16_mloading.fs");
    Model ourModel("models/FNAF/FNAF.obj");
    camera.MovementSpeed = 4;
    // --- OpenAL ---
    ALCdevice* device = alcOpenDevice(NULL);
    ALCcontext* context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    // --- Música de ambiente ---
    std::vector<char> audioData;
    ALenum format;
    ALsizei freq;
    ALuint bufferId = 0, sourceId = 0;
    if (!LoadWavFile("sounds/spooky_escene.wav", audioData, format, freq)) {
        std::cout << "No se pudo cargar el archivo de música ambiente." << std::endl;
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

    // render loop
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

        // per-frame time logic
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -50.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

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

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
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
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
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

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}