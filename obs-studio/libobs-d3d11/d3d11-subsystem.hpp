/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <util/AlignedNew.hpp>
#include <util/windows/win-version.h>

#include <vector>
#include <string>
#include <memory>

#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <util/base.h>
#include <graphics/matrix4.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/HRError.hpp>

// #define DISASSEMBLE_SHADERS

struct shader_var;
struct shader_sampler;
struct gs_vertex_shader;

using namespace std;

/*
 * Just to clarify, all structs, and all public.  These are exporting only
 * via encapsulated C bindings, not C++ bindings, so the whole concept of
 * "public" and "private" does not matter at all for this subproject.
 */
//获取win版本
static inline uint32_t GetWinVer()
{
	struct win_version_info ver;
	get_win_ver(&ver);

	return (ver.major << 8) | ver.minor;
}

static inline DXGI_FORMAT ConvertGSTextureFormat(gs_color_format format)
{
	switch (format) {
	case GS_UNKNOWN:     return DXGI_FORMAT_UNKNOWN;
	case GS_A8:          return DXGI_FORMAT_A8_UNORM;
	case GS_R8:          return DXGI_FORMAT_R8_UNORM;
	case GS_RGBA:        return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX:        return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA:        return DXGI_FORMAT_B8G8R8A8_UNORM;
	case GS_R10G10B10A2: return DXGI_FORMAT_R10G10B10A2_UNORM;
	case GS_RGBA16:      return DXGI_FORMAT_R16G16B16A16_UNORM;
	case GS_R16:         return DXGI_FORMAT_R16_UNORM;
	case GS_RGBA16F:     return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case GS_RGBA32F:     return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case GS_RG16F:       return DXGI_FORMAT_R16G16_FLOAT;
	case GS_RG32F:       return DXGI_FORMAT_R32G32_FLOAT;
	case GS_R16F:        return DXGI_FORMAT_R16_FLOAT;
	case GS_R32F:        return DXGI_FORMAT_R32_FLOAT;
	case GS_DXT1:        return DXGI_FORMAT_BC1_UNORM;
	case GS_DXT3:        return DXGI_FORMAT_BC2_UNORM;
	case GS_DXT5:        return DXGI_FORMAT_BC3_UNORM;
	}

	return DXGI_FORMAT_UNKNOWN;
}
//纹理格式
static inline gs_color_format ConvertDXGITextureFormat(DXGI_FORMAT format)
{
	switch ((unsigned long)format) {
    case DXGI_FORMAT_A8_UNORM:           return GS_A8;//仅Alpha纹理格式
	case DXGI_FORMAT_R8_UNORM:           return GS_R8;
    case DXGI_FORMAT_R8G8B8A8_UNORM:     return GS_RGBA;//颜色纹理格式，并带有alpha通道。
	case DXGI_FORMAT_B8G8R8X8_UNORM:     return GS_BGRX;
	case DXGI_FORMAT_B8G8R8A8_UNORM:     return GS_BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:  return GS_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM: return GS_RGBA16;
	case DXGI_FORMAT_R16_UNORM:          return GS_R16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT: return GS_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT: return GS_RGBA32F;
	case DXGI_FORMAT_R16G16_FLOAT:       return GS_RG16F;
	case DXGI_FORMAT_R32G32_FLOAT:       return GS_RG32F;
	case DXGI_FORMAT_R16_FLOAT:          return GS_R16F;
	case DXGI_FORMAT_R32_FLOAT:          return GS_R32F;
    case DXGI_FORMAT_BC1_UNORM:          return GS_DXT1;//压缩的颜色纹理格式。
	case DXGI_FORMAT_BC2_UNORM:          return GS_DXT3;
	case DXGI_FORMAT_BC3_UNORM:          return GS_DXT5;
	}

	return GS_UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSZStencilFormat(gs_zstencil_format format)
{
	switch (format) {
	case GS_ZS_NONE:     return DXGI_FORMAT_UNKNOWN;
	case GS_Z16:         return DXGI_FORMAT_D16_UNORM;
	case GS_Z24_S8:      return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case GS_Z32F:        return DXGI_FORMAT_D32_FLOAT;
	case GS_Z32F_S8X24:  return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static inline D3D11_COMPARISON_FUNC ConvertGSDepthTest(gs_depth_test test)
{
	switch (test) {
	case GS_NEVER:    return D3D11_COMPARISON_NEVER;
	case GS_LESS:     return D3D11_COMPARISON_LESS;
	case GS_LEQUAL:   return D3D11_COMPARISON_LESS_EQUAL;
	case GS_EQUAL:    return D3D11_COMPARISON_EQUAL;
	case GS_GEQUAL:   return D3D11_COMPARISON_GREATER_EQUAL;
	case GS_GREATER:  return D3D11_COMPARISON_GREATER;
	case GS_NOTEQUAL: return D3D11_COMPARISON_NOT_EQUAL;
	case GS_ALWAYS:   return D3D11_COMPARISON_ALWAYS;
	}

	return D3D11_COMPARISON_NEVER;
}

static inline D3D11_STENCIL_OP ConvertGSStencilOp(gs_stencil_op_type op)
{
	switch (op) {
	case GS_KEEP:    return D3D11_STENCIL_OP_KEEP;
	case GS_ZERO:    return D3D11_STENCIL_OP_ZERO;
	case GS_REPLACE: return D3D11_STENCIL_OP_REPLACE;
	case GS_INCR:    return D3D11_STENCIL_OP_INCR;
	case GS_DECR:    return D3D11_STENCIL_OP_DECR;
	case GS_INVERT:  return D3D11_STENCIL_OP_INVERT;
	}

	return D3D11_STENCIL_OP_KEEP;
}

static inline D3D11_BLEND ConvertGSBlendType(gs_blend_type type)
{
	switch (type) {
	case GS_BLEND_ZERO:        return D3D11_BLEND_ZERO;
	case GS_BLEND_ONE:         return D3D11_BLEND_ONE;
	case GS_BLEND_SRCCOLOR:    return D3D11_BLEND_SRC_COLOR;
	case GS_BLEND_INVSRCCOLOR: return D3D11_BLEND_INV_SRC_COLOR;
	case GS_BLEND_SRCALPHA:    return D3D11_BLEND_SRC_ALPHA;
	case GS_BLEND_INVSRCALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
	case GS_BLEND_DSTCOLOR:    return D3D11_BLEND_DEST_COLOR;
	case GS_BLEND_INVDSTCOLOR: return D3D11_BLEND_INV_DEST_COLOR;
	case GS_BLEND_DSTALPHA:    return D3D11_BLEND_DEST_ALPHA;
	case GS_BLEND_INVDSTALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
	case GS_BLEND_SRCALPHASAT: return D3D11_BLEND_SRC_ALPHA_SAT;
	}

	return D3D11_BLEND_ONE;
}

static inline D3D11_CULL_MODE ConvertGSCullMode(gs_cull_mode mode)
{
	switch (mode) {
	case GS_BACK:    return D3D11_CULL_BACK;
	case GS_FRONT:   return D3D11_CULL_FRONT;
	case GS_NEITHER: return D3D11_CULL_NONE;
	}

	return D3D11_CULL_BACK;
}

static inline D3D11_PRIMITIVE_TOPOLOGY ConvertGSTopology(gs_draw_mode mode)
{
	switch (mode) {
	case GS_POINTS:    return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case GS_LINES:     return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case GS_LINESTRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case GS_TRIS:      return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case GS_TRISTRIP:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}

	return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
}

/* exception-safe RAII wrapper for vertex buffer data (NOTE: not copy-safe) */
struct VBDataPtr {
	gs_vb_data *data;

	inline VBDataPtr(gs_vb_data *data) : data(data) {}
	inline ~VBDataPtr() {gs_vbdata_destroy(data);}
};

enum class gs_type {
	gs_vertex_buffer,
	gs_index_buffer,
	gs_texture_2d,
	gs_zstencil_buffer,
	gs_stage_surface,
	gs_sampler_state,
	gs_vertex_shader,
	gs_pixel_shader,
	gs_duplicator,
	gs_swap_chain,
};

struct gs_obj {
	gs_device_t *device;
	gs_type obj_type;
	gs_obj *next;
	gs_obj **prev_next;

	inline gs_obj() :
		device(nullptr),
		next(nullptr),
		prev_next(nullptr)
	{}

	gs_obj(gs_device_t *device, gs_type type);
	virtual ~gs_obj();
};

struct gs_vertex_buffer : gs_obj {
	ComPtr<ID3D11Buffer>         vertexBuffer;//缓冲区通常存储顶点或索引数据
	ComPtr<ID3D11Buffer>         normalBuffer;
	ComPtr<ID3D11Buffer>         colorBuffer;
	ComPtr<ID3D11Buffer>         tangentBuffer;
	vector<ComPtr<ID3D11Buffer>> uvBuffers;

	bool           dynamic;
	VBDataPtr      vbd;
	size_t         numVerts;
	vector<size_t> uvSizes;

	void FlushBuffer(ID3D11Buffer *buffer, void *array,
			size_t elementSize);

	void MakeBufferList(gs_vertex_shader *shader,
			vector<ID3D11Buffer*> &buffers,
			vector<uint32_t> &strides);

	void InitBuffer(const size_t elementSize,
			const size_t numVerts, void *array,
			ID3D11Buffer **buffer);

	void BuildBuffers();

	inline void Release()
	{
		vertexBuffer.Release();
		normalBuffer.Release();
		colorBuffer.Release();
		tangentBuffer.Release();
		uvBuffers.clear();
	}

	inline void Rebuild();

	gs_vertex_buffer(gs_device_t *device, struct gs_vb_data *data,
			uint32_t flags);
};

/* exception-safe RAII wrapper for index buffer data (NOTE: not copy-safe) */
struct DataPtr {
	void *data;

	inline DataPtr(void *data) : data(data) {}
	inline ~DataPtr() {bfree(data);}
};

struct gs_index_buffer : gs_obj {
	ComPtr<ID3D11Buffer> indexBuffer;
	bool                 dynamic;
	gs_index_type        type;
	size_t               indexSize;
	size_t               num;
	DataPtr              indices;

	D3D11_BUFFER_DESC bd = {};
	D3D11_SUBRESOURCE_DATA srd = {};

	void InitBuffer();

	inline void Rebuild(ID3D11Device *dev);

	inline void Release() {indexBuffer.Release();}

	gs_index_buffer(gs_device_t *device, enum gs_index_type type,
			void *indices, size_t num, uint32_t flags);
};

struct gs_texture : gs_obj {
	gs_texture_type type;
	uint32_t        levels;
	gs_color_format format;

	ComPtr<ID3D11ShaderResourceView> shaderRes;
	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc = {};

	inline void Rebuild(ID3D11Device *dev);

	inline gs_texture(gs_texture_type type, uint32_t levels,
			gs_color_format format)
		: type   (type),
		  levels (levels),
		  format (format)
	{
	}

	inline gs_texture(gs_device *device, gs_type obj_type,
			gs_texture_type type)
		: gs_obj (device, obj_type),
		  type   (type)
	{
	}

	inline gs_texture(gs_device *device, gs_type obj_type,
			gs_texture_type type,
			uint32_t levels, gs_color_format format)
		: gs_obj (device, obj_type),
		  type   (type),
		  levels (levels),
		  format (format)
	{
	}
};

struct gs_texture_2d : gs_texture {
    ComPtr<ID3D11Texture2D>          texture;//用于2D数据，这也是最常用的纹理资源类型,2d纹理
    ComPtr<ID3D11RenderTargetView>   renderTarget[6];//CreateRenderTargetView来创建一个渲染目标视图,渲染目标视图含有ID3D11RenderTargetView类型
    ComPtr<IDXGISurface1>            gdiSurface;//IDXGISurface1接口通过添加对使用Windows图形设备接口(GDI)的支持

	uint32_t        width = 0, height = 0;
	DXGI_FORMAT     dxgiFormat = DXGI_FORMAT_UNKNOWN;
	bool            isRenderTarget = false;
	bool            isGDICompatible = false;
	bool            isDynamic = false;
	bool            isShared = false;
	bool            genMipmaps = false;
	uint32_t        sharedHandle = 0;

	vector<vector<uint8_t>> data;
	vector<D3D11_SUBRESOURCE_DATA> srd;
	D3D11_TEXTURE2D_DESC td = {};

	void InitSRD(vector<D3D11_SUBRESOURCE_DATA> &srd);
	void InitTexture(const uint8_t **data);
	void InitResourceView();
	void InitRenderTargets();
	void BackupTexture(const uint8_t **data);

	void RebuildSharedTextureFallback();
	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		texture.Release();
		for (auto &rt : renderTarget)
			rt.Release();
		gdiSurface.Release();
		shaderRes.Release();
	}

	inline gs_texture_2d()
		: gs_texture (GS_TEXTURE_2D, 0, GS_UNKNOWN)
	{
	}

	gs_texture_2d(gs_device_t *device, uint32_t width, uint32_t height,
			gs_color_format colorFormat, uint32_t levels,
			const uint8_t **data, uint32_t flags,
			gs_texture_type type, bool gdiCompatible, bool shared);

	gs_texture_2d(gs_device_t *device, uint32_t handle);
};

struct gs_zstencil_buffer : gs_obj {
	ComPtr<ID3D11Texture2D>        texture;
	ComPtr<ID3D11DepthStencilView> view;

	uint32_t           width, height;
	gs_zstencil_format format;
	DXGI_FORMAT        dxgiFormat;

	D3D11_TEXTURE2D_DESC td = {};
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};

	void InitBuffer();

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		texture.Release();
		view.Release();
	}

	inline gs_zstencil_buffer()
		: width      (0),
		  height     (0),
		  dxgiFormat (DXGI_FORMAT_UNKNOWN)
	{
	}

	gs_zstencil_buffer(gs_device_t *device, uint32_t width, uint32_t height,
			gs_zstencil_format format);
};

struct gs_stage_surface : gs_obj {
	ComPtr<ID3D11Texture2D> texture;
	D3D11_TEXTURE2D_DESC td = {};

	uint32_t        width, height;
	gs_color_format format;
	DXGI_FORMAT     dxgiFormat;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		texture.Release();
	}

	gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height,
			gs_color_format colorFormat);
};

struct gs_sampler_state : gs_obj {
	ComPtr<ID3D11SamplerState> state;
	D3D11_SAMPLER_DESC         sd = {};
	gs_sampler_info            info;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release() {state.Release();}

	gs_sampler_state(gs_device_t *device, const gs_sampler_info *info);
};

struct gs_shader_param {
	string                         name;
	gs_shader_param_type           type;

	uint32_t                       textureID;
	struct gs_sampler_state        *nextSampler = nullptr;

	int                            arrayCount;

	size_t                         pos;

	vector<uint8_t>                curValue;
	vector<uint8_t>                defaultValue;
	bool                           changed;

	gs_shader_param(shader_var &var, uint32_t &texCounter);
};

struct ShaderError {
	ComPtr<ID3D10Blob> errors;
	HRESULT hr;

	inline ShaderError(const ComPtr<ID3D10Blob> &errors, HRESULT hr)
		: errors (errors),
		  hr     (hr)
	{
	}
};

struct gs_shader : gs_obj {
	gs_shader_type          type;
	vector<gs_shader_param> params;
	ComPtr<ID3D11Buffer>    constants;
	size_t                  constantSize;

	D3D11_BUFFER_DESC       bd = {};
	vector<uint8_t>         data;

	inline void UpdateParam(vector<uint8_t> &constData,
			gs_shader_param &param, bool &upload);
	void UploadParams();

	void BuildConstantBuffer();
	void Compile(const char *shaderStr, const char *file,
			const char *target, ID3D10Blob **shader);

	inline gs_shader(gs_device_t *device, gs_type obj_type,
			gs_shader_type type)
		: gs_obj       (device, obj_type),
		  type         (type),
		  constantSize (0)
	{
	}

	virtual ~gs_shader() {}
};

struct ShaderSampler {
	string           name;
	gs_sampler_state sampler;

	inline ShaderSampler(const char *name, gs_device_t *device,
			gs_sampler_info *info)
		: name    (name),
		  sampler (device, info)
	{
	}
};

struct gs_vertex_shader : gs_shader {
    ComPtr<ID3D11VertexShader> shader;//顶点着色器
    ComPtr<ID3D11InputLayout>  layout;//顶点布局

	gs_shader_param *world, *viewProj;

    vector<D3D11_INPUT_ELEMENT_DESC> layoutData;//D3D11_INPUT_ELEMENT_DESC来描述顶点的布局信息

	bool     hasNormals;
	bool     hasColors;
	bool     hasTangents;
	uint32_t nTexUnits;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		shader.Release();
		layout.Release();
		constants.Release();
	}

	inline uint32_t NumBuffersExpected() const
	{
		uint32_t count = nTexUnits+1;
		if (hasNormals)  count++;
		if (hasColors)   count++;
		if (hasTangents) count++;

		return count;
	}

	void GetBuffersExpected(const vector<D3D11_INPUT_ELEMENT_DESC> &inputs);

	gs_vertex_shader(gs_device_t *device, const char *file,
			const char *shaderString);
};

struct gs_duplicator : gs_obj {
    ComPtr<IDXGIOutputDuplication> duplicator; //桌面拷贝
	gs_texture_2d *texture;
	int idx;

	void Start();

	inline void Release()
	{
		duplicator.Release();
	}

	gs_duplicator(gs_device_t *device, int monitor_idx);
	~gs_duplicator();
};

struct gs_pixel_shader : gs_shader {
    ComPtr<ID3D11PixelShader> shader;//像素着色器
	vector<unique_ptr<ShaderSampler>> samplers;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		shader.Release();
		constants.Release();
	}

	inline void GetSamplerStates(ID3D11SamplerState **states)
	{
		size_t i;
		for (i = 0; i < samplers.size(); i++)
			states[i] = samplers[i]->sampler.state;
		for (; i < GS_MAX_TEXTURES; i++)
			states[i] = NULL;
	}

	gs_pixel_shader(gs_device_t *device, const char *file,
			const char *shaderString);
};

struct gs_swap_chain : gs_obj {
	uint32_t                       numBuffers;
	HWND                           hwnd;
	gs_init_data                   initData;
	DXGI_SWAP_CHAIN_DESC           swapDesc = {};

	gs_texture_2d                  target;
	gs_zstencil_buffer             zs;
    ComPtr<IDXGISwapChain>         swap;//一个IDXGISwapChain接口实现一个或多个Surface来存储呈现输出前的渲染数据

	void InitTarget(uint32_t cx, uint32_t cy);
	void InitZStencilBuffer(uint32_t cx, uint32_t cy);
	void Resize(uint32_t cx, uint32_t cy);
	void Init();

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		target.Release();
		zs.Release();
		swap.Release();
	}

	gs_swap_chain(gs_device *device, const gs_init_data *data);
};

struct BlendState {
	bool          blendEnabled;
	gs_blend_type srcFactorC;
	gs_blend_type destFactorC;
	gs_blend_type srcFactorA;
	gs_blend_type destFactorA;

	bool          redEnabled;
	bool          greenEnabled;
	bool          blueEnabled;
	bool          alphaEnabled;

	inline BlendState()
		: blendEnabled (true),
		  srcFactorC   (GS_BLEND_SRCALPHA),
		  destFactorC  (GS_BLEND_INVSRCALPHA),
		  srcFactorA   (GS_BLEND_ONE),
		  destFactorA  (GS_BLEND_ONE),
		  redEnabled   (true),
		  greenEnabled (true),
		  blueEnabled  (true),
		  alphaEnabled (true)
	{
	}

	inline BlendState(const BlendState &state)
	{
		memcpy(this, &state, sizeof(BlendState));
	}
};

struct SavedBlendState : BlendState {
	ComPtr<ID3D11BlendState> state;
	D3D11_BLEND_DESC         bd;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		state.Release();
	}

	inline SavedBlendState(const BlendState &val, D3D11_BLEND_DESC &desc)
		: BlendState(val), bd(desc)
	{
	}
};

struct StencilSide {
	gs_depth_test test;
	gs_stencil_op_type fail;
	gs_stencil_op_type zfail;
	gs_stencil_op_type zpass;

	inline StencilSide()
		: test  (GS_ALWAYS),
		  fail  (GS_KEEP),
		  zfail (GS_KEEP),
		  zpass (GS_KEEP)
	{
	}
};

struct ZStencilState {
	bool          depthEnabled;
	bool          depthWriteEnabled;
	gs_depth_test depthFunc;

	bool          stencilEnabled;
	bool          stencilWriteEnabled;
	StencilSide   stencilFront;
	StencilSide   stencilBack;

	inline ZStencilState()
		: depthEnabled        (true),
		  depthWriteEnabled   (true),
		  depthFunc           (GS_LESS),
		  stencilEnabled      (false),
		  stencilWriteEnabled (true)
	{
	}

	inline ZStencilState(const ZStencilState &state)
	{
		memcpy(this, &state, sizeof(ZStencilState));
	}
};

struct SavedZStencilState : ZStencilState {
	ComPtr<ID3D11DepthStencilState> state;
	D3D11_DEPTH_STENCIL_DESC        dsd;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		state.Release();
	}

	inline SavedZStencilState(const ZStencilState &val,
			D3D11_DEPTH_STENCIL_DESC desc)
		: ZStencilState (val),
		  dsd           (desc)
	{
	}
};

struct RasterState {
	gs_cull_mode cullMode;
	bool         scissorEnabled;

	inline RasterState()
		: cullMode       (GS_BACK),
		  scissorEnabled (false)
	{
	}

	inline RasterState(const RasterState &state)
	{
		memcpy(this, &state, sizeof(RasterState));
	}
};

struct SavedRasterState : RasterState {
	ComPtr<ID3D11RasterizerState> state;
	D3D11_RASTERIZER_DESC         rd;

	inline void Rebuild(ID3D11Device *dev);

	inline void Release()
	{
		state.Release();
	}

	inline SavedRasterState(const RasterState &val,
			D3D11_RASTERIZER_DESC &desc)
	       : RasterState (val),
	         rd          (desc)
	{
	}
};

struct mat4float {
	float mat[16];
};

struct gs_device {
    // ComPtr智能指针
    ComPtr<IDXGIFactory1>       factory;//dxgi工厂
    ComPtr<IDXGIAdapter1>       adapter;//dxgi设配器
    ComPtr<ID3D11Device>        device;//Device用来创建资源和枚举一个显示适配器性能。在Direct3D 11中，Device可以通过调用D3D11CreateDevice 和 D3D11CreateDeviceAndSwapChain来创建一个device。

    //设备上下文(device context)包含了环境和设置.更能为具体的说一个设备上下文使用设备对象所创建的资源类设置管道状态以及建立渲染指令.Direct3D11的设备上下文有两种类型,一个是实时渲染,另外一个则是延时渲染,两种设备上下文都可以ID3D11DeviceContext来表示.
    ComPtr<ID3D11DeviceContext> context;
	uint32_t                    adpIdx = 0;

	gs_texture_2d               *curRenderTarget = nullptr;
	gs_zstencil_buffer          *curZStencilBuffer = nullptr;
	int                         curRenderSide = 0;
	gs_texture                  *curTextures[GS_MAX_TEXTURES];
	gs_sampler_state            *curSamplers[GS_MAX_TEXTURES];
    gs_vertex_buffer            *curVertexBuffer = nullptr;//当前顶点缓存
    gs_index_buffer             *curIndexBuffer = nullptr;//索引缓存
	gs_vertex_shader            *curVertexShader = nullptr;
	gs_pixel_shader             *curPixelShader = nullptr;
	gs_swap_chain               *curSwapChain = nullptr;

	gs_vertex_buffer            *lastVertexBuffer = nullptr;
	gs_vertex_shader            *lastVertexShader = nullptr;

	bool                        zstencilStateChanged = true;
	bool                        rasterStateChanged = true;
	bool                        blendStateChanged = true;
	ZStencilState               zstencilState;
	RasterState                 rasterState;
	BlendState                  blendState;
	vector<SavedZStencilState>  zstencilStates;
	vector<SavedRasterState>    rasterStates;
	vector<SavedBlendState>     blendStates;
	ID3D11DepthStencilState     *curDepthStencilState = nullptr;
	ID3D11RasterizerState       *curRasterState = nullptr;
	ID3D11BlendState            *curBlendState = nullptr;
	D3D11_PRIMITIVE_TOPOLOGY    curToplogy;

	pD3DCompile                 d3dCompile = nullptr;
#ifdef DISASSEMBLE_SHADERS
	pD3DDisassemble             d3dDisassemble = nullptr;
#endif

	gs_rect                     viewport;

	vector<mat4float>           projStack;

	matrix4                     curProjMatrix;
	matrix4                     curViewMatrix;
	matrix4                     curViewProjMatrix;

	gs_obj                      *first_obj = nullptr;

	void InitCompiler();
	void InitFactory(uint32_t adapterIdx);
	void InitDevice(uint32_t adapterIdx);

	ID3D11DepthStencilState *AddZStencilState();
	ID3D11RasterizerState   *AddRasterState();
	ID3D11BlendState        *AddBlendState();
	void UpdateZStencilState();
	void UpdateRasterState();
	void UpdateBlendState();

	void LoadVertexBufferData();

	inline void CopyTex(ID3D11Texture2D *dst,
			uint32_t dst_x, uint32_t dst_y,
			gs_texture_t *src, uint32_t src_x, uint32_t src_y,
			uint32_t src_w, uint32_t src_h);

	void UpdateViewProjMatrix();

	void RebuildDevice();

	gs_device(uint32_t adapterIdx);
	~gs_device();
};
