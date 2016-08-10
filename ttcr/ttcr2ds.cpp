//
//  main.cpp
//  ttcf2ds
//
//  Created by Bernard Giroux on 2014-04-04.
//  Copyright (c) 2014 Bernard Giroux. All rights reserved.
//

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>
#include <thread>

#include "Grid2Duc.h"
#include "Rcv.h"
#include "Src.h"
#include "ttcr_io.h"
#include "grids.h"

using namespace std;
using namespace ttcr;


template<typename T>
int body(const input_parameters &par) {
    
	std::vector< Src<T>> src;
    for ( size_t n=0; n<par.srcfiles.size(); ++ n ) {
        src.push_back( Src<T>( par.srcfiles[n] ) );
		string end = " ... ";
		if ( n < par.srcfiles.size() - 1 ) end = " ...\n";
        if ( par.verbose ) cout << "Reading source file " << par.srcfiles[n] << end;
        src[n].init();
    }
    if ( par.verbose ) cout << "done.\n";
	
	size_t const nTx = src.size();
	size_t num_threads = 1;
	if ( par.nt == 0 ) {
		size_t const hardware_threads = std::thread::hardware_concurrency();
		size_t const min_per_thread=5;
		size_t const max_threads = (nTx+min_per_thread-1)/min_per_thread;
		num_threads = std::min((hardware_threads!=0?hardware_threads:2), max_threads);
	} else {
		num_threads = par.nt < nTx ? par.nt : nTx;
	}
	
    size_t const blk_size = (nTx%num_threads ? 1 : 0) + nTx/num_threads;

	string::size_type idx;
    
    idx = par.modelfile.rfind('.');
    string extension = "";
    if (idx != string::npos) {
        extension = par.modelfile.substr(idx);
    }
    
//    Grid2Duc<T,uint32_t, Node3Dcsp<T,uint32_t>, sxyz<T>> *g=nullptr;
    Grid2D<T,uint32_t,sxyz<T>> *g=nullptr;
    if (extension == ".vtu") {
#ifdef VTK
        g = unstruct2Ds_vtu<T>(par, num_threads);
#else
		cerr << "Error: Program not compiled with VTK support" << endl;
		return 1;
#endif
    } else if (extension == ".msh") {
        g = unstruct2Ds<T>(par, num_threads, src.size());
    } else {
        cerr << par.modelfile << " Unknown extenstion: " << extension << endl;
        return 1;
    }
    
    if ( g == nullptr ) {
        cerr << "Error: grid cannot be built\n";
		return 1;
    }

	Rcv<T> rcv( par.rcvfile );
    if ( par.verbose ) cout << "Reading receiver file " << par.rcvfile << " ... ";
    rcv.init( src.size() );
    if ( par.verbose ) cout << "done.\n";
    
    if ( par.verbose ) {
        if ( par.singlePrecision ) {
            cout << "Calculations will be done in single precision.\n";
        } else {
            cout << "Calculations will be done in double precision.\n";
        }
    }
	if ( par.verbose && num_threads>1 ) {
		cout << "Calculations will be done using " << num_threads
        << " threads with " << blk_size << " shots per thread.\n";
	}

	chrono::high_resolution_clock::time_point begin, end;
	vector<vector<vector<sxyz<T>>>> r_data(src.size());
	
    if ( par.verbose ) { cout << "Computing traveltimes ... "; cout.flush(); }
	if ( par.time ) { begin = chrono::high_resolution_clock::now(); }

	if ( par.verbose ) { cout << "Computing traveltimes ... "; cout.flush(); }
	if ( par.time ) { begin = chrono::high_resolution_clock::now(); }
	if ( par.saveRaypaths ) {
		if ( num_threads == 1 ) {
			for ( size_t n=0; n<src.size(); ++n ) {
                g->raytrace(src[n].get_coord(), src[n].get_t0(), rcv.get_coord(),
                             rcv.get_tt(n), r_data[n]);
			}
		} else {
			// threaded jobs
			
			vector<thread> threads(num_threads-1);
			size_t blk_start = 0;
			for ( size_t i=0; i<num_threads-1; ++i ) {
                
				size_t blk_end = blk_start + blk_size;
				
				threads[i]=thread( [&g,&src,&rcv,&r_data,blk_start,blk_end,i]{
                    
					for ( size_t n=blk_start; n<blk_end; ++n ) {
                        g->raytrace(src[n].get_coord(), src[n].get_t0(), rcv.get_coord(),
                                      rcv.get_tt(n), r_data[n], i+1);
					}
				});
				
				blk_start = blk_end;
			}
			for ( size_t n=blk_start; n<nTx; ++n ) {
                g->raytrace(src[n].get_coord(), src[n].get_t0(), rcv.get_coord(),
                            rcv.get_tt(n), r_data[n], 0);
			}
			
			for_each(threads.begin(),threads.end(), mem_fn(&thread::join));
		}
	} else {
		if ( num_threads == 1 ) {
			for ( size_t n=0; n<src.size(); ++n ) {
				g->raytrace(src[n].get_coord(), src[n].get_t0(), rcv.get_coord(),
							rcv.get_tt(n));
			}
		} else {
			// threaded jobs
			
			vector<thread> threads(num_threads-1);
			size_t blk_start = 0;
			for ( size_t i=0; i<num_threads-1; ++i ) {
				
				size_t blk_end = blk_start + blk_size;
                
				threads[i]=thread( [&g,&src,&rcv,blk_start,blk_end,i]{
                    
					for ( size_t n=blk_start; n<blk_end; ++n ) {
						g->raytrace(src[n].get_coord(), src[n].get_t0(), rcv.get_coord(),
									rcv.get_tt(n), i+1);
					}
				});
				
				blk_start = blk_end;
			}
			for ( size_t n=blk_start; n<nTx; ++n ) {
                g->raytrace(src[n].get_coord(), src[n].get_t0(), rcv.get_coord(),
                            rcv.get_tt(n), 0);
			}
			
			for_each(threads.begin(),threads.end(), mem_fn(&thread::join));
		}
	}
	if ( par.time ) { end = chrono::high_resolution_clock::now(); }
    if ( par.verbose ) cout << "done.\n";
	if ( par.time ) {
		cout.precision(12);
		cout << "Time to perform raytracing: " <<
        chrono::duration<double>(end-begin).count() << '\n';
	}
	
	if ( par.saveGridTT>0 ) {
		//  will overwrite if nsrc>1
		string filename = par.basename+"_all_tt";
		g->saveTT(filename, 0, 0, par.saveGridTT==2);
        
//		string filename = par.basename+"_all_tt.dat";
//		g->saveTT(filename, 0);
	}
    

	
    delete g;
    
    if ( src.size() == 1 ) {
		string filename = par.basename+"_tt.dat";
		
		if ( par.verbose ) cout << "Saving traveltimes in " << filename <<  " ... ";
		rcv.save_tt(filename, 0);
		if ( par.verbose ) cout << "done.\n";
		
		if ( par.saveRaypaths ) {
			filename = par.basename+"_rp.vtp";
			if ( par.verbose ) cout << "Saving raypaths in " << filename <<  " ... ";
			saveRayPaths(filename, r_data[0]);
			if ( par.verbose ) cout << "done.\n";
		}
	} else {
        for ( size_t ns=0; ns<src.size(); ++ns ) {
            
            string srcname = par.srcfiles[ns];
            size_t pos = srcname.rfind("/");
            srcname.erase(0, pos+1);
			pos = srcname.rfind(".");
            size_t len = srcname.length()-pos;
            srcname.erase(pos, len);
            
            string filename = par.basename+"_"+srcname+"_tt.dat";
			
            if ( par.verbose ) cout << "Saving traveltimes in " << filename <<  " ... ";
            rcv.save_tt(filename, ns);
            if ( par.verbose ) cout << "done.\n";
			
            if ( par.saveRaypaths ) {
                filename = par.basename+"_"+srcname+"_rp.vtp";
                if ( par.verbose ) cout << "Saving raypaths of direct waves in " << filename <<  " ... ";
                saveRayPaths(filename, r_data[ns]);
                if ( par.verbose ) cout << "done.\n";
			}
			if ( par.verbose ) cout << '\n';
        }
	}
    
    if ( par.verbose ) cout << "Normal termination of program.\n";
    return 0;
}

int main(int argc, char * argv[])
{

    input_parameters par;
    
	string fname = parse_input(argc, argv, par);
	if ( par.verbose ) {
		cout << "\n*** Program ttcr2ds ***\n\n"
        << "Raytracing on undulated surfaces\n";
	}
    
    get_params(fname, par);
	
	if ( par.verbose ) {
        switch (par.method) {
            case SHORTEST_PATH:
                cout << "Shortest path method selected.\n";
                break;
            case FAST_SWEEPING:
                cout << "Fast sweeping method not available for ttcr2ds.\n";
                return 1;
            case FAST_MARCHING:
                cout << "Fast marching method not available for ttcr2ds.\n";
                return 1;
            default:
                break;
        }
    }
	
	if ( par.singlePrecision ) {
		return body<float>(par);
	} else {
		return body<double>(par);
	}

}

