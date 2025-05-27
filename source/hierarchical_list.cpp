#include "hierarchical_list.h"

void HLNode::addNext(HLNode* child) {
    if (!child) return; // Ïðîâåðêà íà nullptr

    HLNode* current = this;
    // Èäåì äî êîíöà öåïî÷êè next-óçëîâ
    while (current->pnext != nullptr) {
        current = current->pnext;
    }
    current->pnext = child;
}

void HLNode::addChild(HLNode* child) {
    if (!child) return; // Ïðîâåðêà íà nullptr

    if (this->pdown == nullptr) {
        // Åñëè íåò âëîæåííûõ óçëîâ, ïðîñòî äîáàâëÿåì
        this->pdown = child;
    }
    else {
        // Åñëè åñòü âëîæåííûå óçëû, äîáàâëÿåì â êîíåö öåïî÷êè
        HLNode* current = this->pdown;
        while (current->pnext != nullptr) {
            current = current->pnext;
        }
        current->pnext = child;
    }
}