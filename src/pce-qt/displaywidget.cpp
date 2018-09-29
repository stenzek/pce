#include "pce-qt/displaywidget.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"
#include "common/display_renderer.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QResizeEvent>
#include <algorithm>

// TODO: D3D support.

DisplayWidget::DisplayWidget(QWidget* parent) : QOpenGLWidget(parent)
{
  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

  QSurfaceFormat format;
  format.setProfile(QSurfaceFormat::NoProfile);
  format.setVersion(3, 0);
  format.setRedBufferSize(8);
  format.setGreenBufferSize(8);
  format.setBlueBufferSize(8);
  format.setSwapInterval(1);
  setFormat(format);
}

DisplayWidget::~DisplayWidget()
{
  makeCurrent();

  m_display_renderer.reset();

  doneCurrent();
}

void DisplayWidget::keyPressEvent(QKeyEvent* event)
{
  emit onKeyPressed(event);
}

void DisplayWidget::keyReleaseEvent(QKeyEvent* event)
{
  emit onKeyReleased(event);
}

// We can't include glad.h here...
extern "C" {
int gladLoadGLES2Loader(void* (*)(const char*));
int gladLoadGLLoader(void* (*)(const char*));
}

void DisplayWidget::initializeGL()
{
  printf("InitializeGL\n");

  auto get_func = [](const char* name) -> void* { return QOpenGLContext::currentContext()->getProcAddress(name); };
  int load_result;
  if (format().renderableType() == QSurfaceFormat::OpenGLES)
    load_result = gladLoadGLES2Loader(get_func);
  else
    load_result = gladLoadGLLoader(get_func);
  if (!load_result)
    Panic("GLAD load failed");

  const auto display_widget_size = size();
  m_display_renderer = DisplayRenderer::Create(DisplayRenderer::BackendType::OpenGL,
                                               reinterpret_cast<DisplayRenderer::WindowHandleType>(winId()),
                                               size().width(), size().height());
  if (!m_display_renderer)
    Panic("Failed to create display renderer");

  update();
}

void DisplayWidget::paintGL()
{
  m_display_renderer->RenderDisplays();
  update();
}

void DisplayWidget::resizeGL(int w, int h)
{
  m_display_renderer->WindowResized(u32(w), u32(h));
  update();
}
