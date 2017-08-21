/* Created by Darren Otgaar on 2016/08/14. http://www.github.com/otgaard/zap */
#include "renderer.hpp"

using namespace zap::renderer;

bool renderer::initialise() {
    initialised_ = state_stack_.initialise();
    return initialised_;
}

void renderer::draw(const zap::engine::mesh_base* mesh_ptr, const zap::renderer::render_context* context_ptr,
                              const zap::renderer::render_args& args) {
    if(curr_context_ != context_ptr) {  // Check if the context is already bound and re-use, otherwise detach first
        if(curr_context_ != nullptr) {
            if(curr_context_->get_state() != nullptr) pop_state();
            curr_context_->release();
        }
        curr_context_ = context_ptr;
        curr_context_->bind(args);
        if(curr_context_->get_state() != nullptr) push_state(curr_context_->get_state());
    } else {
        curr_context_->bind(args);
    }

    if(curr_mesh_ != mesh_ptr) {
        if(curr_mesh_ != nullptr) curr_mesh_->release();
        curr_mesh_ = mesh_ptr;
        curr_mesh_->bind();
    }

    mesh_ptr->draw();
}

void renderer::draw(const renderer::mesh_base* mesh_ptr, const render_context* context_ptr) {
    if(curr_context_ != context_ptr) {  // Check if the context is already bound and re-use, otherwise detach first
        if(curr_context_ != nullptr) {
            if(curr_context_->get_state() != nullptr) pop_state();
            curr_context_->release();
        }
        curr_context_ = context_ptr;
        curr_context_->rebind();
        if(curr_context_->get_state() != nullptr) push_state(curr_context_->get_state());
    } else {
        curr_context_->rebind();
    }

    if(curr_mesh_ != mesh_ptr) {
        if(curr_mesh_ != nullptr) curr_mesh_->release();
        curr_mesh_ = mesh_ptr;
        curr_mesh_->bind();
    }

    mesh_ptr->draw();
}
