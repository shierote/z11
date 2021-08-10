#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include "helper.h"
#include "z11-client-protocol.h"

char *vertex_shader;
const char *fragment_shader;
static char *create_vertex_shader_with_model_matrix(int dx, int dy, int dz);

int main(int argc, char const *argv[])
{
  int dx = 0;
  int dy = 0;
  int dz = 20;
  int face_count;
  Face *raw_faces;

  if (argc <= 1) {
    fprintf(stderr, "help:\n\t%s <filename>\n", argv[0]);
    exit(1);
  }
  if (argc > 2) dx = atoi(argv[2]);
  if (argc > 3) dy = atoi(argv[3]);
  if (argc > 4) dz = atoi(argv[4]);
  vertex_shader = create_vertex_shader_with_model_matrix(dx, dy, dz);

  raw_faces = z_helper_stl(argv[1], &face_count);
  if (raw_faces == NULL) exit(1);
  fprintf(stderr, "Loaded STL file (faces count: %d)\n", face_count);

  struct z11_global *global = z_helper_global();

  int size_of_faces = sizeof(Face) * face_count;
  int mem_size = size_of_faces;
  int fd = create_shared_fd(mem_size);
  void *shm_data = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (shm_data == MAP_FAILED) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
    fprintf(stderr, "line mmap failed: %m\n");
#pragma GCC diagnostic pop
    close(fd);
    exit(1);
  }

  Face *shm_faces = (Face *)shm_data;
  memcpy(shm_faces, raw_faces, sizeof(Face) * face_count);

  struct wl_shm_pool *pool = wl_shm_create_pool(global->shm, fd, size_of_faces);
  struct wl_raw_buffer *face_buffer = wl_shm_pool_create_raw_buffer(pool, 0, size_of_faces);
  wl_shm_pool_destroy(pool);

  struct z11_gl_vertex_buffer *face_vertex_buffer = z11_gl_create_vertex_buffer(global->gl);

  struct z11_gl_shader_program *shader_program =
      z11_gl_create_shader_program(global->gl, vertex_shader, fragment_shader);

  struct z11_render_element *render_element = z11_compositor_create_render_element(global->compositor);
  z11_render_element_attach_vertex_buffer(render_element, face_vertex_buffer, sizeof(Point));
  z11_render_element_attach_shader_program(render_element, shader_program);
  z11_render_element_append_vertex_input_attribute(render_element, 0,
                                                   Z11_GL_VERTEX_INPUT_ATTRIBUTE_FORMAT_FLOAT_VECTOR3, 0);

  z11_render_element_set_topology(render_element, Z11_GL_TOPOLOGY_TRIANGLES);

  z11_gl_vertex_buffer_allocate(face_vertex_buffer, size_of_faces, face_buffer);
  z11_render_element_commit(render_element);

  int ret;
  while (wl_display_dispatch_pending(global->display) != -1) {
    ret = wl_display_flush(global->display);
    if (ret == -1) break;
  }
}

static char *create_vertex_shader_with_model_matrix(int dx, int dy, int dz)
{
  char *shader = (char *)malloc(sizeof(char) * 400);
  sprintf(shader,
          (  //
              "#version 410\n"
              "uniform mat4 matrix;\n"
              "uniform mat4 model;\n"
              "layout(location = 0) in vec4 position;\n"
              "layout(location = 1) in vec2 v2UVcoordsIn;\n"
              "layout(location = 2) in vec3 v3NormalIn;\n"
              "out vec2 v2UVcoords;\n"
              "void main()\n"
              "{\n"
              "  mat4 model = mat4(\n"
              "  1, 0, 0, 0,\n"
              "  0, 0, 1, 0,\n"
              "  0, 1, 0, 0,\n"
              "  %d, %d, %d, 1\n"
              "  );\n"
              "  v2UVcoords = v2UVcoordsIn;\n"
              "  gl_Position = matrix * model * position;\n"
              "}\n"  //
              ),
          dx, dy, dz);
  return shader;
}
const char *fragment_shader =
    "#version 410 core\n"
    "in vec2 v2UVcoords;\n"
    "out vec4 outputColor;\n"
    "void main()\n"
    "{\n"
    "  outputColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
    "}\n";
