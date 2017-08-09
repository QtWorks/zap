/* Created by Darren Otgaar on 2016/03/22. http://www.github.com/otgaard/zap */
#include "gl_api.hpp"

using namespace zap::engine;

bool zap::engine::_gl_error_log(const char* file, int line) {
    using namespace gl;

    GLenum err = glGetError();
    if(err != GL_NO_ERROR) {
        std::string error;
        switch(err) {
            case GL_INVALID_OPERATION: error = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_ENUM: error = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: error = "GL_INVALID_VALUE"; break;
            case GL_OUT_OF_MEMORY: error = "GL_OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        LOG_ERR("OpenGL Error:", error, "in", file, "@", line);
        return true;
    }

    return false;
}

bool zap::engine::_gl_error_check() {
    return gl::glGetError() != GL_NO_ERROR;
}

void gl::bind_buffer_base(buffer_type type, int location, uint32_t bo) {
    glBindBufferBase(zap::engine::gl::gl_type(type), location, bo);
}

#include "engine.hpp"

bool zap::engine::init() {
    using namespace gl;

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if(err != GLEW_OK) {
        LOG("GLEW failed to initialise: ", glewGetErrorString(err));
        return false;
    }

#ifdef LOGGING_ENABLED
    LOG("Suppressing error generated by GLEW", glGetError());
#else
    glGetError();
#endif
    return true;
}
