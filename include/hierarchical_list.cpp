#include "hierarchical_list.h"

void HLNode::addNext(HLNode* child) {
    if (!child) return; // Проверка на nullptr

    HLNode* current = this;
    // Идем до конца цепочки next-узлов
    while (current->pnext != nullptr) {
        current = current->pnext;
    }
    current->pnext = child;
}

void HLNode::addChild(HLNode* child) {
    if (!child) return; // Проверка на nullptr

    if (this->pdown == nullptr) {
        // Если нет вложенных узлов, просто добавляем
        this->pdown = child;
    }
    else {
        // Если есть вложенные узлы, добавляем в конец цепочки
        HLNode* current = this->pdown;
        while (current->pnext != nullptr) {
            current = current->pnext;
        }
        current->pnext = child;
    }
}

HLNode::~HLNode() {
    delete pdown; // Удалит все узлы в ветке pdown
    delete pnext; // Удалит все узлы в ветке pnext
}