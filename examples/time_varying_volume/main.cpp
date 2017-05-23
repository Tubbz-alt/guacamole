/******************************************************************************
 * guacamole - delicious VR                                                   *
 *                                                                            *
 * Copyright: (c) 2011-2013 Bauhaus-Universität Weimar                        *
 * Contact:   felix.lauer@uni-weimar.de / simon.schneegans@uni-weimar.de      *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify it    *
 * under the terms of the GNU General Public License as published by the Free *
 * Software Foundation, either version 3 of the License, or (at your option)  *
 * any later version.                                                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful, but        *
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY *
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License   *
 * for more details.                                                          *
 *                                                                            *
 * You should have received a copy of the GNU General Public License along    *
 * with this program. If not, see <http://www.gnu.org/licenses/>.             *
 *                                                                            *
 ******************************************************************************/

#include <functional>

#include <gua/guacamole.hpp>
#include <gua/renderer/TV_3Loader.hpp>
#include <gua/renderer/TV_3VolumePass.hpp>
#include <gua/renderer/TV_3SurfacePass.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/node/TV_3Node.hpp>
#include <gua/renderer/ToneMappingPass.hpp>
#include <gua/renderer/DebugViewPass.hpp>
#include <gua/utils/Trackball.hpp>

// forward mouse interaction to trackball
void mouse_button(gua::utils::Trackball& trackball,
                  int mousebutton,
                  int action,
                  int mods) {
  gua::utils::Trackball::button_type button;
  gua::utils::Trackball::state_type state;

  switch (mousebutton) {
    case 0:
      button = gua::utils::Trackball::left;
      break;
    case 2:
      button = gua::utils::Trackball::middle;
      break;
    case 1:
      button = gua::utils::Trackball::right;
      break;
  };

  switch (action) {
    case 0:
      state = gua::utils::Trackball::released;
      break;
    case 1:
      state = gua::utils::Trackball::pressed;
      break;
  };

  trackball.mouse(button, state, trackball.posx(), trackball.posy());
}

int main(int argc, char** argv) {


  std::string in_vol_resource_path = "/mnt/pitoti/MA_Adrian/supernova_parts.v_rsc";
  std::string in_vol_resource_path2 = "/home/wabi7015/Programming/tv_3/resources/volume_data/head_w256_h256_d225_c1_b8.raw";
  // initialize guacamole
  gua::init(argc, argv);


  // setup scene
  gua::SceneGraph graph("main_scenegraph");

  auto transform = graph.add_node<gua::node::TransformNode>("/", "transform");
  auto plane_transform = graph.add_node<gua::node::TransformNode>("/", "plane_transform");
  gua::TriMeshLoader loader;


  auto plane(loader.create_geometry_from_file(
      "plane", "data/objects/plane.obj",
      gua::TriMeshLoader::NORMALIZE_POSITION |
          gua::TriMeshLoader::NORMALIZE_SCALE));
  graph.add_node("/plane_transform", plane);
  plane->scale(10.0f, 10.0, 10.0);
  plane->rotate(20.0f, 1.0, 0.0, 0.0);
  plane->translate(0.0, 0.0, -10.0);
/*
  teapot->set_draw_bounding_box(true);
*/





  gua::TV_3Loader tv_3_loader;
  auto test_volume(tv_3_loader.load_geometry(
      "test_volume",
       //"./data/objects/Bucky_uncertainty_data_w32_h32_d32_c1_b8.raw",
       in_vol_resource_path2,
       //"/mnt/pitoti/MA_Adrian/Supernova/Supernova_t1317_w432_h432_d432_b32_c1.raw",
      //"/mnt/pitoti/MA_Adrian/Supernova_w432_h432_d432_c1_b32.raw",
      //"/mnt/pitoti/MA_Adrian/16_bit_downsampled_adrian/downsampled_16_bit_t24_w716_h695_d283_c1_b16.raw",
      //"/mnt/data_internal/volume_data/medical/reptile_ct/16bitcoronal_w1024_h1024_d1080_c1_b16.raw",
      //"/home/wabi7015/Programming/tv_3/resources/volume_data/head.v_rsc",
      gua::TV_3Loader::NORMALIZE_POSITION |
      gua::TV_3Loader::NORMALIZE_SCALE));
  graph.add_node("/transform", test_volume);
  test_volume->set_draw_bounding_box(true);


//  reinterpret_cast<gua::node::TV_3Node*>(test_volume.get())->register_clipping_geometry(std::shared_ptr<gua::node::TriMeshNode>(reinterpret_cast<gua::node::TriMeshNode*>(teapot.get()) ) );

/*
  auto portal = graph.add_node<gua::node::TexturedQuadNode>("/", "portal");
  portal->data.set_size(gua::math::vec2(1.2f, 0.8f));
  portal->data.set_texture("portal");
  portal->translate(0.5f, 0.f, -0.2f);
  portal->rotate(-30, 0.f, 1.f, 0.f);
*/
  auto light2 = graph.add_node<gua::node::LightNode>("/", "light2");
  light2->data.set_type(gua::node::LightNode::Type::SPOT);
  light2->data.brightness = 100.0f;
  light2->scale(25.f);
  //light2->rotate(-90.0f, 1.0f, 0.0f, 0.0f);
  light2->translate(0.0f, 0.0f, 5.0f);

  light2->data.set_enable_shadows(true);                                                         
  light2->data.set_shadow_map_size(4096);

  light2->data.set_shadow_near_clipping_in_sun_direction(0.1f);
  light2->data.set_shadow_far_clipping_in_sun_direction(100.f);



  auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
  screen->data.set_size(gua::math::vec2(1.92f, 1.08f));
  screen->translate(0, 0, 1.0);
/*
  auto portal_screen =
      graph.add_node<gua::node::ScreenNode>("/", "portal_screen");
  portal_screen->translate(0.0, 0.0, 5.0);
  portal_screen->rotate(90, 0.0, 1.0, 0.0);
  portal_screen->data.set_size(gua::math::vec2(1.2f, 0.8f));
*/
  // add mouse interaction
  gua::utils::Trackball trackball(0.01, 0.002, 0.2);

  // setup rendering pipeline and window
  auto resolution = gua::math::vec2ui(1920, 1080);
/*
  auto portal_camera =
      graph.add_node<gua::node::CameraNode>("/portal_screen", "portal_cam");
  portal_camera->translate(0, 0, 2.0);
  portal_camera->config.set_resolution(gua::math::vec2ui(1200, 800));
  portal_camera->config.set_screen_path("/portal_screen");
  portal_camera->config.set_scene_graph_name("main_scenegraph");
  portal_camera->config.set_output_texture_name("portal");
  portal_camera->config.set_enable_stereo(false);
*/
  auto portal_pipe = std::make_shared<gua::PipelineDescription>();
  portal_pipe->add_pass(std::make_shared<gua::TriMeshPassDescription>());
  portal_pipe->add_pass(std::make_shared<gua::TV_3SurfacePassDescription>());
  portal_pipe->add_pass(
      std::make_shared<gua::LightVisibilityPassDescription>());

  auto resolve_pass = std::make_shared<gua::ResolvePassDescription>();
  resolve_pass->background_mode(
      gua::ResolvePassDescription::BackgroundMode::QUAD_TEXTURE);
  resolve_pass->tone_mapping_exposure(1.0f);

  portal_pipe->add_pass(resolve_pass);
  //portal_pipe->add_pass(std::make_shared<gua::TV_3SurfacePassDescription>());
  portal_pipe->add_pass(std::make_shared<gua::DebugViewPassDescription>());

  //portal_camera->set_pipeline_description(portal_pipe);

  auto camera = graph.add_node<gua::node::CameraNode>("/screen", "cam");
  camera->translate(0, 0, 10.0);
  camera->config.set_resolution(resolution);
  camera->config.set_screen_path("/screen");
  camera->config.set_scene_graph_name("main_scenegraph");
  camera->config.set_output_window_name("main_window");
  camera->config.set_enable_stereo(false);
  camera->set_pipeline_description(portal_pipe);
  //camera->set_pre_render_cameras({portal_camera});

  camera->get_pipeline_description()->get_resolve_pass()->tone_mapping_exposure(
    1.0f);
  camera->get_pipeline_description()->add_pass(
    std::make_shared<gua::DebugViewPassDescription>());

  auto window = std::make_shared<gua::GlfwWindow>();
  gua::WindowDatabase::instance()->add("main_window", window);

  window->config.set_enable_vsync(false);
  window->config.set_size(resolution);
  window->config.set_resolution(resolution);
  window->config.set_stereo_mode(gua::StereoMode::MONO);

  window->on_resize.connect([&](gua::math::vec2ui const& new_size) {
    window->config.set_resolution(new_size);
    camera->config.set_resolution(new_size);
    screen->data.set_size(
        gua::math::vec2(0.001 * new_size.x, 0.001 * new_size.y));
  });
  window->on_move_cursor.connect(
      [&](gua::math::vec2 const& pos) { trackball.motion(pos.x, pos.y); });
  window->on_button_press.connect(
      std::bind(mouse_button, std::ref(trackball), std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));

  gua::Renderer renderer;

  // application loop
  gua::events::MainLoop loop;
  gua::events::Ticker ticker(loop, 1.0 / 500.0);

  ticker.on_tick.connect([&]() {

    // apply trackball matrix to object
    gua::math::mat4 modelmatrix =
        scm::math::make_translation(trackball.shiftx(), trackball.shifty(),
                                    trackball.distance()) *
        gua::math::mat4(trackball.rotation());

    transform->set_transform(modelmatrix);

    if (window->should_close()) {
      renderer.stop();
      window->close();
      loop.stop();
    } else {
      renderer.queue_draw({&graph});
    }
  });

  loop.start();

  return 0;
}
