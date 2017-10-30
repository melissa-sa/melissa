/******************************************************************
*                            Melissa                              *
*-----------------------------------------------------------------*
*   COPYRIGHT (C) 2017  by INRIA and EDF. ALL RIGHTS RESERVED.    *
*                                                                 *
* This source is covered by the BSD 3-Clause License.             *
* Refer to the  LICENCE file for further information.             *
*                                                                 *
*-----------------------------------------------------------------*
*  Original Contributors:                                         *
*    Theophile Terraz,                                            *
*    Bruno Raffin,                                                *
*    Alejandro Ribes,                                             *
*    Bertrand Iooss,                                              *
******************************************************************/

!-------------------------------------------------------------------------------

!                      Code_Saturne version 4.2.1-patch
!                      --------------------------
! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2016 EDF S.A.
!
! This program is free software; you can redistribute it and/or modify it under
! the terms of the GNU General Public License as published by the Free Software
! Foundation; either version 2 of the License, or (at your option) any later
! version.
!
! This program is distributed in the hope that it will be useful, but WITHOUT
! ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
! FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
! details.
!
! You should have received a copy of the GNU General Public License along with
! this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
! Street, Fifth Floor, Boston, MA 02110-1301, USA.

!-------------------------------------------------------------------------------

!===============================================================================
! Function:
! ---------

! Basic example of cs_user_boundary_conditions subroutine.f90
!
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!> \param[in]     nvar          total number of variables
!> \param[in]     nscal         total number of scalars
!> \param[out]    icodcl        boundary condition code:
!>                               - 1 Dirichlet
!>                               - 2 Radiative outlet
!>                               - 3 Neumann
!>                               - 4 sliding and
!>                                 \f$ \vect{u} \cdot \vect{n} = 0 \f$
!>                               - 5 smooth wall and
!>                                 \f$ \vect{u} \cdot \vect{n} = 0 \f$
!>                               - 6 rough wall and
!>                                 \f$ \vect{u} \cdot \vect{n} = 0 \f$
!>                               - 9 free inlet/outlet
!>                                 (input mass flux blocked to 0)
!> \param[in]     itrifb        indirection for boundary faces ordering
!> \param[in,out] itypfb        boundary face types
!> \param[out]    izfppp        boundary face zone number
!> \param[in]     dt            time step (per cell)
!> \param[in,out] rcodcl        boundary condition values:
!>                               - rcodcl(1) value of the dirichlet
!>                               - rcodcl(2) value of the exterior exchange
!>                                 coefficient (infinite if no exchange)
!>                               - rcodcl(3) value flux density
!>                                 (negative if gain) in w/m2 or roughness
!>                                 in m if icodcl=6
!>                                 -# for the velocity \f$ (\mu+\mu_T)
!>                                    \gradt \, \vect{u} \cdot \vect{n}  \f$
!>                                 -# for the pressure \f$ \Delta t
!>                                    \grad P \cdot \vect{n}  \f$
!>                                 -# for a scalar \f$ cp \left( K +
!>                                     \dfrac{K_T}{\sigma_T} \right)
!>                                     \grad T \cdot \vect{n} \f$
!_______________________________________________________________________________

subroutine cs_f_user_boundary_conditions &
 ( nvar   , nscal  ,                                              &
   icodcl , itrifb , itypfb , izfppp ,                            &
   dt     ,                                                       &
   rcodcl )

!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use optcal
use cstphy
use cstnum
use entsor
use parall
use period
use ihmpre
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use ppcpfu
use atincl
use ctincl
! use elincl
use cs_fuel_incl
use mesh
use field
use cs_c_bindings

!===============================================================================

implicit none

! Arguments

integer          nvar   , nscal

integer          icodcl(nfabor,nvarcl)
integer          itrifb(nfabor), itypfb(nfabor)
integer          izfppp(nfabor)

double precision dt(ncelet)
double precision rcodcl(nfabor,nvarcl,3)

! Local variables

!< [loc_var_dec]
integer          ifac, iel, ii
integer          ilelt, nlelt
double precision uref2
double precision rhomoy, xdh
double precision xitur
double precision :: ymin, ymax, y, largeur
double precision :: temps_declenchement_haut, param_duree_injection_haut, param_intensite_haut
double precision :: temps_declenchement_bas, param_duree_injection_bas, param_intensite_bas
double precision :: param_mult_vit, param_largeur_haut, param_largeur_bas

integer, allocatable, dimension(:) :: lstelt
double precision, dimension(:), pointer :: bfpro_rom

!===============================================================================

!===============================================================================
! Initialization
!===============================================================================

allocate(lstelt(nfabor))  ! temporary array for boundary faces selection

call field_get_val_s(ibrom, bfpro_rom)

temps_declenchement_haut=0.8
temps_declenchement_bas=0.8
param_duree_injection_haut=0.1
param_duree_injection_bas=0.1
param_intensite_haut=1.0
param_intensite_bas=1.0
param_mult_vit=1.0
param_largeur_haut=1.0
param_largeur_bas=1.0

!===============================================================================
! Assign boundary conditions to boundary faces here

! For each subset:
! - use selection criteria to filter boundary faces of a given subset
! - loop on faces from a subset
!   - set the boundary condition for each face
!===============================================================================


ymin = 1e12
ymax = -1e12

call getfbr('left', nlelt, lstelt)

do ilelt = 1, nlelt

  ifac = lstelt(ilelt)
  iel = ifabor(ifac)

  itypfb(ifac) = ientre

  if (cdgfbo(2,ifac) > 0) then
    if (ymin .gt. cdgfbo(2,ifac)) ymin = cdgfbo(2,ifac)
    if (ymax .lt. cdgfbo(2,ifac)) ymax = cdgfbo(2,ifac)
  endif

enddo

call parmin(ymin)
call parmax(ymax)

do ilelt = 1, nlelt

  ifac = lstelt(ilelt)
  iel = ifabor(ifac)

  itypfb(ifac) = ientre

  rcodcl(ifac,iu,1) = 10.0d0 * param_mult_vit
  rcodcl(ifac,iv,1) = 0.0d0 * param_mult_vit
  rcodcl(ifac,iw,1) = 0.0d0 * param_mult_vit

  uref2 = rcodcl(ifac,iu,1)**2  &
        + rcodcl(ifac,iv,1)**2  &
        + rcodcl(ifac,iw,1)**2
  uref2 = max(uref2,1.d-12)

  uref2 = 1.0d0

  !   Turbulence example computed using equations valid for a pipe.

  !   We will be careful to specify a hydraulic diameter adapted
  !     to the current inlet.

  !   We will also be careful if necessary to use a more precise
  !     formula for the dynamic viscosity use in the calculation of
  !     the Reynolds number (especially if it is variable, it may be
  !     useful to take the law from 'usphyv'. Here, we use by default
  !     the 'viscl0" value.
  !   Regarding the density, we have access to its value at boundary
  !     faces (romb) so this value is the one used here (specifically,
  !     it is consistent with the processing in 'usphyv', in case of
  !     variable density)

  !     Hydraulic diameter
  xdh     = 0.045d0

  !   Calculation of turbulent inlet conditions using
  !     standard laws for a circular pipe
  !     (their initialization is not needed here but is good practice).
  rhomoy  = bfpro_rom(ifac)

  call turbulence_bc_inlet_hyd_diam(ifac, uref2, xdh, rhomoy, viscl0, rcodcl)

  ! Handle scalars
  if (cdgfbo(2,ifac) > 0) then
    if (nscal.gt.0) then
      largeur = (ymax - ymin) * param_largeur_haut
      rcodcl(ifac,isca(1),1) = 0
      if (ttcabs .gt. temps_declenchement_haut .and. ttcabs .lt. (temps_declenchement_haut + param_duree_injection_haut)) then
        y = cdgfbo(2,ifac)
        if ((y .lt. (ymax - ((ymax-ymin) - largeur)/2) .and. y .gt. (ymin + ((ymax-ymin) - largeur)/2))) then
          rcodcl(ifac,isca(1),1) = param_intensite_haut
        endif
      endif
    endif
  else ! (cdgfbo(2,ifac) < 0
    if (nscal.gt.0) then
      largeur = (ymax - ymin) * param_largeur_bas
      rcodcl(ifac,isca(1),1) = 0
      if (ttcabs .gt. temps_declenchement_bas .and. ttcabs .lt. (temps_declenchement_bas + param_duree_injection_bas)) then
        y = cdgfbo(2,ifac)
        if ((y .lt. (-1 * ymin - ((ymax-ymin) - largeur)/2) .and. y .gt. (-1 * ymax + ((ymax-ymin) - largeur)/2))) then
          rcodcl(ifac,isca(1),1) = param_intensite_bas
        endif
      endif
    endif
  endif
  if (nscal.gt.1) then
      rcodcl(ifac,isca(2),1) = 20.d0
  endif

enddo

!--------
! Formats
!--------

!----
! End
!----

!< [finalize]
deallocate(lstelt)  ! temporary array for boundary faces selection
!< [finalize]

return
end subroutine cs_f_user_boundary_conditions
