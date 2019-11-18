#include <iostream>
#include <vector>
#include <map>

struct Tree {
public:
    int label;
    int depth;
    Tree * parent;
    std::vector<Tree *> subtrees;

    Tree(int l, Tree * p, int d) {
        label = l;
        depth = d;
        parent = p;
        subtrees = std::vector<Tree *>();
    }

    Tree(std::vector<int>::const_iterator repr_begin,
            std::vector<int>::const_iterator repr_end,
            Tree * p = nullptr,
            int d = 0) {
        parent = p;
        depth = d;
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
                auto sub = new Tree(sub_begin, sub_end, this, d + 1);
                subtrees.push_back(sub);
                sub_begin = sub_end;
            }
        }
        if (sub_end != repr_end) {
            std::cerr << "Not a Tree: redundant elements" << std::endl;
            exit(-1);
        }
    }

    explicit Tree(const std::vector<int>& repr) : Tree(repr.cbegin(), repr.cend(), nullptr, 0) {}


};

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}