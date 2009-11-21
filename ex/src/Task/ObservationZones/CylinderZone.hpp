/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

  M Roberts (original release)
  Robin Birch <robinb@ruffnready.co.uk>
  Samuel Gisiger <samuel.gisiger@triadis.ch>
  Jeff Goodenough <jeff@enborne.f2s.com>
  Alastair Harrison <aharrison@magic.force9.co.uk>
  Scott Penrose <scottp@dd.com.au>
  John Wharington <jwharington@gmail.com>
  Lars H <lars_hn@hotmail.com>
  Rob Dunning <rob@raspberryridgesheepfarm.com>
  Russell King <rmk@arm.linux.org.uk>
  Paolo Ventafridda <coolwind@email.it>
  Tobias Lohner <tobias@lohner-net.de>
  Mirek Jezek <mjezek@ipplc.cz>
  Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  }
*/

#ifndef CYLINDERZONE_HPP
#define CYLINDERZONE_HPP

#include "Task/Tasks/BaseTask/ObservationZonePoint.hpp"
#include <assert.h>

class CylinderZone : public ObservationZonePoint {
public:
  CylinderZone(const GEOPOINT &loc, const double _radius=10000.0):
    ObservationZonePoint(loc),
    Radius(_radius)
  {
  };

/** 
 * Clone with shift (for use when we want to create a new taskpoint
 * retaining the OZ type of another) 
 * 
 * @param _location Location of copy
 * 
 * @return New object
 */
  virtual CylinderZone* clone(const GEOPOINT &_location) {
    return new CylinderZone(_location, Radius);
  }

  virtual bool isInSector(const AIRCRAFT_STATE &ref) const
  {
    return distance(ref.Location)<=Radius;
  }  

  GEOPOINT get_boundary_parametric(double) const;

  virtual double score_adjustment();

/** 
 * Check transition constraints (always true for cylinders)
 * 
 * @param ref_now Current aircraft state
 * @param ref_last Previous aircraft state
 * 
 * @return True if constraints are satisfied
 */
  virtual bool transition_constraint(const AIRCRAFT_STATE & ref_now, 
                                     const AIRCRAFT_STATE & ref_last) {
    return true;
  }

/** 
 * Set radius property
 * 
 * @param new_radius Radius (m) of cylinder
 */
  virtual void setRadius(double new_radius) {
    assert(new_radius>0);
    Radius = new_radius;
  }

/** 
 * Get radius property value
 * 
 * @return Radius (m) of cylinder
 */
  double getRadius() const {
    return Radius;
  }

/** 
 * Test whether an OZ is equivalent to this one
 * 
 * @param other OZ to compare to
 * 
 * @return True if same type and OZ parameters
 */

  virtual bool equals(const ObservationZonePoint* other) const;

/** 
 * Generate a random location inside the OZ (to be used for testing)
 * 
 * @return Location of point
 */
  virtual GEOPOINT randomPointInSector() const;

protected:
  double Radius;
};

#endif
