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

#ifndef GUA_PLOD_LOADER_HPP
#define GUA_PLOD_LOADER_HPP

#include <gua/renderer/LoaderBase.hpp>

namespace gua{

class Node;
class InnerNode;
class GeometryNode;
class PLODRessource;

class PLODLoader : public LoaderBase{
 public:

  /**
   * Default constructor.
   *
   * Constructs a new and empty PBRLoader.
   */
   PLODLoader();

  /**
   * Constructor from a file.
   *
   * Creates a new PBRLoader from a given file.
   *
   * \param file_name        The file to load the pointclouds data from.
   * \param material_name    The material name that was set to the parent node
   */
  std::shared_ptr<Node> load(std::string const& file_name,
                             unsigned flags);

  /**
   * Constructor from memory buffer.
   *
   * Creates a new MeshLoader from a existing memory buffer.
   *
   * \param buffer_name      The buffer to load the meh's data from.
   * \param buffer_size      The buffer's size.
   */
  std::vector<PLODRessource*> const load_from_buffer(char const* buffer_name,
                                                    unsigned buffer_size,
                                                    bool build_kd_tree);

  bool is_supported(std::string const& file_name) const;

 private:

  unsigned node_counter_;

  static unsigned model_counter_;
};


}

#endif  // GUA_PLOD_LOADER_HPP