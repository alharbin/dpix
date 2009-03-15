# Author: Michael Burns (mburns@cs.princeton.edu)
# Copyright (c) 2009 Michael Burns
#
# This program is distributed under the terms of the
# GNU General Public License. See the COPYING file
# for details.

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
