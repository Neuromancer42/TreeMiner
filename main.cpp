#include <iostream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <deque>
#include <set>
#include <fstream>
#include <chrono>

struct Node {
    int id;
    typedef std::map<int, std::vector<Node *>> LabelMap;
    int label;
    int pos;
    Node * parent;
    // in a pattern tree, subtrees acts as a stack
    std::vector<Node *> subtrees;
    int subnode_count;

    Node(int id,
         std::vector<int>::const_iterator repr_begin,
         std::vector<int>::const_iterator repr_end,
         Node * p = nullptr,
         int start_pos = 0) : id(id), parent(p), pos(start_pos) {
        if (repr_begin == repr_end) {
            std::cerr << "Not a Node: empty" << std::endl;
            exit(-1);
        }
        if (*repr_begin < 0) {
            std::cerr << "Not a Node: unmatched -1" << std::endl;
            exit(-1);
        }
        label = *repr_begin;
        auto sub_end = repr_begin + 1;
        auto sub_begin = sub_end;
        int depth_count = 1;
        int cur_count = 0;
        subnode_count = 0;
        while (depth_count > 0) {
            if (sub_end == repr_end) {
                std::cerr << "Not a Node: incomplete" << std::endl;
                exit(-1);
            }
            if (*sub_end < 0) {
                --depth_count;
            } else {
                ++depth_count;
                ++cur_count;
            }
            ++sub_end;
            if (depth_count == 1) {
                auto sub = new Node(id, sub_begin, sub_end, this, pos + subnode_count + 1);
#if DEBUG
                if (cur_count * 2 != sub_end - sub_begin) {
                    std::cerr << "Not a Node: sub_tree malformed" << std::endl;
                    exit(-1);
                }
#endif
                subtrees.push_back(sub);
                sub_begin = sub_end;
                subnode_count += cur_count;
                cur_count = 0;
            }
        }
        if (sub_end != repr_end) {
            std::cerr << "Not a Node: redundant elements" << std::endl;
            exit(-1);
        }
        //std::sort(subtrees.begin(), subtrees.end(), [](Node * a, Node * b){ return a->label < b->label; });
    }

    explicit Node(int id, const std::vector<int>& repr) : Node(id, repr.cbegin(), repr.cend(), nullptr, 1) {}

    Node(int id, int l, Node * p, int d) : id(id), label(l), pos(d), parent(p), subtrees(), subnode_count(0) {}

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

    std::vector<Node *> get_label_nodes(int target_label) {
        auto v = std::vector<Node *>();
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

    std::vector<int> get_labels() {
        auto s = std::vector<int>();
        s.push_back(label);
        for (auto sub : subtrees) {
            auto sub_s = sub->get_labels();
            s.insert(s.end(), sub_s.begin(), sub_s.end());
        }
        return s;
    }

    Node * push_new_node(int sub_label) {
        auto sub_node = new Node(id, sub_label, this, pos + subnode_count + 1);
        subtrees.push_back(sub_node);
        for (Node * cur = this; cur != nullptr; cur = cur->parent) {
            cur->subnode_count++;
        }
        return sub_node;
    }

    // the parameter node is used for check stack correctness
    void pop_node(Node * node) {
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
        for (Node * cur = this; cur != nullptr; cur = cur->parent) {
            cur->subnode_count--;
        }
    }
};

struct ProjectInstance {
    int tree_id;
    // attached nodes in sub-pattern tree mapped to its subtrees in raw tree
    std::map<Node *, std::vector<Node *>> proj;

    // mapped refers to the location of new node in pattern tree
    // attached refers to the location that the new node attachs in pattern tree
    std::vector<ProjectInstance> split(Node * mapped) {
        auto new_proj_trees = std::vector<ProjectInstance>();
        int label = mapped->label;
        Node * attached = mapped->parent;
#if DEBUG
        if (attached == nullptr) {
            std::cerr << "Pattern construction error: no root" << std::endl;
            exit(-1);
        }
#endif
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
                auto new_proj_tree = ProjectInstance(core, mapped);

                // 2. collateral nodes of core are attached to attached
                for (auto cur = core; cur != cand_proj_tree; cur = cur->parent) {
                    for (auto sub : cur->parent->subtrees) {
                        if (sub->pos > cur->pos) {
                            new_proj_tree.proj[attached].push_back(sub);
                        }
                    }
                }

                // 3. other attaching points after core are preserved
                for (auto& ap : proj) {
                    for (auto sub : ap.second) {
                        if (sub->pos > core->pos) {
                            new_proj_tree.proj[ap.first].push_back(sub);
                        }
                    }
                }

                // prune the new_proj_tree
                //new_proj_tree.proj.erase(std::remove_if(new_proj_tree.proj.begin(), new_proj_tree.proj.end(),
                //        [](std::map<Node *, std::vector<Node *>>::iterator it){return it->second.empty();}
                //        ), new_proj_tree.proj.end());
                new_proj_trees.push_back(new_proj_tree);
            }
        }

        return new_proj_trees;
    }

    // map a occurrance into a sub_pattern, and transform its subnodes into a projected tree
    ProjectInstance(Node * occ, Node * mapped) {
        tree_id = occ->id;
        proj = std::map<Node *, std::vector<Node *>>();
        if (!occ->subtrees.empty()) {
            proj[mapped] = occ->subtrees;
        }
    }
};

// Scan prodb to find all frequent growth elements
std::vector<std::pair<int, Node *>> get_growth_elements(const std::vector<ProjectInstance>& prodb, int min_sup) {
    // candidates of growth elements, (label, attached nodes) -> set of tree ids
    auto candidates = std::map<std::pair<int, Node *>, std::set<int>>();
    for (auto& proj_tree : prodb) {
        int id = proj_tree.tree_id;
        for (auto& p : proj_tree.proj) {
            Node * attached = p.first;
            for (auto sub : p.second) {
                auto labels = sub->get_labels();
                for (int l : labels) {
                    candidates[std::make_pair(l, attached)].insert(id);
                }
            }
        }
    }
    auto ges = std::vector<std::pair<int, Node *>>();
    for (auto& p : candidates) {
        if (p.second.size() >= min_sup) {
            ges.push_back(p.first);
        }
    }
    return ges;
}

// returns number and max size of frequent pattern
std::pair<int, int> Fre(Node * sub_pattern, int size, const std::vector<ProjectInstance>& prodb, int min_sup) {
    int num = 0, maxsize = 0;
    auto ges = get_growth_elements(prodb, min_sup);
    for (auto ge : ges) {
        int label = ge.first;
        Node * attached = ge.second;

        // extend sub_pattern S to S' in-place
        auto new_node = attached->push_new_node(label);

#if DEBUG
        {
            Node * cur = new_node;
            while (cur->parent != nullptr) {
                cur = cur->parent;
            }
            if (cur->subnode_count + 1 != new_node->pos) {
                std::cerr << "Pattern construction error: wrong indexing" << std::endl;
                exit(-1);
            }
        }
        // output S'
        new_node->print_whole_tree();
#endif
        num++;
        if (size + 1 > maxsize) {
            maxsize = size + 1;
        }

        // generate new ProDB from previous ProDB
        auto new_prodb = std::vector<ProjectInstance>();
        for (auto proj_tree : prodb) {
            auto new_proj_trees = proj_tree.split(new_node);
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
std::pair<int, int> PrefixESpan(const std::vector<Node*>& db, int min_sup) {
    int num = 0, maxsize = 0;
    // collect all labels, with its frequency and occurrance
    std::map<int, int> freq_map;
    std::map<int, std::vector<Node*>> occur_map;
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
            auto s = new Node(-1, label, nullptr, 1);

#ifdef DEBUG
            // print the frequent pattern
            s->print_whole_tree();
#endif
            num++;
            if (1 > maxsize) {
                maxsize = 1;
            }


            // generate a new ProDB
            // for each occurrance, build a new projected tree
            auto prodb = std::vector<ProjectInstance>();
            for (auto occ : occur_map[label]) {
                auto proj_tree = ProjectInstance(occ, s);
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
    auto t1 = new Node(1, t1_vec);
    auto t2 = new Node(2, t2_vec);
    if (t1->to_vector() != t1_vec) {
        std::cerr << "Test Error: t1's representation is inconsistent" << std::endl;
        exit(-1);
    }
    if (t2->to_vector() != t2_vec) {
        std::cerr << "Test Error: t2's representation is inconsistent" << std::endl;
        exit(-1);
    }
    std::vector<Node *> db = {t1, t2};
    auto ans = PrefixESpan(db, 2);
    if (ans.second != 3) {
        std::cerr << "Test Error: wrong max size" << std::endl;
        exit(-1);
    } else {
        std::cout << "Pass simple test" << std::endl;
    }
}
#endif

void process_file(const std::string& filename, double sup_percent) {
    auto db = std::vector<Node *>();
    int tree_num = 0;

    auto start = std::chrono::high_resolution_clock::now();
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
            auto tree = new Node(tree_num, vec);
            db.push_back(tree);
        }
    }

    int min_sup = (int)(tree_num * sup_percent / 100);

    auto ans = PrefixESpan(db, min_sup);

    auto finish = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
    std::cerr << "DB name: " << filename << std::endl;
    std::cerr << "Num of trees: " << tree_num << std::endl;
    std::cerr << "Support percent: " << sup_percent << '%' << std::endl;
    std::cerr << "Num of frequent patterns: " << ans.first << std::endl;
    std::cerr << "Maxsize of frequent patterns: " << ans.second << std::endl;
    std::cerr << "Time usage: " << elapsed.count() << "ms" << std::endl;
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
    double percent = std::stod(argv[2]);
    process_file(filename, percent);
#endif
    return 0;
}