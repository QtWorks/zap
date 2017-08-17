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
#include <renderer/renderer.hpp>
#include <generators/generator.hpp>
#include <generators/geometry/geometry3.hpp>
#include <renderer/light.hpp>
#include <renderer/shader_builder.hpp>

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
using p3n3t2_geo3_tri = generators::geometry3<vtx_p3n3t2_t, primitive_type::PT_TRIANGLES>;
using p3n3t2_geo3_ts = generators::geometry3<vtx_p3n3t2_t, primitive_type::PT_TRIANGLE_STRIP>;
using p3n3t2_geo3_tf = generators::geometry3<vtx_p3n3t2_t, primitive_type::PT_TRIANGLE_FAN>;

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
    generator gen_;
    std::vector<visual_t> visuals_;
    zap::renderer::renderer rndr_;
    std::vector<std::unique_ptr<mesh_base>> meshes_;
    std::unique_ptr<render_context> context_;
    std::vector<texture> textures_;
    std::vector<sampler> samplers_;
    uniform_buffer<camera_block> camera_block_;
    uniform_buffer<material_block> material_block_;
    uniform_buffer<builder_task<>::lights_block_t> lights_block_;
};

bool scene_graph_test::initialise() {
    clear(0.f, 0.f, 0.f, 0.f);

    if(!gen_.initialise()) {
        LOG_ERR("Failed to initialise generator or renderer");
        return false;
    }

    builder_task<> task;
    task.method = builder_task<>::lighting_method::LM_GOURAUD;
    context_ = shader_builder::build_basic_lights(task);

    // Create some textures
    render_task req{256, 128, render_task::basis_function::USER_FUNCTION};
    req.mipmaps = true;

    req.scale.set(20.f, 20.f);
    textures_.emplace_back(gen_.render_spherical(req, [](float x, float y, float z, generator& gen) {
        int ix = maths::floor(x), iy = maths::floor(y), iz = maths::floor(z);
        float value = clamp(.707f + .5f*gen.pnoise(x - ix, y - iy, z - iz, ix, iy, iz));
        vec3b colour = lerp(value, vec3b(255, 0, 0), vec3b(255, 255, 0));
        return rgb888_t{colour};
    }));

    req.scale.set(2.f, 2.f);
    textures_.emplace_back(gen_.render_spherical(req, [](float x, float y, float z, generator& gen) {
        float lat = .2f*TWO_PI<float>*std::acos(y/2.f);
        float lon = .4f*TWO_PI<float>*std::atan(x/z);
        int ilat = maths::floor(lat), ilon = maths::floor(lon);
        bool parity = ((ilat+1)+ilon)%2 == 0;
        return rgb888_t{parity ? vec3b{0, 0, 0} : vec3b{255, 255, 255}};
    }));

    req.scale.set(20.f, 20.f);
    textures_.emplace_back(gen_.render_cylindrical(req, [](float x, float y, float z, generator& gen) {
        int ix = maths::floor(x), iy = maths::floor(y), iz = maths::floor(z);
        float value = clamp(.707f + .5f*gen.vnoise(x - ix, y - iy, z - iz, ix, iy, iz));
        vec3b colour = lerp(value, vec3b(0, 255, 0), vec3b(128, 128, 64));
        return rgb888_t{colour};
    }));

    req.width = 512; req.height = 512;
    req.scale.set(1.f, 1.f);
    textures_.emplace_back(gen_.render_planar(req, [](float x, float y, generator& gen) {
        int ix = maths::floor(100*x), iy = maths::floor(100*y);
        byte c = ((ix+1)+iy) % 2 ? 0 : 255;
        return rgb888_t{ c, c, c };
    }));

    samplers_.emplace_back();
    samplers_[0].allocate();
    samplers_[0].initialise();
    samplers_[0].set_mag_filter(tex_filter::TF_LINEAR);
    samplers_[0].set_min_filter(tex_filter::TF_LINEAR_MIPMAP_LINEAR);

    samplers_.emplace_back();
    samplers_[1].allocate();
    samplers_[1].initialise();
    samplers_[1].set_anisotropy(16.f);
    samplers_[1].set_mag_filter(tex_filter::TF_NEAREST);
    samplers_[1].set_min_filter(tex_filter::TF_LINEAR_MIPMAP_LINEAR);

    // Warning: Don't realloc textures or samplers after this...
    context_->add_sampler(&textures_[0], &samplers_[0],
                          &textures_[1], &samplers_[0],
                          &textures_[2], &samplers_[0],
                          &textures_[3], &samplers_[1]);

    if(!context_->initialise()) {
        LOG_ERR("Failed to initialise render_context");
        return false;
    }

    lights_block_.allocate();
    lights_block_.bind();
    lights_block_.initialise();
    lights_block_.release();
    context_->add_uniform_buffer("lights", &lights_block_);

    material_block mat;
    mat.material_emissive.set(0.f, 0.f, 0.f, 0.f);
    mat.material_ambient.set(0.1f, 0.f, 0.f, 0.f);
    mat.material_diffuse.set(.4f, .4f, .4f, 1.f);
    mat.material_specular.set(.7f, .7f, .7f, 0.f);
    mat.material_exponent = 50.f;

    material_block_.allocate();
    material_block_.bind();
    material_block_.initialise(mat);
    material_block_.release();
    context_->add_uniform_buffer("material", &material_block_);

    camera_block_.allocate();
    camera_block_.bind();
    camera_block_.initialise(cam_.get_uniform_block(false));
    camera_block_.release();
    context_->add_uniform_buffer("camera", &camera_block_);

    auto quad_mesh = p3n3t2_geo3_tf::make_mesh(p3n3t2_geo3_tf::make_quad(200.f, 200.f));
    visual_t quad{quad_mesh.get(), context_.get()};
    meshes_.emplace_back(std::move(quad_mesh));

    quad.rotate(make_rotation(vec3f{1.f, 0.f, 0.f}, PI<float>/2.f));
    visuals_.push_back(quad);

    auto sphere_mesh = p3n3t2_geo3_tri::make_mesh(p3n3t2_geo3_tri::make_UVsphere<float,uint32_t>(30, 60, .5f, false));
    visual_t sphere{sphere_mesh.get(), context_.get()};
    meshes_.emplace_back(std::move(sphere_mesh));

    sphere.translate(vec3f{-3.f, 2.5f, 0.f});
    visuals_.push_back(sphere);
    sphere.translate(vec3f{+0.f, 2.5f, 0.f});
    visuals_.push_back(sphere);
    sphere.translate(vec3f{+3.f, 2.5f, 0.f});
    visuals_.push_back(sphere);

    auto cylinder_mesh = p3n3t2_geo3_ts::make_mesh(p3n3t2_geo3_ts::make_cylinder(5, 30, 1.f, .5f, false));
    visual_t cylinder{cylinder_mesh.get(), context_.get()};
    meshes_.emplace_back(std::move(cylinder_mesh));

    cylinder.translate(vec3f{-3.f, 1.f, 0.f});
    visuals_.push_back(cylinder);
    cylinder.translate(vec3f{+0.f, 1.f, 0.f});
    visuals_.push_back(cylinder);
    cylinder.translate(vec3f{+3.f, 1.f, 0.f});
    visuals_.push_back(cylinder);

    gl_error_check();
    return true;
}

void scene_graph_test::on_resize(int width, int height) {
    cam_.viewport(0, 0, width, height);
    cam_.world_pos(vec3f{0.f, 2.f, 10.f});
    cam_.look_at(vec3f{0.f, 0.f, -100.f});
    cam_.frustum(45.f, width/float(height), .5f, 100.f);
    camera_block_.bind();
    camera_block_.initialise(cam_.get_uniform_block());
    camera_block_.release();

    lights_block_.bind();
    if(lights_block_.map(buffer_access::BA_WRITE_ONLY)) {
        lights_block_.get()->lights_dir[0].light_dir.set(0.f, 1.f, 0.f, 0.f);
        lights_block_.get()->lights_dir[0].light_colour.set(1.f, 1.f, 1.f, 1.f);
        lights_block_.get()->lights_dir[0].light_ADS.set(.3f, .5f, .5f, 1.f);
        lights_block_.get()->lights_dir[0].light_intensity = 1.f;

        // Distribute the point lights around the scene
        const float angle = TWO_PI<float>/10;
        const auto colA = vec3f{1.f, 0.f, 0.f}, colB = vec3f{1.f, 1.f, 0.f};
        for(int i = 0; i != 10; ++i) {
            float theta = angle * i;
            float x = 15.f*cosf(theta), z = 15.f*sinf(theta);
            vec3f vP = cam_.world_to_view() * vec3f{x, 3.f, z};
            lights_block_.get()->lights_point[i].light_position.set(vP, 1.f);
            lights_block_.get()->lights_point[i].light_attenuation.set(.4f, 0.f, 0.f, 0.f);
            lights_block_.get()->lights_point[i].light_colour.set(lerp(i/10.f, colA, colB), 0.f);
            lights_block_.get()->lights_point[i].light_ADS.set(1.f, 1.f, 1.f, 1.f);
            lights_block_.get()->lights_point[i].light_intensity = .5f;
        }

        vec3f lP = vec3f{-5.f, 10.f, 10.f};
        vec3f vP = cam_.world_to_view() * lP;
        vec3f d = cam_.world_to_view() * -normalise(lP);
        lights_block_.get()->lights_spot[0].light_position.set(vP.x, vP.y, vP.z, 0.f);
        lights_block_.get()->lights_spot[0].light_dir.set(d.x, d.y, d.z, 0.f);
        lights_block_.get()->lights_spot[0].light_attenuation.set(.5f, .0f, 0.f, 0.f);
        lights_block_.get()->lights_spot[0].light_colour.set(1.f, 0.f, 0.f, 1.f);
        lights_block_.get()->lights_spot[0].light_ADS.set(.1f, .5f, .9f, 0.f);
        lights_block_.get()->lights_spot[0].light_cos_angle = .5f;
        lights_block_.get()->lights_spot[0].light_exponent = .5f;
        lights_block_.get()->lights_spot[0].light_intensity = 1.f;

        lP = vec3f{0.f, 10.f, 10.f};
        vP = cam_.world_to_view() * lP;
        d = cam_.world_to_view() * -normalise(lP);
        lights_block_.get()->lights_spot[1].light_position.set(vP.x, vP.y, vP.z, 0.f);
        lights_block_.get()->lights_spot[1].light_dir.set(d.x, d.y, d.z, 0.f);
        lights_block_.get()->lights_spot[1].light_attenuation.set(.5f, .0f, 0.f, 0.f);
        lights_block_.get()->lights_spot[1].light_colour.set(0.f, 1.f, 0.f, 1.f);
        lights_block_.get()->lights_spot[1].light_ADS.set(.1f, .5f, .9f, 0.f);
        lights_block_.get()->lights_spot[1].light_cos_angle = .5f;
        lights_block_.get()->lights_spot[1].light_exponent = .5f;
        lights_block_.get()->lights_spot[1].light_intensity = 1.f;

        lP = vec3f{+5.f, 10.f, 10.f};
        vP = cam_.world_to_view() * lP;
        d = cam_.world_to_view() * -normalise(lP);      // [0,0,0] - lP
        lights_block_.get()->lights_spot[2].light_position.set(vP.x, vP.y, vP.z, 0.f);
        lights_block_.get()->lights_spot[2].light_dir.set(d.x, d.y, d.z, 0.f);
        lights_block_.get()->lights_spot[2].light_attenuation.set(.5f, .0f, 0.f, 0.f);
        lights_block_.get()->lights_spot[2].light_colour.set(0.f, 0.f, 1.f, 1.f);
        lights_block_.get()->lights_spot[2].light_ADS.set(.1f, .5f, .9f, 0.f);
        lights_block_.get()->lights_spot[2].light_cos_angle = .5f;
        lights_block_.get()->lights_spot[2].light_exponent = .5f;
        lights_block_.get()->lights_spot[2].light_intensity = 1.f;

        lights_block_.get()->light_count.x = 1;
        lights_block_.get()->light_count.y = 10;
        lights_block_.get()->light_count.z = 3;
        lights_block_.unmap();
    }
    lights_block_.release();

    gl_error_check();
}

void scene_graph_test::update(double t, float dt) {
    static float inc = 0.f;
    auto rot = make_rotation(vec3f{0.f, 1.f, 0.f}, inc) * make_rotation(vec3f{1.f, 0.f, 0.f}, PI<float>/2);
    for(int i = 1; i != 7; ++i) visuals_[i].rotate(rot);
    inc += dt;

    // Cycle through the lights
    static float timer = 0.f;
    static int counter = 0;
    timer += dt;
    if((counter != 7 && timer > 3.f) || (counter == 7 && timer > 6.f)) {
        counter++;
        if(counter == 10) counter = 0;
        lights_block_.bind();
        if(lights_block_.map(buffer_access::BA_WRITE_ONLY)) {
            if(counter & 0x01) lights_block_.get()->lights_spot[0].light_intensity = .75f;
            else lights_block_.get()->lights_spot[0].light_intensity = 0.f;
            if(counter & 0x02) lights_block_.get()->lights_spot[1].light_intensity = .75f;
            else lights_block_.get()->lights_spot[1].light_intensity = 0.f;
            if(counter & 0x04) lights_block_.get()->lights_spot[2].light_intensity = .75f;
            else lights_block_.get()->lights_spot[2].light_intensity = 0.f;

            for(int i = 0; i != 10; ++i) {
                lights_block_.get()->lights_point[i].light_intensity = i == counter ? 1.f : 0.f;
            }

            lights_block_.unmap();
        }
        lights_block_.release();
        timer = 0.f;
    }

    gl_error_check();
}

void scene_graph_test::draw() {
    for(int i = 0; i != 7; ++i) {
        context_->set_parameter("mvp_matrix", cam_.proj_view() * visuals_[i].world_transform().gl_matrix());
        auto MV = cam_.world_to_view() * visuals_[i].world_transform().gl_matrix();
        context_->set_parameter("mv_matrix", MV);
        context_->set_parameter("normal_matrix", MV.inverse().transpose().rotation());
        //context_->set_texture_unit("diffuse_tex", i == 0 ? 3 : (i-1)%3);
        visuals_[i].draw(rndr_);
    }
    gl_error_check();
}

void scene_graph_test::shutdown() {
    gl_error_check();
}

int main(int argc, char* argv[]) {
    scene_graph_test app;
    return app.run();
}
