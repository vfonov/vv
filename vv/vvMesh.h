/*=========================================================================

 Program:   vv
 Language:  C++
 Author :   Joel Schaerer (joel.schaerer@insa-lyon.fr)
 Program:   vv

Copyright (C) 2008
Léon Bérard cancer center http://oncora1.lyon.fnclcc.fr
CREATIS-LRMN http://www.creatis.insa-lyon.fr

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#ifndef vvMesh_h
#define vvMesh_h

#include <string>
#include <utility>
#include <vector>

#include <itkObjectFactory.h>

#include "vvImage.h"

class vtkPolyData;
class vtkImageData;

/** A vvMesh is assumed to be either a 3D closed surface 
 *  or a series of surfaces 
 *  It uses a dual representation: both binary mask and mesh**/
class vvMesh : public itk::LightObject
{
public:
    typedef vvMesh Self;
    typedef itk::SmartPointer<Self> Pointer;
    itkNewMacro(Self);

    void ReadFromVTK(const char * filename);
    std::string structure_name;
    ///Contour color, [0,1]
    double r,g,b;

    vtkPolyData* GetMesh(unsigned int i) const {return meshes[i];}
    void AddMesh(vtkPolyData* p);
    ///Removes all meshes in the object
    void RemoveMeshes();
    unsigned int GetNumberOfMeshes() { return meshes.size(); }

    vtkImageData* GetMask(unsigned int i) const {return masks[i];}
    void AddMask(vtkImageData* im);
    void RemoveMasks();
    unsigned int GetNumberOfMasks() { return masks.size(); }

    ///Pretty-print information about the mesh
    void Print() const;
    ///Copies the meta-informations from another mesh
    void CopyInformation(vvMesh::Pointer input);
    void SetSpacing(double spacing) {slice_spacing=spacing;}
    double GetSpacing() {return slice_spacing;}
    ///Recompute the meshes from the masks
    void ComputeMeshes();
    /**Recompute the masks from the meshes.
     * extrude means that the mesh must be extruded before binarizing,
     * which is useful when creating a mesh from a stack of slices */
    void ComputeMasks(vtkImageData* sample,bool extrude=false);
    ///Create a new vvMesh by propagating the mesh with a displacement VF
    void propagateContour(vvImage::Pointer vf);
protected:
    ///The spacing between the planar contour, assumed to be constant
    double slice_spacing;
    std::vector<vtkPolyData *> meshes;
    std::vector<vtkImageData *> masks;

    vvMesh();
    ~vvMesh();
};

///Propagate a contour using a vector of motion fields, returns a 4D contour

#endif
