require 'sketchup.rb'

module SelectionFilter
    def SelectionFilter.filter(type)
        new_selection = Sketchup.active_model.selection.entries.select { |entity| entity.is_a?(type) }
        Sketchup.active_model.selection.clear
        Sketchup.active_model.selection.add(new_selection)
        end
    end

if not file_loaded?('selection_filter.rb')
    plugins_menu = UI.menu('Plugins')
    if plugins_menu
        sub_menu = plugins_menu.add_submenu('Selection Filter')
        sub_menu.add_item('Faces Only') { SelectionFilter.filter(Sketchup::Face) }
        sub_menu.add_item('Edges Only') { SelectionFilter.filter(Sketchup::Edge) }
        end
        
#    UI.add_context_menu_handler do |menu|
#        selected_item = Collada.get_single_selection(Sketchup::Group, false)
#        if selected_item
#            if Collada::AnimationPath.is_path?(selected_item)
#                menu.add_item('Clear Animation Path') { Collada::AnimationPath.unset_path(selected_item) }
#                else
#                menu.add_item('Set Animation Path') { Collada::AnimationPath.set_path(selected_item) }
#                end                
#            end
#        end

    end

file_loaded('selection_filter.rb')
