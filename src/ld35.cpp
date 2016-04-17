#include <stdio.h>
#include <stdlib.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "game.h"

const GLuint WIDTH = 800, HEIGHT = 600;

GameState gs;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            gs.keys[key] = GL_TRUE;
        } else if (action == GLFW_RELEASE) {
            gs.keys[key] = GL_FALSE;
            gs.keys_held[key] = GL_FALSE;
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    gs.mouse_pos.x = xpos;
    gs.mouse_pos.y = ypos; 
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        gs.left_mouse_held = GL_TRUE;
    } else {
        gs.left_mouse_held = GL_FALSE;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        gs.right_mouse_held = GL_TRUE;
    } else {
        gs.right_mouse_held = GL_FALSE;
    }
}

int main(void)
{
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Malformed", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

    if (gl3wInit()) {
        fprintf(stderr, "failed to initialize OpenGL\n");
        return -1;
    }

    if (!gl3wIsSupported(3, 3)) {
        fprintf(stderr, "OpenGL 3.3 not supported\n");
        return -1;
    }

    printf("OpenGL %s, GLSL %s\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    gs = game_init(WIDTH, HEIGHT);
    /* Loop until the user closes the window */

    GLfloat delta_time = 0.0f;
    GLfloat last_frame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        GLfloat current_frame = (GLfloat) glfwGetTime();
        delta_time = current_frame - last_frame;
        last_frame = current_frame; 
        /* Poll for and process events */
        glfwPollEvents();

        /* Render here */
        glClearColor(0.2902f, 0.4784f, 0.4863f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        game_update_and_render(&gs, delta_time);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
    }

    game_destroy(&gs);

    glfwTerminate();
    return 0;
}