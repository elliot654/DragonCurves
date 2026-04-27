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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) //resizes the window
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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

    vector<float> line = {-0.025f, 0.0f, 0.025f, 0.0f}; //the starting line hehe
    line.reserve(1 << 22); //reserves space to stop reallocation on interation
    float minX = -0.015f;
    float maxX = 0.015f;
    float minY = 0.0f;
    float maxY = 0.0f;
    vector<float> clone;
    unsigned int shaderProgram = assembleShaders(); 
    float zoom = 1.0f;
    float currentLimit = 1.0f;

    unsigned int VAO;
    glGenVertexArrays(1, &VAO); //assigns pointer for vao object
    glBindVertexArray(VAO); //make this object the active vao

    unsigned int VBO;
    glGenBuffers(1, &VBO); //vertex buffer object holds vertex data to be sent to the gpu vram together
    glBindBuffer(GL_ARRAY_BUFFER, VBO); //set buffer as active array
    glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_STATIC_DRAW);//fill the buffer with line data
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0); //instruct opengl on how to interpret the vertex data in the buffer and applys the vao
    glEnableVertexAttribArray(0);
    glEnable(GL_LINE_SMOOTH);

    while (!glfwWindowShouldClose(window)) //main loop checks for exit commands
    {
        processInput(window);
        clone.clear();
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f); //reset
        glClear(GL_COLOR_BUFFER_BIT);

        mat4 transform(1.0f); //create new matrix for transforms
        
        for (int i = line.size() - 4; i >= 0; i -= 2) //copies line into clone except for the last point
        {
            clone.push_back(line[i]);
            clone.push_back(line[i + 1]);
        }

        float px = line[line.size() - 2]; //gets the end point of the previous step to use as a pivot point
        float py = line[line.size() - 1];

        transform = translate(transform, vec3(px, py, 0.0f)); //sets pivot point
        transform = rotate(transform, radians(90.0f), vec3(0, 0, 1)); //does rotation
        transform = translate(transform, vec3(-px, -py, 0.0f)); //returns to origin (otherwise the first would add a bias of that distance each iteration)

        for (size_t i = 0; i < clone.size(); i += 2)
        {
            vec4 p(clone[i], clone[i + 1], 0.0f, 1.0f);

            p = transform * p;

            clone[i] = p.x;
            clone[i + 1] = p.y;

            // Incrementally update bounds
            minX = glm::min(minX, p.x);
            maxX = glm::max(maxX, p.x);
            minY = glm::min(minY, p.y);
            maxY = glm::max(maxY, p.y);
        }
        mat4 view(1.0f);
        view = scale(view, vec3(zoom, zoom, 1.0f));
        glUseProgram(shaderProgram);

        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");

        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, value_ptr(view));

        line.insert(line.end(), clone.begin(), clone.end()); //appends clone to existing vector

        float width = maxX - minX;
        float height = maxY - minY;

        if (width > currentLimit || height > currentLimit)
        {
            zoom *= 0.5f;
            currentLimit *= 2.0f;
        }

        glBufferData(GL_ARRAY_BUFFER, line.size() * sizeof(float), line.data(), GL_DYNAMIC_DRAW); //the line data has changed so the buffer needs to be updated
        glDrawArrays( GL_LINE_STRIP,0,line.size() / 2); //draw the line data

        glfwSwapBuffers(window);//double buffering display the draw buffer and update the screen
        glfwPollEvents();//checks for events

        this_thread::sleep_for(chrono::milliseconds(1000));//delay (bad testing code change this later)        
        
    }
    glfwTerminate();//exit properly after closing
    return 0;
}

