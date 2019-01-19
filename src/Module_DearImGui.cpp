#include "Module_DearImGui.hpp"

#include "DearImGui.hpp"

#include "sokol_app.h"
#include "SokolGFX.hpp"

#include <cstdio> // temp!

#include <yama/vector2.hpp>

namespace
{

const char* GetClipboardText(void* userData)
{
    puts("no clipboard text :(");
    return "no-cliboard!";
}

void SetClipboardText(void* userData, const char* text)
{
    printf("no clip :( : %s", text);
}

sg_pipeline m_pipeline;
sg_bindings m_bindings;

/* even though we use 16-bit indices, the vertex buffer may hold
    more then 64k vertices, because it will be rendered with
    in chunks via buffer offsets
*/
const int Max_Vertices = (1 << 17);
const int Max_Indices = Max_Vertices * 3;

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

//~ const char* vs_src =
//~ "//cbuffer params {\n"
//~ "  float2 disp_size;\n"
//~ "//};\n"
//~ "struct vs_in {\n"
//~ "  float2 pos: POSITION;\n"
//~ "  float2 uv: TEXCOORD0;\n"
//~ "  float4 color: COLOR0;\n"
//~ "};\n"
//~ "struct vs_out {\n"
//~ "  float2 uv: TEXCOORD0;\n"
//~ "  float4 color: COLOR0;\n"
//~ "  float4 pos: SV_Position;\n"
//~ "};\n"
//~ "vs_out main(vs_in inp) {\n"
//~ "  vs_out outp;\n"
//~ "  outp.pos = float4(((inp.pos/disp_size)-0.5)*float2(2.0,-2.0), 0.5, 1.0);\n"
//~ "  outp.uv = inp.uv;\n"
//~ "  outp.color = inp.color;\n"
//~ "  return outp;\n"
//~ "}\n";
//~ const char* fs_src =
//~ "Texture2D<float4> tex: register(t0);\n"
//~ "sampler smp: register(s0);\n"
//~ "float4 main(float2 uv: TEXCOORD0, float4 color: COLOR0): SV_Target0 {\n"
//~ "  return tex.Sample(smp, uv) * color;\n"
//~ "}\n";

struct VertexShaderUniforms
{
    ImVec2 screenSize;
    static_assert(std::is_same<decltype(ImVec2::x), float>::value, "Only floats accepted");
};

// used for clicks which are faster than a single frame
bool m_mouseDown[SAPP_MAX_MOUSEBUTTONS];
bool m_mouseUp[SAPP_MAX_MOUSEBUTTONS];

sg_image m_fontsTexture;
sg_shader m_shader;

}

bool Module_DearImGui::init()
{
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

        m_fontsTexture = sg_make_image(&desc);
        io.Fonts->TexID = reinterpret_cast<ImTextureID>(uintptr_t(m_fontsTexture.id));

        io.Fonts->ClearInputData();
        io.Fonts->ClearTexData();
    }

    {
        // dynamic vertex- and index-buffers for imgui-generated geometry
        sg_buffer_desc vbuf = { };
        vbuf.usage = SG_USAGE_STREAM;
        vbuf.size = Max_Vertices * sizeof(ImDrawVert);
        m_bindings.vertex_buffers[0] = sg_make_buffer(&vbuf);

        sg_buffer_desc ibuf = { };
        ibuf.type = SG_BUFFERTYPE_INDEXBUFFER;
        ibuf.usage = SG_USAGE_STREAM;
        ibuf.size = Max_Indices * sizeof(ImDrawIdx);
        m_bindings.index_buffer = sg_make_buffer(&ibuf);
    }

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
        m_shader = sg_make_shader(&desc);
    }

    {
        // pipeline object for imgui rendering
        sg_pipeline_desc desc = { };
        desc.layout.buffers[0].stride = sizeof(ImDrawVert);
        auto& attrs = desc.layout.attrs;
        attrs[0].name = "position"; attrs[0].sem_name = "POSITION"; attrs[0].offset = offsetof(ImDrawVert, pos); attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
        attrs[1].name = "texcoord0"; attrs[1].sem_name = "TEXCOORD"; attrs[1].offset = offsetof(ImDrawVert, uv); attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
        attrs[2].name = "color0"; attrs[2].sem_name = "COLOR"; attrs[2].offset = offsetof(ImDrawVert, col); attrs[2].format = SG_VERTEXFORMAT_UBYTE4N;
        desc.shader = m_shader;
        desc.index_type = SG_INDEXTYPE_UINT16;
        desc.blend.enabled = true;
        desc.blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
        desc.blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        desc.blend.color_write_mask = SG_COLORMASK_RGB;
        m_pipeline = sg_make_pipeline(&desc);
    }

    return true;
}

void Module_DearImGui::cleanup()
{
    sg_destroy_pipeline(m_pipeline);
    sg_destroy_shader(m_shader);
    sg_destroy_buffer(m_bindings.vertex_buffers[0]);
    sg_destroy_buffer(m_bindings.index_buffer);
    sg_destroy_image(m_fontsTexture);
    ImGui::DestroyContext();
}

bool Module_DearImGui::processEvent(const sapp_event& e)
{
    auto& io = ImGui::GetIO();
    io.KeyAlt = (e.modifiers & SAPP_MODIFIER_ALT) != 0;
    io.KeyCtrl = (e.modifiers & SAPP_MODIFIER_CTRL) != 0;
    io.KeyShift = (e.modifiers & SAPP_MODIFIER_SHIFT) != 0;
    io.KeySuper = (e.modifiers & SAPP_MODIFIER_SUPER) != 0;
    switch (e.type)
    {
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        io.MousePos.x = e.mouse_x;
        io.MousePos.y = e.mouse_y;
        m_mouseDown[e.mouse_button] = true;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_MOUSE_UP:
        io.MousePos.x = e.mouse_x;
        io.MousePos.y = e.mouse_y;
        m_mouseUp[e.mouse_button] = true;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        io.MousePos.x = e.mouse_x;
        io.MousePos.y = e.mouse_y;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_MOUSE_ENTER:
    case SAPP_EVENTTYPE_MOUSE_LEAVE:
        for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++)
        {
            m_mouseDown[i] = false;
            m_mouseUp[i] = false;
            io.MouseDown[i] = false;
        }
        return false;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        // magic 6-s. It looks good to me.
        io.MouseWheelH = e.scroll_x/6;
        io.MouseWheel = e.scroll_y/6;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_TOUCHES_BEGAN:
        m_mouseDown[0] = true;
        io.MousePos.x = e.touches[0].pos_x;
        io.MousePos.y = e.touches[0].pos_y;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_TOUCHES_MOVED:
        io.MousePos.x = e.touches[0].pos_x;
        io.MousePos.y = e.touches[0].pos_y;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_TOUCHES_ENDED:
        m_mouseUp[0] = true;
        io.MousePos.x = e.touches[0].pos_x;
        io.MousePos.y = e.touches[0].pos_y;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_TOUCHES_CANCELLED:
        m_mouseDown[0] = m_mouseUp[0] = false;
        return io.WantCaptureMouse;
    case SAPP_EVENTTYPE_KEY_DOWN:
        io.KeysDown[e.key_code] = true;
        return io.WantCaptureKeyboard;
    case SAPP_EVENTTYPE_KEY_UP:
        io.KeysDown[e.key_code] = false;
        return io.WantCaptureKeyboard;
    case SAPP_EVENTTYPE_CHAR:
        io.AddInputCharacter(ImWchar(e.char_code));
        return io.WantTextInput;
    default:
        return false;
    }
}

void Module_DearImGui::update(uint32_t deltaTimeMs)
{
    auto& io = ImGui::GetIO();

    const int width = sapp_width();
    const int height = sapp_height();

    io.DisplaySize = ImVec2(float(width), float(height));
    io.DeltaTime = float(deltaTimeMs) / 1000;

    for (int i = 0; i < SAPP_MAX_MOUSEBUTTONS; i++)
    {
        if (m_mouseDown[i])
        {
            io.MouseDown[i] = true;
            m_mouseDown[i] = !m_mouseUp[i];
        }
        else
        {
            io.MouseDown[i] = false;
        }
        m_mouseUp[i] = false;
    }

    if (io.WantTextInput && !sapp_keyboard_shown())
    {
        sapp_show_keyboard(true);
    }

    if (!io.WantTextInput && sapp_keyboard_shown())
    {
        sapp_show_keyboard(false);
    }

    ImGui::NewFrame();

    io.MouseWheel = 0;
    io.MouseWheelH = 0;
}

void Module_DearImGui::render()
{
    ImGui::Render();

    auto data = ImGui::GetDrawData();
    assert(data);

    if (data->CmdListsCount == 0) return;

    VertexShaderUniforms uniforms;
    uniforms.screenSize = ImGui::GetIO().DisplaySize;

    sg_apply_pipeline(m_pipeline);
    sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &uniforms, sizeof(VertexShaderUniforms));

    for (int i = 0; i < data->CmdListsCount; ++i)
    {
        const auto list = data->CmdLists[i];

        // append vertices and indices to buffers, record start offsets in bindings
        const int voffset = sg_append_buffer(m_bindings.vertex_buffers[0], list->VtxBuffer.Data, list->VtxBuffer.size() * sizeof(ImDrawVert));
        const int ioffset = sg_append_buffer(m_bindings.index_buffer, list->IdxBuffer.Data, list->IdxBuffer.size() * sizeof(ImDrawIdx));

        /* don't render anything if the buffer is in overflow state (this is also
            checked internally in sokol_gfx, draw calls that attempt from
            overflowed buffers will be silently dropped)
        */
        if (sg_query_buffer_overflow(m_bindings.vertex_buffers[0]) ||
            sg_query_buffer_overflow(m_bindings.index_buffer))
        {
            continue;
        }

        m_bindings.vertex_buffer_offsets[0] = voffset;
        m_bindings.index_buffer_offset = ioffset;

        auto savedTextureId = m_fontsTexture.id;
        m_bindings.fs_images[0] = { savedTextureId };
        sg_apply_bindings(&m_bindings);

        int offset = 0;
        for (const auto& cmd : list->CmdBuffer)
        {
            if (cmd.UserCallback)
            {
                cmd.UserCallback(list, &cmd);
            }
            else
            {
                // draw
                const auto curTextureId = uint32_t(reinterpret_cast<uintptr_t>(cmd.TextureId));

                if (curTextureId != savedTextureId)
                {
                    m_bindings.fs_images[0] = { curTextureId };
                    sg_apply_bindings(&m_bindings);
                    savedTextureId = curTextureId;
                }

                //const Texture* t = reinterpret_cast<Texture*>(cmd.TextureId);
                //assert(t);
                //m_program.setParameter(m_textureParam, *t);
                sg_apply_scissor_rect(
                    int(cmd.ClipRect.x),
                    int(cmd.ClipRect.y),
                    int(cmd.ClipRect.z - cmd.ClipRect.x),
                    int(cmd.ClipRect.w - cmd.ClipRect.y),
                    true);
                sg_draw(offset, cmd.ElemCount, 1);
            }
            offset += cmd.ElemCount;
        }
    }
}
