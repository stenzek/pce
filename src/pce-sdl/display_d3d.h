#pragma once

#ifdef WIN32

#include "YBaseLib/Windows/WindowsHeaders.h"
#include "pce-sdl/display_sdl.h"
#include <d3d11.h>
#include <memory>
#include <mutex>
#include <wrl.h>

class DisplayD3D : public DisplaySDL
{
public:
  DisplayD3D();
  ~DisplayD3D();

  static std::unique_ptr<DisplayD3D> Create();

protected:
  bool Initialize() override;
  void OnWindowResized() override;
  void RenderImpl() override;

private:
  bool UpdateFramebufferTexture();
  bool CreateRenderTargetView();

  Microsoft::WRL::ComPtr<ID3D11Device> m_device = nullptr;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context = nullptr;
  Microsoft::WRL::ComPtr<IDXGISwapChain> m_swap_chain = nullptr;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_swap_chain_rtv = nullptr;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertex_shader = nullptr;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixel_shader = nullptr;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depth_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11BlendState> m_blend_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_framebuffer_texture = nullptr;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_framebuffer_texture_srv = nullptr;

  uint32 m_framebuffer_texture_width = 0;
  uint32 m_framebuffer_texture_height = 0;
};

#endif