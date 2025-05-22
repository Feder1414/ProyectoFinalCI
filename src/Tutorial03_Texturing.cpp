/*
 *  Copyright 2019-2024 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#include "Tutorial03_Texturing.hpp"
#include "MapHelper.hpp"
#include "GraphicsUtilities.h"
#include "TextureUtilities.h"
#include "ColorConversion.h"
#include "ShaderSourceFactoryUtils.hpp"
#include "imgui.h"
#include "../imGuIZMO.quat/imGuIZMO.h"
#include "GLTFLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


//
static const std::unordered_map<std::string, std::string> AttrToShaderName =
    {
        {"baseColorTexture", "g_Albedo"},
        {"heightTexture", "g_HeightMap"},
        {"normalTexture", "g_NormalMap"},
        // …
};

static bool LoadHeightMap(const std::string&                filePath,
                          Diligent::GLTF::Model::ImageData& Img)
{
    int w = 0, h = 0, n = 0;
    // Forzamos 1 canal: el valor de rojo (stbi_load devuelve unsigned char*)
    stbi_uc* data = stbi_load(filePath.c_str(), &w, &h, &n, 1);
    if (!data)
        return false;

    // Rellenamos la estructura:
    Img.Width         = w;
    Img.Height        = h;
    Img.NumComponents = 1; // solo R
    Img.ComponentSize = 1; // 1 byte por canal
    Img.pData         = data;
    Img.DataSize      = static_cast<size_t>(w) * h;

    // Formato de textura en Diligent (1 canal 8-bit)
    Img.TexFormat = Diligent::TEX_FORMAT_R8_UNORM;

    // Opcional: marca el formato de archivo según extensión
    if (filePath.size() >= 4)
    {
        auto ext = filePath.substr(filePath.size() - 4);
        if (_stricmp(ext.c_str(), ".png") == 0) Img.FileFormat = Diligent::IMAGE_FILE_FORMAT::IMAGE_FILE_FORMAT_PNG;
        else if (_stricmp(ext.c_str(), ".jpg") == 0 ||
                 _stricmp(ext.c_str(), "jpeg") == 0)
            Img.FileFormat = Diligent::IMAGE_FILE_FORMAT::IMAGE_FILE_FORMAT_JPEG;
    }
    return true;
}


namespace Diligent
{

SampleBase* CreateSample()
{
    return new Tutorial03_Texturing();
}

void Tutorial03_Texturing::CreatePipelineState()
{
    // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "Cube PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_BACK;

    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise      = True;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Pack matrices in row-major order
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    // Presentation engine always expects input in gamma space. Normally, pixel shader output is
    // converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
    // or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
    // has to do the conversion manually.
    ShaderMacro Macros[] = {{"CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma ? "1" : "0"}};
    ShaderCI.Macros      = {Macros, _countof(Macros)};

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    auto*                                          pFXRaw = &DiligentFXShaderSourceStreamFactory::GetInstance();
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pFXFactory{pFXRaw};

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pCompound = CreateCompoundShaderSourceFactory({pShaderSourceFactory, pFXFactory});




    ShaderCI.pShaderSourceStreamFactory = pCompound;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube VS";
        ShaderCI.FilePath        = "cube.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU

    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "Cube PS";
        ShaderCI.FilePath        = "cube.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - normal
        LayoutElement{1, 0, 3, VT_FLOAT32, False},
        // Attribute 2 - texture coordinates
        LayoutElement{2, 0, 2, VT_FLOAT32, False},

        LayoutElement{3, 0, 4, VT_FLOAT32, False}, //Attribute 0- tangent
        //LayoutElement{4, 0, 3, VT_FLOAT32, False}, //Attribute 1- bitangent

        //Attribute 2- normal
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Albedo", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_HeightMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "cbPOM", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "cbLightAttribs", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "cbLightAttribs", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "parallaxConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

        //{SHADER_TYPE_PIXEL, (m_ShadowSettings.iShadowMode==SHADOW_MODE_PCF ? "g_tex2DShadowMap" : "g_tex2DFilterableShadowMap"), SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}

    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };

    SamplerDesc ComparsionSampler;
    ComparsionSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
    ComparsionSampler.MinFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ComparsionSampler.MagFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ComparsionSampler.MipFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Albedo", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL, "g_HeightMap", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL, "g_ShadowMap", ComparsionSampler},
    };




    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Since we did not explicitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly through the pipeline state object.
    //m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    m_pPSO->CreateShaderResourceBinding(&m_SRB, true);

    BufferDesc CBDesc;
    CBDesc.Name           = "Constants objects";
    CBDesc.Size           = sizeof(ConstantsData);
    CBDesc.Usage          = USAGE_DYNAMIC;
    CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(CBDesc, nullptr, &m_BufferConstantsObjects);
    m_SRB->GetVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_BufferConstantsObjects);

   BufferDesc CBDesc2;
    CBDesc2.Name           = "Light Constants objects";
    CBDesc2.Size           = sizeof(LightConstants);
    CBDesc2.Usage          = USAGE_DYNAMIC;
    CBDesc2.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc2.CPUAccessFlags = CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(CBDesc2, nullptr, &m_BufferLightConstants);
    m_SRB->GetVariableByName(SHADER_TYPE_VERTEX, "LightConstants")->Set(m_BufferLightConstants);

    BufferDesc CBDesc3;
    CBDesc3.Name           = "Directional light constants";
    CBDesc3.Size           = sizeof(LightAttribs);
    CBDesc3.Usage          = USAGE_DYNAMIC;
    CBDesc3.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc3.CPUAccessFlags = CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(CBDesc3, nullptr, &m_LightAttribsCB);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "cbLightAttribs")->Set(m_LightAttribsCB);
    m_SRB->GetVariableByName(SHADER_TYPE_VERTEX, "cbLightAttribs")->Set(m_LightAttribsCB);

    BufferDesc CBDesc4;
    CBDesc4.Name           = "Parallax Constants";
    CBDesc4.Size           = sizeof(parallaxConstants);
    CBDesc4.Usage          = USAGE_DYNAMIC;
    CBDesc4.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc4.CPUAccessFlags = CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(CBDesc4, nullptr, &m_ParallaxAttribsCB);
    m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "parallaxConstants")->Set(m_ParallaxAttribsCB);





}

void Tutorial03_Texturing::CreateVertexBuffer()
{
   //
    auto i = 0;
}

void Tutorial03_Texturing::CreateIndexBuffer()
{
    auto pepe = 0;
}

void Tutorial03_Texturing::LoadTexture()
{
    //TextureLoadInfo loadInfo;
    //loadInfo.IsSRGB = true;
    //RefCntAutoPtr<ITexture> Tex;
    //CreateTextureFromFile("DGLogo.png", loadInfo, m_pDevice, &Tex);
    //// Get shader resource view from the texture
    //m_TextureSRV = Tex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

    //// Set texture SRV in the SRB
    //m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(m_TextureSRV);
}

void Tutorial03_Texturing::InitializeScene()
{
    // ATRIBUTOS DE LA LUZ -----------------------------------------------

    m_LightAttribs.ShadowAttribs.iNumCascades     = 4;
    m_LightAttribs.ShadowAttribs.fFixedDepthBias  = 0.0025f;
    m_LightAttribs.ShadowAttribs.iFixedFilterSize = 5;
    m_LightAttribs.ShadowAttribs.fFilterWorldSize = 0.1f;

    m_LightAttribs.f4Direction    = float3(-0.522699475f, -0.481321275f, -0.703671455f);
    m_LightAttribs.f4Intensity    = float4(1, 1.0f, 1.0f, 1);
    m_LightAttribs.f4AmbientLight = float4(0.125f, 0.125f, 0.125f, 1);


    // CAMARA GENERAL Y LUZ -------------------------------------------------------------
    m_Camera.SetPos(float3(7.f, -0.5f, -16.5f));
    //m_Camera.SetPos(float3(0.f, 0.f, 00.0f));
    m_Camera.SetLookAt(float3(0.0f, 5.0f, 0.0f));

    m_Camera.SetRotation(0.48f, -0.145f);
    m_Camera.SetRotationSpeed(0.005f);
    m_Camera.SetMoveSpeed(5.f);
    m_Camera.SetSpeedUpScales(5.f, 10.f);
    m_Camera.Update(m_InputController, static_cast<float>(0.016));

    m_LightCamera.SetPos(float3(7.f, -0.5f, -16.5f));
    m_LightCamera.SetRotation(0.48f, -0.145f);
    m_LightCamera.SetRotationSpeed(0.005f);
    m_LightCamera.SetMoveSpeed(5.f);
    m_LightCamera.SetSpeedUpScales(5.f, 10.f);

    m_LightCamera.Update(m_InputController, static_cast<float>(0.016));

    m_ParallaxAttribs.parallaxMode       = 2;
    m_ParallaxAttribs.generalHeightScale = 0.00f;


    //m_LightCamera.SetLookAt(float3(0.0f, 0.0f, 0.0f));

    // GEOMETRIA MIA FIGURE BASE -------------------------------------------------------------

    m_Cubo = std::make_unique<Cubo>(m_pDevice, m_pPSO, 0);
    m_Cubo->SetPosition(float3{0.0f, 2.0f, 0.0f});
    m_Piso = std::make_unique<Cubo>(m_pDevice, m_pPSO, 1);
    m_Piso->SetPosition(float3{0.0f, -1.0f, 0.0f});
    m_Piso->SetScale(float3{10.0f, 10.0f, 10.0f});




    OutputDebugStringA("Cubo creado\n");

    //OutputDebugStringA("POMMaterial creado\n");
    /*m_Brick->Upload(m_pImmediateContext);
    m_Brick->Bind(m_SRB);*/
    m_Piso->setMaterial(m_RockPath.get());
    m_Cubo->setMaterial(m_Brick2.get());
    

    m_Objetos3D.push_back(std::move(m_Cubo));
    m_Objetos3D.push_back(std::move(m_Piso));


    // MODELOS GTLF-----------------------------------------------------

    GLTF::ModelCreateInfo CI;
    CI.FileName = "torture_chair/scene.gltf";

    //m_Model          = std::make_unique<GLTF::Model>(m_pDevice, ModelCI);

 

    static constexpr GLTF::VertexAttributeDesc MyVertexAttrs[] =
        {
            // Nombre        , BufferId, Tipo, Componentes
            {GLTF::PositionAttributeName, 0, VT_FLOAT32, 3},
            {GLTF::NormalAttributeName, 0, VT_FLOAT32, 3},
            {GLTF::Texcoord0AttributeName, 0, VT_FLOAT32, 2},
            {GLTF::TangentAttributeName, 0, VT_FLOAT32, 4} // ojo: tangente es float4 (xyz + w)
        };

    static constexpr GLTF::TextureAttributeDesc MyTexAttrs[] =
        {
            {GLTF::BaseColorTextureName, GLTF::DefaultBaseColorTextureAttribId},
            {GLTF::NormalTextureName, GLTF::DefaultNormalTextureAttribId},
            // …otros que quieras…
            {"heightTexture", /*elige un índice único, p.e.*/ 5}};
        
        CI.VertexAttributes    = MyVertexAttrs;
        CI.NumVertexAttributes = _countof(MyVertexAttrs);
        
        CI.TextureAttributes	= MyTexAttrs;
        CI.NumTextureAttributes = _countof(MyTexAttrs);

        m_Model = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI);

        m_Model->PrepareGPUResources(m_pDevice, m_pImmediateContext);


        int normSlot = m_Model->GetTextureAttributeIndex(GLTF::NormalTextureName);
        for (auto& mat : m_Model->Materials)
        {
            bool active = mat.IsTextureAttribActive(normSlot);
            OutputDebugStringA(("Normal slot: " + std::to_string(normSlot) + " isActive: " + (active ? "true\n" : "false\n")).c_str());
        }





        //GLTF::Model::ImageData img {};

        //if (LoadHeightMap("MyHeight.png", img))
        //{
        //    // Devuelve un índice en Model.Textures[]
        //    int heightTexId = m_Model->AddTexture(m_pDevice,
        //                                         /*pTextureCache*/ nullptr,
        //                                         /*pResourceMgr*/ nullptr,
        //                                         img,
        //                                         /*GltfSamplerId*/ 0,
        //                                         "HeightMap");

        //    // Asigna ese texture ID al slot 5 de cada material
        //    int slot = m_Model->GetTextureAttributeIndex("heightTexture");
        //    for (auto& M : m_Model->Materials)
        //        M.SetTextureId(slot, heightTexId);

        //    // Ya está en GPU; libera la copia CPU
        //    stbi_image_free((void*)img.pData);
        //}
        //else
        //{
        //    OutputDebugStringA("Error: no pudimos cargar MyHeight.png\n");
        //    // Error: no pudimos cargar MyHeight.png
        //}
        

        // Barril

       GLTF::ModelCreateInfo CI2;

       CI2.FileName = "wine_barrel/scene.gltf";
       CI2.VertexAttributes = MyVertexAttrs;
       CI2.NumVertexAttributes = _countof(MyVertexAttrs);
       CI2.TextureAttributes = MyTexAttrs;
       CI2.NumTextureAttributes = _countof(MyTexAttrs);
       m_Barril = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI2);


       // Rusty cage

       GLTF::ModelCreateInfo CI3;

       CI3.FileName = "rusty_cage/scene.gltf";
       CI3.VertexAttributes = MyVertexAttrs;
       CI3.NumVertexAttributes = _countof(MyVertexAttrs);
       CI3.TextureAttributes = MyTexAttrs;
       CI3.NumTextureAttributes = _countof(MyTexAttrs);
       m_Cage = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI3);

       // Skull
       GLTF::ModelCreateInfo CI4;

       CI4.FileName = "skull/scene.gltf";
       CI4.VertexAttributes = MyVertexAttrs;
       CI4.NumVertexAttributes = _countof(MyVertexAttrs);
       CI4.TextureAttributes = MyTexAttrs;
       CI4.NumTextureAttributes = _countof(MyTexAttrs);

       m_Skull = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI4);

       



       // Cofre

       GLTF::ModelCreateInfo CI5;
       CI5.FileName = "treasure_chest/scene.gltf";
       CI5.VertexAttributes = MyVertexAttrs;
       CI5.NumVertexAttributes = _countof(MyVertexAttrs);
       CI5.TextureAttributes = MyTexAttrs;
       CI5.NumTextureAttributes = _countof(MyTexAttrs);
       m_Chest = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI5);


       //caldero

       GLTF::ModelCreateInfo CI6;
       CI6.FileName = "cauldron/scene.gltf";
       CI6.VertexAttributes = MyVertexAttrs;
       CI6.NumVertexAttributes = _countof(MyVertexAttrs);
       CI6.TextureAttributes = MyTexAttrs;
       CI6.NumTextureAttributes = _countof(MyTexAttrs);
       m_Cauldron = std::make_unique<GLTF::Model>(m_pDevice, m_pImmediateContext, CI6);
       //m_Cauldron->PrepareGPUResources(m_pDevice, m_pImmediateContext);






};
     
        
    


void Tutorial03_Texturing::LoadMaterials() {
    
    m_Brick2 = std::make_unique<POMMaterial>(
        m_pDevice,
        "bricks2.jpg",
        "bricks2_dis.jpg",
        "bricks2_nor.jpg",
        0.057f); // HeightScale opcional

    m_Brick = std::make_unique<POMMaterial>(
        m_pDevice,
        "brick_wall.png",
        "brick_wall_dis.png",
        "brick_wall_nor.png",
        0.057f); // HeightScale opcional
    //m_Brick->CreateSRB(m_pPSO);

    m_Rock = std::make_unique<POMMaterial>(
        m_pDevice,
        "rock.jpg",
        "rock_dis.jpg",
        "rock_nor.jpg",
        0.057f); // HeightScale opcional
    //m_Rock->CreateSRB(m_pPSO);

    m_RockPath = std::make_unique<POMMaterial>(
        m_pDevice,
        "gray_rocks.png",
        "gray_rocks_dis.png",
        "gray_rocks_nor.png",
        0.057f); // HeightScale opcional

    m_Rocks2 = std::make_unique<POMMaterial>(
        m_pDevice,
        "rocksLow.jpg",
        "rocksLow_dis.png",
        "rocksLow_nor.png",
        0.057f); // HeightScale opcional

      //m_Rocks2->CreateSRB(m_pPSO);

    //m_LeatherPadded = std::make_unique<POMMaterial>(
    // m_pDevice,
    // "SOI_LeatherPadded/LeatherPadded_01_BC.png",
    // "SOI_LeatherPadded/LeatherPadded_01_H.png",
    // "SOI_LeatherPadded/LeatherPadded_01_N.png",
    // 0.057f); // HeightScale opcional


}
void Tutorial03_Texturing::Initialize(const SampleInitInfo& InitInfo)
{
    

    SampleBase::Initialize(InitInfo);

    CreatePipelineState();
    CreatePipelineStateGLTF();
    CreateVertexBuffer();
    CreateIndexBuffer();
    createShadowMapManager();
    LoadTexture();
    LoadMaterials();
    InitializeScene();
    
    m_DungeonGenerator.Generate(60, 40, 10, 20, 20);
    m_DungeonGenerator.SavePPM("Dungeon.ppm");



    m_DungeonScene = DungeonScene(m_pDevice, m_pPSO, m_RockPath.get(), m_RockPath.get());
    m_DungeonScene.Build(m_DungeonGenerator);

    OutputDebugStringA("POMMaterial binded\n");

    m_ShadowMap = std::make_unique<ShadowMap>();

    auto actualHeight = m_pSwapChain->GetDesc().Height;
    auto actualWidth  = m_pSwapChain->GetDesc().Width;

    m_ShadowMap->Initialize(m_pDevice, actualWidth, actualHeight);

}

// Render a frame
void Tutorial03_Texturing::Render()
{

    ////Shadow map
    //m_pImmediateContext->SetRenderTargets(0, nullptr, m_ShadowMap->GetDSV(), RESOURCE_STATE_TRANSITION_MODE_NONE);
    //m_pImmediateContext->ClearDepthStencil(m_ShadowMap->GetDSV(), CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    m_pImmediateContext->SetPipelineState(m_ShadowMap->GetShadowPSO());
    OutputDebugStringA("PSO sombras seteado\n");


    RenderShadowPass();



    auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
    // Clear the back buffer
    float4 ClearColor = {0.350f, 0.350f, 0.350f, 1.0f};
    if (m_ConvertPSOutputToGamma)
    {
        // If manual gamma correction is required, we need to clear the render target with sRGB color
        ClearColor = LinearToSRGB(ClearColor);
    }
    m_pImmediateContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_NONE);
    m_pImmediateContext->ClearRenderTarget(pRTV, ClearColor.Data(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

 

    // Set the pipeline state
    m_pImmediateContext->SetPipelineState(m_pPSO);
    // Commit shader resources. RESOURCE_STATE_TRANSITION_MODE_TRANSITION mode
    // makes sure that resources are transitioned to required states.

    RenderizarObjetos();
    //utputDebugStringA("Renderizar objetos\n");

    //m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    //m_pSwapChain->Present();

}

void Tutorial03_Texturing::RenderizarObjetos()
{
    //// Renderizar el cubo
    //   IBuffer* pVB[] = {m_Cubo->GetVertexBuffer()};
    //   Uint64   offset[] = {0};


    //   auto viewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();
    //   PrintFloat4x4(viewProj, "viewProj");


    //   m_pImmediateContext->SetVertexBuffers(0, 1, pVB , 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    //   m_pImmediateContext->SetIndexBuffer(m_Cubo->GetIndexBuffer(), 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);


    //   {
    //       ConstantsData cbData = {};
    //       cbData.g_World       = m_Cubo->GetWorldTransform();

    //       cbData.g_ViewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();

    //       cbData.g_CameraPos = m_Camera.GetPos();





    //       MapHelper<ConstantsData> CBHelper(m_pImmediateContext, m_BufferConstantsObjects, MAP_WRITE, MAP_FLAG_DISCARD);
    //       *CBHelper = cbData;
    //   }
    //
    //
    //   //m_Brick->Upload(m_pImmediateContext);
    //   m_Cubo->commitMaterial(m_SRB, m_pImmediateContext);
    //
    //   m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);


    //   //cbData.g_LightViewProj = lightViewProj;



    //   // Realiza el draw call
    //   DrawIndexedAttribs drawAttrs;
    //   drawAttrs.IndexType  = VT_UINT32;
    //   drawAttrs.NumIndices = m_Cubo->GetNumIndices();
    //   drawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    //   m_pImmediateContext->DrawIndexed(drawAttrs);
    //   OutputDebugStringA("Dibujando cubo\n");

    // Renderizar el piso
    /*RenderizarObjeto(m_Piso.get(), false);
    OutputDebugStringA("Dibujando piso\n");

    RenderizarObjeto(m_Cubo.get(), false);
    OutputDebugStringA("Dibujando cubo\n");*/

    //RenderizarDungeon();


    for (const auto& objeto : m_Objetos3D) {
        RenderizarObjeto(objeto.get(), false);
    }
    auto cascade = float4x4::Translation(float3(0.0f, 0.0f, 0.0f));

    RenderizarObjeto(m_Model.get(), false, cascade, float4x4::Translation(float3(15.0f,0.0f,0.0f)));
    //RenderizarObjeto(m_Barril.get(), false, cascade, float4x4::Translation(float3(10.0f, 3.0f, 0.0f)) * float4x4::Scale(float3(0.01f, 0.01f, 0.01f)));
    RenderizarObjeto(m_Cage.get(), false, cascade, float4x4::Translation(float3(17.50f, 0.0f, 0.0f)) );

    RenderizarObjeto(m_Skull.get(), false, cascade, float4x4::Translation(float3(20.0f, 0.0f, 0.0f)));
    RenderizarObjeto(m_Chest.get(), false, cascade, float4x4::Translation(float3(22.50f, 0.0f, 0.0f)));
    RenderizarObjeto(m_Cauldron.get(), false, cascade, float4x4::Translation(float3(25.0f, 0.0f, 0.0f)));




    
}

 void Tutorial03_Texturing::RenderizarDungeon()
{
    auto cuboDungeon = m_DungeonScene.GetCubeMesh();
    IBuffer* pvB[] = {cuboDungeon->GetVertexBuffer()};
    Uint64   offset[] = {0};

    auto viewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();

    m_pImmediateContext->SetVertexBuffers(0, 1, pvB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    m_pImmediateContext->SetIndexBuffer(cuboDungeon->GetIndexBuffer(), 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->SetPipelineState(m_pPSO);

    
     for (auto& tile : m_DungeonScene.GetInstances()){
         
         {
             ConstantsData cbData{};
             cbData.g_World = tile.World;

             cbData.g_ViewProj  = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();
             cbData.g_CameraPos = m_Camera.GetPos();
             MapHelper<ConstantsData> CBHelper(m_pImmediateContext, m_BufferConstantsObjects, MAP_WRITE, MAP_FLAG_DISCARD);

             *CBHelper = cbData;
         }

          // Constantes de intento de luz spot
         {
             LightConstants cbDataL = {};

             cbDataL.g_LightPos      = m_LightCamera.GetPos();
             cbDataL.g_ViewProjLight = m_LightCamera.GetViewMatrix() * m_LightCamera.GetProjMatrix();


             MapHelper<LightConstants> CBHelperL(m_pImmediateContext, m_BufferLightConstants, MAP_WRITE, MAP_FLAG_DISCARD);
             *CBHelperL = cbDataL;
         }


          // Luz direccional
         {
             MapHelper<LightAttribs> CBHelperLightAttribs(m_pImmediateContext, m_LightAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
             *CBHelperLightAttribs = m_LightAttribs;
         }


         {
             MapHelper<parallaxConstants> CBHelperParallax(m_pImmediateContext, m_ParallaxAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
             *CBHelperParallax = m_ParallaxAttribs;
         }


         if (tile.MaterialId == 0) {
            m_RockPath->Upload(m_pImmediateContext);
            m_RockPath->Bind(m_SRB);
         
         }

         else if (tile.MaterialId ==1) {
            m_Brick2->Upload(m_pImmediateContext);
			m_Brick2->Bind(m_SRB);
		 }

         m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMapMgr.GetSRV(), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
         m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

         DrawIndexedAttribs drawAttrs;
         drawAttrs.IndexType  = VT_UINT32;
         drawAttrs.NumIndices = cuboDungeon->GetNumIndices();
         drawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
         m_pImmediateContext->DrawIndexed(drawAttrs);




	
         
         


        
     }



}


void Tutorial03_Texturing::RenderShadowPass()
{

    //IBuffer* pVB[]    = {m_Cubo->GetVertexBuffer()};
    //Uint64   offset[] = {0};

    //auto viewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();

    //PrintFloat4x4(viewProj, "viewProj");

    //m_pImmediateContext->SetVertexBuffers(0, 1, pVB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    //m_pImmediateContext->SetIndexBuffer(m_Cubo->GetIndexBuffer(), 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    //{
    //    ShadowConstantsData cbData = {};
    //    cbData.g_LightViewProj     = m_LightCamera.GetViewMatrix() * m_LightCamera.GetProjMatrix();
    //    cbData.g_World = m_Cubo->GetWorldTransform();

  
    //    PrintFloat4x4 (m_LightCamera.GetViewMatrix(), "LightView");
    //    PrintFloat4x4 (m_LightCamera.GetProjMatrix(), "LightProj");


    //    MapHelper<ShadowConstantsData> CBHelper(m_pImmediateContext, m_ShadowMap->GetCB(), MAP_WRITE, MAP_FLAG_DISCARD);
    //    *CBHelper = cbData;

    //    
    //}
  
    //m_pImmediateContext->CommitShaderResources(m_ShadowMap->GetSRB(), RESOURCE_STATE_TRANSITION_MODE_NONE);
    ////cbData.g_LightViewProj = lightViewProj;

    //// Realiza el draw call
    //DrawIndexedAttribs drawAttrs;
    //drawAttrs.IndexType  = VT_UINT32;
    //drawAttrs.NumIndices = m_Cubo->GetNumIndices();
    //drawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    //m_pImmediateContext->DrawIndexed(drawAttrs);
    //OutputDebugStringA("Dibujando cubo sombras\n");
    /*RenderizarObjeto(m_Piso.get(), true);
    RenderizarObjeto(m_Cubo.get(), true);*/


    auto iNumShadowCascades = m_LightAttribs.ShadowAttribs.iNumCascades;
    for (int iCascade = 0; iCascade < iNumShadowCascades; ++iCascade)
    {
        const auto CascadeProjMatr = m_ShadowMapMgr.GetCascadeTransform(iCascade).Proj;

        const auto& WorldToLightViewSpaceMatr = m_PackMatrixRowMajor ?
            m_LightAttribs.ShadowAttribs.mWorldToLightView :
            m_LightAttribs.ShadowAttribs.mWorldToLightView.Transpose();

        const auto WorldToLightProjSpaceMatr = WorldToLightViewSpaceMatr * CascadeProjMatr;

        CameraAttribs ShadowCameraAttribs = {};

        ShadowCameraAttribs.mView = m_LightAttribs.ShadowAttribs.mWorldToLightView;
       /* WriteShaderMatrix(&ShadowCameraAttribs.mProj, CascadeProjMatr, !m_PackMatrixRowMajor);
        WriteShaderMatrix(&ShadowCameraAttribs.mViewProj, WorldToLightProjSpaceMatr, !m_PackMatrixRowMajor);*/

        ShadowCameraAttribs.f4ViewportSize.x = static_cast<float>(m_ShadowSettings.Resolution);
        ShadowCameraAttribs.f4ViewportSize.y = static_cast<float>(m_ShadowSettings.Resolution);
        ShadowCameraAttribs.f4ViewportSize.z = 1.f / ShadowCameraAttribs.f4ViewportSize.x;
        ShadowCameraAttribs.f4ViewportSize.w = 1.f / ShadowCameraAttribs.f4ViewportSize.y;

        /*{
            MapHelper<CameraAttribs> CameraData(m_pImmediateContext, m_CameraAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
            *CameraData = ShadowCameraAttribs;
        }*/

        auto* pCascadeDSV = m_ShadowMapMgr.GetCascadeDSV(iCascade);
        m_pImmediateContext->SetRenderTargets(0, nullptr, pCascadeDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->ClearDepthStencil(pCascadeDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        //ViewFrustumExt Frutstum;
        //ExtractViewFrustumPlanesFromMatrix(WorldToLightProjSpaceMatr, Frutstum, m_pDevice->GetDeviceInfo().IsGLDevice());

        ////if (iCascade == 0)
        //DrawMesh(m_pImmediateContext, true, Frutstum);

        /*RenderizarObjeto(m_Piso.get(), true, WorldToLightProjSpaceMatr);
        RenderizarObjeto(m_Cubo.get(), true, WorldToLightProjSpaceMatr);*/

        for (const auto& objeto : m_Objetos3D) {
			RenderizarObjeto(objeto.get(), true, WorldToLightProjSpaceMatr);
		}

    }

    if (m_ShadowSettings.iShadowMode > SHADOW_MODE_PCF)
        m_ShadowMapMgr.ConvertToFilterable(m_pImmediateContext, m_LightAttribs.ShadowAttribs);



}


void Tutorial03_Texturing::RenderizarObjeto(Objeto3D* objeto, bool isShadowPass, float4x4 cascadeProj) {

    IBuffer* pVB[]    = {objeto->GetVertexBuffer()};
    Uint64   offset[] = {0};

    auto viewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();
    PrintFloat4x4(viewProj, "viewProj");

    m_pImmediateContext->SetVertexBuffers(0, 1, pVB, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    m_pImmediateContext->SetIndexBuffer(objeto->GetIndexBuffer(), 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    if (!isShadowPass)
    {
        m_pImmediateContext->SetPipelineState(m_pPSO);
        {
            ConstantsData cbData = {};
            cbData.g_World       = objeto->GetWorldTransform();

            cbData.g_ViewProj = m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix();

            cbData.g_CameraPos = m_Camera.GetPos();

            MapHelper<ConstantsData> CBHelper(m_pImmediateContext, m_BufferConstantsObjects, MAP_WRITE, MAP_FLAG_DISCARD);
            *CBHelper = cbData;
        }

        // Constantes de intento de luz spot
        {
            LightConstants cbDataL = {};

            cbDataL.g_LightPos      = m_LightCamera.GetPos();
            cbDataL.g_ViewProjLight = m_LightCamera.GetViewMatrix() * m_LightCamera.GetProjMatrix();


            MapHelper<LightConstants> CBHelperL(m_pImmediateContext, m_BufferLightConstants, MAP_WRITE, MAP_FLAG_DISCARD);
            *CBHelperL = cbDataL;
        }


        // Luz direccional
        {
            MapHelper<LightAttribs> CBHelperLightAttribs(m_pImmediateContext, m_LightAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
			*CBHelperLightAttribs = m_LightAttribs;

        }


        {
            MapHelper<parallaxConstants> CBHelperParallax(m_pImmediateContext, m_ParallaxAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
            *CBHelperParallax = m_ParallaxAttribs;
        
        
        }



        objeto->commitMaterial(m_SRB, m_pImmediateContext);

       // auto* matSRB =  objeto->getMaterial()->GetSRB();

        //objeto->getMaterial()->Upload(m_pImmediateContext);


        //m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMap->GetSRV(), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        m_SRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMapMgr.GetSRV(), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        //m_pImmediateContext->CommitShaderResources(matSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        m_pImmediateContext->CommitShaderResources(m_SRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        
        
    }

    else{
        
        {
            ShadowConstantsData cbData = {};
            //cbData.g_LightViewProj     = m_LightCamera.GetViewMatrix() * m_LightCamera.GetProjMatrix();
            cbData.g_LightViewProj	 = cascadeProj;
            cbData.g_World             = objeto->GetWorldTransform();


            PrintFloat4x4(m_LightCamera.GetViewMatrix(), "LightView");
            PrintFloat4x4(m_LightCamera.GetProjMatrix(), "LightProj");


            MapHelper<ShadowConstantsData> CBHelper(m_pImmediateContext, m_ShadowMap->GetCB(), MAP_WRITE, MAP_FLAG_DISCARD);
            *CBHelper = cbData;
        }

        m_pImmediateContext->CommitShaderResources(m_ShadowMap->GetSRB(), RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    
    }


    //cbData.g_LightViewProj = lightViewProj;

    // Realiza el draw call
    DrawIndexedAttribs drawAttrs;
    drawAttrs.IndexType  = VT_UINT32;
    drawAttrs.NumIndices = objeto->GetNumIndices();
    drawAttrs.Flags      = DRAW_FLAG_VERIFY_ALL;
    m_pImmediateContext->DrawIndexed(drawAttrs);
    //OutputDebugStringA("Dibujando cubo sombras\n");
    if (!isShadowPass)
	{
		OutputDebugStringA("Dibujando objeto\n");
	}
	else
	{
		OutputDebugStringA("Dibujando objeto sombras\n");
	}

}


void Tutorial03_Texturing::RenderizarObjeto(GLTF::Model* modelo, bool isShadowPass, float4x4 cascadeProj, float4x4 worldMatrix)
{
    // 1) Calcula las transformaciones de la escena por defecto
    
    GLTF::ModelTransforms transforms;
    modelo->ComputeTransforms(modelo->DefaultSceneId, transforms);

    // 2) Recorre cada nodo del modelo
    for (size_t nodeIdx = 0; nodeIdx < modelo->Nodes.size(); ++nodeIdx)
    {
        const auto& node = modelo->Nodes[nodeIdx];
        if (node.pMesh == nullptr)
            continue; // este nodo no tiene geometría

        // 3) Obtén su matrix mundo
        const float4x4& world = transforms.NodeGlobalMatrices[nodeIdx] * worldMatrix;

        // 4) Por cada primitiva dentro de ese mesh
        for (const auto& prim : node.pMesh->Primitives)
        {
            // 5) Bind VB/IB
            Uint64   offsets[] = {0};
            IBuffer* pVB[]     = {modelo->GetVertexBuffer(0)};
            m_pImmediateContext->SetVertexBuffers(
                0, 1, pVB, offsets,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                SET_VERTEX_BUFFERS_FLAG_RESET);
            m_pImmediateContext->SetIndexBuffer(
                modelo->GetIndexBuffer(), 0,
                RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            if (!isShadowPass)
            {
                m_pImmediateContext->SetPipelineState(m_pPSOGLTF);
                // 6a) Actualiza constantes de cámara+world
                ConstantsData cb{world,
                                 m_Camera.GetViewMatrix() * m_Camera.GetProjMatrix(),
                                 m_Camera.GetPos()};
                {
                    MapHelper<ConstantsData> helper(
                        m_pImmediateContext,
                        m_BufferConstantsObjects,
                        MAP_WRITE, MAP_FLAG_DISCARD);
                    *helper = cb;
                }

                materialConstants matCb{};

                matCb.materialId = prim.MaterialId;

				{
					MapHelper<materialConstants> helper(
						m_pImmediateContext,
						m_MaterialAttribsCB,
						MAP_WRITE, MAP_FLAG_DISCARD);
					*helper = matCb;
				}


                 // Constantes de intento de luz spot
                {
                    LightConstants cbDataL = {};

                    cbDataL.g_LightPos      = m_LightCamera.GetPos();
                    cbDataL.g_ViewProjLight = m_LightCamera.GetViewMatrix() * m_LightCamera.GetProjMatrix();


                    MapHelper<LightConstants> CBHelperL(m_pImmediateContext, m_BufferLightConstants, MAP_WRITE, MAP_FLAG_DISCARD);
                    *CBHelperL = cbDataL;
                }


                // Luz direccional
                {
                    MapHelper<LightAttribs> CBHelperLightAttribs(m_pImmediateContext, m_LightAttribsCB, MAP_WRITE, MAP_FLAG_DISCARD);
                    *CBHelperLightAttribs = m_LightAttribs;
		        }

                 m_SRBGLTF->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMapMgr.GetSRV(), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);


                // 6b) Bindea las texturas del material de la primitiva
                auto& mat = modelo->Materials[prim.MaterialId];
                // Recorre sólo los atributos de textura activos
                for (Uint32 i = 0; i < modelo->GetNumTextureAttributes(); ++i)
                {
                    const auto&  texAttr = modelo->GetTextureAttribute(i);
                    const Uint32 slot    = texAttr.Index; // <-- aquí, usa el Id, no "i"
                    if (!mat.IsTextureAttribActive(slot))
                        continue;

                    int   texId   = mat.GetTextureId(slot);
                    auto* pTex    = modelo->GetTexture(texId);
                    auto* pTexSRV = pTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);

                    auto correctName = AttrToShaderName.at(texAttr.Name);
                    OutputDebugStringA((correctName + " Original Name: " + texAttr.Name + "\n").c_str());

                    m_SRBGLTF->GetVariableByName(
                                 SHADER_TYPE_PIXEL, correctName.c_str())
                        ->Set(pTexSRV, SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
                }


                // 6c) Commit y draw
                m_pImmediateContext->CommitShaderResources(
                    m_SRBGLTF, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }
            else
            {
                // Pase de sombra: sólo world + cascadeProj
                ShadowConstantsData scb{world, cascadeProj};
                {
                    MapHelper<ShadowConstantsData> helper(
                        m_pImmediateContext,
                        m_ShadowMap->GetCB(),
                        MAP_WRITE, MAP_FLAG_DISCARD);
                    *helper = scb;
                }
                m_pImmediateContext->CommitShaderResources(
                    m_ShadowMap->GetSRB(),
                    RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            }

            // 7) DrawIndexed
            DrawIndexedAttribs drawAttrs;
            drawAttrs.IndexType          = VT_UINT32;
            drawAttrs.NumIndices         = prim.IndexCount;
            drawAttrs.FirstIndexLocation = prim.FirstIndex;
            drawAttrs.BaseVertex         = modelo->GetBaseVertex();
            drawAttrs.Flags              = DRAW_FLAG_VERIFY_ALL;
            m_pImmediateContext->DrawIndexed(drawAttrs);
        }
    }
}



void Tutorial03_Texturing::WindowResize(Uint32 Width, Uint32 Height)
{
    if (Width == 0 || Height == 0)
        return;

    // Update projection matrix.
    float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    m_Camera.SetProjAttribs(0.1f, 100.f, AspectRatio, PI_F / 4.f,
                            m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceInfo().NDC.MinZ == -1);

    m_LightCamera.SetProjAttribs(16.0f, 50.f, AspectRatio, PI_F / 4.f,
							m_pSwapChain->GetDesc().PreTransform, m_pDevice->GetDeviceInfo().NDC.MinZ == -1);

    //// Check if the image needs to be recreated.
    //if (m_pColorRT != nullptr &&
    //    m_pColorRT->GetDesc().Width == Width &&
    //    m_pColorRT->GetDesc().Height == Height)
    //    return;

    //m_pColorRT = nullptr;

    //// Create window-size color image.
    //TextureDesc RTDesc       = {};
    //RTDesc.Name              = "Color buffer";
    //RTDesc.Type              = RESOURCE_DIM_TEX_2D;
    //RTDesc.Width             = Width;
    //RTDesc.Height            = Height;
    //RTDesc.BindFlags         = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
    //RTDesc.ClearValue.Format = m_ColorBufferFormat;
    //RTDesc.Format            = m_ColorBufferFormat;

    //m_pDevice->CreateTasdfasdfasexture(RTDesc, nullptr, &m_pColorRT);
}


void Tutorial03_Texturing::Update(double CurrTime, double ElapsedTime)
{


  
    SampleBase::Update(CurrTime, ElapsedTime);
    UpdateUI();

    m_Camera.Update(m_InputController, static_cast<float>(ElapsedTime));

    ShadowMapManager::DistributeCascadeInfo DistrInfo;
    DistrInfo.pCameraView   = &m_Camera.GetViewMatrix();
    DistrInfo.pCameraProj   = &m_Camera.GetProjMatrix();
    float3 f3LightDirection = float3(m_LightAttribs.f4Direction.x, m_LightAttribs.f4Direction.y, m_LightAttribs.f4Direction.z);
    DistrInfo.pLightDir     = &f3LightDirection;

    DistrInfo.fPartitioningFactor = m_ShadowSettings.PartitioningFactor;
    DistrInfo.SnapCascades        = m_ShadowSettings.SnapCascades;
    DistrInfo.EqualizeExtents     = m_ShadowSettings.EqualizeExtents;
    DistrInfo.StabilizeExtents    = m_ShadowSettings.StabilizeExtents;
    DistrInfo.PackMatrixRowMajor  = m_PackMatrixRowMajor;

    m_ShadowMapMgr.DistributeCascades(DistrInfo, m_LightAttribs.ShadowAttribs);


   


    // Apply rotation
    //float4x4 CubeModelTransform = float4x4::RotationY(static_cast<float>(CurrTime) * 1.0f) * float4x4::RotationX(-PI_F * 0.1f);

    //// Camera is at (0, 0, -5) looking along the Z axis
    //float4x4 View = float4x4::Translation(0.f, 0.0f, 5.0f);

    //// Get pretransform matrix that rotates the scene according the surface orientation
    //auto SrfPreTransform = GetSurfacePretransformMatrix(float3{0, 0, 1});

    //// Get projection matrix adjusted to the current screen orientation
    //auto Proj = GetAdjustedProjectionMatrix(PI_F / 4.0f, 0.1f, 100.f);

    //// Compute world-view-projection matrix
    //m_WorldViewProjMatrix = CubeModelTransform * View * SrfPreTransform * Proj;
}


void Tutorial03_Texturing::PrintFloat4x4(const Diligent::float4x4& mat, const std::string id)
{
    std::ostringstream oss;
    oss << "Matriz:" + id << std::endl;
    for (int row = 0; row < 4; ++row)
    {
        oss << "[ ";
        for (int col = 0; col < 4; ++col)
        {
            oss << mat[row][col] << " ";
        }
        oss << "]" << std::endl;
    }
    std::string output = oss.str();
    OutputDebugStringA(output.c_str());


};

void Tutorial03_Texturing::CreatePipelineStateGLTF() {

     // Pipeline state object encompasses configuration of all GPU stages

    GraphicsPipelineStateCreateInfo PSOCreateInfo;

    // Pipeline state name is used by the engine to report issues.
    // It is always a good idea to give objects descriptive names.
    PSOCreateInfo.PSODesc.Name = "GLTF PSO";

    // This is a graphics pipeline
    PSOCreateInfo.PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    // clang-format off
    // This tutorial will render to a single render target
    PSOCreateInfo.GraphicsPipeline.NumRenderTargets             = 1;
    // Set render target format which is the format of the swap chain's color buffer
    PSOCreateInfo.GraphicsPipeline.RTVFormats[0]                = m_pSwapChain->GetDesc().ColorBufferFormat;
    // Set depth buffer format which is the format of the swap chain's back buffer
    PSOCreateInfo.GraphicsPipeline.DSVFormat                    = m_pSwapChain->GetDesc().DepthBufferFormat;
    // Primitive topology defines what kind of primitives will be rendered by this pipeline state
    PSOCreateInfo.GraphicsPipeline.PrimitiveTopology            = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // Cull back faces
    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.CullMode      = CULL_MODE_NONE;

    PSOCreateInfo.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise      = True;
    // Enable depth testing
    PSOCreateInfo.GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    // clang-format on

    ShaderCreateInfo ShaderCI;
    // Tell the system that the shader source code is in HLSL.
    // For OpenGL, the engine will convert this into GLSL under the hood.
    ShaderCI.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

    // OpenGL backend requires emulated combined HLSL texture samplers (g_Texture + g_Texture_sampler combination)
    ShaderCI.Desc.UseCombinedTextureSamplers = true;

    // Pack matrices in row-major order
    ShaderCI.CompileFlags = SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;

    // Presentation engine always expects input in gamma space. Normally, pixel shader output is
    // converted from linear to gamma space by the GPU. However, some platforms (e.g. Android in GLES mode,
    // or Emscripten in WebGL mode) do not support gamma-correction. In this case the application
    // has to do the conversion manually.
    ShaderMacro Macros[] = {{"CONVERT_PS_OUTPUT_TO_GAMMA", m_ConvertPSOutputToGamma ? "1" : "0"}};
    ShaderCI.Macros      = {Macros, _countof(Macros)};

    // Create a shader source stream factory to load shaders from files.
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
    m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(nullptr, &pShaderSourceFactory);

    auto*                                          pFXRaw = &DiligentFXShaderSourceStreamFactory::GetInstance();
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pFXFactory{pFXRaw};

    RefCntAutoPtr<IShaderSourceInputStreamFactory> pCompound = CreateCompoundShaderSourceFactory({pShaderSourceFactory, pFXFactory});




    ShaderCI.pShaderSourceStreamFactory = pCompound;
    // Create a vertex shader
    RefCntAutoPtr<IShader> pVS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_VERTEX;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "GLTF VS";
        ShaderCI.FilePath        = "cube.vsh";
        m_pDevice->CreateShader(ShaderCI, &pVS);
        // Create dynamic uniform buffer that will store our transformation matrix
        // Dynamic buffers can be frequently updated by the CPU
    }

    // Create a pixel shader
    RefCntAutoPtr<IShader> pPS;
    {
        ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
        ShaderCI.EntryPoint      = "main";
        ShaderCI.Desc.Name       = "GLTF PS";
        ShaderCI.FilePath        = "gltf.psh";
        m_pDevice->CreateShader(ShaderCI, &pPS);
    }

    // clang-format off
    // Define vertex shader input layout
    LayoutElement LayoutElems[] =
    {
        // Attribute 0 - vertex position
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1 - normal
        LayoutElement{1, 0, 3, VT_FLOAT32, False},
        // Attribute 2 - texture coordinates
        LayoutElement{2, 0, 2, VT_FLOAT32, False},

        LayoutElement{3, 0, 4, VT_FLOAT32, False}, //Attribute 0- tangent
        //LayoutElement{4, 0, 3, VT_FLOAT32, False}, //Attribute 1- bitangent

        //Attribute 2- normal
    };
    // clang-format on

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    PSOCreateInfo.GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    PSOCreateInfo.GraphicsPipeline.InputLayout.NumElements    = _countof(LayoutElems);

    // Define variable type that will be used by default
    PSOCreateInfo.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

    // clang-format off
    // Shader variables should typically be mutable, which means they are expected
    // to change on a per-instance basis
    ShaderResourceVariableDesc Vars[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Albedo", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_HeightMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_ShadowMap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "Constants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "cbPOM", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "cbLightAttribs", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_VERTEX, "cbLightAttribs", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "parallaxConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "materialConstants", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},

        //{SHADER_TYPE_PIXEL, (m_ShadowSettings.iShadowMode==SHADOW_MODE_PCF ? "g_tex2DShadowMap" : "g_tex2DFilterableShadowMap"), SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE}

    };
    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.Variables    = Vars;
    PSOCreateInfo.PSODesc.ResourceLayout.NumVariables = _countof(Vars);

    // clang-format off
    // Define immutable sampler for g_Texture. Immutable samplers should be used whenever possible
    SamplerDesc SamLinearClampDesc
    {
        FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, FILTER_TYPE_LINEAR, 
        TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP, TEXTURE_ADDRESS_CLAMP
    };

    SamplerDesc ComparsionSampler;
    ComparsionSampler.ComparisonFunc = COMPARISON_FUNC_LESS;
    ComparsionSampler.MinFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ComparsionSampler.MagFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ComparsionSampler.MipFilter      = FILTER_TYPE_COMPARISON_LINEAR;
    ImmutableSamplerDesc ImtblSamplers[] = 
    {
        {SHADER_TYPE_PIXEL, "g_Albedo", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL, "g_HeightMap", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL, "g_NormalMap", SamLinearClampDesc},
        {SHADER_TYPE_PIXEL, "g_ShadowMap", ComparsionSampler},
    };




    // clang-format on
    PSOCreateInfo.PSODesc.ResourceLayout.ImmutableSamplers    = ImtblSamplers;
    PSOCreateInfo.PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSOGLTF);

    // Since we did not explicitly specify the type for 'Constants' variable, default
    // type (SHADER_RESOURCE_VARIABLE_TYPE_STATIC) will be used. Static variables
    // never change and are bound directly through the pipeline state object.
    //m_pPSO->GetStaticVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_VSConstants);

    // Since we are using mutable variable, we must create a shader resource binding object
    // http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/
    m_pPSOGLTF->CreateShaderResourceBinding(&m_SRBGLTF, true);

    BufferDesc CBDesc;
    CBDesc.Name           = "GLTF Constants objects";
    CBDesc.Size           = sizeof(materialConstants);
    CBDesc.Usage          = USAGE_DYNAMIC;
    CBDesc.BindFlags      = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    m_pDevice->CreateBuffer(CBDesc, nullptr, &m_MaterialAttribsCB);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_PIXEL, "materialConstants")->Set(m_MaterialAttribsCB);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_VERTEX, "Constants")->Set(m_BufferConstantsObjects);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_VERTEX, "LightConstants")->Set(m_BufferLightConstants);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_PIXEL, "cbLightAttribs")->Set(m_LightAttribsCB);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_VERTEX, "cbLightAttribs")->Set(m_LightAttribsCB);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_PIXEL, "cbPOM")->Set(m_ParallaxAttribsCB);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_PIXEL, "parallaxConstants")->Set(m_ParallaxAttribsCB);
    m_SRBGLTF->GetVariableByName(SHADER_TYPE_PIXEL, "g_ShadowMap")->Set(m_ShadowMapMgr.GetSRV());






}


void Tutorial03_Texturing::createShadowMapManager() {

    /*ShadowMapManager::InitInfo SMMgrInitInfo;
    SMMgrInitInfo.Format               = m_ShadowSettings.Format;
    SMMgrInitInfo.Resolution           = m_ShadowSettings.Resolution;
    SMMgrInitInfo.NumCascades          = 3;
    SMMgrInitInfo.ShadowMode           = m_ShadowSettings.iShadowMode;
    SMMgrInitInfo.Is32BitFilterableFmt = m_ShadowSettings.Is32BitFilterableFmt;*/

    // 2-A)  Ajusta lo que quieras
    m_ShadowSettings.Resolution           = 2048;
    m_ShadowSettings.iShadowMode          = SHADOW_MODE_PCF; // ó VSM, EVSM2…
    m_ShadowSettings.Format               = TEX_FORMAT_D16_UNORM;
    m_ShadowSettings.Is32BitFilterableFmt = false;

    m_LightAttribs.ShadowAttribs.iNumCascades     = 4;
    m_LightAttribs.ShadowAttribs.iFixedFilterSize = 3; // 3x3 PCF
    m_LightAttribs.ShadowAttribs.fFilterWorldSize = 0.1f;
    m_LightAttribs.f4Direction                    = float3(-0.5f, -1.0f, -0.3f); // ¡normalízala!

    //// 2-B)  CBs
    //CreateUniformBuffer(m_pDevice, sizeof(CameraAttribs),
    //                    "CameraAttribs", &m_CameraAttribsCB);
    //CreateUniformBuffer(m_pDevice, sizeof(LightAttribs),
    //                    "LightAttribs", &m_LightAttribsCB);

    // 2-C)  Samplers que ShadowMapManager necesita
    if (!m_pCmpSampler)
    {
        SamplerDesc S;
        S.MinFilter = S.MagFilter = S.MipFilter = FILTER_TYPE_COMPARISON_LINEAR;
        S.ComparisonFunc                        = COMPARISON_FUNC_LESS;
        m_pDevice->CreateSampler(S, &m_pCmpSampler);
    }
    if (!m_pAnisoSampler)
    {
        SamplerDesc S;
        S.MinFilter = S.MagFilter = S.MipFilter = FILTER_TYPE_ANISOTROPIC;
        S.MaxAnisotropy                         = m_LightAttribs.ShadowAttribs.iMaxAnisotropy;
        m_pDevice->CreateSampler(S, &m_pAnisoSampler);
    }

    // 2-D)  ShadowMapManager
    ShadowMapManager::InitInfo Info;
    Info.Format                      = m_ShadowSettings.Format;
    Info.Resolution                  = m_ShadowSettings.Resolution;
    Info.NumCascades                 = m_LightAttribs.ShadowAttribs.iNumCascades;
    Info.ShadowMode                  = m_ShadowSettings.iShadowMode;
    Info.Is32BitFilterableFmt        = m_ShadowSettings.Is32BitFilterableFmt;
    Info.pComparisonSampler          = m_pCmpSampler;
    Info.pFilterableShadowMapSampler = m_pAnisoSampler;

    m_ShadowMapMgr.Initialize(m_pDevice, nullptr, Info);
    

}


void Tutorial03_Texturing::UpdateUI(){
    ImGui::Begin("Light settings");
    ImGui::gizmo3D("Light direction", reinterpret_cast<float3&>(m_LightAttribs.f4Direction), ImGui::GetTextLineHeight() * 10);

    /*ImGui::Text("Light intensity");
    ImGui::SliderFloat("Intensity", &m_LightAttribs.f4Intensity.x, 0.0f, 1.0f);*/

    ImGui::Separator();

    ImGui::Text("Parallax settings");
    ImGui::SliderFloat("Height scale", &m_ParallaxAttribs.generalHeightScale, 0.0f, 1.0f);
    ImGui::SliderInt("Parallax mode", &m_ParallaxAttribs.parallaxMode, 0, 2);



    ImGui::End();


}


} // namespace Diligent
