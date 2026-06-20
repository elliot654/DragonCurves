#include "DragonCurves.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <thread>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"uniform mat4 transform;\n"
"void main()\n"
"{\n"
"   gl_Position = transform * vec4(aPos, 0.0, 1.0);\n"
"}\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
"}\0";

mat4 fitToView(float minX, float maxX, float minY, float maxY, float aspect)
{
    const float cx = (minX + maxX) * 0.5f;
    const float cy = (minY + maxY) * 0.5f;
    const float halfW = (maxX - minX) * 0.5f;
    const float halfH = (maxY - minY) * 0.5f;
    const float scale = (1.0f / 1.1f) / glm::max(halfW, halfH / aspect);

    mat4 view(1.0f);
    view = glm::scale(view, vec3(scale, scale, 1.0f));
    view = translate(view, vec3(-cx, -cy, 0.0f));
    return view;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) //resizes the window
{
    glViewport(0, 0, width, height);
}

unsigned int assembleShaders() {
    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER); //create an empty shader object
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); //shader object, number of sources, source, null
    glCompileShader(vertexShader);
    int  success; //error checking for shader compliation
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }

    unsigned int fragmentShader; //same as above with a fragment shader both are required
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }

    unsigned int shaderProgram; //link the two above shaders into a shader program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::COMPILATION_FAILED\n" << infoLog << endl;
    }
    glDeleteShader(vertexShader); //clean up not needed after theyre linked to the program shader
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main()
{
    glfwInit(); //start glfw (the os handler to make windows)

    GLFWwindow* window = glfwCreateWindow(2560, 1440, "Dragon Curves", NULL, NULL); //make window
    if (window == NULL) //check window creation worked
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); //set thread context to window

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) //start glad (opengl loader)
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, 2560, 1440); //the usable space in the window
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback); //registers the resize function

    vector<float> line = {-0.5f, 0.0f, 0.5f, 0.0f}; //the starting line hehe
    line.reserve(1 << 22); //reserves space to stop reallocation on interation
    float minX = -0.5f, maxX = 0.5f, minY = 0.0f, maxY = 0.0f;
    double lastStep = glfwGetTime();
    const double STEP_INTERVAL = 1.0;
    int iteration = 0;
    unsigned int shaderProgram = assembleShaders(); 

    unsigned int VAO;
    glGenVertexArrays(1, &VAO); //assigns pointer for vao object
    glBindVertexArray(VAO); //make this object the active vao

    unsigned int VBO;
    glGenBuffers(1, &VBO); //vertex buffer object holds vertex data to be sent to the gpu vram together
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //set buffer as active array
    glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_DYNAMIC_DRAW);//fill the buffer with line data
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); //instruct opengl on how to interpret the vertex data in the buffer and applys the vao
    glEnableVertexAttribArray(0);
    glEnable(GL_LINE_SMOOTH);



    while (!glfwWindowShouldClose(window)) //main loop checks for exit commands
    {
        const double now = glfwGetTime();
        if (now - lastStep >= STEP_INTERVAL)
        {
            const size_t oldSize = line.size();

            float px = line[oldSize - 2];
            float py = line[oldSize - 1];

            for (int i = oldSize - 4; i >= 0; i -= 2)
            {
                float dx = line[i] - px;
                float dy = line[i + 1] - py;
                float nx = -dy + px;
                float ny = dx + py;

                minX = glm::min(minX, nx);
                maxX = glm::max(maxX, nx);
                minY = glm::min(minY, ny);
                maxY = glm::max(maxY, ny);

                line.push_back(nx);
                line.push_back(ny);
            }
            glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_DYNAMIC_DRAW); //the line data has changed so the buffer needs to be updated
            lastStep = now;
            iteration++;
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //reset
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        const float aspect = (float)h / (float)w;
        mat4 view = fitToView(minX, maxX, minY, maxY, aspect);
        string title = "Dragon Curves - iteration " + to_string(iteration);
        glfwSetWindowTitle(window, title.c_str());
        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, value_ptr(view));
        glDrawArrays( GL_LINE_STRIP,0,line.size() / 2); //draw the line data

        glfwSwapBuffers(window);//double buffering display the draw buffer and update the screen
        glfwWaitEventsTimeout(glm::max(0.0, STEP_INTERVAL - (glfwGetTime() - lastStep)));
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();//exit properly after closing
    return 0;
}

