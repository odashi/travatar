#include <travatar/caser.h>
#include <travatar/dict.h>
#include <travatar/hyper-graph.h>

#include <boost/locale.hpp>
#include <boost/foreach.hpp>

using namespace std;

namespace travatar {

Caser::Caser() {
    loc_ = boost::locale::generator().generate("en_US.UTF-8");
}

Caser::~Caser() { }

std::string Caser::ToLower(const std::string & wid) {
    return boost::locale::to_lower(wid, loc_);
}
WordId Caser::ToLower(WordId wid) {
    return Dict::WID(boost::locale::to_lower(Dict::WSym(wid), loc_));
}
void Caser::ToLower(Sentence & sent) {
    for(int i = 0; i < (int)sent.size(); i++)
        sent[i] = ToLower(sent[i]);
}
void Caser::ToLower(HyperGraph & graph) {
    ToLower(graph.GetWords());
    BOOST_FOREACH(HyperNode* node, graph.GetNodes()) {
        if(node->IsTerminal()) 
            node->SetSym(ToLower(node->GetSym()));
    }
}

std::string Caser::ToTitle(const std::string & wid) {
    return boost::locale::to_title(wid, loc_);
}
WordId Caser::ToTitle(WordId wid) {
    return Dict::WID(boost::locale::to_title(Dict::WSym(wid), loc_));
}
void Caser::ToTitle(Sentence & sent) {
    vector<bool> first = SentenceFirst(sent);
    for(int i = 0; i < (int)first.size(); i++) {
        if(first[i])
            sent[i] = ToTitle(sent[i]);
    }
}
void Caser::ToTitle(HyperGraph & graph) {
    ToTitle(graph.GetWords());
    vector<bool> first = SentenceFirst(graph.GetWords());
    BOOST_FOREACH(HyperNode* node, graph.GetNodes()) {
        if(node->IsTerminal() && first[node->GetSpan().first])
            node->SetSym(ToTitle(node->GetSym()));
    }
}

std::string Caser::TrueCase(const std::string & wid) {
    boost::unordered_map<std::string, std::string>::const_iterator it = truecase_map_.find(ToLower(wid));
    return (it != truecase_map_.end()) ? it->second : wid;
}
WordId Caser::TrueCase(WordId wid) {
    return Dict::WID(TrueCase(Dict::WSym(wid)));
}
void Caser::TrueCase(Sentence & sent) {
    vector<bool> first = SentenceFirst(sent);
    for(int i = 0; i < (int)first.size(); i++) {
        if(first[i])
            sent[i] = TrueCase(sent[i]);
    }
}
void Caser::TrueCase(HyperGraph & graph) {
    TrueCase(graph.GetWords());
    vector<bool> first = SentenceFirst(graph.GetWords());
    BOOST_FOREACH(HyperNode* node, graph.GetNodes()) {
        if(node->IsTerminal() && first[node->GetSpan().first])
            node->SetSym(TrueCase(node->GetSym()));
    }
}

std::vector<bool> Caser::SentenceFirst(const Sentence & sent) {
    vector<bool> first(sent.size(), false);
    if(sent.size())
        first[0] = true;
    return first;
}

void Caser::AddTrueValue(const std::string & str) {
    truecase_map_[ToLower(str)] = str;
}

} // namespace travatar
