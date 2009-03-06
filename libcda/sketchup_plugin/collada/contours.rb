module Collada
    class Contours
        attr_accessor :vertex_indices, :normal_indices, :count
        
        def initialize
            @vertex_indices = []
            @normal_indices = []
            @count = 0
            end
            
        def xml(parent, id)
            el_lines = parent.add_element('contours', { 'count' => @count })
                    
            el_lines.add_element('input', { 'semantic' => 'VERTEX', 'source' => "\##{id}-vertex", 'offset' => 0 })
            el_lines.add_element('input', { 'semantic' => 'NORMAL', 'source' => "\##{id}-normal", 'offset' => 1 })

            # shoudl never happen, can remove eventually
            if @vertex_indices.length != @normal_indices.length
                p "Error, mismatch length!"
                end

            el_lines.add_element('p') \
                .text = @vertex_indices.zip(@normal_indices) * ' '
            end
        end
    end
