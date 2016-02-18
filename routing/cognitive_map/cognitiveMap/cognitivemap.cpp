#include "cognitivemap.h"
#include "../../../geometry/Point.h"
#include "../../../geometry/SubRoom.h"
#include "../../../geometry/Building.h"
#include "../../../pedestrian/Pedestrian.h"
#include "../../../visiLibity/source_code/visilibity.hpp"



CognitiveMap::CognitiveMap()
{

}


CognitiveMap::CognitiveMap(ptrBuilding b, ptrPed ped)
{
    _building=b;
    _ped=ped;

    _network = std::make_shared<GraphNetwork>(b,ped);

    std::string str(b->GetProjectRootDir()+"cogmap.xml");
    _outputhandler = std::make_shared<CogMapOutputHandler>(str.c_str());

    const double fps = UPDATE_RATE;
    _outputhandler->WriteToFileHeader(ped->GetID(),fps);
    _frame=0;

    _YAHPointer.SetPos(_ped->GetPos());

}

CognitiveMap::~CognitiveMap()
{

}

void CognitiveMap::UpdateMap()
{
    AddWaypoints(TriggerAssoziations(LookForLandmarks()));
    if (_waypContainerSorted.empty())
        return;
    if (_waypContainerSorted.top()->WaypointReached(_YAHPointer.GetPos()))
    {
        //ptrWaypoint cWaypoint = _waypContainerSorted.top();

        //cWaypoint->SetPriority(_waypContainerSorted.size());
        _waypContainerSorted.pop();

        //_waypContainerSorted.push(cWaypoint);
        //_waypContainer.pop();
        //Log->Write("Prio:\t"+std::to_string(_waypContainerSorted.top()->GetPriority()));
    }
}

void CognitiveMap::UpdateDirection()
{
    if (_ped->GetV()._x!=0 || _ped->GetV()._y!=0)
    {
        double angle = std::acos(_ped->GetV()._x/(std::sqrt(std::pow(_ped->GetV()._x,2)+std::pow(_ped->GetV()._y,2))));
        if (_ped->GetV()._y<0)
            angle=-angle;
        _YAHPointer.SetDirection(angle);
    }
}

void CognitiveMap::UpdateYAHPointer(const Point& move)
{
    _YAHPointer.UpdateYAH(move);
}

void CognitiveMap::AddLandmarksSC(std::vector<ptrLandmark> landmarks)
{
    for (ptrLandmark landmark:landmarks)
    {
        if (std::find(_landmarksSubConcious.begin(), _landmarksSubConcious.end(), landmark)!=_landmarksSubConcious.end())
        {
            continue;
        }
        else
        {
            _landmarksSubConcious.push_back(landmark);

        }
    }

}

void CognitiveMap::AddLandmarks(std::vector<ptrLandmark> landmarks)
{
    for (ptrLandmark landmark:landmarks)
    {
        if (std::find(_landmarks.begin(), _landmarks.end(), landmark)!=_landmarks.end())
        {
            continue;
        }
        else
        {
            _landmarks.push_back(landmark);

        }
    }
}

std::vector<ptrLandmark> CognitiveMap::LookForLandmarks()
{
    SubRoom * sub_room = _building->GetRoom(_ped->GetRoomID())->GetSubRoom(_ped->GetSubRoomID());

    std::vector<ptrLandmark> landmarks_found;

    for (ptrLandmark landmark:_landmarksSubConcious)
    {
        if (landmark->GetRoom()==sub_room)
        {
           landmarks_found.push_back(landmark);

        }
    }
    AddLandmarks(landmarks_found);

    return landmarks_found;
}

Waypoints CognitiveMap::TriggerAssoziations(const std::vector<ptrLandmark> &landmarks) const
{
    Waypoints waypoints;
    for (ptrLandmark landmark:landmarks)
    {
        Associations associations = landmark->GetAssociations();
        for (ptrAssociation association:associations)
        {
            waypoints.push_back(association->GetAssociation(landmark));
        }
    }
    return waypoints;
}

void CognitiveMap::AddWaypoints(Waypoints waypoints)
{
    for (ptrWaypoint waypoint:_waypContainer)
    {
        waypoint->SetPriority(waypoint->GetPriority()+waypoints.size());
    }
    int n=0;
    for (ptrWaypoint waypoint:waypoints)
    {
        if (std::find(_waypContainer.begin(), _waypContainer.end(), waypoint)!=_waypContainer.end())
        {
            continue;
        }
        waypoint->SetPriority(n);
        _waypContainer.push_back(waypoint);
        _waypContainerSorted.push(waypoint);
        n++;
    }

}

void CognitiveMap::AssessDoors()
{
    SubRoom * sub_room = _building->GetRoom(_ped->GetRoomID())->GetSubRoom(_ped->GetSubRoomID());
    GraphVertex * vertex = (*_network->GetNavigationGraph())[sub_room];
    const GraphVertex::EdgesContainer edges = *(vertex->GetAllEdges());

    //const Point rpWaypoint=_waypContainer[0]->GetPos();

    if (!_waypContainer.empty())
    {
        std::vector<GraphEdge* > sortedEdges = SortConShortestPath(_waypContainerSorted.top(),edges);
        //Log->Write(std::to_string(nextDoor->GetCrossing()->GetID()));

        for (unsigned int i=0; i<sortedEdges.size(); ++i)
        {
            sortedEdges[i]->SetFactor(0.5+0.1*i,"SpatialKnowledge");
            //Log->Write("INFO:\t "+std::to_string(sortedEdges[i]->GetCrossing()->GetID()));
            //Log->Write("INFO:\t "+std::to_string(sortedEdges[i]->GetFactor()));
        }

        //Log->Write(std::to_string(nextDoor->GetCrossing()->GetID()));
        //Log->Write("INFO: Door assessed!");
    }

}

std::vector<GraphEdge *> CognitiveMap::SortConShortestPath(ptrWaypoint waypoint, const GraphVertex::EdgesContainer edges)
{

    std::list<double> sortedPathLengths;
    sortedPathLengths.push_back(ShortestPathDistance((*edges.begin()),waypoint));
    std::list<GraphEdge* > sortedEdges;
    sortedEdges.push_back((*edges.begin()));

    auto itsortedEdges = sortedEdges.begin();
    auto it = edges.begin();
    ++it;
    ///starting at the second element
    for (it; it!=edges.end(); ++it)
    {
        double pathLengthDoorWayP = ShortestPathDistance((*it),waypoint);

        //Point vectorPathPedDoor = (*it)->GetCrossing()->GetCentre()-_ped->GetPos();
        itsortedEdges = sortedEdges.begin();
        //double pathLength = pathLengthDoorWayP+std::sqrt(std::pow(vectorPathPedDoor._x,2)+std::pow(vectorPathPedDoor._y,2));
        bool inserted=false;
        for (auto itLengths=sortedPathLengths.begin(); itLengths!=sortedPathLengths.end(); ++itLengths)
        {
            if (pathLengthDoorWayP >= *itLengths)
            {
                ++itsortedEdges;
                continue;
            }

            sortedPathLengths.insert(itLengths,pathLengthDoorWayP);
            sortedEdges.insert(itsortedEdges,(*it));
            inserted=true;
            break;
        }
        if (!inserted)
        {
            sortedPathLengths.push_back(pathLengthDoorWayP);
            sortedEdges.push_back((*it));
        }
    }

//    Log->Write("subroom:\t");
//    Log->Write(std::to_string(_ped->GetSubRoomID()));
//    Log->Write("edgeOnShortest:");
//    Log->Write(std::to_string(edgeOnShortest->GetCrossing()->GetID()))
    std::vector<GraphEdge* > vectorSortedEdges;
    for (GraphEdge* edge:sortedEdges)
    {
        vectorSortedEdges.push_back(edge);
        //Log->Write("INFO:\t "+std::to_string(edge->GetCrossing()->GetID()));

    }
    //Log->Write("INFO:\t Next");
    return vectorSortedEdges;

}

//bool CognitiveMap::IsAroundWaypoint(const Waypoint& waypoint, GraphEdge *edge) const
//{
//    Triangle triangle(_ped->GetPos(),waypoint);
//    Point point(edge->GetCrossing()->GetCentre());
//    return triangle.Contains(point);
//}

ptrGraphNetwork CognitiveMap::GetGraphNetwork() const
{
    return _network;
}

double CognitiveMap::ShortestPathDistance(const GraphEdge* edge, const ptrWaypoint waypoint)
{

    SubRoom* sub_room = _building->GetRoom(_ped->GetRoomID())->GetSubRoom(_ped->GetSubRoomID());
    std::vector<Point> points = sub_room->GetPolygon();
    VisiLibity::Polygon boundary=VisiLibity::Polygon();
    VisiLibity::Polygon room=VisiLibity::Polygon();
    for (Point point:points)
    {
       room.push_back(VisiLibity::Point(point._x,point._y));
    }

//    for (int i=0; i<room.n();++i)
//    {
//        Log->Write("RoomVertices:");
//        std::cout << std::to_string(room[i].x()) << "\t" << std::to_string(room[i].y()) << std::endl;
//    }

    boundary.push_back(VisiLibity::Point(-100,-100));
    boundary.push_back(VisiLibity::Point(100,-100));
    boundary.push_back(VisiLibity::Point(100,100));
    boundary.push_back(VisiLibity::Point(-100,100));

    std::vector<VisiLibity::Polygon> polygons;
    polygons.push_back(boundary);
    polygons.push_back(room);

    VisiLibity::Environment environment(polygons);
    //environment.reverse_holes();

    VisiLibity::Point edgeP(edge->GetCrossing()->GetCentre()._x,edge->GetCrossing()->GetCentre()._y);
    Point pointOnShortestRoute = waypoint->PointOnShortestRoute(edge->GetCrossing()->GetCentre());
    //Log->Write(std::to_string(pointOnShortestRoute._x)+" "+std::to_string(pointOnShortestRoute._y));
    VisiLibity::Point wayP(pointOnShortestRoute._x,pointOnShortestRoute._y);//,waypoint->GetPos()._y);

    VisiLibity::Polyline polyline=environment.shortest_path(edgeP,wayP,0.1);
//    for (int i=0; i<polyline.size();++i)
//    {
//        Log->Write("Polyline:");
//        std::cout << std::to_string(polyline[i].x()) << "\t" << std::to_string(polyline[i].y()) << std::endl;
//    }
//    Log->Write("ShortestPathLength");
//    Log->Write(std::to_string(polyline.length()));

    return polyline.length();
}

const Point &CognitiveMap::GetOwnPos()
{
    return _YAHPointer.GetPos();
}

void CognitiveMap::WriteToFile()
{
    ++_frame;
    string data;
    char tmp[CLENGTH] = "";

    sprintf(tmp, "<frame ID=\"%d\">\n", _frame);
    data.append(tmp);


    for (ptrLandmark landmark:_landmarks)
    {
        char tmp1[CLENGTH] = "";
        sprintf(tmp1, "<landmark ID=\"%d\"\t"
               "x=\"%.6f\"\ty=\"%.6f\"\t"
               "z=\"%.6f\"\t"
               "rA=\"%.2f\"\trB=\"%.2f\"/>\n",
               landmark->GetId(), landmark->GetPos()._x,
               landmark->GetPos()._y,0.0 ,landmark->GetA(), landmark->GetB());

        data.append(tmp1);
    }
    for (ptrWaypoint waypoint:_waypContainer)
    {
        char tmp2[CLENGTH] = "";
        sprintf(tmp2, "<waypoint ID=\"%d\"\t"
               "x=\"%.6f\"\ty=\"%.6f\"\t"
               "z=\"%.6f\"\t"
               "rA=\"%.2f\"\trB=\"%.2f\"/>\n",
               waypoint->GetId(), waypoint->GetPos()._x,
               waypoint->GetPos()._y,0.0 ,waypoint->GetA(), waypoint->GetB());
        data.append(tmp2);
    }

    char tmp3[CLENGTH]="";
    sprintf(tmp3, "<YAHPointer "
           "x=\"%.6f\"\ty=\"%.6f\"\t"
           "z=\"%.6f\"\t"
           "dir=\"%.2f\"/>\n",
           _YAHPointer.GetPos()._x,
           _YAHPointer.GetPos()._y,0.0 ,_YAHPointer.GetDirection());

    data.append(tmp3);


    data.append("</frame>\n");
    _outputhandler->WriteToFile(data);
}



