/**
 * \file
 *
 * \author Valentin Bruder
 *
 * \copyright Copyright (C) 2018 Valentin Bruder
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <QObject>
#include <QWidget>
#include <QDir>
#include <QPointer>
#include <QQuaternion>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QOpenGLTexture>
#include <QPropertyAnimation>
#include <qopenglfunctions_4_3_core.h>
#include <QPainter>
#include <QElapsedTimer>
#include <qcheckbox.h>
#include <QRandomGenerator>
#include <QDirIterator>

#include "src/core/volumerendercl.h"

#include <inc/TOBIIRESEARCH/tobii_research.h>
#include <inc/TOBIIRESEARCH/tobii_research_eyetracker.h>
#include <inc/TOBIIRESEARCH/tobii_research_streams.h>
#include <inc/TOBIIRESEARCH/tobii_research_calibration.h>


struct Benchmark
{
    bool active = false;
    quint64 iteration = 0;
    quint64 gaze_iterations = 100;    // gaze iterations per camera state
    QString logFileName = "";
    QFile f;

    bool isCameraIteration()
    {
//        std::cout << this->iteration << " " << (this->iteration % this->gaze_iterations) << std::endl;
        return (this->iteration % this->gaze_iterations) == 0;
    }

    void writeState(const QString &str)
    {
        if (!f.isOpen() && !logFileName.isEmpty())
        {
            this->f.setFileName(this->logFileName);
            bool ok = f.open(QFile::WriteOnly);
            if (!ok)
            {
                qWarning() << "Failed to open benchmark file" << logFileName;
                return;
            }
        }
        QTextStream fileStream(&f);
        fileStream << str;
    }
};

class VolumeRenderWidget : public QOpenGLWidget, protected QOpenGLFunctions_4_3_Core
{
    Q_OBJECT

public:
    explicit VolumeRenderWidget(QWidget *parent = nullptr);
    virtual ~VolumeRenderWidget() override;

    void setupVertexAttribs();

    void setVolumeData(const QString &fileName);

    void setIndexandSamplingMap(const QString &fileNameIndexMap,
                                const QString &fileNameSamplingMap,
                                const QString &fileNameNeighborIndex,
                                const QString &fileNameNeighborWeights);

    bool hasData() const;

    const QVector4D getVolumeResolution() const;

    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

    void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    void keyReleaseEvent(QKeyEvent *event) Q_DECL_OVERRIDE;

    void updateView(float dx = 0, float dy = 0);

    bool getLoadingFinished() const;
    void setLoadingFinished(const bool loadingFinished);

    QVector3D getCamTranslation() const;
    void setCamTranslation(const QVector3D &translation);

    QQuaternion getCamRotation() const;
    void setCamRotation(const QQuaternion &rotQuat);

	enum RenderingMethod { Standard, LBG_Sampling };
	void setRenderingMethod(int rm);	/* sets the current rending method
	updates RenderingParameters of the kernel and calls update() to update the screen. */

	/*
	Enables or disables eyetracking and calls update() to update the screen.
	Does not change the value of _contRendering.
	*/
	void setEyetracking(bool eyetracking);
	void showSelectEyetrackingDevice();
#if _WIN32
	void actionSelectMonitor();
    static bool MonitorEnumProc(HMONITOR monitor, HDC hdcMnitor, LPRECT rect, LPARAM param);
#endif

public slots:
    void cleanup();
    void resetCam();

    void updateSamplingRate(double samplingRate);
    void updateTransferFunction(QGradientStops stops);
    std::vector<unsigned char> getRawTransferFunction(QGradientStops stops) const;
    void setRawTransferFunction(std::vector<unsigned char> tff);

#undef Bool
    void setTffInterpolation(const QString method);
    void setCamOrtho(const bool camOrtho);
    void setContRendering(const bool setContRendering);
    void setIllumination(const int illum);
    void setLinearInterpolation(const bool linear);
    void setContours(const bool contours);
    void setAerial(const bool aerial);
    /**
     * @brief Set image order empty space skipping.
     * @param useEss
     */
    void setImgEss(bool useEss);
    /**
     * @brief Set object order empty space skipping.
     * @param useEss
     */
    void setObjEss(bool useEss);
    void setDrawBox(bool box);
    void setBackgroundColor(const QColor col);
    void setImageSamplingRate(const double samplingRate);
    void setShowOverlay(bool showOverlay);

    void saveFrame();
    void toggleVideoRecording();
    void toggleViewRecording();
	void toggleInteractionLogging();
    void setTimeStep(int timestep);
    void setAmbientOcclusion(bool ao);

    void generateLowResVolume();

    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;

    void showSelectOpenCL();
    void reloadKernels();
    void toggleBenchmark();  //QString logFileName = QString(), int gaze_iterations = -1);

	void do_all_Benchmarks();	// do all the benchmarks for some given volumes and transferfunctions

    void playInteractionSequence(const QString &fileName);
signals:
    void fpsChanged(double);
    void frameSizeChanged(QSize);
    void timeSeriesLoaded(int);

protected:
    // Qt specific QOpenGLWidget methods
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(const int w, const int h) Q_DECL_OVERRIDE; // generates the two GL textures, GL_TEXTURE0 is active afterwards

private:
    void updateViewMatrix();
    void setGazeFromCursorPos(const QPoint &cursorPos);
    void paintOrientationAxis(QPainter &p);
    void paintFps(QPainter &p, const double fps, const double lastTime);
    double getFps(double offset=0.0);

	// Different methods called from within the paintGL()-method
	void paintGL_standard();
	void paintGL_LBG_sampling();

	// Eyetracking
	bool check_eyetracker_availability(bool eyetracking);	// Checks if the currently selected eyetracker (_eyetracker) exists
	static void gaze_data_callback(TobiiResearchGazeData *gaze_data, void *user_data);

    /**
     * @brief Initialize the OpenCL volume renderer.
     * @param useGL use OpenGL context sharing
     * @param useCPU use CPU as OpenCL device for rendering
     */
    void initVolumeRenderer(bool useGL = true, const bool useCPU = false);

	/**
	 * @brief create the OpenGL output texture to display on the screen quad.
	 * @param width Width of the texture in pixels.
	 * @param height Height of the texture in pixels.
	 */
    void generateOutputTextures(const int width, const int height, GLuint* texture, GLuint tex_unit);

	/**
	 * @brief Log camera configurations rotation and zoom) to two files selected by the user.
	 */
    void recordViewConfig() const;

	/**
	 * @brief Log a user interaction to file.
	 * @param str String containing the user interaction in the fowllowing format:
	 *	          time stamp, interaction type, interaction parameters
	 */
	void logInteraction(const QString &str) const;
    void setSequenceStep(QString line);

    // -------Member variables--------
    //
    // OpenGL
    QOpenGLVertexArrayObject _screenQuadVao;
    QOpenGLShaderProgram _spScreenQuad;
    QOpenGLShaderProgram _spOverlaysGL;
    QOpenGLBuffer _quadVbo;

    QMatrix4x4 _screenQuadProjMX;
    QMatrix4x4 _viewMX;
    QMatrix4x4 _modelMX;
    QMatrix4x4 _coordViewMX;
    QMatrix4x4 _overlayProjMX;
    QMatrix4x4 _overlayModelMX;

    QPoint _tffRange;
    GLuint _outTexId;	// TexId for final output Texture, bound to tex_unit 0
	GLuint _tmpTexId;	// TexId for temporary Textures like pre interpolation, bound to tex_unit 1
    VolumeRenderCL _volumerender;
    QEasingCurve _tffInterpol;
    int _timestep;

	// Eyetracking
	TobiiResearchEyeTracker* _eyetracker;	// points to the currently selected eyetracker
	TobiiResearchGazeData _gaze_data;	// holds the latest collected data from the eyetracking callback
	cl_float2 _last_valid_gaze_position;

	// Monitorselection
	QPoint _monitor_offset;	// The offset which is used for the transformation of the gaze point
	int _curr_monitor_width;	// The width of the currently selected monitor which is used for the transformation of the gaze point
	int _curr_monitor_height;	// The height of the currently selected monitor which is used for the transformation of the gaze point

    // global rendering flags
    QPoint _lastLocalCursorPos;
    QQuaternion _rotQuat;
    QVector3D _translation;

	bool _useEyetracking;
    bool _noUpdate;
    bool _loadingFinished;
    bool _writeImage;
    bool _recordVideo;
    qint64 _imgCount;
    QVector<double> _times;
    double _imgSamplingRate;       // image oversampling rate
    bool _useGL;
    bool _showOverlay;
    bool _logView;
	bool _logInteraction;
    QString _viewLogFile;
	QString _interactionLogFile;
    bool _contRendering;
    QGradientStops _tffStops;
	QElapsedTimer _timer;
	RenderingMethod _renderingMethod;	// selects the rendering method which will be called within paintGL()
    QRandomGenerator64 _prng;
    Benchmark _bench;
    QStringList _interactionSequence;
    int _interactionSequencePos = 0;
    bool _playInteraction = false;
};
