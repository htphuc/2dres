/* *********************************************************************
 *                                                                    *
 * File Name: Prog.cc                                                 *
 *                                                                    *
 * Class Name: Prog                                                   *
 *                                                                    *
 * Goal: Prog class that steers all the others                        *
 *                                                                    *
 * Copyright (C) 04/2002  Arthur Moncorge                             *
 *                        arthur.moncorge@ensta.org                   *
 *                                                                    *
 * This program is free software; you can redistribute it and/or      *
 * modify it under the terms of the GNU General Public License        *
 * as published by the Free Software Foundation; either version 2     *
 * of the License, or (at your option) any later version.             *
 *                                                                    *
 * This program is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 * GNU General Public License for more details.                       *
 *                                                                    *
 * You should have received a copy of the GNU General Public License  *
 * along with this program; if not, write to the Free Software        *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,             *
 * MA 02111-1307, USA.                                                *
 *                                                                    *
 *********************************************************************/
#include "Prog.hh"

Prog::Prog(prog_arg prog_val) {
  Np=0;
  Nt=0;
  Ne=0;
  hmin=0.0;
  hmax=0.0;
  Refp=0;
  Coorp=0;
  Coort=0;
  Coore=0;
  area=0;
  SumMass=0;
  invMl=0;
  invM=0;
  Al=0;
  Ihat=0;
  Ixhat=0;
  Iyhat=0;
  IyRT=0;
  T_edge=0;
  edge=0;
  segINJE=0;
  segPROD=0;
  segBORD=0;
  total_area=0.0;
  Rinje=0.0;
  init_alpha=0;
  coort_file = 0;
  coorp_file = 0;
  alpha_out = 0;
  fracmat = 0;
  result = 0;
  production_log = 0;
  result_prefix = 0;

  meshfile       = prog_val.meshfile;
  Rinje          = prog_val.Rinje;
  dt             = prog_val.dt;
  mu1            = prog_val.mu1;
  mu2            = prog_val.mu2;
  p0             = prog_val.p0;
  u_in           = prog_val.u_in;
  e_g            = prog_val.e_g;
  MAXTIME        = prog_val.MAXTIME;
  visu_step      = prog_val.visu_step;
  init_alpha     = prog_val.init_alpha;
  coort_file     = prog_val.coort_file;
  coorp_file     = prog_val.coorp_file;
  alpha_out      = prog_val.alpha_out;
  fracmat        = prog_val.fracmat;
  result         = prog_val.result;
  production_log = prog_val.production_log;
  result_prefix  = prog_val.result_prefix;

  cout << "Create Prog" << endl;
  int debug = 1;
  FILE* file;
  int t=0;
  int i=0;

  struct Mesh::mesh_data            mesh_val;
  struct MixHy::mixhy_arg          mixhy_val;
  struct IterAlph::iteralph_arg iteralph_val;

  char newline;

  nloop     = 4;
  limitgrad = 1; // We use MUSCL when =1
  inject0_1 = 0; // We inject 0 into 1, inject0=1, used for debugging


  // Create mesh
  mesh_  = new Mesh(meshfile);

  // Get mesh data
  mesh_->get(&mesh_val);
  set_mesh_values(mesh_val);

  double flux_total = u_in * mesh_->injLength_;
  flux_in = -u_in/mesh_->injLength_; // m/s

  dt = 0.5 * nloop*hmin/u_in;
  cout << " timestep: " << dt << endl;
  cout << " total area of the mesh is: " << total_area << endl;
  cout << " t sweep is : " << total_area/(flux_total*dt) << endl;

  // Allocate Global memory
  alloc();

  // Create Mixte Hybrid object
  put_mixhy_arg(&mixhy_val);
  mixte_ = new MixHy(mixhy_val);

  // Create Advection Alpha Object
  put_iteralph_arg(&iteralph_val);
  advect_ = new IterAlph(iteralph_val);
  time_ = 0.0;
}

void Prog::updateP() {
  mixte_->update(alpha, pressure, flux);
}

void Prog::updateA() {
  time_ += dt;
  advect_->iteration_alpha(flux, alpha);
}


void Prog::alloc() {
  FILE* file;
  int i=0;

  alpha   = new double[Nt];
  pressure= new double[Nt];
  mobility_ = new double[Nt];
  flux = new double* [Nt];

  for (i=0; i<Nt; i++) {
    mobility_[i] = 1.0;
    flux[i] = new double[3];
    memset(flux[i], 0, 3*sizeof(double));
  }

  memset(alpha, 0, Nt*sizeof(double));
  memset(pressure, 0, Nt*sizeof(double));
  if(inject0_1 == 1) { // 0 injected into 1, only for debugging
    for (i=0; i<Nt; i++)
      alpha[i]=1.0;
  }

  if(init_alpha == 1) {
    // a file alpha_out.mat is saved at each time step
    // this should really be a file name that the user supplys
    cout << "read alpha" << endl;
    file = fopen("alpha_in.mat","r");
    for (i=0; i<Nt; i++) {
      fscanf(file, "%lf", &(alpha[i]));
    }
    fclose(file);
  }
}


Prog::~Prog() {
  int i=0;

  cout << "Destructor Prog" << endl;
  delete [] alpha;
  delete [] pressure;
  delete [] mobility_;

  for (i=0; i<Nt; i++) {
    delete[] flux[i];
  }
  delete[] flux;
  delete mesh_;
  delete mixte_;
  delete advect_;
}


void Prog::set_mesh_values(struct Mesh::mesh_data mesh_val) {
  Np      = mesh_val.Np;
  Nt      = mesh_val.Nt;
  Ne      = mesh_val.Ne;
  hmin    = mesh_val.hmin;
  hmax    = mesh_val.hmax;
  Refp    = mesh_val.Refp;
  Coorp   = mesh_val.Coorp;
  Coort   = mesh_val.Coort;
  Coore   = mesh_val.Coore;
  area    = mesh_val.area;
  SumMass = mesh_val.SumMass;
  invMl   = mesh_val.invMl;
  invM    = mesh_val.invM;
  Al      = mesh_val.Al;
  Ihat    = mesh_val.Ihat;
  Ixhat   = mesh_val.Ixhat;
  Iyhat   = mesh_val.Iyhat;
  IyRT    = mesh_val.IyRT;
  T_edge  = mesh_val.T_edge;
  edge    = mesh_val.edge;
  segINJE = mesh_val.segINJE;
  segPROD = mesh_val.segPROD;
  segBORD = mesh_val.segBORD;
  total_area = mesh_val.total_area;
}


void Prog::put_mixhy_arg(struct MixHy::mixhy_arg *mixhy_val) {
  mixhy_val->Nt          = Nt;
  mixhy_val->Ne          = Ne;
  mixhy_val->Coore       = Coore;
  mixhy_val->Al          = Al;
  mixhy_val->IyRT        = IyRT;
  mixhy_val->segINJE     = segINJE;
  mixhy_val->segPROD     = segPROD;
  mixhy_val->invM        = invM;
  mixhy_val->invMl       = invMl;
  mixhy_val->edge        = edge;
  mixhy_val->T_edge      = T_edge;
  mixhy_val->mu1          = mu1;
  mixhy_val->mu2          = mu2;
  mixhy_val->flux_in      = flux_in;
  mixhy_val->e_g          = e_g;
  mixhy_val->MAXTIME      = MAXTIME;
  mixhy_val->dt           = dt;
  mixhy_val->production_log   = production_log;
  mixhy_val->productionname   = productionname;
  mixhy_val->mobility = mobility_;
}

void Prog::put_iteralph_arg(struct IterAlph::iteralph_arg *iteralph_val) {
  iteralph_val->Nt        = Nt;
  iteralph_val->Np        = Np;
  iteralph_val->dt        = dt;
  iteralph_val->flux_in   = flux_in;
  iteralph_val->segINJE   = segINJE;
  iteralph_val->nloop     = nloop;
  iteralph_val->limitgrad = limitgrad;
  iteralph_val->inject0_1 = inject0_1;
  iteralph_val->area      = area;
  iteralph_val->T_edge    = T_edge;
  iteralph_val->Ihat      = Ihat;
  iteralph_val->Ixhat     = Ixhat;
  iteralph_val->Iyhat     = Iyhat;
  iteralph_val->Coort     = Coort;
  iteralph_val->Coorp     = Coorp;
  iteralph_val->SumMass   = SumMass;
}


// Create Visualiation Object
  // put_visu_arg(&visu_val);
  // Visu visu(visu_val);

  // /* ******************** Main loop ***********************/
  // for (t=0; t<MAXTIME; t++) {
  //   if (t==MAXTIME-1) {
  //     cout << "FINAL RUN" << endl;
  //     mixte.production_log=1;
  //   }
  //   mixte.update(t, alpha, pressure, flux);
  //   advect.iteration_alpha(flux, alpha);
  //   cout << "iteration_alpha t=" << t << endl;

  //   //visu.update_arival_time(alpha, t*dt);

  //   if (alpha_out) {
  //     // save alpha on triangles after each time step
  //     file = fopen(alpha_outname,"w");
  //     for (i=0; i<Nt; i++) {
  //       fprintf(file, "%g\n", alpha[i]);
  //     }
  //     fclose(file);
  //   }
  //   if (fmod(t, visu_step) == 0){
  //     // save the data to create movie with matlab
  //     visu.update(pressure, alpha, t);
  //   }
  // }
  // /* *********************************************************/

  // // final write to flood ammount
  // visu.update(pressure, alpha, t-1);
  // //visu.write_arrival_time(result_prefix);
  // //visu.write_well_flux(result_prefix, flux,T_edge);
  // /* Free memory */
  // cout << "Computation Finished" << endl;
