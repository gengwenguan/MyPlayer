#ifndef YUV42_PWIDGET_H
#define YUV42_PWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>

class YUV420PWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit YUV420PWidget(QWidget *parent = nullptr);
    ~YUV420PWidget();

    void updateYUVFrame(int width, int height, const uint8_t *y, const uint8_t *u, const uint8_t *v);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void initShaders();
    void initTextures();

    QOpenGLShaderProgram *m_program;
    QOpenGLBuffer m_vbo;

    GLuint m_textureY;
    GLuint m_textureU;
    GLuint m_textureV;

    int m_videoWidth;
    int m_videoHeight;
    const uint8_t *m_y;
    const uint8_t *m_u;
    const uint8_t *m_v;
    bool m_texturesInitialized;
};

#endif // YUV42_PWIDGET_H
