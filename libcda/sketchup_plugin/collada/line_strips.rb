module Collada
    class LineStrips
        attr_accessor :vertex_indices
        
        def initialize
            @vertex_indices = []
            end
            
        def xml(parent, id)
            el_lines = parent.add_element('linestrips', { 'count' => 1 })
                    
            el_lines.add_element('input', { 'semantic' => 'VERTEX', 'source' => "\##{id}-vertex", 'offset' => 0 })
            
            el_lines.add_element('p') \
                .text = @vertex_indices * ' '
            end
        end
    end
