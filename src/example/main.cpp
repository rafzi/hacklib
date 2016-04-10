#define GLEW_STATIC

#include "hacklib/Main.h"
#include "hacklib/WindowOverlay.h"
#include <chrono>
#include <cstdio>

#include <GL/glew.h>


#define GLSL_SRC(src) "#version 150 core\n" #src


hl::StaticInit<class ExampleMain> g_main;


const GLchar *vertexSource = GLSL_SRC(
    in vec2 position;
    uniform mat4 world;
    uniform mat4 view;
    uniform mat4 proj;

    void main()
    {
        gl_Position = proj * view * world * vec4(position, 0.0, 1.0);
    }
);
const GLchar *fragmentSource = GLSL_SRC(
    out vec4 outColor;

    void main()
    {
        outColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
);


class GLDTest
{
public:
    /*
    cleanup:
    glDeleteProgram(m_program);
    glDeleteShader(m_fragShader);
    glDeleteShader(m_vertShader);
    glDeleteBuffers(1, &m_vbTest);
    glDeleteVertexArrays(1, &m_vaTest);
    */
    bool init()
    {
        glewExperimental = GL_TRUE;
        GLenum result = glewInit();
        if (result != GLEW_OK)
        {
            printf("glew init failed: %s\n", glewGetErrorString(result));
            return false;
        }

        if (glGenVertexArrays == nullptr)
        {
            printf("glew didnt do its thing\n");
            return false;
        }

        glGenVertexArrays(1, &m_vaTest);
        glBindVertexArray(m_vaTest);

        glGenBuffers(1, &m_vbTest);
        GLfloat verts[] = {
            -0.5f, 0.5f,
            0.5f, 0.5f,
            0.5f, -0.5f,
            -0.5f, -0.5f
        };
        glBindBuffer(GL_ARRAY_BUFFER, m_vbTest);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

        glGenBuffers(1, &m_ibTest);
        GLuint inds[] = {
            0, 1, 2,
            2, 3, 0
        };
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibTest);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(inds), inds, GL_STATIC_DRAW);

        m_vertShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(m_vertShader, 1, &vertexSource, NULL);
        glCompileShader(m_vertShader);
        GLint vertStatus;
        glGetShaderiv(m_vertShader, GL_COMPILE_STATUS, &vertStatus);
        if (vertStatus != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(m_vertShader, 512, NULL, buffer);
            printf("Shader compile output:\n%s\n", buffer);
            return false;
        }

        m_fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(m_fragShader, 1, &fragmentSource, NULL);
        glCompileShader(m_fragShader);
        GLint fragStatus;
        glGetShaderiv(m_fragShader, GL_COMPILE_STATUS, &fragStatus);
        if (fragStatus != GL_TRUE)
        {
            char buffer[512];
            glGetShaderInfoLog(m_fragShader, 512, NULL, buffer);
            printf("Shader compile output:\n%s\n", buffer);
            return false;
        }

        m_program = glCreateProgram();
        glAttachShader(m_program, m_vertShader);
        glAttachShader(m_program, m_fragShader);
        glBindFragDataLocation(m_program, 0, "outColor");
        glLinkProgram(m_program);
        glUseProgram(m_program);

        GLint posAttr = glGetAttribLocation(m_program, "position");
        glEnableVertexAttribArray(posAttr);
        glVertexAttribPointer(posAttr, 2, GL_FLOAT, GL_FALSE, 0, 0);

        GLint uniModel = glGetUniformLocation(m_program, "model");
        GLint uniView = glGetUniformLocation(m_program, "view");
        GLint uniProj = glGetUniformLocation(m_program, "proj");

        return true;
    }
    void draw()
    {
        /*
        /// multiple bufs:

        // bind once
        glBindVertexArray(ssquareVAOid);

        glUseProgram(shaderProgId);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(catTextureId);
        glUniformMatrix4fv(localToWorldId, 1, GL_FALSE, pointerOnFloatArray);

        glDrawArrray(GL_TRIANGLE, numVertToSkip, numVertToDraw);

        // second primitive:

        glBindTexture(dogTextureId);
        glUniformMatrix4fv(localToWorldId, 1, GL_FALSE, pointerOnFloatArrayForSecondSquareMatrix);

        glDrawArrray(GL_TRIANGLE, numVertToSkip, numVertToDraw);


        -> glDrawArrays(GL_TRIANGLES, 0, 3);
        */
    }

private:
    GLuint m_vaTest;
    GLuint m_vbTest;
    GLuint m_ibTest;
    GLuint m_vertShader;
    GLuint m_fragShader;
    GLuint m_program;
};


class ExampleMain : public hl::Main
{
public:
    bool init() override
    {
        printf("hl::Main::init\n");

        m_started = std::chrono::system_clock::now();

        if (m_overlay.create() != hl::WindowOverlay::Error::Okay)
        {
            printf("could not create overlay\n");
        }

        // need to have context.
        m_overlay.resetContext();

        GLDTest test;
        if (!test.init())
        {
            printf("test init failed\n");
            return false;
        }
        printf("done\n");

        return true;
    }

    bool step() override
    {
        //printf("hl::Main::step\n");

        if (std::chrono::system_clock::now() - m_started > std::chrono::seconds(5))
            return false;

        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        m_overlay.beginDraw();

        glViewport(0, 0, m_overlay.getWidth(), m_overlay.getHeight());

        // orthographic projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, m_overlay.getWidth(), m_overlay.getHeight(), 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glPushMatrix();
        glTranslatef(50, 50, 0);
        glBegin(GL_QUADS);
        glColor4f(1, 0, 0, 0.5);
        glVertex2f(0, 0);
        glVertex2f(0, 100);
        glVertex2f(200, 100);
        glVertex2f(200, 0);
        glEnd();
        glPopMatrix();

        glPushMatrix();
        glTranslatef(80, 80, 0);
        glBegin(GL_QUADS);
        glColor4f(0, 1, 0, 0.5);
        glVertex2f(0, 0);
        glVertex2f(0, 100);
        glVertex2f(200, 100);
        glVertex2f(200, 0);
        glEnd();
        glPopMatrix();

        glLineWidth(3.0);
        glColor4f(0.5, 1.0, 0, 0.5);
        glBegin(GL_LINES);
        glVertex2f(100, 100);
        glVertex2f(200, 200);
        glEnd();
        glBegin(GL_LINES);
        glVertex2f(0, 0);
        glVertex2f(0.5, 0.5);
        glEnd();

        m_overlay.swapBuffers();

        return true;
    }

    void shutdown() override
    {
        printf("hl::Main::shutdown\n");
    }

private:
    std::chrono::system_clock::time_point m_started;
    hl::WindowOverlay m_overlay;

};
