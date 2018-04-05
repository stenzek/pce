#pragma once

#ifdef WIN32

#include "YBaseLib/Windows/WindowsHeaders.h"
#include "pce/display.h"
#include <SDL.h>
#include <d3d11.h>
#include <memory>
#include <mutex>
#include <wrl.h>

class DisplayD3D : public Display
{
public:
  DisplayD3D();
  ~DisplayD3D();

  static std::unique_ptr<Display> Create();

  SDL_Window* GetSDLWindow() const { return m_window; }

  void ResizeDisplay(uint32 width = 0, uint32 height = 0) override;
  void ResizeFramebuffer(uint32 width, uint32 height) override;
  void DisplayFramebuffer() override;

  bool IsFullscreen() const override;
  void SetFullscreen(bool enable) override;

  void OnWindowResized() override;
  void MakeCurrent() override;

private:
  bool CreateRenderTargetView();
  void MapFramebufferTexture();
  void UnmapFramebufferTexture();
  void ResizeSwapChain();

  SDL_Window* m_window = nullptr;
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

  uint32 m_window_width = 0;
  uint32 m_window_height = 0;
  bool m_window_resized = false;
  std::mutex m_present_mutex;
};

#endif