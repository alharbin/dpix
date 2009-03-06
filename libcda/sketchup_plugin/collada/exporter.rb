require 'sketchup.rb'
require 'rubystdlib.rb'
require 'rexml/Document'
load 'collada/id_manager.rb'
load 'collada/mesh.rb'
load 'collada/triangles.rb'
load 'collada/lines.rb'
load 'collada/node.rb'
load 'collada/material.rb'
load 'collada/component.rb'
load 'collada/contours.rb'
load 'collada/animation_path.rb'
load 'collada/line_strips.rb'
load 'collada/scene.rb'
load 'collada/optimized.rb'

module Collada
    def Collada.export_model(object, do_lines)
        file_path = Sketchup.active_model.path
        dir_name = file_path[0, (file_path.length - File.basename(file_path).length)]
        file_name = File.basename(file_path, '.skp') + '.dae'
        save_file_path = UI.savepanel('Export to Collada File...', dir_name, file_name)
        if save_file_path
            file = File.new(save_file_path, 'w')
            REXML::Formatters::Optimized.new.write(Collada::Scene.new(object, do_lines).xml, file)
            file.close
            end
        end
        
    def Collada.export_console(object, do_lines)
        output=''
        REXML::Formatters::Optimized.new.write(Collada::Scene.new(object, do_lines).xml, output)
        puts output
        end

    def Collada.get_single_selection(type, verbose)
        if Sketchup.active_model.selection.entries.length != 1
            UI.messagebox('Invalid number items selected', MB_OK, 'Error') if verbose
            return nil
            end
            
        selected_item = Sketchup.active_model.selection.entries[0]
        if not selected_item.is_a?(type)
            UI.messagebox('Selected item must be a ' + type.to_s, MB_OK, 'Error') if verbose
            return nil
            end
            
        return selected_item
        end
        
    def Collada.set_animation_path
        selected_item = Collada.get_single_selection(Sketchup::Group, true)
        if selected_item          
            AnimationPath.set_path(selected_item)
            end
        end
        
    def Collada.unset_animation_path
        selected_item = Collada.get_single_selection(Sketchup::Group, true)
        if selected_item          
            AnimationPath.unset_path(selected_item)
            end
        end
        
    def Collada.set_animation_path_start
        selected_item = Collada.get_single_selection(Sketchup::Edge, true)
        if selected_item          
            AnimationPath.set_start_edge(selected_item)
            end
        end

    def Collada.unset_animation_path_start
        selected_item = Collada.get_single_selection(Sketchup::Edge, true)
        if selected_item          
            AnimationPath.unset_start_edge(selected_item)
            end
        end
    end

if not file_loaded?('collada/exporter.rb')
    plugins_menu = UI.menu('Plugins')
    if plugins_menu
        collada_menu = plugins_menu.add_submenu('Collada')
        collada_menu.add_item('Export All to .dae...') { Collada.export_model(Sketchup.active_model, true) }
        collada_menu.add_item('Export All to Console') { Collada.export_console(Sketchup.active_model, true) }
        collada_menu.add_separator()
        collada_menu.add_item('Export Polygons to .dae...') { Collada.export_model(Sketchup.active_model, false) }
        collada_menu.add_item('Export Polygons to Console') { Collada.export_console(Sketchup.active_model, false) }
        collada_menu.add_separator()
        collada_menu.add_item('Export Selection to .dae...') { Collada.export_model(Sketchup.active_model.selection, true) }
        collada_menu.add_item('Export Selection to Console') { Collada.export_console(Sketchup.active_model.selection, true) }
        collada_menu.add_separator()
        collada_menu.add_item('Set Animation Path') { Collada.set_animation_path }
        collada_menu.add_item('Clear Animation Path') { Collada.unset_animation_path }
        collada_menu.add_item('Set Start Edge') { Collada.set_animation_path_start }
        collada_menu.add_item('Clear Start Edge') { Collada.unset_animation_path_start }
        collada_menu.add_separator()
        collada_menu.add_item('Reload Exporter') { load 'collada/exporter.rb' }
        end
        
    UI.add_context_menu_handler do |menu|
        selected_item = Collada.get_single_selection(Sketchup::Group, false)
        if selected_item
            if Collada::AnimationPath.is_path?(selected_item)
                menu.add_item('Clear Animation Path') { Collada::AnimationPath.unset_path(selected_item) }
                else
                menu.add_item('Set Animation Path') { Collada::AnimationPath.set_path(selected_item) }
                end                
            end
        end

    UI.add_context_menu_handler do |menu|
        selected_item = Collada.get_single_selection(Sketchup::Edge, false)
        if selected_item
            if Collada::AnimationPath.is_start_edge?(selected_item)
                menu.add_item('Clear Start Edge') { Collada::AnimationPath.unset_start_edge(selected_item) }
                else
                menu.add_item('Set Start Edge') { Collada::AnimationPath.set_start_edge(selected_item) }
                end                
            end
        end
    end

file_loaded('collada/exporter.rb')
