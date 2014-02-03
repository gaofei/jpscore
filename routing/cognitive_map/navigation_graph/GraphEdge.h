/**
 * @file   GraphEdge.h
 * @author David Haensel (d.haensel@fz-juelich.de)
 * @date   January, 2014
 * @brief  Edge of a Graph.
 *
 */

#ifndef GRAPHEDGE_H_
#define GRAPHEDGE_H_


class SubRoom;
class GraphVertex;

/**
 * @brief Graph Edge.
 *
 */

class GraphEdge {

public:
    /****************************
     * Constructors & Destructors
     ****************************/

    GraphEdge(GraphVertex const * const s, GraphVertex const * const d, SubRoom const * const sr);
    GraphEdge(GraphEdge const & ge);
    virtual ~GraphEdge();

    // Getter collection
    const GraphVertex * getDest() const;
    const GraphVertex * getSrc() const;
    const SubRoom * getSubRoom() const;


private:
    GraphVertex const * const src;
    GraphVertex const * const dest;
    SubRoom const * const subroom;

};


#endif /* GRAPHEDGE_H_ */
