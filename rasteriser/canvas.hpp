/* Created by Darren Otgaar on 2016/08/20. http://www.github.com/otgaard/zap */
#ifndef ZAP_CANVAS_HPP
#define ZAP_CANVAS_HPP

// A 2D Software Rasteriser for rasterising lines, circles, ellipses, text, using clipping & ranged updates and Pixel
// Buffer usage in OpenGL.  The basis for a small vector graphics engine.

#include <maths/vec2.hpp>
#include <maths/vec3.hpp>
#include <engine/pixmap.hpp>

namespace zap { namespace rasteriser {
    using vec2i = maths::vec2i;
    using vec3b = maths::vec3b;

    class canvas {
    public:
        canvas();
        canvas(int width, int height);
        ~canvas();

        inline int width() const { return raster_.width(); }
        inline int height() const { return raster_.height(); }

        void resize(int width, int height);

        void clear(byte r=255, byte g=255, byte b=255);
        void clear(const vec3b& rgb);

        void pen_colour(const vec3b& rgb) { pen_colour_ = rgb; }
        const vec3b& pen_colour() const { return pen_colour_; }

        void line(int x1, int y1, int x2, int y2);

        const std::vector<zap::engine::rgb888_t>& buffer() const { return raster_.buffer(); }

    protected:
        void initialise();

        void vertical_line(int x1, int y1, int y2);
        void horizontal_line(int x1, int x2, int y1);
        void diagonal_line(int x1, int y1, int x2, int y2);

        vec3b pen_colour_;
        zap::engine::pixmap<zap::engine::rgb888_t> raster_;
    };
}}

#endif //ZAP_CANVAS_HPP
