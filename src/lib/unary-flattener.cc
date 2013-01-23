#include <travatar/unary-flattener.h>
#include <travatar/generic-string.h>
#include <travatar/hyper-graph.h>
#include <travatar/dict.h>
#include <travatar/util.h>
#include <boost/foreach.hpp>

using namespace travatar;
using namespace std;
using namespace boost;

// Binarize the graph to the right
HyperGraph * UnaryFlattener::TransformGraph(const HyperGraph & hg) {
    // First copy the graph
    HyperGraph * ret = new HyperGraph(hg);
    BOOST_FOREACH(HyperNode * node, ret->GetNodes()) {
        if(node->NumEdges() > 1) {
            // THROW_ERROR("Flatten does not currently work for hypergraphs of degree > 1");
        } else if(node->NumEdges() == 0||node->GetEdges()[0]->NumTails() != 1) {
            continue;
        }
        HyperEdge * old_edge = node->GetEdges()[0];
        node->GetEdges().clear();
        HyperNode * old_child = old_edge->GetTails()[0];
        if(old_child->NumEdges() > 0) {
            node->GetEdges().push_back(old_child->GetEdges()[0]);
            node->GetEdges()[0]->SetHead(node);
            node->SetSym(Dict::WID(Dict::WSym(node->GetSym())+"_"+
                                   Dict::WSym(old_child->GetSym())));
        } 
    }

}
