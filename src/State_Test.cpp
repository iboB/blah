#include "State_Test.hpp"

#include "AppState.hpp"
#include "Application.hpp"

#include "DearImGui.hpp"

#include "SokolGFX.hpp"

#include <yama/yama.hpp>

using namespace yama;

namespace
{

const char* vs_src =
"#version 330\n"
"uniform mat4 u_projView;\n"
"uniform mat4 u_model;\n"
"in vec4 a_position;\n"
"void main() {\n"
"    gl_Position = u_projView * u_model * a_position;\n"
"}\n";

const char* fs_src =
"#version 330\n"
"uniform vec4 u_color;\n"
"out vec4 frag_color;\n"
"void main() {\n"
"    frag_color = u_color;\n"
"}\n";


struct VUniforms
{
    matrix projView;
    matrix model;
};

struct FUniforms
{
    vector4 color;
};

class TestState : public AppState
{
    const char* name() const override { return "test"; }

    sg_pipeline m_pipeline;
    sg_bindings m_bindings;

    sg_shader m_shader;

    bool initialize() override
    {
        sg_pass_action action = {};
        action.colors[0].action = SG_ACTION_CLEAR;
        action.colors[0].val[0] = 0.0f;
        action.colors[0].val[1] = 0.5f;
        action.colors[0].val[2] = 0.7f;
        action.colors[0].val[3] = 1.0f;
        action.depth.action = SG_ACTION_CLEAR;
        action.depth.val = 1;
        action.stencil.action = SG_ACTION_DONTCARE;
        Application::setDefaultPassAction(action);

        {
            sg_shader_desc desc = { };

            auto& vub = desc.vs.uniform_blocks[0];
            vub.size = sizeof(VUniforms);
            vub.uniforms[0].name = "u_projView";
            vub.uniforms[0].type = SG_UNIFORMTYPE_MAT4;
            vub.uniforms[1].name = "u_model";
            vub.uniforms[1].type = SG_UNIFORMTYPE_MAT4;

            auto& fub = desc.fs.uniform_blocks[0];
            fub.size = sizeof(FUniforms);
            fub.uniforms[0].name = "u_color";
            fub.uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;

            desc.vs.source = vs_src;
            desc.fs.source = fs_src;
            m_shader = sg_make_shader(&desc);
        }

        {
            sg_pipeline_desc desc = { };
            desc.layout.buffers[0].stride = sizeof(vector3);
            auto& attrs = desc.layout.attrs;
            attrs[0].name = "a_position"; attrs[0].sem_name = "POSITION"; attrs[0].offset = 0; attrs[0].format = SG_VERTEXFORMAT_FLOAT3;
            desc.shader = m_shader;
            desc.index_type = SG_INDEXTYPE_UINT16;
            desc.blend.enabled = true;
            desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
            desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
            desc.blend.color_write_mask = SG_COLORMASK_RGB;
            desc.rasterizer.face_winding = SG_FACEWINDING_CCW;
            desc.rasterizer.cull_mode = SG_CULLMODE_NONE;
            desc.depth_stencil.depth_compare_func = SG_COMPAREFUNC_LESS;
            m_pipeline = sg_make_pipeline(&desc);
        }

        vector3 vertices[] =
        {
            {-0.3f, -0.3f, 0.2f},
            {0.7f, -0.3f, 0.2f},
            {-0.3f, 0.7f, 0.2f},
            {0, 0, 0},
            {2, 0, 0},
            {0, 1, 0},
        };

        uint16_t indices[] =
        {
            0, 1, 2, 3, 4, 5
        };


        {
            // dynamic vertex- and index-buffers for imgui-generated geometry
            sg_buffer_desc vbuf = { };
            vbuf.usage = SG_USAGE_IMMUTABLE;
            vbuf.size = sizeof(vertices);
            vbuf.content = vertices;
            m_bindings.vertex_buffers[0] = sg_make_buffer(&vbuf);

            sg_buffer_desc ibuf = { };
            ibuf.type = SG_BUFFERTYPE_INDEXBUFFER;
            ibuf.usage = SG_USAGE_IMMUTABLE;
            ibuf.size = sizeof(vertices);
            ibuf.content = indices;
            m_bindings.index_buffer = sg_make_buffer(&ibuf);
        }

        return true;
    }

    void cleanup() override
    {
        sg_destroy_pipeline(m_pipeline);
        sg_destroy_shader(m_shader);
        sg_destroy_buffer(m_bindings.vertex_buffers[0]);
        sg_destroy_buffer(m_bindings.index_buffer);
    }

    bool processEvent(const sapp_event& e) override
    {
        return false;
    }

    void update(uint32_t dt) override
    {
        ImGui::ShowDemoWindow();
    }

    void render() override
    {
        sg_apply_pipeline(m_pipeline);
        sg_apply_bindings(&m_bindings);

        float w = 4;
        float h = 4;
        float d = 10;

        matrix proj = matrix::perspective_fov_rh(yama::constants::PI() / 2, 1, 1, 100);
        matrix view = matrix::look_towards_rh(v(0, 0, 5), v(0, 0, -1), v(0, 1, 0));
        matrix projView = proj * view;

        VUniforms vu;
        vu.projView = projView;
        //vu.projView = projView;
        vu.model = matrix::identity();

        sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vu, sizeof(VUniforms));

        FUniforms fu;
        fu.color = v(1, 0, 0, 1);

        sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fu, sizeof(FUniforms));

        sg_draw(0, 6, 1);
    }

};

}

AppStatePtr State_Test()
{
    return std::make_unique<TestState>();
}
