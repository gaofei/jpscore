/**
 * \file        IODispatcher.h
 * \date        Nov 20, 2010
 * \version     v0.5
 * \copyright   <2009-2014> Forschungszentrum Jülich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * \section Description
 *
 *
 **/


#ifndef _IODISPATCHER_H
#define _IODISPATCHER_H

#include "OutputHandler.h"
#include "../geometry/Building.h"

#include <cstring>
#include <vector>

extern OutputHandler* Log;

class IODispatcher {
private:
     std::vector<OutputHandler* > pHandlers;
     std::string WritePed(Pedestrian* ped);

public:
     IODispatcher();
     IODispatcher(const IODispatcher& orig);
     virtual ~IODispatcher();

     void AddIO(OutputHandler* ioh);
     const std::vector<OutputHandler*>& GetIOHandlers();
     void Write(const std::string& str);
     virtual void WriteHeader(int nPeds, double fps, Building* building, int seed);
     virtual void WriteGeometry(Building* building);
     virtual void WriteFrame(int frameNr, Building* building);
     virtual void WriteFooter();
     int AreaLevel(std::string caption);


     template<typename A>
     bool IsElementInVector(const std::vector<A> &vec, A& el) {
          typename std::vector<A>::const_iterator it;
          it = std::find (vec.begin(), vec.end(), el);
          if(it==vec.end()) {
               return false;
          } else {
               return true;
          }
     }


};


class TrajectoriesFLAT:public IODispatcher {

public:
     TrajectoriesFLAT();
     virtual ~TrajectoriesFLAT() {};

     virtual void WriteHeader(int nPeds, double fps, Building* building, int seed);
     virtual void WriteGeometry(Building* building);
     virtual void WriteFrame(int frameNr, Building* building);
     virtual void WriteFooter();

};

class TrajectoriesVTK:public IODispatcher {

public:
     TrajectoriesVTK();
     virtual ~TrajectoriesVTK() {};

     virtual void WriteHeader(int nPeds, double fps, Building* building, int seed);
     virtual void WriteGeometry(Building* building);
     virtual void WriteFrame(int frameNr, Building* building);
     virtual void WriteFooter();

};

class TrajectoriesXML_MESH:public IODispatcher {

public:
     TrajectoriesXML_MESH() {};
     virtual ~TrajectoriesXML_MESH() {};

     //virtual void WriteHeader(int nPeds, double fps, Building* building, int seed);
     //virtual void WriteFrame(int frameNr, Building* building);
     //virtual void WriteFooter();
     virtual void WriteGeometry(Building* building);
};

class TrajectoriesJPSV06: public IODispatcher {

public:
     TrajectoriesJPSV06() {};
     virtual ~TrajectoriesJPSV06() {};

     virtual void WriteHeader(int nPeds, double fps, Building* building, int seed);
     virtual void WriteGeometry(Building* building);
     virtual void WriteFrame(int frameNr, Building* building);
     virtual void WriteFooter();
};

#endif  /* _IODISPATCHER_H */
