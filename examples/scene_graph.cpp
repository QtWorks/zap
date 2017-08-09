//
// Created by Darren Otgaar on 2017/07/27.
//

/**
 * Tests the scene graph classes
 */

#include <tools/log.hpp>
#include <maths/io.hpp>
#include <host/GLFW/application.hpp>

#include <scene_graph/node.hpp>
#include <scene_graph/visual.hpp>

#include <renderer/camera.hpp>
#include <generators/geometry/geometry3.hpp>

using namespace zap;
using namespace zap::maths;
using namespace zap::engine;
using namespace zap::graphics;
using namespace zap::renderer;
using namespace zap::scene_graph;

using spheref = geometry::sphere<float>;
using spatial_t = spatial<transform4f, spheref>;
using node_t = node<spatial_t>;
using visual_t = visual<spatial_t>;
using p3_geo3 = generators::geometry3<vtx_p3_t, primitive_type::PT_TRIANGLES>;

using cam_block = uniform_block<
    core::cam_view<mat4f>,
    core::cam_projection<mat4f>,
    core::cam_proj_view<mat4f>,
    core::viewport<vec4i>
>;
using cam_ubuffer = uniform_buffer<cam_block>;

const char* const basic_vshdr = GLSL(
    layout (std140) uniform cam_block {
        mat4 cam_view;
        mat4 cam_projection;
        mat4 cam_proj_view;
        vec4 viewport;
    };

    in vec3 position;
    void main() {
        gl_Position = cam_proj_view * vec4(position, 1.0);
    }
);

const char* const basic_fshdr = GLSL(
    uniform vec4 colour = vec4(1., 1., 1., 1.);
    out vec4 frag_colour;
    void main() {
        frag_colour = colour;
    }
);

class scene_graph_test : public application {
public:
    scene_graph_test() : application{"scene_graph_test", 1280, 900, false}, cam_{true} { }

    bool initialise() final;
    void update(double t, float dt) final;
    void draw() final;
    void shutdown() final;

    void on_resize(int width, int height) final;

protected:
    camera cam_;
    std::vector<std::unique_ptr<mesh_base>> meshes_;
    std::vector<render_context> contexts_;

};

bool scene_graph_test::initialise() {
    clear(0.f, 0.f, 0.f, 0.f);

    auto cube = p3_geo3::make_cube(vec3f(.1f, .5f, .25f));
    auto cube_mesh = make_mesh<vtx_p3_t, primitive_type::PT_TRIANGLES>(cube);
    meshes_.emplace_back(std::move(cube_mesh));

    auto sphere = p3_geo3::make_UVsphere<float, uint32_t>(30, 60, .2f, true);
    auto sphere_mesh = make_mesh<vtx_p3_t, primitive_type::PT_TRIANGLES, uint32_t>(sphere);
    meshes_.emplace_back(std::move(sphere_mesh));

    contexts_.emplace_back(basic_vshdr, basic_fshdr);
    if(!contexts_.back().initialise()) {
        LOG_ERR("Failed to initialise context");
        return false;
    }

    //wire_frame(true);
    depth_test(true);
    bf_culling(false);

    gl_error_check();
    return true;
}

void scene_graph_test::on_resize(int width, int height) {
    cam_.viewport(0, 0, width, height);
    cam_.world_pos(vec3f{2.f, 2.f, 5.f});
    cam_.look_at(vec3f{0.f, 0.f, 0.f});
    cam_.frustum(45.f, width/float(height), .5f, 100.f);
    for(auto& ctx : contexts_) {
        if(ctx.has_parameter("PVM")) ctx.set_parameter("PVM", cam_.proj_view()*make_scale(3.f, 3.f, 3.f));
    }
    gl_error_check();
}

void scene_graph_test::update(double t, float dt) {
    gl_error_check();
}

void scene_graph_test::draw() {
    const vec4f colours[2] = {vec4f{1.f, 1.f, 0.f, 1.f}, vec4f{0.f, 0.f, 1.f, 1.f}};
    contexts_.back().bind();
    int counter = 0;
    for(auto& mesh : meshes_) {
        mesh->bind();
        contexts_.back().set_parameter("colour", colours[counter++]);
        mesh->draw();
        mesh->release();
    }
    contexts_.back().release();
    gl_error_check();
}

void scene_graph_test::shutdown() {
    gl_error_check();
}

int main(int argc, char* argv[]) {
    scene_graph_test app;
    return app.run();
}

// Generators