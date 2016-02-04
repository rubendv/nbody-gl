#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <random>

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

const GLuint WIDTH = 800, HEIGHT = 600;

constexpr double G = 1e-5;

class Body {
public:
    glm::vec2 position;
    glm::vec2 velocity;
    double mass;
    double radius;
};

class World {
public:
    std::vector<Body> bodies;

    void tick(double dt) {
        for (Body &current : bodies) {
            glm::vec2 acceleration(0.0);
            for (const Body &other : bodies) {
                glm::vec2 to_other = other.position - current.position;
                double distance2 = glm::length2(to_other);
                if(distance2 < 0.0001) {
                    continue;
                }
                acceleration += glm::vec2(G * other.mass / distance2) * glm::normalize(to_other);
            }
            current.velocity += acceleration * glm::vec2(dt);
        }
        for (Body &current : bodies) {
            current.position += current.velocity * glm::vec2(dt);
        }
    }
};

std::vector<glm::vec2> create_circle_fan(size_t segments) {
    std::vector<glm::vec2> vertices(segments+2, glm::vec2(0.0));
    for (size_t i = 0; i <= segments; ++i) {
        double offset = 2 * M_PI / static_cast<double>(segments) * i;
        vertices[i + 1] = glm::vec2(std::cos(offset), std::sin(offset));
    }
    return vertices;
}

GLuint compile_shader_stage(GLenum stage, const std::string &source) {
    GLuint id = glCreateShader(stage);
    const char *c_str = source.c_str();
    glShaderSource(id, 1, &c_str, nullptr);
    glCompileShader(id);
    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint info_log_length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, 0);
        glGetShaderInfoLog(id, info_log_length, nullptr, &info_log[0]);
        std::cerr << "Error compiling shader stage: " << info_log << std::endl;
        throw std::runtime_error(info_log);
    }
    return id;
}

GLuint compile_shader_program(const std::string &vertex_shader, const std::string &fragment_shader) {
    GLuint vertex_id = compile_shader_stage(GL_VERTEX_SHADER, vertex_shader);
    GLuint fragment_id = compile_shader_stage(GL_FRAGMENT_SHADER, fragment_shader);
    GLuint program_id = glCreateProgram();
    glAttachShader(program_id, vertex_id);
    glAttachShader(program_id, fragment_id);
    glLinkProgram(program_id);
    GLint status;
    glGetProgramiv(program_id, GL_LINK_STATUS, &status);
    if (!status) {
        GLint info_log_length;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, 0);
        glGetProgramInfoLog(program_id, info_log_length, nullptr, &info_log[0]);
        std::cerr << "Error compiling shader program: " << info_log << std::endl;
        throw std::runtime_error(info_log);
    }
    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);
    return program_id;
}

int main() {
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    auto window = glfwCreateWindow(WIDTH, HEIGHT, "N-Body", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (!window) {
        std::cout << "Failed to create GLFW window: " << std::endl;
        glfwTerminate();
        return -1;
    }

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    GLuint program = compile_shader_program(
            "#version 400\n"
                    "layout(location = 0) in vec2 position;"
                    "uniform mat4 mvp;"
                    "void main() {"
                    "   gl_Position = mvp * vec4(position, 0, 1);"
                    "}",
            "#version 400\n"
                    "out vec4 color;"
                    "void main() {"
                    "   color = vec4(0.8, 0.7, 0.7, 1.0);"
                    "}"
    );

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);

    World world;

    std::random_device r;
    std::default_random_engine e(r());
    std::uniform_real_distribution<float> position_dist(-0.8f, 0.8f);
    std::uniform_real_distribution<double> mass_dist(0, 2);

    for(size_t i = 0; i < 500; ++i) {
        Body body;
        body.position.x = position_dist(e);
        body.position.y = position_dist(e);
        body.mass = std::pow(10.0, mass_dist(e));
        body.radius = std::pow(body.mass, 1.0/3.0)/200;
        world.bodies.push_back(body);
    }

    auto fan_vertices = create_circle_fan(32);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, fan_vertices.size() * sizeof(glm::vec2), &fan_vertices[0], GL_STATIC_DRAW);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    float aspect = static_cast<float>(width)/static_cast<float>(height);
    glm::mat4 projection = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    auto mvp_location = glGetUniformLocation(program, "mvp");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        world.tick(0.01);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glBindVertexArray(vao);
        for(auto body : world.bodies) {
            glm::mat4 model = glm::scale(glm::translate(glm::mat4(), glm::vec3(body.position, 0.0)), glm::vec3(body.radius));
            glm::mat4 mvp = projection * model;
            glUniformMatrix4fv(mvp_location, 1, GL_FALSE, glm::value_ptr(mvp));
            glDrawArrays(GL_TRIANGLE_FAN, 0, fan_vertices.size());
        }

        glfwSwapBuffers(window);
    }

    // Terminates GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}
