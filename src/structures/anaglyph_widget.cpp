// SPDX-License-Identifier: LGPL-3.0-or-later
// SlabRender
// Author: Ivo Filot <ivo@ivofilot.nl>


#include "anaglyph_widget.h"

#include <algorithm>
#include <QPalette>

namespace {

void apply_qt_window_clear_color(QOpenGLWidget* widget) {
    const QColor window_color = widget->palette().color(QPalette::Window);
    glClearColor(window_color.redF(), window_color.greenF(), window_color.blueF(), 1.0f);
}

}

AnaglyphWidget::AnaglyphWidget(QWidget *parent)
    : QOpenGLWidget(parent) {
    const QString operatingsystem = QSysInfo::productType();

    // create shader manager
    this->shader_manager = std::make_unique<ShaderProgramManager>();

    // set camera position
    this->camera_position = QVector3D(0.0, 0.0, 40.0f);

    // load axis models
    this->model_loader = std::make_unique<ModelLoader>();
    this->axes_models.emplace_back(model_loader->load_model(":/assets/models/arrow.obj"));

    // set default matrix orientation on start-up
    this->reset_matrices();
}

/**
 * @brief AnaglyphWidget destructor.
 */
AnaglyphWidget::~AnaglyphWidget() {
    cleanup();
}

/**
 * @brief minimumSizeHint.
 */
QSize AnaglyphWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

/**
 * @brief sizeHint.
 */
QSize AnaglyphWidget::sizeHint() const {
    return QSize(400, 400);
}

/**
 * @brief cleanup.
 */
void AnaglyphWidget::cleanup() {
    makeCurrent();
    this->release_models();
    doneCurrent();
}

/**
 * @brief slot_load_structure.
 */
void AnaglyphWidget::slot_load_structure(int structure_id) {
    if(structure_id < 0) {
        this->set_structure(nullptr);
    } else {
        qDebug() << "Loading structure: " << this->structure_paths[structure_id] << " in AnaglyphWidget";

        auto path = this->structure_paths[structure_id];
        this->set_structure(this->sl.load_file(path).back());
    }
}

void AnaglyphWidget::set_structure(const std::shared_ptr<Structure>& structure) {
    this->structure = structure;
    this->selected_atom = -1;

    if (this->structure) {
        this->structure->update();
        this->pb.set_unitcell(this->structure->get_unitcell());
    }

    this->update();
}

/**
 * @brief      Initialize OpenGL environment
 */
void AnaglyphWidget::initializeGL() {
    qDebug() << "Initialise OpenGL engine.";
    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &AnaglyphWidget::cleanup);

    this->initializeOpenGLFunctions();
    for (unsigned int i = 0; i < this->axes_models.size(); ++i) {
        this->axes_models[i]->load_to_vao();
    }

    apply_qt_window_clear_color(this);

    this->load_shaders();

    // initialize frame buffers
    glGenFramebuffers(2, this->framebuffers);
    glGenTextures(2, this->texture_color_buffers);
    glGenRenderbuffers(2, this->rbo);

    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffers[i]);
        glBindTexture(GL_TEXTURE_2D, this->texture_color_buffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->geometry().width(), this->geometry().height(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->texture_color_buffers[i], 0);

        glBindRenderbuffer(GL_RENDERBUFFER, this->rbo[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, this->geometry().width(), this->geometry().height());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, this->rbo[i]);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            qDebug() << "Framebuffer is not complete.";
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // create screen quad vao
    // TODO maybe move this away from here?
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    this->quad_vao.create();
    this->quad_vao.bind();

    this->quad_vbo.create();
    this->quad_vbo.setUsagePattern(QOpenGLBuffer::StaticDraw);
    this->quad_vbo.bind();
    this->quad_vbo.allocate(quadVertices, sizeof(quadVertices));
    f->glEnableVertexAttribArray(0);
    f->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    f->glEnableVertexAttribArray(1);
    f->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    this->quad_vao.release();

    // build primitive sphere, cylinder and unitcell
    this->pb.build_models();
}

/**
 * @brief      Render scene
 */
void AnaglyphWidget::paintGL() {
    apply_qt_window_clear_color(this);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    glBlendEquation(GL_FUNC_ADD);

    // bind program
    this->shader_manager->get_shader_program("model_shader")->bind();

    static const QVector3D lookat = QVector3D(0.0f, 0.0f, 0.0f); // origin
    static const QVector3D up = QVector3D(0.0, 1.0, 0.0);
    this->view.setToIdentity();
    this->view.lookAt(this->camera_position, lookat, up);
    this->paint_model();

    // early exit if no stereo is active
    this->shader_manager->get_shader_program("model_shader")->release();

    // draw axes on the screen
    if(this->flag_axis_enabled) {
        this->draw_axes();
    }
}

/**
 * @brief      Paint the models in the models vector to the screen
 *
 * Note that the centering of the system is already conducted in either
 * the Structure class for the atoms and bonds or in the PrimitiveBuilder class
 * for the unitcell.
 */
void AnaglyphWidget::paint_model() {
    if(this->structure) {
        // get openGL functions
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

        // draw unitcell
        ShaderProgram *unitcell_shader = this->shader_manager->get_shader_program("unitcell_shader");
        unitcell_shader->bind();

        this->model.setToIdentity();
        this->model.translate(-this->camera_translation);
        this->model *= this->arcball_rotation * this->rotation_matrix;
        this->mvp = this->projection * this->view * this->model;

        unitcell_shader->set_uniform("mvp", this->mvp);
        unitcell_shader->set_uniform("color", QVector3D(0.0f, 0.0f, 0.0f));

        this->pb.get_vao_unitcell()->bind();
        f->glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
        this->pb.get_vao_unitcell()->release();
        unitcell_shader->release();

        // draw particles
        ShaderProgram *model_shader = this->shader_manager->get_shader_program("model_shader");
        model_shader->bind();

        model_shader->set_uniform("view", this->view);
        model_shader->set_uniform("lightpos", QVector3D(0.f, 0.0f, 1000.f));

        auto base = this->model;
        base.setToIdentity();
        base.translate(-this->camera_translation);
        base *= this->arcball_rotation * this->rotation_matrix;

        // render atoms
        this->pb.get_vao_sphere()->bind();
        for(unsigned int i=0; i<this->structure->get_atoms().size(); i++) {
            const Atom& atom = this->structure->get_atom(i);
            const unsigned int atom_index = i + 1; // atom rules are 1-based
            this->model = base;
            this->model.translate(QVector3D(atom.x, atom.y, atom.z));
            this->model.scale(AtomSettings::get().get_atom_radius_from_elnr(atom.atnr, atom_index));
            this->mvp = this->projection * this->view * this->model;
            model_shader->set_uniform("mvp", this->mvp);
            model_shader->set_uniform("model", this->model);
            QVector3D col = AtomSettings::get().get_atom_color_from_elnr(atom.atnr, atom_index);

            if(this->selected_atom >= 0 && this->selected_atom == i) {
                col = (col + QVector3D(1.0, 1.0, 1.0)) / 2.0;
            }

            model_shader->set_uniform("color", QVector4D(col[0],col[1],col[2],1));
            f->glDrawElements(GL_TRIANGLES, this->pb.get_num_vertices_sphere(), GL_UNSIGNED_INT, 0);
        }
        this->pb.get_vao_sphere()->release();

        // render bonds
        this->pb.get_vao_cylinder()->bind();
        for(const Bond& bond: this->structure->get_bonds()) {
            this->model = base;
            this->model.translate(QVector3D(bond.atom1.x, bond.atom1.y, bond.atom1.z));
            this->model.rotate(bond.angle / M_PI * 180.f, QVector3D(bond.axis[0], bond.axis[1], bond.axis[2]));

            float r1 = AtomSettings::get().get_atom_radius_from_elnr(bond.atom1.atnr, bond.atom_id_1);
            float r2 = AtomSettings::get().get_atom_radius_from_elnr(bond.atom2.atnr, bond.atom_id_2);
            float r = std::min(r1,r2) / 2.0f;

            this->model.scale(QVector3D(r, r, bond.length));
            this->mvp = this->projection * this->view * this->model;
            model_shader->set_uniform("mvp", this->mvp);
            model_shader->set_uniform("model", this->model);
            model_shader->set_uniform("color", QVector4D(0.5,0.5,0.5,1));
            f->glDrawElements(GL_TRIANGLES, this->pb.get_num_vertices_cylinder(), GL_UNSIGNED_INT, 0);
        }
        this->pb.get_vao_cylinder()->release();

        model_shader->release();
    }
}

/**
 * @brief      Resize window
 *
 * @param[in]  width   screen width
 * @param[in]  height  screen height
 */
void AnaglyphWidget::resizeGL(int w, int h) {

    float ratio = GLfloat(w) / h;
    float zoom = this->camera_position[2];
    this->projection.setToIdentity();
    this->projection.ortho(-zoom/2.0f, zoom/2.0f, -zoom / ratio /2.0f, zoom / ratio / 2.0f, 0.01f, 1000.0f);

    // store sizes
    this->screen_width = w;
    this->screen_height = h;

    // resize textures and render buffers
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, this->texture_color_buffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glBindRenderbuffer(GL_RENDERBUFFER, this->rbo[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    }
}

/**
 * @brief      Parse mouse press event
 *
 * @param      event  The event
 */
void AnaglyphWidget::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        // enable arcball rotation mode
        this->arcball_rotation_flag = true;

        // store positions of mouse
        this->m_lastPos = event->pos();

        // update angles
        emit(signal_object_angles());
    }

    if(event->button() & Qt::RightButton) {
        QVector3D ray_origin;
        QVector3D ray_direction;
        this->calculate_ray(event->pos(), &ray_origin, &ray_direction);

        if(this->structure) {
            int idx = this->get_atom_raycast(ray_origin, ray_direction);
            if(idx >= 0) {
                if(this->selected_atom == idx) {
                    this->selected_atom = -1;
                } else {
                    this->selected_atom = idx;
                }
                this->update();
                emit(signal_atom_selected(idx));
            }
        }
    }
}

/**
 * @brief      Parse mouse release event
 *
 * @param      event  The event
 */
void AnaglyphWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (this->arcball_rotation_flag && !(event->buttons() & Qt::LeftButton) ) {
        // make arcball rotation permanent (note that the multiplication order for matrices matters)
        this->rotation_matrix = this->arcball_rotation * this->rotation_matrix;

        // reset the arcball rotation matrix
        this->arcball_rotation.setToIdentity();

        // unset arcball rotation mode
        this->arcball_rotation_flag = false;

        // emit update angles
        emit(signal_object_angles());
    }
}

/**
 * @brief      Parse mouse move event
 *
 * @param      event  The event
 */
void AnaglyphWidget::mouseMoveEvent(QMouseEvent *event) {
    if(this->arcball_rotation_flag) { // drag event
        // implementation adapted from
        // https://en.wikibooks.org/wiki/OpenGL_Programming/Modern_OpenGL_Tutorial_Arcball
        if(event->x() != this->m_lastPos.x() || event->y() != this->m_lastPos.y()) {

            // calculate arcball vectors
            QVector3D va = this->get_arcball_vector(this->m_lastPos.x(), this->m_lastPos.y());
            QVector3D vb = this->get_arcball_vector(event->x(), event->y());

            // calculate angle between vectors
            float dotprod = QVector3D::dotProduct(va, vb);
            if (qFabs(dotprod) > 0.9999f) {
                return;
            }
            float angle = qAcos(qMin(1.0f, dotprod));

            // determine rotation vector in camera space
            QVector4D axis_cam_space = QVector4D(QVector3D::crossProduct(va, vb).normalized());

            // calculate matrix to change basis from camera to model space
            QMatrix3x3 camera_to_model_trans = this->view.inverted().toGenericMatrix<3,3>();

            // determine rotation vector in model space
            QVector4D axis_model_space = QMatrix4x4(camera_to_model_trans) * axis_cam_space;

            // set the rotation
            this->set_arcball_rotation(qRadiansToDegrees(angle), axis_model_space);

            // update angles
            emit(signal_object_angles());
        }
    } else if(event->buttons() & Qt::RightButton) {
        // Nothing for the time being.
    }
}

/**
 * @brief      Calculate the arcball vector for mouse rotation
 *
 * @param      int x    x-position of the mouse cursor on the screen
 * @param      int y    y-position of the mouse cursor on the screen
 */
QVector3D AnaglyphWidget::get_arcball_vector(int x, int y) {
    QVector3D P = QVector3D(1.0f * (float)x / (float)this->geometry().width() * 2.0f - 1.0f,
                            1.0f * (float)y / (float)this->geometry().height() * 2.0f - 1.0f,
                            0.0f);
    P[1] = -P[1];

    float OP_squared = P[0] * P[0] + P[1] * P[1];

    if(OP_squared <= 1.0f)
        P[2] = sqrt(1.0f - OP_squared);
    else
        P = P.normalized();
    return P;
}

/**
 * @brief set_arcball_rotation.
 */
void AnaglyphWidget::set_arcball_rotation(float arcball_angle, const QVector4D& arcball_vector) {
    this->arcball_rotation.setToIdentity();
    this->arcball_rotation.rotate(arcball_angle, QVector3D(arcball_vector));
    this->update();
}

/**
 * @brief set_euler_angles.
 */
void AnaglyphWidget::set_euler_angles(const QVector3D& euler_angles) {
    this->arcball_rotation.setToIdentity();
    this->rotation_matrix.setToIdentity();
    this->rotation_matrix.rotate(QQuaternion::fromEulerAngles(euler_angles));
    emit(signal_object_angles());
    this->update();
}

/**
 * @brief set_zoom_level.
 */
void AnaglyphWidget::set_zoom_level(float zoom_level) {
    this->camera_position[2] = std::max(zoom_level, 5.0f);
    float ratio = (float)this->width() / (float)this->height();
    float zoom = this->camera_position[2];
    this->projection.setToIdentity();
    this->projection.ortho(-zoom/2.0f, zoom/2.0f, -zoom / ratio /2.0f, zoom / ratio / 2.0f, 0.01f, 1000.0f);
    emit(signal_zoom_level());
    this->update();
}


/**
 * @brief      Parse mouse wheel event
 *
 * @param      event  The event
 */
void AnaglyphWidget::wheelEvent(QWheelEvent *event) {
    this->camera_position += event->angleDelta().y() * 0.01f * QVector3D(0.0, 0.0, 1.0);
    float ratio = (float)this->width() / (float)this->height();
    float zoom = this->camera_position[2];
    this->projection.setToIdentity();
    this->projection.ortho(-zoom/2.0f, zoom/2.0f, -zoom / ratio /2.0f, zoom / ratio / 2.0f, 0.01f, 1000.0f);

    // prevent camera position from becoming positive or from zooming in too far
    if(this->camera_position[2] < 5.0) {
        this->camera_position[2] = 5.0;
    }

    emit(signal_zoom_level());
    this->update();
}

/**
 * @brief update.
 */
void AnaglyphWidget::update() {
    QOpenGLWidget::update();
    emit(signal_object_angles());

}

/**
 * @brief process_input.
 */
void AnaglyphWidget::process_input() {
    // also apply a z-axis rotation if rotation is enabled
    if(this->flag_rotation) {
        this->rotation_matrix.rotate(0.3, QVector3D(0,0,1));
    }

    QMatrix4x4 mrotation;
    mrotation.setToIdentity();

    this->rotation_matrix = mrotation * this->rotation_matrix;

    this->update();
}

/**
 * @brief      Load OpenGL shaders
 */
void AnaglyphWidget::load_shaders() {
    // create regular shaders
    shader_manager->create_shader_program("model_shader", ShaderProgramType::ModelShader, ":/assets/shaders/phong.vs", ":/assets/shaders/phong.fs");
    shader_manager->create_shader_program("axes_shader", ShaderProgramType::AxesShader, ":/assets/shaders/axes.vs", ":/assets/shaders/axes.fs");
    shader_manager->create_shader_program("unitcell_shader", ShaderProgramType::UnitcellShader, ":/assets/shaders/line.vs", ":/assets/shaders/line.fs");

    // create shaders for the stereographic projections
    shader_manager->create_shader_program("stereo_anaglyph_red_cyan", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_anaglyph_red_cyan.fs");
    shader_manager->create_shader_program("stereo_interlaced_checkerboard_lr", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_interlaced_checkerboard_lr.fs");
    shader_manager->create_shader_program("stereo_interlaced_checkerboard_rl", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_interlaced_checkerboard_rl.fs");
    shader_manager->create_shader_program("stereo_interlaced_columns_lr", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_interlaced_columns_lr.fs");
    shader_manager->create_shader_program("stereo_interlaced_columns_rl", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_interlaced_columns_rl.fs");
    shader_manager->create_shader_program("stereo_interlaced_rows_lr", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_interlaced_rows_lr.fs");
    shader_manager->create_shader_program("stereo_interlaced_rows_rl", ShaderProgramType::StereoscopicShader, ":/assets/shaders/stereo.vs", ":/assets/shaders/stereo_interlaced_rows_rl.fs");
}

/**
 * @brief      Release models
 */
void AnaglyphWidget::release_models() {

}

/**
 * @brief window_move_event.
 */
void AnaglyphWidget::window_move_event() {
    this->top_left = mapToGlobal(QPoint(0, 0));
    this->update();
}

/**
 * @brief set_stereo.
 */
void AnaglyphWidget::set_stereo(QString stereo_name) {
    if (!stereo_name.isNull()) {
        // set stereoscopic projection
        this->stereographic_type_name = stereo_name;
    } else {
        // no stereoscopic projection
        this->stereographic_type_name = "NONE";
    }

    this->update();
}

/**
 * @brief      Draw world axes in bottom-right of the screen
 */
void AnaglyphWidget::draw_axes() {
    ShaderProgram* axes_shader = this->shader_manager->get_shader_program("axes_shader");
    axes_shader->bind();

    const QVector3D red(98.8f/100.0f, 20.8f/100.0f, 32.5f/100.0f);
    const QVector3D green(54.9f/100.0f, 86.7f/100.0f, 0.0f);
    const QVector3D blue(15.7f/100.0f, 60.0f/100.0f, 100.0f/100.0f);

    // set view port, projection and view matrices
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
    f->glViewport(0.75f * this->screen_width, 0.0f, this->screen_width * 0.25f, this->screen_height * 0.25f);

    QMatrix4x4 projection_ortho;
    projection_ortho.setToIdentity();
    float ratio = (float)this->screen_height / (float)this->screen_width;
    static const float sz = 25.0f;
    projection_ortho.ortho(-sz, sz, -sz * ratio, sz * ratio, 0.1f, 1000.0f);

    this->view.setToIdentity();
    const QVector3D axes_camera_position(0.0f, 0.0f, 10.0f);
    this->view.lookAt(axes_camera_position, QVector3D(0.0, 0.0, 0.0), QVector3D(0.0, 1.0, 0.0));
    axes_shader->set_uniform("view", this->view);
    axes_shader->set_uniform("light_pos", axes_camera_position);
    QMatrix4x4 axis_rotation;
    this->model.setToIdentity();

    // *******************
    // draw the three axes
    // *******************

    // z-axis
    axis_rotation.setToIdentity();
    this->model = this->arcball_rotation * this->rotation_matrix * axis_rotation;
    this->mvp = projection_ortho * this->view * this->model;
    axes_shader->set_uniform("model", this->model);
    axes_shader->set_uniform("mvp", this->mvp);
    axes_shader->set_uniform("color", blue);
    this->axes_models[0]->draw();

    // y-axis
    axis_rotation.setToIdentity();
    axis_rotation.rotate(-90.0f, QVector3D(1.0, 0.0, 0.0));
    this->model = this->arcball_rotation * this->rotation_matrix * axis_rotation;
    this->mvp = projection_ortho * this->view * this->model;
    axes_shader->set_uniform("model", this->model);
    axes_shader->set_uniform("mvp", this->mvp);
    axes_shader->set_uniform("color", green);
    this->axes_models[0]->draw();

    // x-axis
    axis_rotation.setToIdentity();
    axis_rotation.rotate(90.0f, QVector3D(0.0, 1.0, 0.0));
    this->model = this->arcball_rotation * this->rotation_matrix * axis_rotation;
    this->mvp = projection_ortho * this->view * this->model;
    axes_shader->set_uniform("model", this->model);
    axes_shader->set_uniform("mvp", this->mvp);
    axes_shader->set_uniform("color", red);
    this->axes_models[0]->draw();

    axes_shader->release();
}

/**
 * @brief      Reset rotation matrices
 */
void AnaglyphWidget::reset_matrices() {
    this->rotation_matrix.setToIdentity();
    //this->rotation_matrix.rotate(20.0, QVector3D(1,0,0));
    //this->rotation_matrix.rotate(30.0, QVector3D(0,0,1));
    this->arcball_rotation.setToIdentity();
}

/**
 * @brief       calculate a ray originating based on mouse position and current view
 *
 * @param       mouse position
 * @param       pointer to vector holding ray origin
 * @param       pointer to vector holding ray direction
 * @return      void
 */
void AnaglyphWidget::calculate_ray(const QPoint& mouse_position, QVector3D* ray_origin, QVector3D* ray_direction) {
    const float screen_width = (float)this->width();
    const float screen_height = (float)this->height();

    const QVector3D ray_nds = QVector3D((2.0f * (float)mouse_position.x()) / screen_width - 1.0f,
                                         1.0f - (2.0f * (float)mouse_position.y()) / screen_height,
                                         1.0);

    // this is for PERSPECTIVE PROJECTION
    //    const QVector4D ray_clip(ray_nds[0], ray_nds[1], -1.0, 1.0);

    //    QVector4D ray_eye = this->projection.inverted() * ray_clip;
    //    ray_eye = QVector4D(ray_eye[0], ray_eye[1], -1.0, 0.0);
    //    *ray_direction = (this->view.inverted() * ray_eye).toVector3D().normalized();

    //    // the origin of the ray in perspective projection is simply the position
    //    // of the camera in world space
    //    *ray_origin = this->camera_position;

    // THIS IS FOR ORTHOGRAPHIC PROJECTION
    const QVector4D ray_clip(ray_nds[0], ray_nds[1], 0.0, 1.0);

    // the position on the 'camera screen' determines the origin of the
    // ray vector in orthographic projection
    QVector4D ray_eye = this->projection.inverted() * ray_clip;
    ray_eye = QVector4D(ray_eye[0], ray_eye[1], 0.0, 0.0);
    *ray_origin = (this->camera_position + this->view.inverted() * ray_eye).toVector3D();

    // if the projection is orthographic, the ray vector is the same
    // as the view direction of the camera (in world space)
    *ray_direction = -this->camera_position.normalized();
}

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
int AnaglyphWidget::get_atom_raycast(const QVector3D& ray_origin, const QVector3D& ray_vector) {
    int selected_atom = -1;
    float z = -1000;

    QMatrix4x4 model;

    auto base = this->model;
    base.setToIdentity();
    base.translate(-this->camera_translation);
    base *= this->arcball_rotation * this->rotation_matrix;

    for(unsigned int i=0; i<this->structure->get_atoms().size(); i++) {
        const Atom& atom = this->structure->get_atom(i);

        auto p = atom.get_pos();
        QVector3D pos = base.map(QVector3D(p[0], p[1], p[2]));

        const unsigned int atom_index = i + 1; // atom rules are 1-based
        float radius = AtomSettings::get().get_atom_radius_from_elnr(atom.atnr, atom_index);
        float b = QVector3D::dotProduct(ray_vector, ray_origin - pos);
        float c = QVector3D::dotProduct(ray_origin - pos, ray_origin - pos) - (radius * radius);

        if(b*b >= c ) { // hit
            if(pos[2] > z) {
                selected_atom = i;
                z = pos[2];
            }
        }
    }

    return selected_atom;
}
