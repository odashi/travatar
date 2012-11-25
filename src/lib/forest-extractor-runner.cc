#include <iostream>
#include <fstream>
#include <travatar/util.h>
#include <travatar/tree-io.h>
#include <travatar/rule-extractor.h>
#include <travatar/forest-extractor-runner.h>
#include <travatar/config-forest-extractor-runner.h>
#include <travatar/binarizer-directional.h>
#include <travatar/binarizer-cky.h>
#include <travatar/rule-composer.h>
#include <travatar/rule-filter.h>
#include <travatar/hyper-graph.h>
#include <travatar/alignment.h>
#include <travatar/dict.h>
#include <boost/scoped_ptr.hpp>

using namespace travatar;
using namespace std;
using namespace boost;

// Run the model
void ForestExtractorRunner::Run(const ConfigForestExtractorRunner & config) {

    // Set the debugging level
    GlobalVars::debug = config.GetInt("debug");

    // Create the tree parser and rule extractor
    scoped_ptr<TreeIO> tree_io;
    if(config.GetString("input_format") == "penn")
        tree_io.reset(new PennTreeIO);
    else
        THROW_ERROR("Invalid TreeIO type: " << config.GetString("input_format"));
    ForestExtractor extractor;
    extractor.SetMaxAttach(config.GetInt("attach_len"));
    extractor.SetMaxNonterm(config.GetInt("nonterm_len"));
    scoped_ptr<GraphTransformer> binarizer, composer;
    // Create the binarizer
    if(config.GetString("binarize") == "left") {
        binarizer.reset(new BinarizerDirectional(BinarizerDirectional::BINARIZE_LEFT));
    } else if(config.GetString("binarize") == "right") {
        binarizer.reset(new BinarizerDirectional(BinarizerDirectional::BINARIZE_RIGHT));
    } else if(config.GetString("binarize") == "cky") {
        binarizer.reset(new BinarizerCKY());
    } else if(config.GetString("binarize") != "none") {
        THROW_ERROR("Invalid binarizer type " << config.GetString("binarizer"));
    }
    // Create the binarizer
    if(config.GetInt("compose") > 1)
        composer.reset(new RuleComposer(config.GetInt("compose")));
    // Open the files
    const vector<string> & argv = config.GetMainArgs();
    ifstream src_in(argv[0].c_str());
    if(!src_in) THROW_ERROR("Could not find src file: " << argv[0]);
    ifstream trg_in(argv[1].c_str());
    if(!trg_in) THROW_ERROR("Could not find trg file: " << argv[1]);
    ifstream align_in(argv[2].c_str());
    if(!align_in) THROW_ERROR("Could not find align file: " << argv[2]);
    // Create rule filters
    vector< shared_ptr<RuleFilter> > rule_filters;
    rule_filters.push_back(shared_ptr<RuleFilter>(new PseudoNodeFilter));
    rule_filters.push_back(shared_ptr<RuleFilter>(new CountFilter(config.GetDouble("partial_count_thresh"))));
    rule_filters.push_back(shared_ptr<RuleFilter>(new RuleSizeFilter(config.GetInt("term_len"), config.GetInt("nonterm_len"))));
    // Get the lines
    string src_line, trg_line, align_line;
    int has_src, has_trg, has_align;
    int sent = 0;
    cerr << "Extracting rules (.=10,000, !=100,000 sentences)" << endl;
    while(true) {
        // Load one line from each file and check that they all exist
        has_src = getline(src_in, src_line) ? 1 : 0;
        has_trg = getline(trg_in, trg_line) ? 1 : 0;
        has_align = getline(align_in, align_line) ? 1 : 0;
        if(has_src + has_trg + has_align == 0) break;
        if(has_src + has_trg + has_align != 3)
            THROW_ERROR("File sizes don't match: src="<<has_src
                        <<", trg="<<has_trg<<", align="<<has_align);
        // Parse into the appropriate data structures
        istringstream src_iss(src_line);
        scoped_ptr<HyperGraph> src_graph(tree_io->ReadTree(src_iss));
        // { /* DEBUG */ JSONTreeIO io; io.WriteTree(*src_graph, cerr); cerr << endl; }
        // Binarizer if necessary
        if(binarizer.get() != NULL)
            src_graph.reset(binarizer->TransformGraph(*src_graph));
        // { /* DEBUG */ JSONTreeIO io; io.WriteTree(*src_graph, cerr); cerr << endl; }
        // Get target words and alignment
        Sentence trg_sent = Dict::ParseWords(trg_line);
        Alignment align = Alignment::FromString(align_line);
        // Do the rule extraction
        scoped_ptr<HyperGraph> rule_graph(
            extractor.ExtractMinimalRules(*src_graph, align));
        // { /* DEBUG */ JSONTreeIO io; io.WriteTree(*rule_graph, cerr); cerr << endl; }
        // Compose together
        if(composer.get() != NULL)
            rule_graph.reset(composer->TransformGraph(*rule_graph));
        // { /* DEBUG */ JSONTreeIO io; io.WriteTree(*rule_graph, cerr); cerr << endl; }
        // Null attacher if necessary (TODO, make looking up the configuration better)
        if(config.GetString("attach") == "top")
            rule_graph.reset(extractor.AttachNullsTop(*rule_graph,align,trg_sent.size()));
        else if(config.GetString("attach") == "exhaustive")
            rule_graph.reset(extractor.AttachNullsExhaustive(*rule_graph,align,trg_sent.size()));
        else if(config.GetString("attach") != "none")
            THROW_ERROR("Bad value for argument -attach: " << config.GetString("attach"));
        // { /* DEBUG */ JSONTreeIO io; io.WriteTree(*rule_graph, cerr); cerr << endl; }
        // If we want to normalize to partial counts, do so
        if(config.GetBool("normalize_probs"))
            rule_graph->InsideOutsideNormalize();
        // { /* DEBUG */ JSONTreeIO io; io.WriteTree(*rule_graph, cerr); cerr << endl; }
        // Print each of the rules as long as they pass the filter
        BOOST_FOREACH(HyperEdge* edge, rule_graph->GetEdges()) {
            int filt;
            for(filt = 0; 
                filt < (int)rule_filters.size() && 
                rule_filters[filt]->PassesFilter(*edge, src_graph->GetWords(), trg_sent);
                filt++);
            if(filt == (int)rule_filters.size()) {
                cout << extractor.RuleToString(*edge, 
                                               src_graph->GetWords(), 
                                               trg_sent) << endl;
            }
        }
        sent++;
        if(sent % 10000 == 0) {
            cerr << (sent % 100000 == 0 ? '!' : '.'); cerr.flush();
        }
    }
    cerr << endl;
}
