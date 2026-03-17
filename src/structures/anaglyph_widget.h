// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QString>
#include <QGuiApplication>
#include <QScreen>
#include <QSysInfo>
#include <QDebug>
#include <QTimer>
#include <QElapsedTimer>

#include "qvector3d.h"
#include "qvector2d.h"
#include "qvector4d.h"
#include <QtCore/qmath.h>
#include <QtCore/qvariant.h>

#include <memory>
#include <sstream>
#include <fstream>
#include <math.h>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/norm.hpp>

#include "model.h"
#include "model_loader.h"
#include "shader_program_manager.h"
#include "shader_program_types.h"
#include "primitivebuilder.h"
#include "structure_loader.h"
#include "atom_settings.h"

QT_FORWARD_DECLARE_CLASS(QOpenGLShaderProgram)

class AnaglyphWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

private:
    QPoint m_lastPos;

    QString root_path;

    std::unique_ptr<ShaderProgramManager> shader_manager;
    std::unique_ptr<ModelLoader> model_loader;
    std::vector<std::unique_ptr<Model>> axes_models;

    QPoint top_left;

    unsigned int framebuffers[2];
    unsigned int texture_color_buffers[2];
    unsigned int rbo[2];

    QOpenGLVertexArrayObject quad_vao;
    QOpenGLBuffer quad_vbo;

    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 rotation_matrix;         // stores rotation state of object
    QMatrix4x4 model;
    QMatrix4x4 mvp;
    QVector3D camera_position;
    QVector3D camera_translation;
    float unitcell_scale = 1.0;

    QMatrix4x4 arcball_rotation;        // temporary rotation when dragging with mouse
    bool arcball_rotation_flag = false; // whether arcball rotation is active

    QOpenGLVertexArrayObject vao;
    QOpenGLBuffer vbo[4];

    int screen_width;
    int screen_height;

    bool flag_axis_enabled = true;                  // whether to enable display of axes
    bool flag_rotation = false;                     // whether to enable display of axes

    // stereographic projections
    bool flag_stereographic_projection = false;     // whether stereographic rendering is used
    QString stereographic_type_name = "NONE";

    PrimitiveBuilder pb;
    StructureLoader sl;

    // current structure to display
    std::shared_ptr<Structure> structure;

    // list of paths to structures
    QStringList structure_paths;

    int selected_atom = -1;
    std::vector<std::vector<QVector3D>> vibration_modes_{};
    std::vector<double> vibration_frequencies_{};
    int active_vibration_mode_index_{-1};
    QTimer vibration_timer_{};
    QElapsedTimer vibration_clock_{};

public:
   /**
    * @brief AnaglyphWidget.
    */
    AnaglyphWidget(QWidget *parent = 0);

    /**
     * @brief set_object.
     */
    double set_object(const std::string& filename);

    /**
     * @brief set_structure_paths.
     */
    inline void set_structure_paths(const QStringList& _structure_paths) {
        this->structure_paths = _structure_paths;
    }

    /**
     * @brief get_structure.
     */
    inline const auto& get_structure() const {
        return this->structure;
    }

    /**
     * @brief Set structure directly (used by cached preview loading).
     */
    void set_structure(const std::shared_ptr<Structure>& structure);
    void set_vibration_modes(const std::vector<std::vector<QVector3D>>& modes, const std::vector<double>& frequencies);
    void set_active_vibration_mode(int mode_index);

    /**
     * @brief get_euler_angles.
     */
    inline QVector3D get_euler_angles() const {
        /**
         * @brief fromRotationMatrix.
         */
        return QQuaternion::fromRotationMatrix((this->arcball_rotation*this->rotation_matrix).normalMatrix()).toEulerAngles();
    }

    /**
     * @brief      Paint the models in the models vector to the screen
     *
     * Note that the centering of the system is already conducted in either
     * the Structure class for the atoms and bonds or in the PrimitiveBuilder class
     * for the unitcell.
     */
    void paint_model();

    /**
     * @brief window_move_event.
     */
    void window_move_event();

    /**
     * @brief set_stereo.
     */
    void set_stereo(QString stereo_name);

    QSize minimumSizeHint() const Q_DECL_OVERRIDE;

    QSize sizeHint() const Q_DECL_OVERRIDE;

    /**
     * @brief      Toggle the display of the world axes
     */
    inline void toggle_world_axes() {
        this->flag_axis_enabled = !flag_axis_enabled;
        this->update();
    }

    /**
     * @brief      Toggle the display of the world axes
     */
    inline void toggle_rotation_z() {
        this->flag_rotation = !flag_rotation;
    }

    /**
     * @brief      Whether world axes are being displayed
     *
     * @return     The flag world axes.
     */
    inline bool get_flag_world_axes() const {
        return this->flag_axis_enabled;
    }

    /**
     * @brief      Get whether model is rotating
     *
     * @return     The flag rotation.
     */
    inline bool get_flag_rotation() const {
        return this->flag_rotation;
    }

    /**
     * @brief      Stops a rotation.
     */
    inline void stop_rotation() {
        this->flag_rotation = false;
    }

    /**
     * @brief set_unitcell_scale.
     */
    inline void set_unitcell_scale(float _scale) {
        this->unitcell_scale = _scale;
    }

    /**
     * @brief get_camera_position.
     */
    inline const QVector3D& get_camera_position() const {
        return this->camera_position;
    }

    /**
     * @brief set_euler_angles.
     */
    void set_euler_angles(const QVector3D& euler_angles);

    /**
     * @brief set_zoom_level.
     */
    void set_zoom_level(float zoom_level);

    /**
     * @brief AnaglyphWidget.
     */
    ~AnaglyphWidget();

    /**
     * @brief Update render
     */
    void update();

public slots:
    /**
     * @brief cleanup.
     */
    void cleanup();

    /**
     * @brief slot_load_structure.
     */
    void slot_load_structure(int structure_id);

signals:
    /**
     * @brief frameNumberChanged.
     */
    void frameNumberChanged(size_t frames);

    /**
     * @brief signal_atom_selected.
     */
    void signal_atom_selected(int atom_id);

    /**
     * @brief signal_object_angles.
     */
    void signal_object_angles();

    /**
     * @brief signal_zoom_level.
     */
    void signal_zoom_level();

protected:
    /**
     * @brief      Initialize OpenGL environment
     */
    void initializeGL() Q_DECL_OVERRIDE;

    /**
     * @brief      Render scene
     */
    void paintGL() Q_DECL_OVERRIDE;

    /**
     * @brief      Resize window
     *
     * @param[in]  width   screen width
     * @param[in]  height  screen height
     */
    void resizeGL(int width, int height) Q_DECL_OVERRIDE;

    /**
     * @brief      Parse mouse press event
     *
     * @param      event  The event
     */
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    /**
     * @brief      Parse mouse release event
     *
     * @param      event  The event
     */
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    /**
     * @brief      Parse mouse move event
     *
     * @param      event  The event
     */
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    /**
     * @brief      Calculate the arcball vector for mouse rotation
     *
     * @param[in]  x, y  The mouse position
     * @param[out] P  The arcball vector
     */
    QVector3D get_arcball_vector(int x, int y);

    /**
     * @brief      Set the arcball vector rotation (angle and vector) to model and updates
     *
     * @param      arcball_angle, arcball_vector
     */
    void set_arcball_rotation(float arcball_angle, const QVector4D& arcball_vector);

    /**
     * @brief      Parse mouse wheel event
     *
     * @param      event  The event
     */
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;

private:
    /**
     * @brief      Load OpenGL shaders
     */
    void load_shaders();

    /**
     * @brief      Release models
     */
    void release_models();

    /**
     * @brief      Draw world axes in bottom-right of the screen
     */
    void draw_axes();

    /**
     * @brief      Reset rotation matrices
     */
    void reset_matrices();

    /**
     * @brief calculate_ray.
     */
    void calculate_ray(const QPoint& mouse_position, QVector3D* ray_origin, QVector3D* ray_direction);

    /**
     * @brief Select the atom using a raycast
     * @param ray_origin
     * @param ray_vector
     *
     * Note that this implementation depends on the direction the camera
     * is looking. This app has the camera located in the +Z direction so
     * selection should be based on largest z-value.
     *
     * @return index of the atom
     */
    /**
     * @brief get_atom_raycast.
     */
    int get_atom_raycast(const QVector3D& ray_origin, const QVector3D& ray_vector);

private slots:
    /**
     * @brief process_input.
     */
    void process_input();
};
