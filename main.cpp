#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <deque>
#include <set>
#include <fstream>

struct Tree {
    int id;
    typedef std::map<int, std::vector<Tree *>> LabelMap;
    int label;
    int depth;
    Tree * parent;
    // in a pattern tree, subtrees acts as a stack
    std::vector<Tree *> subtrees;

    Tree(int id,
            std::vector<int>::const_iterator repr_begin,
            std::vector<int>::const_iterator repr_end,
            Tree * p = nullptr,
            int d = 0) : id(id), parent(p), depth(d) {
        if (repr_begin == repr_end) {
            std::cerr << "Not a Tree: empty" << std::endl;
            exit(-1);
        }
        if (*repr_begin < 0) {
            std::cerr << "Not a Tree: unmatched -1" << std::endl;
            exit(-1);
        }
        label = *repr_begin;
        auto sub_end = repr_begin + 1;
        auto sub_begin = sub_end;
        int depth_count = 1;
        while (depth_count > 0) {
            if (sub_end == repr_end) {
                std::cerr << "Not a Tree: incomplete" << std::endl;
                exit(-1);
            }
            if (*sub_end < 0) {
                --depth_count;
            } else {
                ++depth_count;
            }
            ++sub_end;
            if (depth_count == 1) {
                auto sub = new Tree(id, sub_begin, sub_end, this, d + 1);
                subtrees.push_back(sub);
                sub_begin = sub_end;
            }
        }
        if (sub_end != repr_end) {
            std::cerr << "Not a Tree: redundant elements" << std::endl;
            exit(-1);
        }
        //std::sort(subtrees.begin(), subtrees.end(), [](Tree * a, Tree * b){ return a->label < b->label; });
    }

    explicit Tree(int id, const std::vector<int>& repr) : Tree(id, repr.cbegin(), repr.cend(), nullptr, 0) {}

    Tree(int id, int l, Tree * p, int d) : id(id), label(l), depth(d), parent(p), subtrees() {}

    std::vector<int> to_vector() {
        auto v = std::vector<int>();
        v.push_back(label);
        for (auto sub : subtrees) {
            // recursively push subtrees' elements
            auto sub_v = sub->to_vector();
            v.insert(v.end(), sub_v.begin(), sub_v.end());
        }
        v.push_back(-1);
        return v;
    }

    std::string to_string() {
        std::ostringstream ss;
        auto vec = to_vector();
#if DEBUG
        if (vec.empty()) {
            std::cerr << "TransformError: empty tree" << std::endl;
            exit(-1);
        }
        if (vec.size() % 2 != 0) {
            std::cerr << "TransformError: backtrack -1" << std::endl;
            exit(-1);
        }
#endif
        for (auto v : vec) {
            ss << v << ' ';
        }
        return ss.str();
    }

    void print_whole_tree() {
        auto r = this;
        while (r->parent != nullptr) {
            r = r->parent;
        }
        std::cout << r->to_string() << std::endl;
    }

    std::vector<Tree *> get_label_nodes(int target_label) {
        auto v = std::vector<Tree *>();
        if (label == target_label) {
            v.push_back(this);
        }
        for (auto sub : subtrees) {
            auto sub_v = sub->get_label_nodes(target_label);
            v.insert(v.end(), sub_v.begin(), sub_v.end());
        }
        return v;
    }

    LabelMap get_label_map() {
        auto m = LabelMap();
        m[label].push_back(this);
        for (auto sub : subtrees) {
            auto sub_m = sub->get_label_map();
            for (auto p : sub_m) {
                int sub_l = p.first;
                m[sub_l].insert(m[sub_l].end(), p.second.begin(), p.second.end());
            }
        }
        return m;
    }

    std::set<int> get_labels() {
        auto s = std::set<int>();
        s.insert(label);
        for (auto sub : subtrees) {
            auto sub_s = sub->get_labels();
            s.insert(sub_s.begin(), sub_s.end());
        }
        return s;
    }

    Tree * push_new_node(int sub_label) {
        auto sub_node = new Tree(id, sub_label, this, depth+1);
        subtrees.push_back(sub_node);
        return sub_node;
    }

    // the parameter node is used for check stack correctness
    void pop_node(Tree * node) {
#if DEBUG
        if (subtrees.empty()) {
            std::cerr << "Stack error: pop from an empty tree" << std::endl;
            exit(-1);
        }
        if (subtrees.back() != node) {
            std::cerr << "Stack error: pop a different node from pushed" << std::endl;
            exit(-1);
        }
#endif
        subtrees.pop_back();
    }
};

struct ProjectedTree {
    int tree_id;
    // attached nodes in sub-pattern tree mapped to its subtrees in raw tree
    std::map<Tree *, std::vector<Tree *>> proj;

    // mapped refers to the location of new node in pattern tree
    // attached refers to the location that the new node attachs in pattern tree
    std::vector<ProjectedTree> split(int label, Tree * mapped, Tree * attached) {
        auto new_proj_trees = std::vector<ProjectedTree>();

        // check if the projected tree has expected attaching node
        auto p = proj.find(attached);
        if (p == proj.end()) {
            return new_proj_trees;
        }
        auto cand_proj_trees = p->second;

        // find all occurrences of label in attached subtrees
        for (auto cand_proj_tree : cand_proj_trees) {
            auto cand_cores = cand_proj_tree->get_label_nodes(label);
            for (auto core : cand_cores) {
                // for each occurence of new label, build a new projected tree

                // 1. subtrees of core are attached to mapped
                auto new_proj_tree = ProjectedTree(core, mapped);

                // 2. collateral nodes of core are attached to attached
                for (auto cur = core; cur != cand_proj_tree; cur = cur->parent) {
                    for (auto sub : cur->parent->subtrees) {
                        if (sub != cur) {
                            new_proj_tree.proj[attached].push_back(sub);
                        }
                    }
                }

                // 3. other attaching points are preserved
                for (auto ap : proj) {
                    if (ap.first != attached) {
                        // 3.1 other attaching poitns
                        new_proj_tree.proj[ap.first] = ap.second;
                    } else {
                        // 3.2 attached, all except cand_proj_tree
                        for (auto cand : ap.second) {
                            if (cand != cand_proj_tree) {
                                new_proj_tree.proj[ap.first].push_back(cand);
                            }
                        }
                    }
                }

                // prune the new_proj_tree
                //new_proj_tree.proj.erase(std::remove_if(new_proj_tree.proj.begin(), new_proj_tree.proj.end(),
                //        [](std::map<Tree *, std::vector<Tree *>>::iterator it){return it->second.empty();}
                //        ), new_proj_tree.proj.end());
                new_proj_trees.push_back(new_proj_tree);
            }
        }

        return new_proj_trees;
    }

    // map a occurrance into a sub_pattern, and transform its subnodes into a projected tree
    ProjectedTree(Tree * occ, Tree * mapped) {
        tree_id = occ->id;
        proj[mapped] = occ->subtrees;
    }
};

// Scan prodb to find all frequent growth elements
std::vector<std::pair<int, Tree *>> get_growth_elements(const std::vector<ProjectedTree>& prodb, int min_sup) {
    // candidates of growth elements, (label, attached nodes) -> set of tree ids
    auto candidates = std::map<std::pair<int, Tree *>, std::set<int>>();
    for (auto proj_tree : prodb) {
        int id = proj_tree.tree_id;
        for (auto p : proj_tree.proj) {
            Tree * attached = p.first;
            for (auto sub : p.second) {
                auto labels = sub->get_labels();
                for (int l : labels) {
                    candidates[std::make_pair(l, attached)].insert(id);
                }
            }
        }
    }
    auto ges = std::vector<std::pair<int, Tree *>>();
    for (auto p : candidates) {
        if (p.second.size() >= min_sup) {
            ges.push_back(p.first);
        }
    }
    return ges;
}

// returns number and max size of frequent pattern
std::pair<int, int> Fre(Tree * sub_pattern, int size, std::vector<ProjectedTree> prodb, int min_sup) {
    int num = 0, maxsize = 0;
    auto ges = get_growth_elements(prodb, min_sup);
    for (auto ge : ges) {
        int label = ge.first;
        Tree * attached = ge.second;

        // extend sub_pattern S to S' in-place
        auto new_node = attached->push_new_node(label);

#if PRINT_PATTERN
        // output S'
        new_node->print_whole_tree();
#endif
        num++;
        if (size + 1 > maxsize) {
            maxsize = size + 1;
        }

        // generate new ProDB from previous ProDB
        auto new_prodb = std::vector<ProjectedTree>();
        for (auto proj_tree : prodb) {
            auto new_proj_trees = proj_tree.split(label, new_node, attached);
            new_prodb.insert(new_prodb.end(), new_proj_trees.begin(), new_proj_trees.end());
        }

        // recursively find larger frequent pattern
        auto super_ans = Fre(sub_pattern, size + 1, new_prodb, min_sup);
        num += super_ans.first;
        if (super_ans.second > maxsize) {
            maxsize = super_ans.second;
        }

        // backtracking, remove the new node
        attached->pop_node(new_node);
        delete new_node;
    }
    return std::make_pair(num, maxsize);
}

// returns number and max size of frequent pattern
std::pair<int, int> PrefixESpan(std::vector<Tree*> db, int min_sup) {
    int num = 0, maxsize = 0;
    // collect all labels, with its frequency and occurance
    std::map<int, int> freq_map;
    std::map<int, std::vector<Tree*>> occur_map;
    for (auto tree : db) {
        for (auto p : tree->get_label_map()) {
            int label = p.first;
            freq_map[label]++;
            occur_map[label].insert(occur_map[label].end(), p.second.begin(), p.second.end());
        }
    }
    for (auto p : freq_map) {
        int label = p.first;
        int freq = p.second;
        if (freq >= min_sup) {
            // generate a new pattern tree
            auto s = new Tree(-1, label, nullptr, 0);

#ifdef PRINT_PATTERN
            // print the frequent pattern
            s->print_whole_tree();
#endif
            num++;
            if (1 > maxsize) {
                maxsize = 1;
            }


            // generate a new ProDB
            // for each occurrance, build a new projected tree
            auto prodb = std::vector<ProjectedTree>();
            for (auto occ : occur_map[label]) {
                auto proj_tree = ProjectedTree(occ, s);
                prodb.push_back(proj_tree);
            }
            auto super_ans = Fre(s, 1, prodb, min_sup);
            num += super_ans.first;
            if (super_ans.second > maxsize) {
                maxsize = super_ans.second;
            }
        }
    }
    return std::make_pair(num, maxsize);
}

#if DEBUG
void test() {
    auto t1_vec = std::vector<int>({2, 1, 3, 5, -1, -1, -1, 1, 2, -1, 4, -1, -1, -1});
    auto t2_vec = std::vector<int>({1, 2, 2, -1, 4, -1, -1, 3, -1, -1});
    auto t1 = new Tree(1, t1_vec);
    auto t2 = new Tree(2, t2_vec);
    if (t1->to_vector() != t1_vec) {
        std::cerr << "Test Error: t1's representation is inconsistent" << std::endl;
        exit(-1);
    }
    if (t2->to_vector() != t2_vec) {
        std::cerr << "Test Error: t2's representation is inconsistent" << std::endl;
        exit(-1);
    }
    std::vector<Tree *> db = {t1, t2};
    auto ans = PrefixESpan(db, 2);
    if (ans.second != 3) {
        std::cerr << "Test Error: wrong max size" << std::endl;
        exit(-1);
    } else {
        std::cout << "Pass simple test" << std::endl;
    }
}
#endif

void process_file(std::string filename, int sup_percent) {
    auto db = std::vector<Tree *>();
    int tree_num = 0;

    // parse the file
    std::ifstream infile;
    infile.open(filename);
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        auto vec = std::vector<int>();
        int x;
        while (iss >> x) {
            vec.push_back(x);
        }
        if (!vec.empty()) {
            tree_num++;
            auto tree = new Tree(tree_num, vec);
            db.push_back(tree);
        }
    }

    int min_sup = (int)std::ceil(tree_num * sup_percent / 100.0);

    auto ans = PrefixESpan(db, min_sup);
    std::cerr << "DB name: " << filename << std::endl;
    std::cerr << "Num of trees: " << tree_num << std::endl;
    std::cerr << "Support percent: " << sup_percent << '%' << std::endl;
    std::cerr << "Num of frequent patterns: " << ans.first << std::endl;
    std::cerr << "Maxsize of frequent patterns: " << ans.second << std::endl;
}

int main(int argc, char** argv) {
#if DEBUG
    test();
#else
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <file> <percentage>" << std::endl;
        exit(-1);
    }
    std::string filename(argv[1]);
    int percent = std::stoi(argv[2]);
    process_file(filename, percent);
#endif
    return 0;
}