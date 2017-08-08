/* Created by Darren Otgaar on 2017/08/08. http://www.github.com/otgaard/zap */
#include "state_stack.hpp"
#include "gl_api.hpp"

using namespace zap;
using namespace zap::engine;
using namespace zap::engine::gl;

extern const GLenum gl_src_blend_mode[(int)render_state::blend_state::src_blend_mode::SBM_SIZE];
extern const GLenum gl_dst_blend_mode[(int)render_state::blend_state::dst_blend_mode::DBM_SIZE];
extern const GLenum gl_compare_mode[(int)render_state::compare_mode::CM_SIZE];

state_stack::state_stack() : base_state_(true) {
}

bool state_stack::initialise() {
    initialise(base_state_.get_blend_state());
    blend_stack_.push(base_state_.get_blend_state());

    stack_.push(&base_state_);
    return true;
}

void zap::engine::state_stack::push_state(const render_state* state) {
    if(state != peek()) {
        if(state->get_blend_state() != nullptr) push_blend_state(state->get_blend_state());

        stack_.push(state);
    }
}

void zap::engine::state_stack::pop() {
    if(stack_.size() > 1) {
        if(peek()->get_blend_state() != nullptr) pop_blend_state();

        stack_.pop();
    }
}

void zap::engine::state_stack::push_blend_state(const blend_state* state) {
    if(state == nullptr || state == curr_blend_state()) return;
    transition(curr_blend_state(), state);
    blend_stack_.push(state);
}

void zap::engine::state_stack::pop_blend_state() {
    if(blend_stack_.size() > 1) {
        auto curr = curr_blend_state();
        blend_stack_.pop();
        auto prev = curr_blend_state();
        transition(curr, prev);
    }
}

void zap::engine::state_stack::initialise(const blend_state* state) {
    if(state == nullptr) return;
    if(state->blend_enabled) glEnable(GL_BLEND);
    else                     glDisable(GL_BLEND);
    gl_error_check();
    glBlendFunc(gl_src_blend_mode[(int)state->src_mode], gl_dst_blend_mode[(int)state->dst_mode]);
    gl_error_check();
    glBlendColor(state->colour.x, state->colour.y, state->colour.z, state->colour.w);
}

void zap::engine::state_stack::transition(const blend_state* source, const blend_state* target) {
    if(target->blend_enabled) {
        if(!source->blend_enabled) glEnable(GL_BLEND);
    } else {
        if(source->blend_enabled)  glDisable(GL_BLEND);
    }

    if(target->src_mode != source->src_mode || target->dst_mode != source->dst_mode)
        glBlendFunc(gl_src_blend_mode[(int)target->src_mode], gl_dst_blend_mode[(int)target->dst_mode]);
    if(target->colour != source->colour)
        glBlendColor(target->colour.x, target->colour.y, target->colour.z, target->colour.w);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OPENGL TABLES

const GLenum gl_src_blend_mode[(int)render_state::blend_state::src_blend_mode::SBM_SIZE] = {
        GL_ZERO,
        GL_ONE,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,
        GL_ONE_MINUS_DST_ALPHA,
        GL_SRC_ALPHA_SATURATE,
        GL_CONSTANT_COLOR,
        GL_ONE_MINUS_CONSTANT_COLOR,
        GL_CONSTANT_ALPHA,
        GL_ONE_MINUS_CONSTANT_ALPHA
};

const GLenum gl_dst_blend_mode[(int)render_state::blend_state::dst_blend_mode::DBM_SIZE] = {
        GL_ZERO,
        GL_ONE,
        GL_SRC_COLOR,
        GL_ONE_MINUS_SRC_COLOR,
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA,
        GL_DST_ALPHA,
        GL_DST_COLOR,
        GL_ONE_MINUS_DST_ALPHA,
        GL_CONSTANT_COLOR,
        GL_ONE_MINUS_CONSTANT_COLOR,
        GL_CONSTANT_ALPHA,
        GL_ONE_MINUS_CONSTANT_ALPHA
};

const GLenum gl_compare_mode[(int)render_state::compare_mode::CM_SIZE] = {
        GL_NEVER,
        GL_LESS,
        GL_EQUAL,
        GL_LEQUAL,
        GL_GREATER,
        GL_NOTEQUAL,
        GL_GEQUAL,
        GL_ALWAYS
};
