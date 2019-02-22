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

// class header
#include <gua/virtual_texturing/VirtualTexture2D.hpp>

// guacamole headers
#include <gua/platform.hpp>
#include <gua/utils/Logger.hpp>
#include <gua/math/math.hpp>

#include <gua/databases/TextureDatabase.hpp>

// external headers
#include <boost/shared_ptr.hpp>
#include <scm/gl_core/data_formats.h>
#include <scm/gl_core/log.h>
#include <scm/gl_core/render_device.h>
#include <scm/gl_core/texture_objects.h>

#include <scm/gl_util/data/imaging/texture_loader.h>
#include <scm/gl_util/data/imaging/texture_image_data.h>

#include <lamure/vt/VTConfig.h>
#include <lamure/vt/ren/CutDatabase.h>
#include <lamure/vt/pre/AtlasFile.h>

#include <iostream>
#include <fstream>
#include <regex>

#define PHYSICAL_TEXTURE_MAX_NUM_LAYERS 256
#define PHYSICAL_TEXTURE_MAX_RES_PER_AXIS 8192

#define MAX_TEXTURES 1024

namespace gua {
  std::map<std::size_t,
    std::shared_ptr<LayeredPhysicalTexture2D> > VirtualTexture2D::physical_texture_ptr_per_context_ 
      = std::map<std::size_t, std::shared_ptr<LayeredPhysicalTexture2D> >();

  std::map<std::size_t, VTInfo> VirtualTexture2D::vt_info_per_context_ 
      = std::map<std::size_t, VTInfo>();

  std::map<std::size_t, scm::gl::buffer_ptr> VirtualTexture2D::vt_addresses_ubo_per_context_
      = std::map<std::size_t, scm::gl::buffer_ptr>();

  bool VirtualTexture2D::initialized_vt_system = false;

  VirtualTexture2D::VirtualTexture2D(std::string const& atlas_filename,
                                     std::size_t physical_texture_tile_slot_size,
                                     scm::gl::sampler_state_desc const& state_descripton) {

    std::string const ini_filename = std::regex_replace(atlas_filename, std::regex(".atlas"), ".ini");

    if(!initialized_vt_system) {
      ::vt::VTConfig::CONFIG_PATH = ini_filename;
      ::vt::VTConfig::get_instance().define_size_physical_texture(PHYSICAL_TEXTURE_MAX_NUM_LAYERS, PHYSICAL_TEXTURE_MAX_RES_PER_AXIS);
    
      initialized_vt_system = true;
    }

    tile_size_ = ::vt::VTConfig::get_instance().get_size_tile();

    lamure_texture_id_ = ::vt::CutDatabase::get_instance().register_dataset(atlas_filename);

    atlas_file_path_ = atlas_filename;
    ini_file_path_   = ini_filename;

    ::vt::pre::AtlasFile current_atlas_file(atlas_filename.c_str());

    max_depth_ = current_atlas_file.getDepth();
  }



  void VirtualTexture2D::upload_to(RenderContext const& ctx) const {
    auto index_texture_hierarchy_context_iterator = index_texture_mip_map_per_context_.find(ctx.id);

    if(nullptr == nearest_mip_map_sampler_state_) {
      nearest_mip_map_sampler_state_ = ctx.render_device->create_sampler_state(scm::gl::FILTER_MIN_MAG_MIP_NEAREST, scm::gl::WRAP_CLAMP_TO_EDGE);
    }

    if(index_texture_hierarchy_context_iterator == index_texture_mip_map_per_context_.end()) {
        uint32_t size_index_texture = (uint32_t) vt::QuadTree::get_tiles_per_row(max_depth_);
        auto index_texture_level_ptr = ctx.render_device->create_texture_2d(
                                                                            scm::math::vec2ui(size_index_texture, size_index_texture), 
                                                                            scm::gl::FORMAT_RGBA_8UI,
                                                                            max_depth_ + 1);

        for(uint32_t i = 0; i < max_depth_ + 1; ++i) {
          ctx.render_context->clear_image_data(index_texture_level_ptr, i, scm::gl::FORMAT_RGBA_8UI, 0);
          pbo_sizes_ += std::pow(std::pow(2, i),2);

        }

        pbo_sizes_ *= 4;
        index_texture_pbo_front_per_context_[ctx.id] = ctx.render_device->create_buffer(scm::gl::BIND_PIXEL_UNPACK_BUFFER,
                                                                                        scm::gl::USAGE_DYNAMIC_DRAW,
                                                                                        pbo_sizes_,
                                                                                        0);

        index_texture_pbo_back_per_context_[ctx.id] = ctx.render_device->create_buffer(scm::gl::BIND_PIXEL_UNPACK_BUFFER,
                                                                                        scm::gl::USAGE_DYNAMIC_DRAW,
                                                                                        pbo_sizes_,
                                                                                        0);

        mapped_pbos_back_per_context_[ctx.id] = ctx.render_context->map_buffer_range(index_texture_pbo_back_per_context_[ctx.id], 0, pbo_sizes_, scm::gl::ACCESS_WRITE_INVALIDATE_RANGE);

        index_texture_mip_map_per_context_[ctx.id] = index_texture_level_ptr;   

        ctx.render_context->make_resident(index_texture_level_ptr, nearest_mip_map_sampler_state_);

        upload_vt_handle_to_ubo(ctx);
    }
  }

// do not touch 
//#define USE_PBO

  void VirtualTexture2D::update_index_texture_hierarchy(RenderContext const& ctx, 
                                                        std::vector<std::pair<uint16_t, uint8_t*>> const& level_update_pairs) {
    
#ifdef USE_PBO
    ctx.render_context->unmap_buffer(index_texture_pbo_back_per_context_[ctx.id]);
    std::swap(index_texture_pbo_back_per_context_[ctx.id], index_texture_pbo_front_per_context_[ctx.id]);
    mapped_pbos_back_per_context_[ctx.id] = ctx.render_context->map_buffer_range(index_texture_pbo_back_per_context_[ctx.id], 0, pbo_sizes_, scm::gl::ACCESS_WRITE_INVALIDATE_RANGE);
#endif
    for(auto const& update_pair : level_update_pairs) {
      uint32_t updated_level = update_pair.first;
      uint32_t size_index_texture = (uint32_t) ::vt::QuadTree::get_tiles_per_row(updated_level);

      scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
      scm::math::vec3ui dimensions = scm::math::vec3ui(size_index_texture, size_index_texture, 1);

      auto& current_index_texture_hierarchy = index_texture_mip_map_per_context_[ctx.id];

uint32_t max_level = max_depth_;
#ifndef USE_PBO
      ctx.render_context->update_sub_texture(current_index_texture_hierarchy,
                                             scm::gl::texture_region(origin, dimensions), 
                                             max_level-updated_level, scm::gl::FORMAT_RGBA_8UI,
                                             update_pair.second );
#endif

#ifdef USE_PBO

      uint64_t current_write_offset = 0;

      uint64_t current_amount_bytes_to_write = size_index_texture*size_index_texture * 4;

      memcpy( ((char*) mapped_pbos_back_per_context_[ctx.id]) + current_write_offset,
              update_pair.second, current_amount_bytes_to_write);

      pbo_level_mapping_back_per_context_[ctx.id].emplace_back(updated_level, current_write_offset);

      current_write_offset += current_amount_bytes_to_write;
#endif
    }

#ifdef USE_PBO
    ctx.render_context->bind_unpack_buffer(index_texture_pbo_front_per_context_[ctx.id]);
    auto& current_index_texture_hierarchy = index_texture_mip_map_per_context_[ctx.id];
    
    for(auto const& write_entry : pbo_level_mapping_front_per_context_[ctx.id]) {

      uint32_t updated_level = write_entry.first;
      uint64_t read_offset = write_entry.second;
      uint32_t size_index_texture = (uint32_t) ::vt::QuadTree::get_tiles_per_row(updated_level);

      scm::math::vec3ui origin = scm::math::vec3ui(0, 0, 0);
      scm::math::vec3ui dimensions = scm::math::vec3ui(size_index_texture, size_index_texture, 1);

      ctx.render_context->update_sub_texture(current_index_texture_hierarchy,
                                             scm::gl::texture_region(origin, dimensions), 
                                             max_depth_-updated_level, scm::gl::FORMAT_RGBA_8UI,
                                             //,
                                              (char*) (0) /*+ read_offset*/ );
    }
    ctx.render_context->bind_unpack_buffer(0);
#endif
    std::swap(pbo_level_mapping_back_per_context_[ctx.id], pbo_level_mapping_front_per_context_[ctx.id]);
  }

  void VirtualTexture2D::upload_vt_handle_to_ubo(RenderContext const& ctx) const {
    if(vt_addresses_ubo_per_context_.end() == vt_addresses_ubo_per_context_.find(ctx.id) ) {
      vt_addresses_ubo_per_context_[ctx.id] = ctx.render_device->create_buffer(scm::gl::BIND_UNIFORM_BUFFER, scm::gl::USAGE_STATIC_DRAW,
                                                                               MAX_TEXTURES * sizeof(scm::math::vec2ui));
    }

    auto& current_vt_addresses_ubo = vt_addresses_ubo_per_context_[ctx.id];



    uint64_t handle = index_texture_mip_map_per_context_[ctx.id]->native_handle();

    uint64_t physical_texture_cpu_address = (handle & 0x00000000ffffffff) | (handle & 0xffffffff00000000);
    //math::vec2ui swapped_texture_adress = math::vec2ui(handle & 0x00000000ffffffff, handle & 0xffffffff00000000);

    uint64_t *mapped_physical_texture_address_ubo = (uint64_t *) ctx.render_context->map_buffer(current_vt_addresses_ubo,
                                                                                                scm::gl::ACCESS_WRITE_ONLY);

    uint32_t current_global_texture_id = gua::TextureDatabase::instance()->get_global_texture_id_by_path(atlas_file_path_);

    uint64_t current_handle_write_offset = 2 * current_global_texture_id;

    memcpy((char*)(&mapped_physical_texture_address_ubo[current_handle_write_offset]), &physical_texture_cpu_address, sizeof(uint64_t));
    
    memcpy((char*)(&mapped_physical_texture_address_ubo[current_handle_write_offset + 1]), &max_depth_, sizeof(int32_t));
    

    ctx.render_context->unmap_buffer(current_vt_addresses_ubo);
    ctx.render_context->bind_uniform_buffer(current_vt_addresses_ubo, 4);
  }

}
