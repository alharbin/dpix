# Author: Michael Burns (mburns@cs.princeton.edu)
# Copyright (c) 2009 Michael Burns
#
# This program is distributed under the terms of the
# GNU General Public License. See the COPYING file
# for details.

module Collada
    def Collada.p_to_s(p)
        p = p.collect { |n| if n.abs < 1e-10 then 0.0 else n end }
        sprintf("%f %f %f", p[0], p[1], p[2])
        end
    
    DEFAULT_MATERIAL = Material.new('defaultFaceMaterial', [1, 1, 1, 1])
    
    class Scene
        attr_reader :meshes
        attr_accessor :id_manager
        
        def initialize(model, do_lines)
            @model = model
            @do_lines = do_lines
            process
            end
            
        def process
            @meshes = []
            
            # holds collada components
            @components = []
            # holds collada component nodes
            @component_nodes = []
            # maps [sketchup component definitions, collada materials] to collada component nodes
            @component_defs_hash = {}
            # maps sketchup component definitions to lists of collada materials
            @component_def_materials_hash = {}
            # holds collada materials
            @materials = [DEFAULT_MATERIAL]
            # maps sketchup material definitions to collada materials
            @material_defs_hash = {}
            # maps entity lists to meshes
            @entity_mesh_hash = {}
            
            @id_manager = IdManager.new(%w{ mesh path Scene Group })
            @id_manager.get_id(DEFAULT_MATERIAL.id)
            
            @material_defs_hash[nil] = DEFAULT_MATERIAL

            find_components_materials(@model, DEFAULT_MATERIAL, Geom::Transformation.new)
            process_components()

            @root_node = process_object(Node.new, @model, DEFAULT_MATERIAL, Geom::Transformation.new)
            
            end

        def is_transformation_flipped?(transformation)
            matrix = transformation.to_a
            # 0 4 8  12
            # 1 5 9  13
            # 2 6 10 14
            # 3 7 11 15
            det = matrix[0] * (matrix[5] * matrix[10] - matrix[6] * matrix[9]) - \
                  matrix[4] * (matrix[1] * matrix[10] - matrix[2] * matrix[9]) + \
                  matrix[8] * (matrix[1] * matrix[6] - matrix[2] * matrix[5])
            return det < 0
            end
            
        def process_components()
            @component_def_materials_hash.each do |key, materials|
                component_def = key[0]
                is_flipped = key[1]
                if is_object_colorable?(component_def)
                    materials_to_add = materials
                    else
                    materials_to_add = { DEFAULT_MATERIAL => true }
                    end
                    
                materials_to_add.keys.each do |material|
                    key = [component_def, is_flipped, material]
                    component_node = Node.new
                    component_node.name = component_def.name
                    if is_flipped
                        component_node.name += '-flip'
                        end
                    if material != DEFAULT_MATERIAL
                        component_node.name += "-#{material.id}"
                        end
                    component_node.material = material
                    component_node.id = @id_manager.get_id(component_node.name)
                    @component_defs_hash[key] = component_node
                    @component_nodes << component_node
                    end
                end

            @component_defs_hash.each do |key, component_node|
                component_def = key[0]
                is_flipped = key[1]
                material = key[2]
                matrix = Geom::Transformation.new.to_a
                if is_flipped
                    matrix[0] = -1
                    end
                    
                process_object(component_node, component_def, material, Geom::Transformation.new(matrix))
                end
            end
        
        # could easily condense this a bit
        def is_object_colorable?(object)
            case object
                when Sketchup::Group, Sketchup::ComponentDefinition
                # if object is colored, then it is not colorable
                if object.material
                    return false
                    # if object is not colored, check children to see if any are colorable
                    else
                    colorable_children = object.entities.select { |entity| is_object_colorable?(entity) }
                    return !colorable_children.empty?()
                    end
                
                when Sketchup::ComponentInstance
                if object.material
                    return false
                    else
                    return is_object_colorable?(object.definition)
                    end
                
                when Sketchup::Edge
                return false
                    
                when Sketchup::Face
                return object.material == nil
                
                else
                print "Unhandled entity type #{object.class}\n"
                return false
                end
            end
        
        # traverses the scene rooted at object, adding component definitions to @component_defs_hash and materials to @material_defs_hash
        # follows component definitions the first time they are found
        def find_components_materials(object, parent_material, current_transformation)
            # check to see if object has material
            if object.respond_to?(:material) and object.material then
                # check if this is first occurance of material
                if not @material_defs_hash[object.material]
                    # allocate new material and store in hash
                    material = Material.from_material(@id_manager.get_id(object.material.name), object.material)
                    @material_defs_hash[object.material] = material
                    @materials << material
                    end
                    
                # update parent material to current
                parent_material = @material_defs_hash[object.material]
                end

            case object
                # follow groups and model and selection
                when Sketchup::Selection
                object.entries.each { |e| find_components_materials(e, parent_material, current_transformation) }
                
                when Sketchup::Model
                object.entities.each { |e| find_components_materials(e, parent_material, current_transformation) }

                when Sketchup::Group
                object.entities.each { |e| find_components_materials(e, parent_material, current_transformation * object.transformation) }
                
                # if found a new component definition
                when Sketchup::ComponentDefinition
                is_flipped = is_transformation_flipped?(current_transformation)
                object_key = [object, is_flipped]
                if not @component_def_materials_hash[object_key]
                    @component_def_materials_hash[object_key] = {}
                    end
                    
                if not @component_def_materials_hash[object_key][parent_material]
                    @component_def_materials_hash[object_key][parent_material] = true

                    matrix = Geom::Transformation.new.to_a
                    if is_flipped
                        matrix[0] = -1
                        end
                    
                    # follow component definition
                    object.entities.each { |e| find_components_materials(e, parent_material, Geom::Transformation.new(matrix)) }
                    end
                
                # follow component instances to definition
                when Sketchup::ComponentInstance
                find_components_materials(object.definition, parent_material, current_transformation * object.transformation)
                end
            end
        
        def process_object(node, object, parent_material, current_transformation)
            if object.respond_to?(:material)
                if object.material
                    node.material = @material_defs_hash[object.material]
                    parent_material = node.material
                    end
                end
                
            case object
                when Sketchup::Group
                node.name = object.name unless object.name.empty?
                current_transformation *= object.transformation unless object.transformation.identity?
                
                when Sketchup::Model, Sketchup::Selection
                node.name = 'Model'

                when Sketchup::ComponentInstance
                current_transformation *= object.transformation unless object.transformation.identity?
                is_flipped = is_transformation_flipped?(current_transformation)
                component_def = @component_defs_hash[[object.definition, is_flipped, parent_material]]
                component_def = @component_defs_hash[[object.definition, is_flipped, DEFAULT_MATERIAL]] unless component_def
                component = Component.new(component_def)
                node.components << component
                if not object.name.empty?
                    node.name = "#{object.name} (#{component.node.name})"
                    else
                    node.name = component.node.name
                    end
                matrix = current_transformation.to_a
                if is_flipped
                    matrix[0] *= -1
                    matrix[1] *= -1
                    matrix[2] *= -1
                    matrix[3] *= -1
                    end
                node.transformation = Geom::Transformation.new(matrix)
                end
            
            if object.respond_to?(:entities)
                entities = object.entities
                end
                
            if object.respond_to?(:entries)
                entities = object.entries
                end
                
            if entities
                groups = entities.select{ |e| e.is_a?(Sketchup::Group) }
                groups.each do |group|
                    if AnimationPath.is_path?(group)
                        object_key = [group, false]
                        if not @entity_mesh_hash[object_key]
                            points = AnimationPath.convert_to_path(group)
                            if points
                                mesh = Mesh.new
                                mesh.add_line_strip(points)
                                mesh.transform(current_transformation, false)
                                mesh.id = @id_manager.get_id('path')
                                @meshes << mesh
                                @entity_mesh_hash[object_key] = AnimationPath.new(mesh)
                                end
                            end
                        node.animation_path = @entity_mesh_hash[object_key]
                        else
                        node.nodes << process_object(Node.new, group, parent_material, current_transformation)
                        end
                    end

                component_insts = entities.select{ |e| e.is_a?(Sketchup::ComponentInstance) }
                component_insts.each do |component_inst|
                    node.nodes << process_object(Node.new, component_inst, parent_material, current_transformation)
                    end
                
                faces = entities.select{ |e| e.is_a?(Sketchup::Face) }
                if @do_lines
                    edges = entities.select{ |e| e.is_a?(Sketchup::Edge) }
                    else
                    edges = []
                    end
                
                if (faces.length > 0 || edges.length > 0) then
                    is_flipped = is_transformation_flipped?(current_transformation)
                    object_key = [object, is_flipped]
                    if not @entity_mesh_hash[object_key]
                        mesh = Mesh.new
                        mesh.add_faces_edges(faces, edges, @material_defs_hash)
                        mesh.transform(current_transformation, is_flipped)
                        mesh.id = @id_manager.get_id('mesh')
                        @meshes << mesh
                        @entity_mesh_hash[object_key] = mesh
                        node.geometries << mesh
                        else
                        node.geometries << @entity_mesh_hash[object_key]
                        end
                    end
                end
                
                node
            end

        def xml
            document = REXML::Document.new
            document << REXML::XMLDecl.new(REXML::XMLDecl::DEFAULT_VERSION, REXML::XMLDecl::DEFAULT_ENCODING)
            el_collada = document.add_element('COLLADA', { 'xmlns' => 'http://www.collada.org/2005/11/COLLADASchema', 'version' => '1.4.1' })
            
            el_library_materials = el_collada.add_element('library_materials')
            @materials.each { |m| m.xml_material(el_library_materials) }
            
            el_library_effects = el_collada.add_element('library_effects')
            @materials.each { |m| m.xml_effect(el_library_effects) }

            el_library_geometries = el_collada.add_element('library_geometries')
            @meshes.each { |m| m.xml_geometry(el_library_geometries) }
            
            if @component_nodes.length > 0
                el_library_nodes = el_collada.add_element('library_nodes')
                @component_nodes.each { |c| c.xml_node(el_library_nodes, @material_defs_hash[nil]) }
                end

            el_visual_scene = el_collada.add_element('library_visual_scenes') \
                .add_element('visual_scene', { 'id' => 'Scene', 'name' => 'Scene' })
                
            @root_node.xml_node(el_visual_scene, @material_defs_hash[nil])
            
            el_collada.add_element('scene') \
                .add_element('instance_visual_scene', { 'url' => '#Scene' })
            
            document
            end
        end
    end
