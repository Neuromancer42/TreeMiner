#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <deque>
#include <set>

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
        ss << label << ","; //TODO: delimiter needs revision
        for (auto sub : subtrees) {
            ss << sub->to_string() << ",";
        }
        ss << -1;
        return ss.str();
    }


    // TODO: merge get_all_nodes and get_label_nodes?
    std::vector<Tree *> get_all_nodes() {
        auto v = std::vector<Tree *>();
        v.push_back(this);
        for (auto sub : subtrees) {
            auto sub_v = sub->get_all_nodes();
            v.insert(v.end(), sub_v.begin(), sub_v.end());
        }
        return v;
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
        auto new_prodbs = std::vector<ProjectedTree>();

        // check if the projected tree has expected attaching node
        auto p = proj.find(attached);
        if (p == proj.end()) {
            return new_prodbs;
        }
        auto cand_proj_trees = p->second;

        // check if the projected tree has expected label
        for (auto cand_proj_tree : cand_proj_trees) {
            auto cand_cores = cand_proj_tree->get_label_nodes(label);
            for (auto core : cand_cores) {
                // TODO: generate a new prodb if a GE-backbone is found
                auto new_prodb = new ProjectedTree({tree_id, std::map<Tree *, std::vector<Tree *>>()});
                for (auto p : proj) {
                    new_prodb
                }
                for (auto sub : core->subtrees) {

                }
            }
        }

        return new_prodbs
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
        if (p.second.size() > min_sup) {
            ges.push_back(p.first);
        }
    }
    return ges;
}

void Fre(Tree * sub_pattern, int size, std::vector<ProjectedTree> prodb, int min_sup) {
    auto ges = get_growth_elements(prodb, min_sup);
    for (auto ge : ges) {
        int label = ge.first;
        Tree * attached = ge.second;

        // extend sub_pattern S to S' in-place
        auto new_node = attached->push_new_node(label);

        // TODO generate new ProDB from previous ProDB
        auto new_prodb = std::vector<ProjectedTree>();
        for (auto proj_tree : prodb) {

        }
    }
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}