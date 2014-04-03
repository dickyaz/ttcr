//
//  VTUReader.h
//  ttcr_u
//
//  Created by Bernard Giroux on 2013-01-10.
//  Copyright (c) 2013 Bernard Giroux. All rights reserved.
//

#ifndef ttcr_u_VTUReader_h
#define ttcr_u_VTUReader_h

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkSmartPointer.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkXMLUnstructuredGridReader.h"

#include "ttcr_t.h"

class VTUReader {
public:
    VTUReader(const char *fname) : filename(fname), valid(false), nNodes(0),
	nElements(0) {
        valid = check_format();
	}
    
	bool isValid() const { return valid; }
    
    size_t getNumberOfElements() {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        return reader->GetOutput()->GetNumberOfCells();
    }
    
    size_t getNumberOfNodes() {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        return reader->GetOutput()->GetNumberOfPoints();
    }
    
    template<typename T>
	void readNodes2D(std::vector<sxz<T> >& nodes) {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        
        double x[3];
        nodes.resize( reader->GetOutput()->GetNumberOfPoints() );
        for ( size_t n=0; n<reader->GetOutput()->GetNumberOfPoints(); ++n ) {
            reader->GetOutput()->GetPoint(n, x);
            nodes[n].x = x[0];
            nodes[n].z = x[2];
        }
    }
    
    template<typename T>
	void readNodes3D(std::vector<sxyz<T> >& nodes) {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();

        double x[3];
        nodes.resize( reader->GetOutput()->GetNumberOfPoints() );
        for ( size_t n=0; n<reader->GetOutput()->GetNumberOfPoints(); ++n ) {
            reader->GetOutput()->GetPoint(n, x);
            nodes[n].x = x[0];
            nodes[n].y = x[1];
            nodes[n].z = x[2];
        }
    }
    
    template<typename T>
	void readTriangleElements(std::vector<triangleElem<T> >& tri) {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        
        vtkSmartPointer<vtkIdList> list = vtkSmartPointer<vtkIdList>::New();
        tri.resize( reader->GetOutput()->GetNumberOfCells() );
        for ( size_t n=0; n<reader->GetOutput()->GetNumberOfCells(); ++n ) {
            if ( reader->GetOutput()->GetCell(n)->GetCellType() != VTK_TRIANGLE ) {
                std::cerr << "Error: VTK file should only contain cells of type triangle\n";
                std::abort();
            }
            reader->GetOutput()->GetCellPoints(n, list);
            tri[n].i[0] = static_cast<T>( list->GetId( 0 ) );
            tri[n].i[1] = static_cast<T>( list->GetId( 1 ) );
            tri[n].i[2] = static_cast<T>( list->GetId( 2 ) );
        }
    }
    
    template<typename T>
	void readTetrahedronElements(std::vector<tetrahedronElem<T> >& tet) {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();

        vtkSmartPointer<vtkIdList> list = vtkSmartPointer<vtkIdList>::New();
        tet.resize( reader->GetOutput()->GetNumberOfCells() );
        for ( size_t n=0; n<reader->GetOutput()->GetNumberOfCells(); ++n ) {
            if ( reader->GetOutput()->GetCell(n)->GetCellType() != VTK_TETRA ) {
                std::cerr << "Error: VTK file should only contain cells of type tetrahedron\n";
                std::abort();
            }
            reader->GetOutput()->GetCellPoints(n, list);
            tet[n].i[0] = static_cast<T>( list->GetId( 0 ) );
            tet[n].i[1] = static_cast<T>( list->GetId( 1 ) );
            tet[n].i[2] = static_cast<T>( list->GetId( 2 ) );
            tet[n].i[3] = static_cast<T>( list->GetId( 3 ) );
        }
    }
    
    template<typename T>
	int readSlowness(std::vector<T>& slowness) {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        
        if ( reader->GetOutput()->GetCellData()->HasArray("Slowness") == 0 &&
			reader->GetOutput()->GetCellData()->HasArray("Velocity") == 0 ) {
            std::cerr << "No Slowness data in file " << filename << std::endl;
            return 0;
        }

		if ( reader->GetOutput()->GetCellData()->HasArray("Slowness") == 1 ) {
			
			vtkSmartPointer<vtkCellData> cd = vtkSmartPointer<vtkCellData>::New();
			cd = reader->GetOutput()->GetCellData();
			vtkSmartPointer<vtkDoubleArray> slo = vtkSmartPointer<vtkDoubleArray>::New();
			slo = vtkDoubleArray::SafeDownCast( cd->GetArray("Slowness") );
			
			if ( slo->GetSize() != reader->GetOutput()->GetNumberOfCells() ) {
				std::cerr << "Problem with Slowness data (wrong size)" << std::endl;
				return 0;
			}
			
			slowness.resize( slo->GetSize() );
			for ( size_t n=0; n<slo->GetSize(); ++n ) {
				slowness[n] = slo->GetComponent(n, 0);
			}
		} else {
			vtkSmartPointer<vtkCellData> cd = vtkSmartPointer<vtkCellData>::New();
			cd = reader->GetOutput()->GetCellData();
			vtkSmartPointer<vtkDoubleArray> vel = vtkSmartPointer<vtkDoubleArray>::New();
			vel = vtkDoubleArray::SafeDownCast( cd->GetArray("Velocity") );
			
			if ( vel->GetSize() != reader->GetOutput()->GetNumberOfCells() ) {
				std::cerr << "Problem with Slowness data (wrong size)" << std::endl;
				return 0;
			}
			
			slowness.resize( vel->GetSize() );
			for ( size_t n=0; n<vel->GetSize(); ++n ) {
				slowness[n] = 1./vel->GetComponent(n, 0);
			}
		}
        
        return 1;
    }
    
private:
    std::string filename;
	bool valid;
	size_t nNodes;
	size_t nElements;

    bool check_format() const {
        vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
        vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
        reader->SetFileName(filename.c_str());
        reader->Update();
        
        if ( reader->GetOutput() ) {
            
            if ( reader->GetOutput()->GetCellData()->HasArray("Slowness") == 0 &&
				reader->GetOutput()->GetCellData()->HasArray("Velocity") == 0 ) {
                std::cerr << "No Slowness data in file " << filename << std::endl;
                return false;
            }
            
			if ( reader->GetOutput()->GetCellData()->HasArray("Slowness") == 1 ) {
				if ( reader->GetOutput()->GetCellData()->GetArray("Slowness")->GetSize() != reader->GetOutput()->GetNumberOfCells() ) {
					std::cerr << "Problem with Slowness data (wrong size)" << std::endl;
					return false;
				}
			} else {
				if ( reader->GetOutput()->GetCellData()->GetArray("Velocity")->GetSize() != reader->GetOutput()->GetNumberOfCells() ) {
					std::cerr << "Problem with Slowness data (wrong size)" << std::endl;
					return false;
				}
			}
            
            return true;
        }
        return false;
    }
};

#endif
