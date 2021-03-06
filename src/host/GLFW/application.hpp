/* Created by Darren Otgaar on 2016/06/10. http://www.github.com/otgaard/zap */
#ifndef ZAP_GLFW_APPLICATION_HPP
#define ZAP_GLFW_APPLICATION_HPP

#if defined(_WIN32)

#if !defined(HOSTGLFW_EXPORT)
#if defined(ZAP_STATIC)
#include "hostglfw_exports_s.h"
#else
#include "hostglfw_exports.h"
#endif
#else
#include HOSTGLFW_EXPORT
#endif

#else
#define ZAPHOSTGLFW_EXPORT
#endif

#include <string>
#include <maths/functions.hpp>
#include <maths/vec2.hpp>

struct GLFWwindow;

struct app_config {
    int multisamples = 1;
    int depth_bits = 24;
    int stencil_bits = 8;
    int gl_major_version = 3;
    int gl_minor_version = 3;
    int swap_interval = 1;
    bool gl_forward_compatibility = true;
    bool gl_core_profile = true;
    bool resizeable_window = false;
    bool fullscreen = false;
};

class ZAPHOSTGLFW_EXPORT application {
public:
    application(const std::string& name, int width, int height);
    virtual ~application() = default;

    virtual bool initialise() { return true; }
    virtual void update(double t, float dt) { }
    virtual void draw() { }
    virtual void shutdown() { } // Note:  All OpenGL resources must be freed before this function returns

    virtual void on_keydown(int ch);
    virtual void on_keyup(int ch);
    virtual void on_resize(int width, int height);
    virtual void on_mousedown(int button);
    virtual void on_mouseup(int button);
    virtual void on_mousemove(double x, double y);
    virtual void on_mousewheel(double xoffset, double yoffset);

    int run(const app_config& config=app_config{});

    zap::maths::vec2i mouse_pos() const { return mouse_; }
    void set_dims(int width, int height) { sc_width_ = width; sc_height_ = height; }
    int width() const { return sc_width_; }
    int height() const { return sc_height_; }
    void resize(int width, int height);
    void set_viewport(int x, int y, int width, int height);

protected:
    int sc_width_;
    int sc_height_;
    zap::maths::vec2i mouse_;

private:
    std::string app_name_;
    zap::maths::timer timer_;
    GLFWwindow* window_ = nullptr;
};

#endif //ZAP_GLFW_APPLICATION_HPP
