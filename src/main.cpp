#include <adwaita.h>
#include <epoxy/gl.h>
#include <gtk/gtk.h>
#include <stdio.h>

static GtkWidget* gl_area = NULL;


static void close_window(GtkWidget* widget) {
    /* Reset the state */
    gl_area = NULL;
}

/* Initialize the GL buffers */
static void init_buffers(GLuint* vao_out, GLuint* buffer_out) {
    GLuint vao, vertex_buffer;

    // from simple polyhedron made with Wings 3d and saved to .obj file
    GLfloat vertex_base[] = {
            -1.f, -1.f,
            -1.f,  1.f,
             1.f,  1.f,
             1.f,  -1.f,
            };

    // We only use one VAO, so we always keep it bound
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);


    // vertices
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_base), vertex_base, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (vao_out != NULL)
        *vao_out = vao;

    if (buffer_out != NULL)
        *buffer_out = vertex_buffer;
}

/* Create and compile a shader */
static GLuint create_shader(int type, const char* src) {
    GLuint shader;
    int status;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        char* buffer;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        buffer = static_cast<char*>(g_malloc(log_len + 1));
        glGetShaderInfoLog(shader, log_len, NULL, buffer);

        g_warning("Compile failure in %s shader:\n%s",
                  type == GL_VERTEX_SHADER ? "vertex" : "fragment",
                  buffer);

        g_free(buffer);

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}

/* Initialize the shaders and link them into a program */
static void init_shaders(GLuint* program_out) {
    GLuint vertex, fragment;
    GLuint program = 0;
    int status;
    /*
  //gl_Position = projection_matrix * view_matrix * model_matrix * vec4(in_position, 1);\
  //vec3 normal_cameraspace = normalize(( view_matrix * model_matrix * vec4(in_normal,0)).xyz);\
  //vec3 cameraVector = normalize(vec3(0, 0, 0) - (view_matrix * model_matrix * vec4(in_position, 1)).xyz);\
  //float cosTheta = clamp( dot( normal_cameraspace, cameraVector ), 0, 1 );\
  //color = vec4(0.3 * in_color.rgb + cosTheta * in_color.rgb, 1);\
*/
    vertex = create_shader(GL_VERTEX_SHADER,
                           "#version 300 es                             \n"
                           "layout(location = 0) in vec2 a_position;    \n"
                           "void main()                                 \n"
                           "{                                           \n"
                           "   gl_Position = vec4(a_position, 0.0, 1.0);\n"
                           "}                                           \n");

    if (vertex == 0) {
        *program_out = 0;
        return;
    }

    fragment = create_shader(GL_FRAGMENT_SHADER,
                             "#version 300 es                                       \n"
                             "precision mediump float;                              \n"
                             "layout(location = 0) out vec4 outColor;               \n"
                             "uniform sampler2D s_texture;                          \n"
                             "void main()                                           \n"
                             "{                                                     \n"
                             "  float v = mod(gl_FragCoord.x + gl_FragCoord.y, 2.0);\n"
                             "  outColor = vec4(v, v, v, 1.0);                      \n"
                             "}                                                     \n");

    if (fragment == 0) {
        glDeleteShader(vertex);
        *program_out = 0;
        return;
    }

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        int log_len;
        char* buffer;

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

        buffer = static_cast<char*>(g_malloc(log_len + 1));
        glGetProgramInfoLog(program, log_len, NULL, buffer);

        g_warning("Linking failure:\n%s", buffer);

        g_free(buffer);

        glDeleteProgram(program);
        program = 0;

        goto out;
    }

    /* Get the location of the "mvp" uniform */
    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

out:
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    if (program_out != NULL)
        *program_out = program;
}

static GLuint position_buffer;
static GLuint program;

/* We need to set up our state when we realize the GtkGLArea widget */
static void realize(GtkWidget* widget) {
    gtk_gl_area_make_current(GTK_GL_AREA(widget));

    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL)
        return;

    init_buffers(NULL, &position_buffer);
    init_shaders(&program);
}

/* We should tear down the state when unrealizing */
static void unrealize(GtkWidget* widget) {
    gtk_gl_area_make_current(GTK_GL_AREA(widget));

    if (gtk_gl_area_get_error(GTK_GL_AREA(widget)) != NULL)
        return;

    glDeleteBuffers(1, &position_buffer);
    glDeleteProgram(program);
}

static void draw_object(void) {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Use our shaders */
    glUseProgram(program);

    /* Use the vertices in our buffer */
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, position_buffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);


    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);

    /* We finished using the buffers and program */
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}


static gboolean render(GtkGLArea* area, GdkGLContext* context) {
    if (gtk_gl_area_get_error(area) != NULL)
        return FALSE;


    draw_object();

    glFlush();

    return TRUE;
}


static GtkWidget* window;

static void activate(GtkApplication* app, gpointer user_data) {
    GtkWidget *box;

//    window = gtk_application_window_new(app);
    window = adw_application_window_new(app);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 600);
    gtk_window_set_title(GTK_WINDOW(window), "Volume");
    g_signal_connect(window, "destroy", G_CALLBACK(close_window), NULL);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
    gtk_widget_set_margin_start(box, 0);
    gtk_widget_set_margin_end(box, 0);
    gtk_widget_set_margin_top(box, 0);
    gtk_widget_set_margin_bottom(box, 0);
    gtk_box_set_spacing(GTK_BOX(box), 6);
//    gtk_window_set_child(GTK_WINDOW(window), box);

    auto toolbar = adw_toolbar_view_new();

    auto header = adw_header_bar_new();
    adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar), header);
    adw_toolbar_view_set_extend_content_to_top_edge(ADW_TOOLBAR_VIEW(toolbar), true);
    adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar), box);
//    adw_toolbar_view_set_top_bar_style(ADW_TOOLBAR_VIEW(toolbar), AdwToolbarStyle::ADW_TOOLBAR_RAISED_BORDER);

    adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), toolbar);

    gl_area = gtk_gl_area_new();
    gtk_widget_set_hexpand(gl_area, TRUE);
    gtk_widget_set_vexpand(gl_area, TRUE);
    gtk_widget_set_size_request(gl_area, 100, 200);
    gtk_box_append(GTK_BOX(box), gl_area);

    // We need to initialize and free GL resources, so we use
    // the realize and unrealize signals on the widget
    //
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), NULL);

    // The main "draw" call for GtkGLArea
    g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

//    gtk_application_set_menubar(GTK_APPLICATION(app), G_MENU_MODEL(menu_bar));
//    gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(window), TRUE);


//    GtkWidget* titlebar = gtk_header_bar_new();

//    auto t = adw_window_title_new("Title", "Subtitle");

    // https://github.com/GNOME/libadwaita/blob/main/examples/hello-world/hello.c

    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char** argv) {
    GtkApplication* app;
    int res;

    auto adw = adw_application_new(nullptr, G_APPLICATION_DEFAULT_FLAGS);
    auto v1 = gtk_get_major_version();
    auto v2 = gtk_get_minor_version();
//    app = gtk_application_new(NULL, G_APPLICATION_DEFAULT_FLAGS);
    app = GTK_APPLICATION(adw);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    res = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return res;
}
