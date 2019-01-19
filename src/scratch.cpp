#include "Application.hpp"

#define SOKOL_IMPL
#define SOKOL_GLCORE33
//~ #define SOKOL_D3D11
//~ #define SOKOL_D3D11_SHADER_COMPILER
#define SOKOL_WIN32_FORCE_MAIN
#if !defined(NDEBUG)
#   define SOKOL_DEBUG 1
#endif
#include "sokol_app.h"
#include "sokol_gfx.h"
//
//sapp_desc sokol_main(int argc, char* argv[]) {
//    auto desc = Application::main(argc, argv);
//
//    return desc;
//}

#include "sokol_time.h"
#include "imgui.h"

/* even though we use 16-bit indices, the vertex buffer may hold
    more then 64k vertices, because it will be rendered with
    in chunks via buffer offsets
*/
const int MaxVertices = (1 << 17);
const int MaxIndices = MaxVertices * 3;

uint64_t last_time = 0;
bool show_test_window = true;
bool show_another_window = false;

sg_draw_state draw_state = { };
auto& m_drawState = draw_state;
sg_pass_action pass_action = { };
bool btn_down[SAPP_MAX_MOUSEBUTTONS] = { };
bool btn_up[SAPP_MAX_MOUSEBUTTONS] = { };

struct VertexShaderUniforms
{
    ImVec2 screenSize;
    static_assert(std::is_same<decltype(ImVec2::x), float>::value, "Only floats accepted");
};


void imgui_draw_cb(ImDrawData*);

extern const char* vs_src;
extern const char* fs_src;

void init(void) {
    // setup sokol-gfx and sokol-time
    sg_desc desc = { };
    desc.mtl_device = sapp_metal_get_device();
    desc.mtl_renderpass_descriptor_cb = sapp_metal_get_renderpass_descriptor;
    desc.mtl_drawable_cb = sapp_metal_get_drawable;
    desc.d3d11_device = sapp_d3d11_get_device();
    desc.d3d11_device_context = sapp_d3d11_get_device_context();
    desc.d3d11_render_target_view_cb = sapp_d3d11_get_render_target_view;
    desc.d3d11_depth_stencil_view_cb = sapp_d3d11_get_depth_stencil_view;
    desc.gl_force_gles2 = sapp_gles2();
    sg_setup(&desc);
    stm_setup();

    auto ctx = ImGui::CreateContext();
    ImGui::SetCurrentContext(ctx);

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;

    io.ClipboardUserData = nullptr;
    //io.GetClipboardTextFn = GetClipboardText;
    //io.SetClipboardTextFn = SetClipboardText;

    io.UserData = nullptr; // something else maybe?

    io.KeyMap[ImGuiKey_Tab] = SAPP_KEYCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = SAPP_KEYCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SAPP_KEYCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = SAPP_KEYCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow] = SAPP_KEYCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = SAPP_KEYCODE_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = SAPP_KEYCODE_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = SAPP_KEYCODE_HOME;
    io.KeyMap[ImGuiKey_End] = SAPP_KEYCODE_END;
    io.KeyMap[ImGuiKey_Delete] = SAPP_KEYCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = SAPP_KEYCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = SAPP_KEYCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter] = SAPP_KEYCODE_ENTER;
    io.KeyMap[ImGuiKey_Escape] = SAPP_KEYCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A] = SAPP_KEYCODE_A;
    io.KeyMap[ImGuiKey_C] = SAPP_KEYCODE_C;
    io.KeyMap[ImGuiKey_V] = SAPP_KEYCODE_V;
    io.KeyMap[ImGuiKey_X] = SAPP_KEYCODE_X;
    io.KeyMap[ImGuiKey_Y] = SAPP_KEYCODE_Y;
    io.KeyMap[ImGuiKey_Z] = SAPP_KEYCODE_Z;

    // font
    {
        ImFontConfig fontCfg;
        fontCfg.FontDataOwnedByAtlas = false;
        fontCfg.OversampleH = 2;
        fontCfg.OversampleV = 2;
        fontCfg.RasterizerMultiply = 1.5f;
        io.Fonts->AddFontDefault();

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        sg_image_desc desc = { };
        desc.width = width;
        desc.height = height;
        desc.pixel_format = SG_PIXELFORMAT_RGBA8;
        desc.wrap_u = SG_WRAP_CLAMP_TO_EDGE;
        desc.wrap_v = SG_WRAP_CLAMP_TO_EDGE;
        desc.min_filter = SG_FILTER_LINEAR;
        desc.mag_filter = SG_FILTER_LINEAR;
        desc.content.subimage[0][0].ptr = pixels;
        desc.content.subimage[0][0].size = width * height * 4;
        m_drawState.fs_images[0] = sg_make_image(&desc);
    }

    {
        // dynamic vertex- and index-buffers for imgui-generated geometry
        sg_buffer_desc vbuf = { };
        vbuf.usage = SG_USAGE_STREAM;
        vbuf.size = MaxVertices * sizeof(ImDrawVert);
        m_drawState.vertex_buffers[0] = sg_make_buffer(&vbuf);

        sg_buffer_desc ibuf = { };
        ibuf.type = SG_BUFFERTYPE_INDEXBUFFER;
        ibuf.usage = SG_USAGE_STREAM;
        ibuf.size = MaxIndices * sizeof(ImDrawIdx);
        m_drawState.index_buffer = sg_make_buffer(&ibuf);
    }

    sg_shader shd;
    {
        // shader object for imgui rendering
        sg_shader_desc desc = { };
        auto& ub = desc.vs.uniform_blocks[0];
        ub.size = sizeof(VertexShaderUniforms);
        ub.uniforms[0].name = "disp_size";
        ub.uniforms[0].type = SG_UNIFORMTYPE_FLOAT2;
        desc.fs.images[0].name = "tex";
        desc.fs.images[0].type = SG_IMAGETYPE_2D;
        desc.vs.source = vs_src;
        desc.fs.source = fs_src;
        shd = sg_make_shader(&desc);
    }

    {
        // pipeline object for imgui rendering
        sg_pipeline_desc desc = { };
        desc.layout.buffers[0].stride = sizeof(ImDrawVert);
        auto& attrs = desc.layout.attrs;
        attrs[0].name = "position"; attrs[0].sem_name = "POSITION"; attrs[0].offset = offsetof(ImDrawVert, pos); attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
        attrs[1].name = "texcoord0"; attrs[1].sem_name = "TEXCOORD"; attrs[1].offset = offsetof(ImDrawVert, uv); attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
        attrs[2].name = "color0"; attrs[2].sem_name = "COLOR"; attrs[2].offset = offsetof(ImDrawVert, col); attrs[2].format = SG_VERTEXFORMAT_UBYTE4N;
        desc.shader = shd;
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.blend.enabled = true;
        desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        desc.blend.color_write_mask = SG_COLORMASK_RGB;
        m_drawState.pipeline = sg_make_pipeline(&desc);
    }

    // initial clear color
    pass_action.colors[0].action = SG_ACTION_CLEAR;
    pass_action.colors[0].val[0] = 0.0f;
    pass_action.colors[0].val[1] = 0.5f;
    pass_action.colors[0].val[2] = 0.7f;
    pass_action.colors[0].val[3] = 1.0f;
}

void frame(void) {
    const int cur_width = sapp_width();
    const int cur_height = sapp_height();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(cur_width), float(cur_height));
    io.DeltaTime = (float)stm_sec(stm_laptime(&last_time));
    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++) {
        if (btn_down[i]) {
            btn_down[i] = false;
            io.MouseDown[i] = true;
        }
        else if (btn_up[i]) {
            btn_up[i] = false;
            io.MouseDown[i] = false;
        }
    }
    if (io.WantTextInput && !sapp_keyboard_shown()) {
        sapp_show_keyboard(true);
    }
    if (!io.WantTextInput && sapp_keyboard_shown()) {
        sapp_show_keyboard(false);
    }
    ImGui::NewFrame();

    // 1. Show a simple window
    // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
    static float f = 0.0f;
    ImGui::Text("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    ImGui::ColorEdit3("clear color", &pass_action.colors[0].val[0]);
    if (ImGui::Button("Test Window")) show_test_window ^= 1;
    if (ImGui::Button("Another Window")) show_another_window ^= 1;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("btn_down: %d %d %d\n", btn_down[0], btn_down[1], btn_down[2]);
    ImGui::Text("btn_up: %d %d %d\n", btn_up[0], btn_up[1], btn_up[2]);

    // 2. Show another simple window, this time using an explicit Begin/End pair
    if (show_another_window) {
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
    if (show_test_window) {
        ImGui::SetNextWindowPos(ImVec2(460, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow();
    }

    // the sokol_gfx draw pass
    sg_begin_default_pass(&pass_action, cur_width, cur_height);
    
    ImGui::EndFrame();
    ImGui::Render();
    
    auto data = ImGui::GetDrawData();
    assert(data);
    
    if (data->CmdListsCount == 0) return;

    VertexShaderUniforms uniforms;
    uniforms.screenSize = ImGui::GetIO().DisplaySize;

    for (int i = 0; i < data->CmdListsCount; ++i)
    {
        const auto list = data->CmdLists[i];

        // append vertices and indices to buffers, record start offsets in draw state
        const int vb_offset = sg_append_buffer(m_drawState.vertex_buffers[0], list->VtxBuffer.Data, list->VtxBuffer.size() * sizeof(ImDrawVert));
        const int ib_offset = sg_append_buffer(m_drawState.index_buffer, list->IdxBuffer.Data, list->IdxBuffer.size() * sizeof(ImDrawIdx));

        /* don't render anything if the buffer is in overflow state (this is also
            checked internally in sokol_gfx, draw calls that attempt from
            overflowed buffers will be silently dropped)
        */
        if (sg_query_buffer_overflow(draw_state.vertex_buffers[0]) ||
            sg_query_buffer_overflow(draw_state.index_buffer))
        {
            continue;
        }

        m_drawState.vertex_buffer_offsets[0] = vb_offset;
        m_drawState.index_buffer_offset = ib_offset;
        sg_apply_draw_state(&m_drawState);
        sg_apply_uniform_block(SG_SHADERSTAGE_VS, 0, &uniforms, sizeof(VertexShaderUniforms));

        int base_element = 0;
        for (const ImDrawCmd& pcmd : list->CmdBuffer) {
            if (pcmd.UserCallback) {
                pcmd.UserCallback(list, &pcmd);
            }
            else {
                const int scissor_x = (int)(pcmd.ClipRect.x);
                const int scissor_y = (int)(pcmd.ClipRect.y);
                const int scissor_w = (int)(pcmd.ClipRect.z - pcmd.ClipRect.x);
                const int scissor_h = (int)(pcmd.ClipRect.w - pcmd.ClipRect.y);
                sg_apply_scissor_rect(scissor_x, scissor_y, scissor_w, scissor_h, true);
                sg_draw(base_element, pcmd.ElemCount, 1);
            }
            base_element += pcmd.ElemCount;
        }
    }

    sg_end_pass();
    sg_commit();
}

void cleanup(void) {
    sg_shutdown();
}

void input(const sapp_event* event) {
    auto& io = ImGui::GetIO();
    io.KeyAlt = (event->modifiers & SAPP_MODIFIER_ALT) != 0;
    io.KeyCtrl = (event->modifiers & SAPP_MODIFIER_CTRL) != 0;
    io.KeyShift = (event->modifiers & SAPP_MODIFIER_SHIFT) != 0;
    io.KeySuper = (event->modifiers & SAPP_MODIFIER_SUPER) != 0;
    switch (event->type) {
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        io.MousePos.x = event->mouse_x;
        io.MousePos.y = event->mouse_y;
        btn_down[event->mouse_button] = true;
        break;
    case SAPP_EVENTTYPE_MOUSE_UP:
        io.MousePos.x = event->mouse_x;
        io.MousePos.y = event->mouse_y;
        btn_up[event->mouse_button] = true;
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        io.MousePos.x = event->mouse_x;
        io.MousePos.y = event->mouse_y;
        break;
    case SAPP_EVENTTYPE_MOUSE_ENTER:
    case SAPP_EVENTTYPE_MOUSE_LEAVE:
        for (int i = 0; i < 3; i++) {
            btn_down[i] = false;
            btn_up[i] = false;
            io.MouseDown[i] = false;
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        io.MouseWheelH = event->scroll_x;
        io.MouseWheel = event->scroll_y;
        break;
    case SAPP_EVENTTYPE_TOUCHES_BEGAN:
        btn_down[0] = true;
        io.MousePos.x = event->touches[0].pos_x;
        io.MousePos.y = event->touches[0].pos_y;
        break;
    case SAPP_EVENTTYPE_TOUCHES_MOVED:
        io.MousePos.x = event->touches[0].pos_x;
        io.MousePos.y = event->touches[0].pos_y;
        break;
    case SAPP_EVENTTYPE_TOUCHES_ENDED:
        btn_up[0] = true;
        io.MousePos.x = event->touches[0].pos_x;
        io.MousePos.y = event->touches[0].pos_y;
        break;
    case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
        btn_up[0] = btn_down[0] = false;
        break;
    case SAPP_EVENTTYPE_KEY_DOWN:
        io.KeysDown[event->key_code] = true;
        break;
    case SAPP_EVENTTYPE_KEY_UP:
        io.KeysDown[event->key_code] = false;
        break;
    case SAPP_EVENTTYPE_CHAR:
        io.AddInputCharacter((ImWchar)event->char_code);
        break;
    default:
        break;
    }
}

sapp_desc sokol_main(int argc, char* argv[]) {
    sapp_desc desc = { };
    desc.init_cb = init;
    desc.frame_cb = frame;
    desc.cleanup_cb = cleanup;
    desc.event_cb = input;
    desc.width = 1024;
    desc.height = 768;
    desc.gl_force_gles2 = true;
    desc.window_title = "Dear ImGui (sokol-app)";
    desc.ios_keyboard_resizes_canvas = false;
    return desc;
}

#if defined(SOKOL_GLCORE33)
const char* vs_src =
"#version 330\n"
"uniform vec2 disp_size;\n"
"in vec2 position;\n"
"in vec2 texcoord0;\n"
"in vec4 color0;\n"
"out vec2 uv;\n"
"out vec4 color;\n"
"void main() {\n"
"    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
"    uv = texcoord0;\n"
"    color = color0;\n"
"}\n";
const char* fs_src =
"#version 330\n"
"uniform sampler2D tex;\n"
"in vec2 uv;\n"
"in vec4 color;\n"
"out vec4 frag_color;\n"
"void main() {\n"
"    frag_color = texture(tex, uv) * color;\n"
"}\n";
#elif defined(SOKOL_GLES2) || defined(SOKOL_GLES3)
const char* vs_src =
"uniform vec2 disp_size;\n"
"attribute vec2 position;\n"
"attribute vec2 texcoord0;\n"
"attribute vec4 color0;\n"
"varying vec2 uv;\n"
"varying vec4 color;\n"
"void main() {\n"
"    gl_Position = vec4(((position/disp_size)-0.5)*vec2(2.0,-2.0), 0.5, 1.0);\n"
"    uv = texcoord0;\n"
"    color = color0;\n"
"}\n";
const char* fs_src =
"precision mediump float;\n"
"uniform sampler2D tex;\n"
"varying vec2 uv;\n"
"varying vec4 color;\n"
"void main() {\n"
"    gl_FragColor = texture2D(tex, uv) * color;\n"
"}\n";
#elif defined(SOKOL_METAL)
const char* vs_src =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"struct params_t {\n"
"  float2 disp_size;\n"
"};\n"
"struct vs_in {\n"
"  float2 pos [[attribute(0)]];\n"
"  float2 uv [[attribute(1)]];\n"
"  float4 color [[attribute(2)]];\n"
"};\n"
"struct vs_out {\n"
"  float4 pos [[position]];\n"
"  float2 uv;\n"
"  float4 color;\n"
"};\n"
"vertex vs_out _main(vs_in in [[stage_in]], constant params_t& params [[buffer(0)]]) {\n"
"  vs_out out;\n"
"  out.pos = float4(((in.pos / params.disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
"  out.uv = in.uv;\n"
"  out.color = in.color;\n"
"  return out;\n"
"}\n";
const char* fs_src =
"#include <metal_stdlib>\n"
"using namespace metal;\n"
"struct fs_in {\n"
"  float2 uv;\n"
"  float4 color;\n"
"};\n"
"fragment float4 _main(fs_in in [[stage_in]], texture2d<float> tex [[texture(0)]], sampler smp [[sampler(0)]]) {\n"
"  return tex.sample(smp, in.uv) * in.color;\n"
"}\n";
#elif defined(SOKOL_D3D11)
const char* vs_src =
"cbuffer params {\n"
"  float2 disp_size;\n"
"};\n"
"struct vs_in {\n"
"  float2 pos: POSITION;\n"
"  float2 uv: TEXCOORD0;\n"
"  float4 color: COLOR0;\n"
"};\n"
"struct vs_out {\n"
"  float2 uv: TEXCOORD0;\n"
"  float4 color: COLOR0;\n"
"  float4 pos: SV_Position;\n"
"};\n"
"vs_out main(vs_in inp) {\n"
"  vs_out outp;\n"
"  outp.pos = float4(((inp.pos/disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
"  outp.uv = inp.uv;\n"
"  outp.color = inp.color;\n"
"  return outp;\n"
"}\n";
const char* fs_src =
"Texture2D<float4> tex: register(t0);\n"
"sampler smp: register(s0);\n"
"float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0): SV_Target0 {\n"
"  return tex.Sample(smp, uv) * color;\n"
"}\n";
#endif