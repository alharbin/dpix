# Author: Michael Burns (mburns@cs.princeton.edu)
# Copyright (c) 2009 Michael Burns
#
# This program is distributed under the terms of the
# GNU General Public License. See the COPYING file
# for details.

module Collada
    def Collada.ccw(a, b, c)
        a[0] * (b[1] - c[1]) - b[0] * (a[1] - c[1]) + c[0] * (a[1] - b[1])
        end
    
    def Collada.contains?(vertices, candidates)
        candidates.each do |c|
            if \
                Collada.ccw(vertices[0], vertices[1], c) > 0 and \
                Collada.ccw(vertices[1], vertices[2], c) > 0 and \
                Collada.ccw(vertices[2], vertices[0], c) > 0 then
                return true
                end
            end
        return false
        end
    
    def Collada.is_convex?(vertices, i)
        Collada.ccw(vertices[i - 1], vertices[i], vertices[(i + 1) % vertices.length]) > 0
        end
    
    def Collada.is_ear?(i, vertices, concave_vertices)
        if concave_vertices.length == 0 then
            return true
            else
            if Collada.is_convex?(vertices, i) then
#                p "#{i} is convex"
                if not Collada.contains?([vertices[i-1], vertices[i], vertices[(i+1)%vertices.length]], concave_vertices) then
                    return true
                    else
                    return false
                    end
                else
                return false
                end
            end
        end
    
    def Collada.triangulate(face)
        vertices = face.vertices.collect { |v| v.position }
        normal = face.normal
        angle = normal.angle_between([0, 0, 1])
        p angle
        rotation_axis = normal * [0, 0, 1]
        if rotation_axis.length < 0.01 then
            rotation_axis = [1, 0, 0]
            end
        transform = Geom::Transformation.rotation([0, 0, 0], rotation_axis, angle)
        xy_vertices = vertices.collect { |v| v.transform(transform) }
        xy_vertices.each_index { |i| xy_vertices[i][2] = i }
        
        concave_vertices = []
        xy_vertices.each_index do |i|
            if not Collada.is_convex?(xy_vertices, i) then
                concave_vertices << xy_vertices[i]
                end
            end
        
#        p concave_vertices
        triangles = []
        i = 2
        while i < xy_vertices.length do
            if Collada.is_ear?(i - 1, xy_vertices, concave_vertices) and xy_vertices.length > 3 then
                p "Found ear with i = #{i}"
                triangles << [xy_vertices[i-2][2].to_i, xy_vertices[i-1][2].to_i, xy_vertices[i][2].to_i]
                xy_vertices.delete_at(i-1)
#                p triangles
#                p xy_vertices
                i -= 1
                if concave_vertices.include?(xy_vertices[i]) and Collada.is_convex?(xy_vertices, i) then
                    concave_vertices.delete(xy_vertices[i])
                    end
                if concave_vertices.include?(xy_vertices[i-1]) and Collada.is_convex?(xy_vertices, i-1) then
                    concave_vertices.delete(xy_vertices[i-1])
                    end
                if i == 1 then
                    i += 1
                    end
                else
                i += 1
                end
#                p "Now i is #{i}"
            end
        triangles << [xy_vertices[0][2].to_i, xy_vertices[1][2].to_i, xy_vertices[2][2].to_i]
#        p triangles
        triangles
        end
    end
