#include "hierarchical_list.h"

void HLNode::addNext(HLNode* child) {
    if (!child) return; // �������� �� nullptr

    HLNode* current = this;
    // ���� �� ����� ������� next-�����
    while (current->pnext != nullptr) {
        current = current->pnext;
    }
    current->pnext = child;
}

void HLNode::addChild(HLNode* child) {
    if (!child) return; // �������� �� nullptr

    if (this->pdown == nullptr) {
        // ���� ��� ��������� �����, ������ ���������
        this->pdown = child;
    }
    else {
        // ���� ���� ��������� ����, ��������� � ����� �������
        HLNode* current = this->pdown;
        while (current->pnext != nullptr) {
            current = current->pnext;
        }
        current->pnext = child;
    }
}

HLNode::~HLNode() {
    delete pdown; // ������ ��� ���� � ����� pdown
    delete pnext; // ������ ��� ���� � ����� pnext
}