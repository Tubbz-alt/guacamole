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
#include <future>

#include <gua/guacamole.hpp>
#include <gua/renderer/TriMeshLoader.hpp>
#include <gua/renderer/PhysicallyBasedShadingPass.hpp>
#include <gua/utils/Trackball.hpp>

// forward mouse interaction to trackball
void mouse_button (gua::utils::Trackball& trackball, int mousebutton, int action, int mods)
{
  gua::utils::Trackball::button_type button;
  gua::utils::Trackball::state_type state;

  switch (mousebutton) {
    case 0: button = gua::utils::Trackball::left; break;
    case 2: button = gua::utils::Trackball::middle; break;
    case 1: button = gua::utils::Trackball::right; break;
  };

  switch (action) {
    case 0: state = gua::utils::Trackball::released; break;
    case 1: state = gua::utils::Trackball::pressed; break;
  };

  trackball.mouse(button, state, trackball.posx(), trackball.posy());
}

int main(int argc, char** argv) {

  // initialize guacamole
  gua::init(argc, argv);

  // setup scene
  gua::SceneGraph graph("main_scenegraph");

  auto load_mat = [](std::string const& file){
    gua::MaterialShaderDescription desc;
    desc.load_from_file(file);
    auto shader(std::make_shared<gua::MaterialShader>(file, desc));
    gua::MaterialShaderDatabase::instance()->add(shader);
    return shader->get_default_material();
  };

  auto pbrMat(load_mat("data/materials/Cerberus.gmd"));
  std::string directory("/opt/3d_models/Cerberus_by_Andrew_Maximov/Textures/");
  pbrMat.set_uniform("AlbedoMap",    directory + "Cerberus_A.tga");
  pbrMat.set_uniform("MetalnessMap", directory + "Cerberus_M.tga");
  pbrMat.set_uniform("RoughnessMap", directory + "Cerberus_R.tga");
  pbrMat.set_uniform("NormalMap",    directory + "Cerberus_N.tga");

  gua::TriMeshLoader loader;

  auto transform = graph.add_node<gua::node::TransformNode>("/", "transform");
  auto cerberus(loader.create_geometry_from_file(
          "cerberus"
        , "/opt/3d_models/Cerberus_by_Andrew_Maximov/Cerberus_LP.3ds"
        , pbrMat
        , gua::TriMeshLoader::NORMALIZE_POSITION
        | gua::TriMeshLoader::NORMALIZE_SCALE
        ));
  graph.add_node("/transform", cerberus);
  cerberus->set_draw_bounding_box(true);
  cerberus->rotate(90, 0.f, 1.f, 0.f);
  cerberus->rotate(90, 0.f, 0.f, 1.f);

  auto pointLight = graph.add_node<gua::node::PointLightNode>("/", "pointLight");
  pointLight->data.color = gua::utils::Color3f(1.0f, 1.0f, 1.0f);
  pointLight->data.brightness = 100.0f; // lm
  pointLight->scale(10.f);
  pointLight->translate(-2.f, 3.f, 5.f);

  auto spotLight = graph.add_node<gua::node::SpotLightNode>("/", "spotLight");
  spotLight->data.set_enable_shadows(true);
  spotLight->data.brightness = 10.0f; // lm
  spotLight->scale(10.f);
  spotLight->rotate(-20, 0.f, 1.f, 0.f);
  spotLight->translate(-1.f, 0.f,  3.f);

  auto screen = graph.add_node<gua::node::ScreenNode>("/", "screen");
  screen->data.set_size(gua::math::vec2(1.92f, 1.08f));
  screen->translate(0, 0, 1.0);

  // add mouse interaction
  gua::utils::Trackball trackball(0.01, 0.002, 0.2);

  // setup rendering pipeline and window
  //auto resolution = gua::math::vec2ui(1920, 1080);
  auto resolution = gua::math::vec2ui(2560,1440);

  std::async(std::launch::async, []() {
    gua::TextureDatabase::instance()->load(
      "/opt/guacamole/resources/skymaps/skymap.jpg");
  });

  for (auto const& file : {
        "Cerberus_A.tga",
        "Cerberus_M.tga",
        "Cerberus_R.tga",
        "Cerberus_N.tga"
        }) {
    gua::TextureDatabase::instance()->load(directory + file);
  }

  gua::PipelineDescription standardPipe;
  standardPipe.add_pass<gua::TriMeshPassDescription>();
  standardPipe.add_pass<gua::EmissivePassDescription>();
  standardPipe.add_pass<gua::LightingPassDescription>();
  standardPipe.add_pass<gua::BackgroundPassDescription>()
    .mode(gua::BackgroundPassDescription::QUAD_TEXTURE)
    //.texture("/opt/guacamole/resources/skymaps/skymap.jpg")
    ;

  gua::PipelineDescription pbrPipe;
  pbrPipe.add_pass<gua::TriMeshPassDescription>();
  pbrPipe.add_pass<gua::EmissivePassDescription>();
  pbrPipe.add_pass<gua::PhysicallyBasedShadingPassDescription>();
#if 0
  pbrPipe.add_pass<gua::BackgroundPassDescription>()
    .mode(gua::BackgroundPassDescription::QUAD_TEXTURE)
    .texture("/opt/guacamole/resources/skymaps/skymap.jpg")
    ;
#endif

  auto camera = graph.add_node<gua::node::CameraNode>("/screen", "cam");
  camera->translate(0, 0, 2.0);
  camera->config.set_resolution(resolution);
  camera->config.set_screen_path("/screen");
  camera->config.set_scene_graph_name("main_scenegraph");
  camera->config.set_output_window_name("main_window");
  camera->config.set_enable_stereo(false);
  camera->config.set_pipeline_description(pbrPipe);

  auto window = std::make_shared<gua::GlfwWindow>();
  gua::WindowDatabase::instance()->add("main_window", window);
  window->config.set_enable_vsync(false);
  window->config.set_size(resolution);
  window->config.set_resolution(resolution);
  window->config.set_stereo_mode(gua::StereoMode::MONO);
  window->on_resize.connect([&](gua::math::vec2ui const& new_size) {
    window->config.set_resolution(new_size);
    camera->config.set_resolution(new_size);
    screen->data.set_size(gua::math::vec2(0.001 * new_size.x, 0.001 * new_size.y));
  });
  window->on_move_cursor.connect([&](gua::math::vec2 const& pos) {
    trackball.motion(pos.x, pos.y);
  });
  window->on_button_press.connect(std::bind(mouse_button, std::ref(trackball), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  window->on_key_press.connect([&](int key, int scancode, int action, int mods) {
      if ('1' == key) {
        camera->config.set_pipeline_description(pbrPipe);
      } else if ('2' == key) {
        camera->config.set_pipeline_description(standardPipe);
      }
    });

  gua::Renderer renderer;

  // application loop
  gua::events::MainLoop loop;
  gua::events::Ticker ticker(loop, 1.0/500.0);

  ticker.on_tick.connect([&]() {

    // apply trackball matrix to object
    auto modelmatrix = scm::math::make_translation(trackball.shiftx(), trackball.shifty(), trackball.distance()) * trackball.rotation();
    transform->set_transform(modelmatrix);

    window->process_events();
    if (window->should_close()) {
      renderer.stop();
      window->close();
      loop.stop();
    } else {
      renderer.queue_draw({&graph}, {camera});
    }
  });

  loop.start();

  return 0;
}
