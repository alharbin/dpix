# Author: Michael Burns (mburns@cs.princeton.edu)
# Copyright (c) 2009 Michael Burns
#
# This program is distributed under the terms of the
# GNU General Public License. See the COPYING file
# for details.i

module Collada
    class Triangles
        attr_accessor :vertex_indices, :normal_indices, :count
        
        def initialize(material)
            @material = material
            @vertex_indices = []
            @normal_indices = []
            @count = 0
            end
            
        def xml(parent, id)
            el_triangles = parent.add_element('triangles', { 'count' => @count, 'material' => @material.id })
            
            el_triangles.add_element('input', { 'semantic' => 'VERTEX', 'source' => "\##{id}-vertex", 'offset' => 0 })
            el_triangles.add_element('input', { 'semantic' => 'NORMAL', 'source' => "\##{id}-normal", 'offset' => 1 })
            
            # shoudl never happen, can remove eventually
            if @vertex_indices.length != @normal_indices.length
                p "Error, mismatch length!"
                end
                
            el_triangles.add_element('p') \
                .text = @vertex_indices.zip(@normal_indices) * ' '
            end
        end
    end
