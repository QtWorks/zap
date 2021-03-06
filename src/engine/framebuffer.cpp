/* Created by Darren Otgaar on 2016/05/27. http://www.github.com/otgaard/zap */
#include "framebuffer.hpp"
#include "gl_api.hpp"

using namespace zap::engine;
using namespace zap::engine::gl;

bool framebuffer::allocate() {
    glGenFramebuffers(1, &framebuffer_);
    LOG("Framebuffer Allocated: ", framebuffer_);
    return is_allocated() && !gl_error_check();
}

void framebuffer::deallocate() {
    for(auto& t : attachments_) t.deallocate();
    glDeleteFramebuffers(1, &framebuffer_);
    LOG("Framebuffer Deallocated:", framebuffer_);
    framebuffer_ = INVALID_IDX;
    gl_error_check();
}

void framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    target_count_ == 0 ? glDrawBuffer(GL_NONE) : glDrawBuffers(uint32_t(target_count_), draw_buffers_.data());
    glGetIntegerv(GL_VIEWPORT, curr_viewport_);
    glGetDoublev(GL_DEPTH_RANGE, curr_depthrange_);
    glViewport(0, 0, uint32_t(width_), uint32_t(height_));
    glDepthRange(0., 1.);
}

void framebuffer::release() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if(mipmaps_) {
        for(size_t i = 0; i != target_count_; ++i) {
            attachments_[i].bind();
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        attachments_[target_count_-1].release();
    }

    glViewport(curr_viewport_[0], curr_viewport_[1], curr_viewport_[2], curr_viewport_[3]);
    glDepthRange(curr_depthrange_[0], curr_depthrange_[1]);
}

bool framebuffer::is_bound() const {
    return false;
}

bool framebuffer::initialise(size_t target_count, size_t width, size_t height, pixel_format format,
                             pixel_datatype datatype, bool mipmaps, bool depthstencil) {
    target_count_ = target_count;
    width_ = width;
    height_ = height;
    pix_format_ = format;
    pix_dtype_ = datatype;
    mipmaps_ = mipmaps;
    depthstencil_ = depthstencil;
    return initialise();
}

bool framebuffer::initialise() {
    assert(is_allocated() && "Framebuffer is not allocated");
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    attachments_.clear();
    attachments_.reserve(target_count_ + depthstencil_);
    draw_buffers_.reserve(target_count_);
    for(uint32_t i = 0; i != target_count_; ++i) {
        attachments_.emplace_back(texture());
        draw_buffers_.push_back(GL_COLOR_ATTACHMENT0 + i);
        attachments_[i].allocate();
        attachments_[i].initialise(texture_type::TT_TEX2D, int32_t(width_), int32_t(height_), 1, pix_format_,
                                   pix_dtype_, false);
        attachments_[i].bind();
        if(mipmaps_) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, draw_buffers_[i], GL_TEXTURE_2D, attachments_[i].resource(), 0);
        gl_error_check();
    }

    if(depthstencil_) {
        attachments_.emplace_back(texture());
        attachments_[target_count_].allocate();
        attachments_[target_count_].initialise(texture_type::TT_TEX2D, int32_t(width_), int32_t(height_), 1,
                                               pixel_format::PF_DEPTH_STENCIL, pixel_datatype::PD_UNSIGNED_INT_24_8,
                                               false);
        attachments_[target_count_].bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                               attachments_[target_count_].resource(), 0);
        gl_error_check();
        attachments_[target_count_].release();
    }

    switch(glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        case GL_FRAMEBUFFER_COMPLETE:
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            LOG("Render Target Error: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
            assert(false);
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            LOG("Render Target Error: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
            assert(false);
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            LOG("Render Target Error: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
            assert(false);
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            LOG("Render Target Error: GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER");
            assert(false);
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            LOG("Render Target Error: GL_FRAMEBUFFER_UNSUPPORTED");
            assert(false);
            break;
        default:
            LOG("Render Target Error: Unspecified (default case)");
            assert(false);
            break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return !gl_error_check();
}

bool framebuffer::read_attachment(const vec4i& viewport, size_t idx) const {
    if(viewport[2] > (int)width() || viewport[3] > (int)height()) {
        LOG_ERR("Invalid viewport specified for framebuffer attachment copy");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glReadBuffer(GL_COLOR_ATTACHMENT0 + uint32_t(idx));
    glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], gl_type(pix_format_), gl_type(pix_dtype_), 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return !gl_error_check();
}
