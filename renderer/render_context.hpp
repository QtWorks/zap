//
// Created by Darren Otgaar on 2017/07/30.
//

#ifndef ZAP_RENDER_CONTEXT_HPP
#define ZAP_RENDER_CONTEXT_HPP

/*
 * The render_context is a container for all the state associated with the mesh (vbuf + ibuf) required to render the
 * visual.  Therefore, the render_context is a strongly-typed container for the shaders, program, uniforms, uniform
 * buffers, textures, render_state, and other values required to draw the object on screen.
 *
 * Long-term goals for the render_context:
 *
 * 1) Binding and usage as fast as possible
 * 2) Support multiple passes
 * 3) Queryable, enumerable, & programmable state (program uniforms, uniform buffers, textures, render_state)
 * 4) State sharing or ownership of resources
 *
 * Notes:
 *
 * 1) It might make sense to store the resource_t directly as a parameter and then bind it without requiring reference
 *    to the texture or sampler.  That would allow textures to be set more like parameters but would move the ownership
 *    problem elsewhere.
 */

#include <memory>
#include <cstring>
#include <algorithm>
#include <engine/program.hpp>
#include <engine/texture.hpp>
#include <engine/sampler.hpp>
#include <engine/render_state.hpp>
#include <engine/uniform_buffer.hpp>

namespace zap { namespace renderer {

using program_ptr = std::unique_ptr<engine::program>;
using texture_ptr = std::unique_ptr<engine::texture>;
using sampler_ptr = std::unique_ptr<engine::sampler>;
using render_state_ptr = std::unique_ptr<engine::render_state>;

class render_context {
public:
    using parameter = engine::parameter;
    using program = engine::program;
    using texture = engine::texture;
    using sampler = engine::sampler;
    using render_state = engine::render_state;

    render_context() = default;
    explicit render_context(program* prog) : program_{prog} { }
    render_context(const std::string& vshdr, const std::string& fshdr) : program_{new program{vshdr, fshdr}}, owns_program_(true) { }
    render_context(program* prog, texture* tex) : program_{prog}, textures_{tex} { }
    ~render_context() {
        if(owns_program_) delete program_;
    }

    bool is_bound() const { return is_bound_; }

    bool initialise() {
        if(!program_) return false;
        if(!program_->is_allocated()) {
            if(!program_->link()) return false;
        }
        parameters_ = program_->get_parameters();
        size_t total = 0;
        offsets_.resize(parameters_.size(), 0);
        for(size_t i = 0; i != parameters_.size(); ++i) {
            offsets_[i] = int(total);
            total += parameters_[i].count * parameters_[i].bytesize();

        }
        uniforms_.resize(total);
        dirty_flags_.resize(parameters_.size(), true);
        dirty_ = true;
        return true;
    }

    int32_t get_index(const std::string& name) const {
        auto it = std::find_if(parameters_.begin(), parameters_.end(), [&name](const auto& parm) { return parm.name == name; });
        if(it != parameters_.end()) return int32_t(it - parameters_.begin());
        else                        return -1;
    }

    bool has_parameter(const std::string& name) const { return get_index(name) != -1; }

    void set_texture_unit(int idx, int unit) {
        assert(idx < int(parameters_.size()) && "Invalid parameter specified");
        assert(unit < int(textures_.size()) && "No texture specified for requested unit");

        using pt = engine::parameter_type;
        auto& parm = parameters_[idx];
        int offset = offsets_[idx];
        if(is_one_of(parm.type, {pt::PT_SAMPLER_1D, pt::PT_SAMPLER_2D, pt::PT_SAMPLER_3D, pt::PT_SAMPLER_CUBE})) {
            memcpy(uniforms_.data()+offset, &unit, parm.bytesize() * parm.count);
            if(is_bound_) {
                program_->bind_uniform(parm.location, parm.type, parm.count, &uniforms_[offset]);
            } else {
                dirty_flags_[idx] = true;
                dirty_ = true;
            }
        }
    }

    void set_texture_unit(const std::string& name, int unit) {
        auto it = std::find_if(parameters_.begin(), parameters_.end(), [&name](const auto& parm) { return parm.name == name; });
        if(it != parameters_.end()) set_texture_unit(int(it - parameters_.begin()), unit);
        else                        LOG_ERR("Parameter does not exist in program:", LOG_GREEN, name);
    }

    template <typename T>
    void set_parameter(int idx, const T* value) {
        assert(idx < int(parameters_.size()) && "Invalid parameter specified");
        auto& parm = parameters_[idx];
        int offset = offsets_[idx];
        if(((engine::parameter_type)engine::pt_descriptor<T>::value) == parm.type) {
            memcpy(uniforms_.data()+offset, value, parm.bytesize() * parm.count);
            if(is_bound_) {
                program_->bind_uniform(parm.location, parm.type, parm.count, &uniforms_[offset]);
            } else {
                dirty_flags_[idx] = true;
                dirty_ = true;
            }
        } else {
            LOG_ERR("Attempted to bind invalid type to parameter:", parm.name);
        }
    }

    template <typename T>
    void set_parameter(int idx, const T& value) {
        set_parameter(idx, &value);
    }

    template <typename T>
    void set_parameter(int idx, const std::initializer_list<T>& value) {
        set_parameter(idx, value.begin());
    }

    template <typename T>
    void set_parameter(const std::string& name, const T* value) {
        auto it = std::find_if(parameters_.begin(), parameters_.end(), [&name](const auto& parm) { return parm.name == name; });
        if(it != parameters_.end()) set_parameter(int(it - parameters_.begin()), value);
        else                        LOG_ERR("Parameter does not exist in program:", LOG_GREEN, name);
    }

    template <typename T>
    void set_parameter(const std::string& name, const T& value) {
        set_parameter(name, &value);
    }

    template <typename T>
    void set_parameter(const std::string& name, const std::initializer_list<T>& value) {
        set_parameter(name, value.begin());
    }

    void add_sampler(const texture* tex_ptr, const sampler* smp_ptr) {
        textures_.emplace_back(tex_ptr);
        samplers_.emplace_back(smp_ptr);
    }
    template <typename... Args>
    void add_sampler(const texture* tex_ptr, const sampler* smp_ptr, Args... ptrs) {
        textures_.emplace_back(tex_ptr);
        samplers_.emplace_back(smp_ptr);
        add_sampler(ptrs...);
    }

    void add_texture(const texture* tex_ptr) {
        textures_.emplace_back(tex_ptr);
    }

    template <typename... Args>
    void add_texture(const texture* tex_ptr, Args... ptrs) {
        textures_.emplace_back(tex_ptr);
        add_texture(ptrs...);
    }

    void set_program(program* ptr) { program_ = ptr; }
    void set_program(const std::string& vshdr, const std::string& fshdr) {
        if(program_ && owns_program_) delete program_;
        program_ = new program{vshdr, fshdr};
        owns_program_ = true;
    }
    program* get_program() const { return program_; }

    void bind() const;
    void release() const;

protected:

private:
    // no ownership yet

    program* program_ = nullptr;
    std::vector<parameter> parameters_;         // move this to a lookup table in the engine later (per program)
    std::vector<const texture*> textures_;
    std::vector<const sampler*> samplers_;
    std::vector<int> offsets_;                  // Store the offset of each parameter (-1) for none
    std::vector<char> uniforms_;                // Store all uniforms in a contiguous block
    mutable std::vector<bool> dirty_flags_;     // A set of dirty flags for each parameter
    mutable bool dirty_ = true;                 // If any uniform has been set on the client but not yet on the server
    mutable bool is_bound_ = false;
    bool owns_program_ = false;                 // If set, this context owns the program and must delete it
};

}}

#endif //ZAP_RENDER_CONTEXT_HPP
