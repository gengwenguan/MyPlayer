#include "yuv420pwidget.h"

YUV420PWidget::YUV420PWidget(QWidget *parent)
    : QOpenGLWidget(parent),
    m_program(nullptr),
    m_videoWidth(0),
    m_videoHeight(0),
    m_texturesInitialized(false)
{
}

YUV420PWidget::~YUV420PWidget()
{
    makeCurrent();
    if (m_program) delete m_program;
    if (m_vbo.isCreated()) m_vbo.destroy();
    glDeleteTextures(1, &m_textureY);
    glDeleteTextures(1, &m_textureU);
    glDeleteTextures(1, &m_textureV);
    doneCurrent();
}

void YUV420PWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    initShaders();
    initTextures();

    // 设置顶点数据 (两个三角形组成一个矩形)
    static const GLfloat vertices[] = {
        // 顶点坐标    纹理坐标
        -1.0f, -1.0f,  0.0f, 1.0f,  // 左下
        1.0f, -1.0f,  1.0f, 1.0f,  // 右下
        -1.0f,  1.0f,  0.0f, 0.0f,  // 左上
        1.0f,  1.0f,  1.0f, 0.0f   // 右上
    };

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices, sizeof(vertices));
}

void YUV420PWidget::initShaders()
{
    m_program = new QOpenGLShaderProgram(this);

    // 顶点着色器
    const char *vshader =
        "attribute vec4 vertexIn;\n"
        "attribute vec2 textureIn;\n"
        "varying vec2 textureOut;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vertexIn;\n"
        "    textureOut = textureIn;\n"
        "}\n";

    // 片段着色器 (YUV420P 转 RGB)
    const char *fshader =
        "varying vec2 textureOut;\n"
        "uniform sampler2D texY;\n"
        "uniform sampler2D texU;\n"
        "uniform sampler2D texV;\n"
        "void main(void)\n"
        "{\n"
        "    float y, u, v, r, g, b;\n"
        "    y = texture2D(texY, textureOut).r;\n"
        "    u = texture2D(texU, textureOut).r - 0.5;\n"
        "    v = texture2D(texV, textureOut).r - 0.5;\n"
        "    r = y + 1.402 * v;\n"
        "    g = y - 0.34414 * u - 0.71414 * v;\n"
        "    b = y + 1.772 * u;\n"
        "    gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}\n";

    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vshader);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fshader);
    m_program->bindAttributeLocation("vertexIn", 0);
    m_program->bindAttributeLocation("textureIn", 1);
    m_program->link();
    m_program->bind();
}

void YUV420PWidget::initTextures()
{
    glGenTextures(1, &m_textureY);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_textureU);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &m_textureV);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_texturesInitialized = true;
}

void YUV420PWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    if (m_videoWidth <= 0 || m_videoHeight <= 0)
        return;

    m_program->bind();
    m_vbo.bind();

    // 设置顶点属性
    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    m_program->setAttributeBuffer(0, GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    m_program->setAttributeBuffer(1, GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));

    // 上传Y分量
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_videoWidth, m_videoHeight, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_y);

    // 上传U分量
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_videoWidth/2, m_videoHeight/2, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_u);

    // 上传V分量
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, m_videoWidth/2, m_videoHeight/2, 0,
                 GL_LUMINANCE, GL_UNSIGNED_BYTE, m_v);

    // 设置纹理单元
    m_program->setUniformValue("texY", 0);
    m_program->setUniformValue("texU", 1);
    m_program->setUniformValue("texV", 2);

    // 绘制
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_vbo.release();
    m_program->release();
}

void YUV420PWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}


void YUV420PWidget::updateYUVFrame(int width, int height, const uint8_t *y, const uint8_t *u, const uint8_t *v)
{
    m_videoWidth = width;
    m_videoHeight = height;
    m_y = y;
    m_u = u;
    m_v = v;
    update();  // 触发重绘
}
