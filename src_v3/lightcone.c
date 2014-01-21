#include "vars.h"
#include "proto.h"

// Set up the lightcone parameters
// ===============================
void set_lightcone(void) {

  Light = LIGHT / UnitLength_in_cm * UnitTime_in_s;

  // Calculate the actual maximum number of replicate boxes we need in all six directions. This allows
  // us to remove replicates that are unnecessary
  double Rcomov_max = Light/Hubble*SphiStd(1.0/(1.0+OutputList[0].Redshift),1.0);
  if ((int)(ceil(fabs((Origin_x - Rcomov_max)/Box))) < Nrep_neg_x) Nrep_neg_x = (int)(ceil(fabs((Origin_x - Rcomov_max)/Box)));
  if ((int)(ceil(fabs((Origin_y - Rcomov_max)/Box))) < Nrep_neg_y) Nrep_neg_y = (int)(ceil(fabs((Origin_y - Rcomov_max)/Box)));
  if ((int)(ceil(fabs((Origin_z - Rcomov_max)/Box))) < Nrep_neg_z) Nrep_neg_z = (int)(ceil(fabs((Origin_z - Rcomov_max)/Box)));
  if ((int)(ceil((Rcomov_max + Origin_x)/Box))-1 < Nrep_pos_x) Nrep_pos_x = (int)(ceil((Rcomov_max + Origin_x)/Box))-1;
  if ((int)(ceil((Rcomov_max + Origin_y)/Box))-1 < Nrep_pos_y) Nrep_pos_y = (int)(ceil((Rcomov_max + Origin_y)/Box))-1;
  if ((int)(ceil((Rcomov_max + Origin_z)/Box))-1 < Nrep_pos_z) Nrep_pos_z = (int)(ceil((Rcomov_max + Origin_z)/Box))-1;

  // Create an array of flags that for each replicate tell us whether or not we need to check the particles in it.
  // A flag of 0 means we check it, 1 means the box is completely inside the lightcone so no need to check it this step 
  // 2 means the box has completely left the lightcone so no need to ever check it again.
  repflag = (int *)calloc((Nrep_neg_x+Nrep_pos_x+1)*(Nrep_neg_y+Nrep_pos_y+1)*(Nrep_neg_z+Nrep_pos_z+1),sizeof(int));
  
  Nrep_neg_max[0] = Nrep_neg_x; Nrep_pos_max[0] = Nrep_pos_x;
  Nrep_neg_max[1] = Nrep_neg_y; Nrep_pos_max[1] = Nrep_pos_y;
  Nrep_neg_max[2] = Nrep_neg_z; Nrep_pos_max[2] = Nrep_pos_z;
  

  return;
}

// Flag replicates that don't need to be looped over this iteration, as they are either completely inside or outside the lightcone
// ===============================================================================================================================
void flag_replicates(double Rcomov_old, double Rcomov_new, double boundary) {

  int i, j, k, ii, jj, kk; 
  int repcount_low, coord;
  double Xvert, Yvert, Zvert, Rvert;
  double dist_face_old, dist_face_new;
  double Rcomov_old2 = Rcomov_old*Rcomov_old;
  double Rcomov_new2 = Rcomov_new*Rcomov_new;

  // Check the maximum number of replicates in each direction and update if necessary
  if ((int)(ceil(fabs((Origin_x - Rcomov_old)/Box))) < Nrep_neg_x) Nrep_neg_x = (int)(ceil(fabs((Origin_x - Rcomov_old)/Box)));
  if ((int)(ceil(fabs((Origin_y - Rcomov_old)/Box))) < Nrep_neg_y) Nrep_neg_y = (int)(ceil(fabs((Origin_y - Rcomov_old)/Box)));
  if ((int)(ceil(fabs((Origin_z - Rcomov_old)/Box))) < Nrep_neg_z) Nrep_neg_z = (int)(ceil(fabs((Origin_z - Rcomov_old)/Box)));
  if ((int)(ceil((Rcomov_old + Origin_x)/Box))-1 < Nrep_pos_x) Nrep_pos_x = (int)(ceil((Rcomov_old + Origin_x)/Box))-1;
  if ((int)(ceil((Rcomov_old + Origin_y)/Box))-1 < Nrep_pos_y) Nrep_pos_y = (int)(ceil((Rcomov_old + Origin_y)/Box))-1;
  if ((int)(ceil((Rcomov_old + Origin_z)/Box))-1 < Nrep_pos_z) Nrep_pos_z = (int)(ceil((Rcomov_old + Origin_z)/Box))-1;

  // Loop over all replicates and if any replicate is completely within the lightcone (i.e. all eight vertices are within Rcomov_new)
  // then flag it so that we don't have to loop over it. NOTE: this method will not work to see if replicates are completely outside the
  // lightcone, as there can easily be a case where the lightcone is inside all eight vertices of a replicated box, yet the lightcone still 
  // passes through it. In this case the routine is much more complicated and involves looping over all 6 faces of the replicate, and computing
  // the shortest distance to the finite plane of the face.
  for (i = -Nrep_neg_x; i<=Nrep_pos_x; i++) {
    for (j = -Nrep_neg_y; j<=Nrep_pos_y; j++) {
      for (k = -Nrep_neg_z; k<=Nrep_pos_z; k++) {

        coord = ((i+Nrep_neg_max[0])*(Nrep_neg_max[1]+Nrep_pos_max[1]+1)+(j+Nrep_neg_max[1]))*(Nrep_neg_max[2]+Nrep_pos_max[2]+1)+(k+Nrep_neg_max[2]);

        // Skip this replicate if we already know it is completely outside the lightcone
        // the particle positions first
        if (repflag[coord] == 2) continue;
 
        // Loop over all the vertices
        repflag[coord] = 0;
        repcount_low = 0;
        for (ii = 0; ii < 2; ii++) {
          for (jj = 0; jj < 2; jj++) {
            for (kk = 0; kk < 2; kk++) {

              Xvert = (i+((ThisTask+ii)/(double)NTask))*Box - Origin_x;
              Yvert = (j+jj)*Box - Origin_y;
              Zvert = (k+kk)*Box - Origin_z;
              Rvert = Xvert*Xvert+Yvert*Yvert+Zvert*Zvert;
                  
              // Include a buffer region to account for the fact that the particle might move beyond the box boundaries, 20Mpc should be enough.
              if (Rvert < Rcomov_new2-boundary) repcount_low++;
            }
          }
        }
            
        // If box is completely inside the lightcone, we flag it and continue into the next repklicate
        if (repcount_low == 8) {
          repflag[coord] = 1;
          continue;
        }

        // Otherwise we check to see if the replicate is completely outside the lightcone. This is where it gets more complicated. 
        // For all the necessary faces of the replicate, we calculate the shortest possible distance from the origin to the face.
        // This is given by the sum of the squares of the distance to the projected origin and the 
        // distance from the projected origin to the nearest line segment, which itself is the sum of the squares 
        // of the distance from the projection of the origin onto the plane to the projection of this onto the 
        // nearest line segment and the distance along the line segment.
        if (repcount_low == 0) {

          dist_face_old = 1.0e30;
          dist_face_new = 1.0e30;

          // Check the two faces perpendicular to the x-axis (don't forget these are different depending on the task).
          if (Origin_x < (i+(ThisTask/(double)NTask))*Box) {
            dist_face_old = Origin_x - (i+(ThisTask/(double)NTask))*Box;
            dist_face_old *= dist_face_old;
            dist_face_old += nearest_dist(Origin_y, Origin_z, j, k, j+1, k+1);
          } else if (Origin_x > (i+((ThisTask+1)/(double)NTask))*Box) {
            dist_face_old = Origin_x - (i+((ThisTask+1)/(double)NTask))*Box;
            dist_face_old *= dist_face_old;
            dist_face_old += nearest_dist(Origin_y, Origin_z, j, k, j+1, k+1);
          }

          // Check the two faces perpendicular to the y-axis.
          if (Origin_y < j*Box) {
            dist_face_new = Origin_y - j*Box;
            dist_face_new *= dist_face_old;
            dist_face_new += nearest_dist(Origin_x, Origin_z, i+(ThisTask/(double)NTask), k, i+((ThisTask+1)/(double)NTask), k+1);
          } else if (Origin_y > (j+1)*Box) {
            dist_face_new = Origin_y - (j+1)*Box;
            dist_face_new *= dist_face_old;
            dist_face_new += nearest_dist(Origin_x, Origin_z, i+(ThisTask/(double)NTask), k, i+((ThisTask+1)/(double)NTask), k+1);
          }
          if (dist_face_old > dist_face_new) dist_face_old = dist_face_new;

          // Check the two faces perpendicular to the z-axis.
          if (Origin_z < k*Box) {
            dist_face_new = Origin_z - k*Box;
            dist_face_new *= dist_face_old;
            dist_face_new += nearest_dist(Origin_x, Origin_y, i+(ThisTask/(double)NTask), j, i+((ThisTask+1)/(double)NTask), j+1);
          } else if (Origin_z > (k+1)*Box) {
            dist_face_new = Origin_z - (k+1)*Box;
            dist_face_new *= dist_face_old;
            dist_face_new += nearest_dist(Origin_x, Origin_y, i+(ThisTask/(double)NTask), j, i+((ThisTask+1)/(double)NTask), j+1);
          }
          if (dist_face_old > dist_face_new) dist_face_old = dist_face_new;
          
          // If dist_face_old is greater than 9.9e29 then the origin is WITHIN the current replicate so we definitely have to loop over it.
          // Otherwise dist_face_old now contains the shortest distance from ANY point on the replicate to the lightcone. Hence if this is greater than Rcomov_old2
          // we never have to loop over this replicate again
          if (dist_face_old < 9.9e29) {
            if (dist_face_old > Rcomov_old2+boundary) repflag[coord] = 2;
          }        
        }
      }
    }
  }

return;
}


// For a given face on a replicate this routine returns the shortest distance between the face, bounded by (ix, iy) and (jx, jy), and the point (px, py)
double nearest_dist(double px, double py, double ix, double iy, double jx, double jy) {

  double dist1, dist2;
  double dist_line_old, dist_line_new;

  dist_line_old = 0.0;
  dist_line_new = 0.0;

  // Check the 4 line segments of the face of the replicate that is contained on the plane. For each necessary line segment we compute 
  // the distance, 'dist1', of the projection of the projected origin on the plane onto an infinite line containing the line segment (2D -> 1D).
  // The distance along the line between the 1D projection and the end of the line segment is then dist2. NOTE: for projections that land
  // inside the face, .i.e. within all four line segments, we don't care about dist1 or dist2 as the shortest distance between the point and the
  // replicate is just dist1. Also, we only have to do a maximum of 2 line segments then as this is the most that any point can see.
  if (px < ix*Box ) {
    dist2 = 0.0;
    dist1 = px - ix*Box;
    if (py < iy*Box) {
      dist2 = py - iy*Box; 
    } else if (py > jy*Box) {
      dist2 = py - jy*Box;
    }          
    dist_line_old = dist1*dist1+dist2*dist2;
  } else if (px > jx*Box) {
    dist2 = 0.0;
    dist1 = px - jx*Box;
    if (py < iy*Box) {
      dist2 = py - iy*Box; 
    } else if (py > jy*Box) {
      dist2 = py - jy*Box;
    }          
    dist_line_old = dist1*dist1+dist2*dist2;
  }
 
  // The third and fourth line segments
  if (py < iy*Box) {
    dist2 = 0.0;
    dist1 = py - iy*Box;
    if (px < ix*Box) {
      dist2 = px - ix*Box; 
    } else if (px > jx*Box) {
      dist2 = px - jx*Box;
    }        
    dist_line_new = dist1*dist1+dist2*dist2;
  } else if (py > jy*Box) {
    dist2 = 0.0;
    dist1 = py - jy*Box;
    if (px < ix*Box) {
      dist2 = px - ix*Box; 
    } else if (px > jx*Box) {
      dist2 = px - jx*Box;
    }        
    dist_line_new = dist1*dist1+dist2*dist2;
  }

  // Find the shortest distance to any of the line segments
  if (dist_line_old > dist_line_new) dist_line_old = dist_line_new;
  
  return dist_line_old;

}



// Drift and output the particles for lightcone simulations
// ========================================================
void Drift_Lightcone(double A, double AFF, double AF, double Di, double Di2, int timeStep) {

  // We'll flag to see if the particle has left the lightcone here and output if it has
  // We don't bother interpolating the velocity, only the position, as the implicit assumption
  // with KDK anyway is that the velocity at the halfway point in the timestep is constant
  // between the initial and final particle positions for that timestep. If we did want to interpolate
  // the velocity, we could move this whole section to the location marked above, then reverse the 
  // updating and periodic wrapping of the particle positions to allow interpolation.
  // To avoid having to store all the replicate of the particles, we also output the particles as soon 
  // as they are flagged, allowing us to loop over all the replicated boxes instead. Finally, because all
  // the lightcone stuff is done here, we don't need memory to store the particle flags.

  size_t bytes;
  int NTAB = 1000;                                       // The length of the particle exit time lookup tables (we spline anyway so not really important)
  int i, j, k, coord, flag, repcount, repcountmax;
  unsigned int n, pc, blockmaxlen;
  unsigned int outputflag, NumPartMax, NumPartMin;
  float * block;
  double dyyy, da1, da2, dv1, dv2;
  double dyyy_tmp, da1_tmp, da2_tmp, AL;
  double Delta_Pos[3];
  double boundary = 20.0;
  double fac = Hubble/pow(AF,1.5);
  double lengthfac = UnitLength_in_cm/3.085678e24;       // Convert positions to Mpc/h
  double velfac    = UnitVelocity_in_cm_per_s/1.0e5;     // Convert velocities to km/s
  double Rcomov_old  = Light/Hubble*SphiStd(A,1.0);
  double Rcomov_new  = Light/Hubble*SphiStd(AFF,1.0);
  double Rcomov_old2 = Rcomov_old*Rcomov_old; 
  double Rcomov_new2 = Rcomov_new*Rcomov_new;
  double Xpart, Ypart, Zpart, Rpart_old, Rpart_new, Rpart_old2, Rpart_new2;   
  double * AL_tab, * da1_tab, * da2_tab, * dyyy_tab;
  gsl_spline * da1_spline, * da2_spline, * dyyy_spline;                                // Spline fits to the exit-time lookup tables
  gsl_interp_accel * da1_acc, * da2_acc, * dyyy_acc; 

  if (StdDA == 0) {
    dyyy=Sq(A,AFF,AF);
  } else if (StdDA == 1) {
    dyyy=(AFF-A)/Qfactor(AF);
  } else {
    dyyy=SqStd(A,AFF);
  }

  da1=growthD(AFF)-Di;    // change in D
  da2=growthD2(AFF)-Di2;  // change in D_{2lpt}

  // Add back LPT velocities if we had subtracted them. 
  // This corresponds to L_+ operator in TZE.
  dv1 = DprimeQ(AF);    // dD_{za}/dy
  dv2 = growthD2v(AF);  // dD_{2lpt}/dy

  // Flag the replicates that don't need looping over
  flag_replicates(Rcomov_old, Rcomov_new, boundary);

  // Create lookup tables for interpolating the time when the particle left the lightcone 
  // and the corresponding growth factors. Using lookup tables (for the growth factors especially) makes a HUGE difference. 
  // (In my small tests it reduced the time by a factor of TEN!!). Not sure why the growthD and growthD2 routines are so slow.
  AL_tab = (double *)malloc(NTAB*sizeof(double));
  da1_tab = (double *)malloc(NTAB*sizeof(double));
  da2_tab = (double *)malloc(NTAB*sizeof(double));
  dyyy_tab = (double *)malloc(NTAB*sizeof(double));
  for (i=0; i<NTAB; i++) {
    AL = (i*(AFF-A))/(NTAB-1.0) + A;
    AL_tab[i] = AL;
    da1_tab[i] = growthD(AL)- Di;
    da2_tab[i] = growthD2(AL) - Di2;
    if (StdDA == 0) {
      dyyy_tab[i]=Sq(A,AL,AF);
    } else if (StdDA == 1) {
      dyyy_tab[i]=(AL-A)/Qfactor(AF);
    } else {
      dyyy_tab[i]=SqStd(A,AL);
    } 
  }
  da1_acc = gsl_interp_accel_alloc();
  da2_acc = gsl_interp_accel_alloc();
  dyyy_acc = gsl_interp_accel_alloc();
  da1_spline = gsl_spline_alloc(gsl_interp_cspline, NTAB);
  da2_spline = gsl_spline_alloc(gsl_interp_cspline, NTAB);
  dyyy_spline = gsl_spline_alloc(gsl_interp_cspline, NTAB);
  gsl_spline_init(da1_spline, AL_tab, da1_tab, NTAB);
  gsl_spline_init(da2_spline, AL_tab, da2_tab, NTAB);
  gsl_spline_init(dyyy_spline, AL_tab, dyyy_tab, NTAB);

  free(AL_tab);
  free(da1_tab);
  free(da2_tab);
  free(dyyy_tab);

  // For the outputting we can only moderate how many tasks output at once if they all enter the output stage at the same time.
  // As such we output at set times rather than when actually necessary. This also alleviates the need for
  // any inter-task communications, which would be necessary if we were to only output once a task has filled
  // it's quota. Unfortunately, as the number of particles on each task will be different we have to make 
  // each task loop over the maximum number of particles on any task, but most tasks will not have to actually do anything.
  // This will result in more outputs than is truly necessary. We also must assume that all looped over replicates of a 
  // given particle are to be outputted even if they are not.

  // Calculate the global maximum number of particles on a task and the global minimum number of particles we can store.
  // Then allocate memory to store the particles that we are outputting. We may have some spare memory from 
  // deallocating the force grids on top of that from deallocating the displacement arrays. 
  ierr = MPI_Allreduce(&NumPart, &NumPartMax, 1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
  ierr = MPI_Allreduce(&NumPart, &NumPartMin, 1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
#ifdef MEMORY_MODE
  block = (float *)malloc(bytes = 6*Total_size*sizeof(float_kind)+3*NumPartMin*sizeof(float));
#else
  block = (float *)malloc(bytes = 3*NumPartMin*sizeof(float_kind));
#endif
  
  // Loop over all replicates and calculate the global maximum repcount.
  // How many particles can we store assuming all tasks replicate the particles by the maximum amount?
  repcount = 0;
  for (i = -Nrep_neg_x; i<=Nrep_pos_x; i++) {
    for (j = -Nrep_neg_y; j<=Nrep_pos_y; j++) {
      for (k = -Nrep_neg_z; k<=Nrep_pos_z; k++) {
        coord = ((i+Nrep_neg_max[0])*(Nrep_neg_max[1]+Nrep_pos_max[1]+1)+(j+Nrep_neg_max[1]))*(Nrep_neg_max[2]+Nrep_pos_max[2]+1)+(k+Nrep_neg_max[2]);
        if (repflag[coord] == 0) repcount++;
      }
    }
  }
  ierr = MPI_Allreduce(&repcount, &repcountmax, 1, MPI_UNSIGNED, MPI_MAX, MPI_COMM_WORLD);
  blockmaxlen = (unsigned int)(bytes / (6 * sizeof(float) * repcountmax));

  // Loop over all particles, modifying the position based on the current replicate
  pc = 0;
  outputflag = 0;
  for(n=0; n<NumPartMax; n++) {

    outputflag++;
    if (n < NumPart) {

      Delta_Pos[0] = (P[n].Vel[0]-sumx)*dyyy+subtractLPT*(P[n].Dz[0]*da1+P[n].D2[0]*da2);
      Delta_Pos[1] = (P[n].Vel[1]-sumy)*dyyy+subtractLPT*(P[n].Dz[1]*da1+P[n].D2[1]*da2);   
      Delta_Pos[2] = (P[n].Vel[2]-sumz)*dyyy+subtractLPT*(P[n].Dz[2]*da1+P[n].D2[2]*da2);     

      // Check that 20Mpc/h boundaries is enough
      if (Delta_Pos[0]*Delta_Pos[0]+Delta_Pos[1]*Delta_Pos[1]+Delta_Pos[2]*Delta_Pos[2] > boundary) {
        printf("\nERROR: Particle displacement greater than boundary for lightcone replicate estimate.\n");
        printf("       increase boundary condition in lightcone.c (line 56)\n\n");
      FatalError("lightcone.c", 212);
      }

      // Loop over all replicates
      for (i = -Nrep_neg_x; i<=Nrep_pos_x; i++) {
        for (j = -Nrep_neg_y; j<=Nrep_pos_y; j++) {
          for (k = -Nrep_neg_z; k<=Nrep_pos_z; k++) {

            coord = ((i+Nrep_neg_max[0])*(Nrep_neg_max[1]+Nrep_pos_max[1]+1)+(j+Nrep_neg_max[1]))*(Nrep_neg_max[2]+Nrep_pos_max[2]+1)+(k+Nrep_neg_max[2]);
            if (repflag[coord] == 0) {

              // Did the particle start the timestep inside the lightcone?
              flag = 0;
              Xpart = P[n].Pos[0] - Origin_x + (i*Box);
              Ypart = P[n].Pos[1] - Origin_y + (j*Box);
              Zpart = P[n].Pos[2] - Origin_z + (k*Box);
              Rpart_old2 = Xpart*Xpart+Ypart*Ypart+Zpart*Zpart;
 
              if (Rpart_old2 <= Rcomov_old2) flag = 1;

              // Have any particles that started inside the lightcone now exited?
              if (flag) {
                Xpart += Delta_Pos[0];
                Ypart += Delta_Pos[1];
                Zpart += Delta_Pos[2];
                Rpart_new2 = Xpart*Xpart+Ypart*Ypart+Zpart*Zpart;

                if (Rpart_new2 > Rcomov_new2) {
  
                  // Interpolate the particle position. We do this by first calculating the exact time at which
                  // the particle exited the lightcone, then updating the position to there.
                  Rpart_old = sqrt(Rpart_old2);
                  Rpart_new = sqrt(Rpart_new2);
                  AL = A + (AFF-A)*((Rcomov_old-Rpart_old)/((Rpart_new-Rpart_old)-(Rcomov_new-Rcomov_old)));
                  da1_tmp = gsl_spline_eval(da1_spline, AL, da1_acc);
                  da2_tmp = gsl_spline_eval(da2_spline, AL, da2_acc);
                  dyyy_tmp = gsl_spline_eval(dyyy_spline, AL, dyyy_acc);
              
                  // Store the interpolated particle position and velocity.
                  block[6 * pc]     = (float)(lengthfac*(P[n].Pos[0] + (P[n].Vel[0]-sumx)*dyyy_tmp+subtractLPT*(P[n].Dz[0]*da1_tmp+P[n].D2[0]*da2_tmp) + (i*Box)));
                  block[6 * pc + 1] = (float)(lengthfac*(P[n].Pos[1] + (P[n].Vel[1]-sumy)*dyyy_tmp+subtractLPT*(P[n].Dz[1]*da1_tmp+P[n].D2[1]*da2_tmp) + (j*Box)));
                  block[6 * pc + 2] = (float)(lengthfac*(P[n].Pos[2] + (P[n].Vel[2]-sumz)*dyyy_tmp+subtractLPT*(P[n].Dz[2]*da1_tmp+P[n].D2[2]*da2_tmp) + (k*Box)));
                  block[6 * pc + 3] = (float)(velfac*fac*(P[n].Vel[0]-sumx+(P[n].Dz[0]*dv1+P[n].D2[0]*dv2)*subtractLPT));
                  block[6 * pc + 4] = (float)(velfac*fac*(P[n].Vel[1]-sumy+(P[n].Dz[1]*dv1+P[n].D2[1]*dv2)*subtractLPT));
                  block[6 * pc + 5] = (float)(velfac*fac*(P[n].Vel[2]-sumz+(P[n].Dz[2]*dv1+P[n].D2[2]*dv2)*subtractLPT));
                  pc++;   
                }
              }
            }
          }
        }
      }
 
      // Update the particle's position
      P[n].Pos[0] = periodic_wrap(P[n].Pos[0]+Delta_Pos[0]);
      P[n].Pos[1] = periodic_wrap(P[n].Pos[1]+Delta_Pos[1]);
      P[n].Pos[2] = periodic_wrap(P[n].Pos[2]+Delta_Pos[2]); 
    }
    
    if (outputflag == blockmaxlen) {
      Output_Lightcone(pc, timeStep, block);
      pc = 0;
      outputflag = 0;
    }
   
  }

  if (outputflag > 0) Output_Lightcone(pc, timeStep, block);
  free(block);
 
  gsl_spline_free(da1_spline);
  gsl_spline_free(da2_spline);
  gsl_spline_free(dyyy_spline);
  gsl_interp_accel_free(da1_acc);
  gsl_interp_accel_free(da2_acc);
  gsl_interp_accel_free(dyyy_acc);

  return;
}

// Output the lightcone data
// =========================
void Output_Lightcone(unsigned int pc, int timeStep, float * block) {

  FILE * fp; 
  char buf[300];
  int nprocgroup, groupTask, masterTask;
#ifdef GADGET_STYLE
  int dummy1, dummy2;
#else
  unsigned int n;
#endif

  nprocgroup = NTask / NumFilesWrittenInParallel;
  if (NTask % NumFilesWrittenInParallel) nprocgroup++;
  masterTask = (ThisTask / nprocgroup) * nprocgroup;
  for(groupTask = 0; groupTask < nprocgroup; groupTask++) {
    if (ThisTask == (masterTask + groupTask)) {
      if(pc > 0) {
        sprintf(buf, "%s/%s_lightcone.%d", OutputDir, FileBase, ThisTask);
        if (timeStep == 0) {
          // Overwrite any pre-existing output files otherwise we'll append onto the end of them.
          if(!(fp = fopen(buf, "w"))) {
            printf("\nERROR: Can't write in file '%s'.\n\n", buf);
            FatalError("lightcone.c", 93);
          }
        } else {
          if(!(fp = fopen(buf, "a"))) {
            printf("\nERROR: Can't write in file '%s'.\n\n", buf);
            FatalError("lightcone.c", 98);
          }
        }
#ifdef GADGET_STYLE
        // write coordinates and velocities in unformatted binary
        dummy1 = sizeof(pc); 
        dummy2 = sizeof(float) * 6 * pc;
        my_fwrite(&dummy1, sizeof(dummy1), 1, fp);
        my_fwrite(&pc, sizeof(pc), 1, fp);
        my_fwrite(&dummy1, sizeof(dummy1), 1, fp);
        my_fwrite(&dummy2, sizeof(dummy2), 1, fp);
        my_fwrite(block, sizeof(float), 6 * pc, fp);
        my_fwrite(&dummy2, sizeof(dummy2), 1, fp);
#else
        // write coordinates and velocities in ASCII
        for(n=0; n<pc; n++) fprintf(fp,"%12.6f %12.6f %12.6f %12.6f %12.6f %12.6f\n",block[6*n],block[6*n+1],block[6*n+2],block[6*n+3],block[6*n+4],block[6*n+5]);
#endif
        fclose(fp);
      }
    }
    ierr = MPI_Barrier(MPI_COMM_WORLD);
  }
 
  return;
}
