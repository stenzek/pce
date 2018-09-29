#pragma once

#include <QtWidgets/QOpenGLWidget>
#include <memory>

class DisplayRenderer;

class DisplayWidget : public QOpenGLWidget
{
  Q_OBJECT

public:
  DisplayWidget(QWidget* parent);
  ~DisplayWidget();

  DisplayRenderer* getDisplayRenderer() const { return m_display_renderer.get(); }

#if 0
  virtual QSize sizeHint() const override;

  void moveGLContextToThread(QThread* thread);
#endif

Q_SIGNALS:
  void onKeyPressed(QKeyEvent* event);
  void onKeyReleased(QKeyEvent* event);

protected:
  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual void keyReleaseEvent(QKeyEvent* event) override;
  virtual void initializeGL() override;
  virtual void resizeGL(int w, int h) override;
  virtual void paintGL() override;

private:
  std::unique_ptr<DisplayRenderer> m_display_renderer;
};
