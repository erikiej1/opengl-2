#include "mainview.h"
#include "math.h"
#include "geometry.h"

#include <QDateTime>

/**
 * @brief MainView::MainView
 *
 * Constructor of MainView
 *
 * @param parent
 */
MainView::MainView(QWidget *parent) : QOpenGLWidget(parent) {
    qDebug() << "MainView constructor";

    connect(&timer, SIGNAL(timeout()), this, SLOT(update()));

    projectionMatrix.perspective(60.0f, width()/height(), 0.1f, 20.0f);
    rotationMatrix.setToIdentity();
    normalMatrix.setToIdentity();

    /* load obj file and initialize model */
    Model *model = new Model(":/models/cat.obj");
    QVector<QVector3D> loadedVertices = model->getVertices();
    QVector<QVector3D> modelNormals = model->getNormals();
    modelSize = loadedVertices.length();
    modelVertices = (Vertex*)malloc (modelSize*sizeof (Vertex));
    for(int i = 0; i < modelSize; i++){
        modelVertices[i] = createVertex(loadedVertices[i].x(),
                                         loadedVertices[i].y(),
                                         loadedVertices[i].z(),
                                         modelNormals[i].x(),
                                         modelNormals[i].y(),
                                         modelNormals[i].z()
                                         );
    }

    modelMatrix.translate(0,-2,-10);
    modelMatrix.scale(10.0f);

    /* OLD CODE FROM PART 1
    // Create Cube
    createCube( -1.0f, -1.0f,  1.0f,
                 1.0f, -1.0f,  1.0f,
                 1.0f,  1.0f,  1.0f,
                -1.0f,  1.0f,  1.0f,
                -1.0f, -1.0f, -1.0f,
                 1.0f, -1.0f, -1.0f,
                 1.0f,  1.0f, -1.0f,
                -1.0f,  1.0f, -1.0f);

    cubeMatrix.translate(2.0f,0.0f,-6.0f);

    // create pyramid
    createPyramid( -1.0f, -1.0f,  1.0f,
                    1.0f, -1.0f,  1.0f,
                    1.0f, -1.0f, -1.0f,
                   -1.0f, -1.0f, -1.0f,
                    0.0f,  1.0f,  0.0f);
    mPyramid.translate(-2.0f,0.0f,-6.0f);
    */
}


/**
 * @brief MainView::~MainView
 *
 * Destructor of MainView
 * This is the last function called, before exit of the program
 * Use this to clean up your variables, buffers etc.
 *
 */
MainView::~MainView() {
    debugLogger->stopLogging();

    qDebug() << "MainView destructor";

    glDeleteBuffers(1,bufferId);
    glDeleteVertexArrays(1,&vaoModel);
}

// --- OpenGL initialization

/**
 * @brief MainView::initializeGL
 *
 * Called upon OpenGL initialization
 * Attaches a debugger and calls other init functions
 */
void MainView::initializeGL() {
    qDebug() << ":: Initializing OpenGL";
    initializeOpenGLFunctions();

    debugLogger = new QOpenGLDebugLogger();
    connect( debugLogger, SIGNAL( messageLogged( QOpenGLDebugMessage ) ),
             this, SLOT( onMessageLogged( QOpenGLDebugMessage ) ), Qt::DirectConnection );

    if ( debugLogger->initialize() ) {
        qDebug() << ":: Logging initialized";
        debugLogger->startLogging( QOpenGLDebugLogger::SynchronousLogging );
        debugLogger->enableMessages();
    }

    QString glVersion;
    glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    qDebug() << ":: Using OpenGL" << qPrintable(glVersion);

    // Enable depth buffer
    glEnable(GL_DEPTH_TEST);

    // Enable backface culling
    glEnable(GL_CULL_FACE);

    // Default is GL_LESS
    glDepthFunc(GL_LEQUAL);

    // Set the color of the screen to be blue on clear (new frame)
    glClearColor(0.2f, 0.5f, 0.7f, 0.0f);

    createShaderProgram();

    glGenBuffers(1, bufferId);
    glGenVertexArrays(1, &vaoModel);
    glBindVertexArray(vaoModel);
    glBindBuffer(GL_ARRAY_BUFFER, bufferId[0]);
    glBufferData(GL_ARRAY_BUFFER, modelSize * sizeof(Vertex), modelVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),0); // point
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)(sizeof(float)*3)); // normal
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer (GL_ARRAY_BUFFER,0);


    /*
     *      OLD CODE FROM PART 1
     *
    glGenVertexArrays(1, &vaoCube);
    glGenVertexArrays(1, &vaoPyramid);

    glBindVertexArray(vaoCube);
    glBindBuffer(GL_ARRAY_BUFFER, bufferId[0]);
    glBufferData(GL_ARRAY_BUFFER, 6*6*sizeof(Vertex), cube, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),0); // point
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)(sizeof(float)*3)); // normal
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer (GL_ARRAY_BUFFER,0);

    glBindVertexArray(vaoPyramid);
    glBindBuffer(GL_ARRAY_BUFFER, bufferId[2]);
    glBufferData(GL_ARRAY_BUFFER, (6+4*3)*sizeof(Vertex), (GLvoid*)pyr, GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),0); // point
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(GLvoid*)(sizeof(float)*3)); // normal
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer (GL_ARRAY_BUFFER,0);
    */
}

Vertex MainView::createVertex(float x, float y, float z, float i, float j, float k){
    Vertex v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.i = i;
    v.j = j;
    v.k = k;
    return v;
}

void MainView::createShaderProgram()
{
    // Create normal shader program
    normalShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_normal.glsl");
    normalShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_normal.glsl");
    normalShaderProgram.link();

    // Create gouraud shader program
    gouraudShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_gouraud.glsl");
    gouraudShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_gouraud.glsl");
    gouraudShaderProgram.link();

    // Create phong shader program
    phongShaderProgram.addShaderFromSourceFile(QOpenGLShader::Vertex,
                                           ":/shaders/vertshader_phong.glsl");
    phongShaderProgram.addShaderFromSourceFile(QOpenGLShader::Fragment,
                                           ":/shaders/fragshader_phong.glsl");
    phongShaderProgram.link();

    locations = normalShaderProgram.uniformLocation("modelTransform");
    projectionLocation = normalShaderProgram.uniformLocation("projectionTransform");
    transformation = normalShaderProgram.uniformLocation("transformation");
    normalTransform = normalShaderProgram.uniformLocation("normalTransform");
}

// --- OpenGL drawing

/**
 * @brief MainView::paintGL
 *
 * Actual function used for drawing to the screen
 *
 */
void MainView::paintGL() {
    // Clear the screen before rendering
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    normalShaderProgram.bind();

    glUniformMatrix4fv(projectionLocation,1,false,projectionMatrix.data());

    QMatrix4x4 transformationMatrix = rotationMatrix * scalingMatrix;

    glUniformMatrix4fv(transformation,1,false,transformationMatrix.data());

    glUniformMatrix3fv(normalTransform,1,false,normalMatrix.data());

    // Draw model
    glUniformMatrix4fv(locations,1,false,modelMatrix.data());
    glBindVertexArray(vaoModel);
    glDrawArrays(GL_TRIANGLES, 0, modelSize);


    /* OLD CODE FROM PART 1

    // Draw Cube
    glUniformMatrix4fv(locations,1,false,cubeMatrix.data());
    glBindVertexArray(vaoCube);
    glDrawArrays(GL_TRIANGLES, 0, 6*6);



    // Draw Pyramid
    glUniformMatrix4fv(locations,1,false,mPyramid.data());
    glBindVertexArray(vaoPyramid);
    glDrawArrays(GL_TRIANGLES, 0, 6+4*3);

    */

    normalShaderProgram.release();
}

/**
 * @brief MainView::resizeGL
 *
 * Called upon resizing of the screen
 *
 * @param newWidth
 * @param newHeight
 */
void MainView::resizeGL(int newWidth, int newHeight) 
{
    // TODO: Update projection to fit the new aspect ratio
    Q_UNUSED(newWidth)
    Q_UNUSED(newHeight)
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(60.0f, newWidth/newHeight, 0.1f, 20.0f);
}

// --- Public interface

void MainView::setRotation(int rotateX, int rotateY, int rotateZ)
{
    qDebug() << "Rotation changed to (" << rotateX << "," << rotateY << "," << rotateZ << ")";
    //Q_UNIMPLEMENTED();
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(rotateX, {1,0,0});
    rotationMatrix.rotate(rotateY, {0,1,0});
    rotationMatrix.rotate(rotateZ, {0,0,1});
    update();

}

void MainView::setScale(int scale)
{
    qDebug() << "Scale changed to " << scale;
    //Q_UNIMPLEMENTED();
    scalingMatrix.setToIdentity();
    float percentage = 0.01f;
    scalingMatrix.scale(scale*percentage);
    update();
}

void MainView::setShadingMode(ShadingMode shading)
{
    switch(shading){
        case MainView::GOURAUD:
            currentShaderProgram. = gouraudShaderProgram; // toewijzing kan niet
            break;
        case MainView::PHONG:
            currentShaderProgram = phongShaderProgram;
            break;
        default:
            currentShaderProgram = normalShaderProgram;
    }
    update();
}

// --- Private helpers

/**
 * @brief MainView::onMessageLogged
 *
 * OpenGL logging function, do not change
 *
 * @param Message
 */
void MainView::onMessageLogged( QOpenGLDebugMessage Message ) {
    qDebug() << " → Log:" << Message;
}

/*************************************************************/
/* Old code from part 1
/*************************************************************/
/*
Square MainView::createSquare(Vertex v1, Vertex v2, Vertex v3, Vertex v4, Vertex v5, Vertex v6){
    Square s;
    s.v1 = v1;
    s.v2 = v2;
    s.v3 = v3;
    s.v4 = v4;
    s.v5 = v5;
    s.v6 = v6;
    return s;
}

Triangle MainView::createTriangle(Vertex v1, Vertex v2, Vertex v3) {
    Triangle t;
    t.v1 = v1;
    t.v2 = v2;
    t.v3 = v3;
    return t;
}

void MainView::createCube (float x1, float y1, float z1,
                 float x2, float y2, float z2,
                 float x3, float y3, float z3,
                 float x4, float y4, float z4,
                 float x5, float y5, float z5,
                 float x6, float y6, float z6,
                 float x7, float y7, float z7,
                 float x8, float y8, float z8 ) {
    cube = (Cube) malloc (sizeof(C));
    cube->s1 = createSquare(
                createVertex( x1, y1, z1, 0.0f,0.0f,1.0f),
                createVertex( x2, y2, z2, 0.0f,0.0f,1.0f),
                createVertex( x3, y3, z3, 0.0f,0.0f,1.0f),
                createVertex( x1, y1, z1, 0.0f,0.0f,1.0f),
                createVertex( x3, y3, z3, 0.0f,0.0f,1.0f),
                createVertex( x4, y4, z4, 0.0f,0.0f,1.0f)
                );
    cube->s2 = createSquare(
                createVertex( x2, y2, z2, 0.0f,1.0f,0.0f),
                createVertex( x6, y6, z6, 0.0f,1.0f,0.0f),
                createVertex( x7, y7, z7, 0.0f,1.0f,0.0f),
                createVertex( x2, y2, z2, 0.0f,1.0f,0.0f),
                createVertex( x7, y7, z7, 0.0f,1.0f,0.0f),
                createVertex( x3, y3, z3, 0.0f,1.0f,0.0f)
                );
    cube->s3 = createSquare(
                createVertex( x4, y4, z4, 1.0f,0.0f,0.0f),
                createVertex( x3, y3, z3, 1.0f,0.0f,0.0f),
                createVertex( x7, y7, z7, 1.0f,0.0f,0.0f),
                createVertex( x4, y4, z4, 1.0f,0.0f,0.0f),
                createVertex( x7, y7, z7, 1.0f,0.0f,0.0f),
                createVertex( x8, y8, z8, 1.0f,0.0f,0.0f)
                );
    cube->s4 = createSquare(
                createVertex( x5, y5, z5, 1.0f,1.0f,1.0f),
                createVertex( x1, y1, z1, 1.0f,1.0f,1.0f),
                createVertex( x4, y4, z4, 1.0f,1.0f,1.0f),
                createVertex( x4, y4, z4, 1.0f,1.0f,1.0f),
                createVertex( x8, y8, z8, 1.0f,1.0f,1.0f),
                createVertex( x5, y5, z5, 1.0f,1.0f,1.0f)
                );
    cube->s5 = createSquare(
                createVertex( x1, y1, z1, 0.0f,1.0f,1.0f),
                createVertex( x5, y5, z5, 0.0f,1.0f,1.0f),
                createVertex( x6, y6, z6, 0.0f,1.0f,1.0f),
                createVertex( x6, y6, z6, 0.0f,1.0f,1.0f),
                createVertex( x2, y2, z2, 0.0f,1.0f,1.0f),
                createVertex( x1, y1, z1, 0.0f,1.0f,1.0f)
                );
    cube->s6 = createSquare(
                createVertex( x7, y7, z7, 1.0f,0.0f,1.0f),
                createVertex( x6, y6, z6, 1.0f,0.0f,1.0f),
                createVertex( x5, y5, z5, 1.0f,0.0f,1.0f),
                createVertex( x8, y8, z8, 1.0f,0.0f,1.0f),
                createVertex( x7, y7, z7, 1.0f,0.0f,1.0f),
                createVertex( x5, y5, z5, 1.0f,0.0f,1.0f)
                );
}

void MainView::createPyramid (float x1, float y1, float z1,
                        float x2, float y2, float z2,
                        float x3, float y3, float z3,
                        float x4, float y4, float z4,
                        float x5, float y5, float z5 ) {
    pyr = (Pyramid) malloc (sizeof(P));
    pyr->bottom = createSquare(
                   createVertex( x1, y1, z1, 1.0f,0.0f,0.0f),
                   createVertex( x4, y4, z4, 1.0f,0.0f,0.0f),
                   createVertex( x3, y3, z3, 1.0f,0.0f,0.0f),
                   createVertex( x3, y3, z3, 1.0f,0.0f,0.0f),
                   createVertex( x2, y2, z2, 1.0f,0.0f,0.0f),
                   createVertex( x1, y1, z1, 1.0f,0.0f,0.0f)
                  );
    pyr->t1 = createTriangle(
                   createVertex( x1, y1, z1, 0.0f,1.0f,0.0f),
                   createVertex( x2, y2, z2, 0.0f,1.0f,0.0f),
                   createVertex( x5, y5, z5, 0.0f,1.0f,0.0f)
               );
    pyr->t2 = createTriangle(
                   createVertex( x2, y2, z2, 0.0f,0.0f,1.0f),
                   createVertex( x3, y3, z3, 0.0f,0.0f,1.0f),
                   createVertex( x5, y5, z5, 0.0f,0.0f,1.0f)
               );
    pyr->t3 = createTriangle(
                   createVertex( x3, y3, z3, 1.0f,1.0f,1.0f),
                   createVertex( x4, y4, z4, 1.0f,1.0f,1.0f),
                   createVertex( x5, y5, z5, 1.0f,1.0f,1.0f)
               );
    pyr->t4 = createTriangle(
                   createVertex( x4, y4, z4, 1.0f,1.0f,0.0f),
                   createVertex( x1, y1, z1, 1.0f,1.0f,0.0f),
                   createVertex( x5, y5, z5, 1.0f,1.0f,0.0f)
               );
}
*/
